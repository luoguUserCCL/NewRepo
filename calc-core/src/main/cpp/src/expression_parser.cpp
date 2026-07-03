#include <calc/expression.h>
#include <calc/calc_engine.h>
#include <calc/set_operations.h>
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

namespace calc {

/**
 * Recursive descent parser for calculator expressions.
 *
 * Grammar (precedence low→high):
 *   definition    := identifier '(' param_list ')' ':=' expr
 *                  | identifier ':=' expr
 *   expr          := or_expr
 *   or_expr       := and_expr ('or' and_expr)*
 *   and_expr      := not_expr ('and' not_expr)*
 *   not_expr      := 'not' not_expr | comparison
 *   comparison    := set_expr (('=' | '<' | '>' | '≤' | '≥' | '≠' | '!=') set_expr)?
 *   set_expr      := addition ('cap' addition | 'cup' addition | 'in' addition)*
 *   addition      := multiplication (('+' | '-') multiplication)*
 *   multiplication := unary (('*' | '/' | '%') unary)*
 *   unary         := '-' unary | power
 *   power         := postfix ('^' power)?   // right-associative
 *   postfix       := primary (function_call_suffix)?
 *   primary       := NUMBER | IDENTIFIER | '(' expr ')' | '{' set_literal '}'
 *                  | 'Iverson' '(' predicate ')' | interval_literal
 *                  | function_call
 *
 * Special tokens:
 *   ':='  → definition operator
 *   '≤'   → leq  (also accepted as '<=')
 *   '≥'   → geq  (also accepted as '>=')
 *   '≠'   → neq  (also accepted as '!=')
 *   'and' → logical AND
 *   'or'  → logical OR
 *   'not' → logical NOT
 *   'in'  → set membership
 *   'cap' → intersection ∩
 *   'cup' → union ∪
 */
class ExpressionParser {
public:
    explicit ExpressionParser(const std::string& input)
        : input_(input), pos_(0) {}

    std::unique_ptr<ASTNode> parse() {
        skipWhitespace();
        auto result = parseDefinition();
        skipWhitespace();
        if (pos_ < input_.length()) {
            throw std::runtime_error("Unexpected character at position " +
                std::to_string(pos_) + ": '" + std::string(1, input_[pos_]) + "'");
        }
        return result;
    }

private:
    std::string input_;
    size_t pos_;

    // ==================== Lexer helpers ====================

    char peek() const {
        return pos_ < input_.length() ? input_[pos_] : '\0';
    }

    char advance() {
        return pos_ < input_.length() ? input_[pos_++] : '\0';
    }

    void skipWhitespace() {
        while (pos_ < input_.length() && std::isspace(input_[pos_])) pos_++;
    }

    bool match(const std::string& token) {
        skipWhitespace();
        if (input_.compare(pos_, token.length(), token) == 0) {
            // Ensure it's not part of a longer identifier
            if (std::isalpha(token[0]) || token[0] == '_') {
                size_t endPos = pos_ + token.length();
                if (endPos < input_.length() &&
                    (std::isalnum(input_[endPos]) || input_[endPos] == '_')) {
                    return false;
                }
            }
            pos_ += token.length();
            return true;
        }
        return false;
    }

    bool matchChar(char c) {
        skipWhitespace();
        if (pos_ < input_.length() && input_[pos_] == c) {
            pos_++;
            return true;
        }
        return false;
    }

    // ==================== Parser rules ====================

    std::unique_ptr<ASTNode> parseDefinition() {
        // Try to match: identifier ':=' expr  or  identifier(params) ':=' expr
        // We need lookahead for this
        size_t savePos = pos_;
        std::string name = tryParseIdentifier();

        if (!name.empty()) {
            skipWhitespace();

            // Check for function definition: name(params) := expr
            if (pos_ < input_.length() && input_[pos_] == '(') {
                // Could be function definition or function call
                size_t parenPos = pos_;
                pos_++; // skip '('
                auto params = tryParseParamList();
                skipWhitespace();
                if (match(":=")) {
                    // Function definition
                    auto body = parseExpr();
                    return ASTNode::makeDefineFunc(name, std::move(params), std::move(body));
                }
                // Not a definition, restore position
                pos_ = savePos;
            }

            // Check for variable definition: name := expr
            if (match(":=")) {
                auto value = parseExpr();
                return ASTNode::makeDefineVar(name, std::move(value));
            }

            // Not a definition, restore position
            pos_ = savePos;
        }

        return parseExpr();
    }

    std::unique_ptr<ASTNode> parseExpr() {
        return parseOr();
    }

    std::unique_ptr<ASTNode> parseOr() {
        auto left = parseAnd();
        while (match("or")) {
            auto right = parseAnd();
            left = ASTNode::makeBinary(BinaryOp::OR, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAnd() {
        auto left = parseNot();
        while (match("and")) {
            auto right = parseNot();
            left = ASTNode::makeBinary(BinaryOp::AND, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseNot() {
        if (match("not")) {
            auto operand = parseNot();
            return ASTNode::makeUnary(UnaryOp::NOT, std::move(operand));
        }
        return parseComparison();
    }

    std::unique_ptr<ASTNode> parseComparison() {
        auto left = parseSetExpr();
        skipWhitespace();

        BinaryOp op;
        if (match("!=") || match("≠"))       op = BinaryOp::NEQ;
        else if (match("<=") || match("≤"))   op = BinaryOp::LEQ;
        else if (match(">=") || match("≥"))   op = BinaryOp::GEQ;
        else if (matchChar('='))              op = BinaryOp::EQ;
        else if (matchChar('<'))              op = BinaryOp::LT;
        else if (matchChar('>'))              op = BinaryOp::GT;
        else return left;

        auto right = parseSetExpr();
        return ASTNode::makeBinary(op, std::move(left), std::move(right));
    }

    std::unique_ptr<ASTNode> parseSetExpr() {
        auto left = parseAddition();
        skipWhitespace();

        while (true) {
            if (match("cap")) {
                auto right = parseAddition();
                left = ASTNode::makeBinary(BinaryOp::CAP, std::move(left), std::move(right));
            } else if (match("cup")) {
                auto right = parseAddition();
                left = ASTNode::makeBinary(BinaryOp::CUP, std::move(left), std::move(right));
            } else if (match("in")) {
                auto right = parseAddition();
                left = ASTNode::makeBinary(BinaryOp::IN, std::move(left), std::move(right));
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAddition() {
        auto left = parseMultiplication();

        while (true) {
            skipWhitespace();
            if (matchChar('+')) {
                auto right = parseMultiplication();
                left = ASTNode::makeBinary(BinaryOp::ADD, std::move(left), std::move(right));
            } else if (matchChar('-')) {
                auto right = parseMultiplication();
                left = ASTNode::makeBinary(BinaryOp::SUB, std::move(left), std::move(right));
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseMultiplication() {
        auto left = parseUnary();

        while (true) {
            skipWhitespace();
            if (matchChar('*')) {
                auto right = parseUnary();
                left = ASTNode::makeBinary(BinaryOp::MUL, std::move(left), std::move(right));
            } else if (matchChar('/')) {
                auto right = parseUnary();
                left = ASTNode::makeBinary(BinaryOp::DIV, std::move(left), std::move(right));
            } else if (matchChar('%')) {
                auto right = parseUnary();
                left = ASTNode::makeBinary(BinaryOp::MOD, std::move(left), std::move(right));
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseUnary() {
        skipWhitespace();
        if (matchChar('-')) {
            auto operand = parseUnary();
            return ASTNode::makeUnary(UnaryOp::NEGATE, std::move(operand));
        }
        return parsePower();
    }

    std::unique_ptr<ASTNode> parsePower() {
        auto base = parsePrimary();
        skipWhitespace();
        if (matchChar('^')) {
            auto exp = parsePower();  // Right-associative recursion
            return ASTNode::makeBinary(BinaryOp::POW, std::move(base), std::move(exp));
        }
        return base;
    }

    std::unique_ptr<ASTNode> parsePrimary() {
        skipWhitespace();

        // Number literal (including base-prefixed: 0b, 0o, 0x)
        if (std::isdigit(peek()) || peek() == '.') {
            return parseNumberLiteral();
        }

        // Parenthesized expression or interval
        if (peek() == '(' || peek() == '[') {
            return parseParenOrInterval();
        }

        // Set literal {1,2,3}
        if (peek() == '{') {
            return parseSetLiteral();
        }

        // Iverson bracket
        if (match("Iverson")) {
            skipWhitespace();
            if (!matchChar('(')) {
                throw std::runtime_error("Expected '(' after Iverson");
            }
            auto predicate = parseExpr();
            skipWhitespace();
            if (!matchChar(')')) {
                throw std::runtime_error("Expected ')' after Iverson predicate");
            }
            return ASTNode::makeIverson(std::move(predicate));
        }

        // Identifier (variable or function call)
        std::string name = tryParseIdentifier();
        if (!name.empty()) {
            skipWhitespace();
            // Function call
            if (peek() == '(') {
                pos_++; // skip '('
                auto args = parseArgList();
                skipWhitespace();
                if (!matchChar(')')) {
                    throw std::runtime_error("Expected ')' after function arguments");
                }
                return ASTNode::makeFunction(name, std::move(args));
            }
            return ASTNode::makeVariable(name);
        }

        throw std::runtime_error("Unexpected character at position " +
            std::to_string(pos_) + ": '" + std::string(1, peek()) + "'");
    }

    // ==================== Helpers ====================

    std::unique_ptr<ASTNode> parseNumberLiteral() {
        std::string numStr;

        // Check for base prefix
        if (peek() == '0') {
            numStr += advance();
            if (peek() == 'b' || peek() == 'B') {
                numStr += advance();
                // Binary
                while (pos_ < input_.length() && (input_[pos_] == '0' || input_[pos_] == '1')) {
                    numStr += advance();
                }
                return ASTNode::makeNumber(BigDecimal::fromBase(numStr, 2));
            } else if (peek() == 'o' || peek() == 'O') {
                numStr += advance();
                // Octal
                while (pos_ < input_.length() && input_[pos_] >= '0' && input_[pos_] <= '7') {
                    numStr += advance();
                }
                return ASTNode::makeNumber(BigDecimal::fromBase(numStr, 8));
            } else if (peek() == 'x' || peek() == 'X') {
                numStr += advance();
                // Hexadecimal
                while (pos_ < input_.length() &&
                       std::isxdigit(input_[pos_])) {
                    numStr += advance();
                }
                return ASTNode::makeNumber(BigDecimal::fromBase(numStr, 16));
            }
            // Could be just 0 or 0.xxx
        }

        // Regular decimal number
        while (pos_ < input_.length() &&
               (std::isdigit(input_[pos_]) || input_[pos_] == '.')) {
            numStr += advance();
        }

        // Scientific notation
        if (pos_ < input_.length() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            numStr += advance();
            if (pos_ < input_.length() && (input_[pos_] == '+' || input_[pos_] == '-')) {
                numStr += advance();
            }
            while (pos_ < input_.length() && std::isdigit(input_[pos_])) {
                numStr += advance();
            }
        }

        if (numStr.empty() || numStr == ".") {
            throw std::runtime_error("Invalid number literal at position " + std::to_string(pos_));
        }

        return ASTNode::makeNumber(BigDecimal(numStr));
    }

    std::unique_ptr<ASTNode> parseParenOrInterval() {
        bool leftClosed = (peek() == '[');
        char openChar = advance(); // consume '(' or '['

        auto first = parseExpr();
        skipWhitespace();

        // Check if this is an interval: (expr, expr) or [expr, expr]
        if (matchChar(',')) {
            auto second = parseExpr();
            skipWhitespace();

            bool rightClosed = false;
            if (peek() == ']') {
                rightClosed = true;
                advance();
            } else if (peek() == ')') {
                advance();
            } else {
                throw std::runtime_error("Expected ')' or ']' to close interval");
            }

            BoundType leftBound = leftClosed ? BoundType::CLOSED : BoundType::OPEN;
            BoundType rightBound = rightClosed ? BoundType::CLOSED : BoundType::OPEN;
            return ASTNode::makeInterval(leftBound, rightBound,
                                          std::move(first), std::move(second));
        }

        // Regular parenthesized expression
        if (!matchChar(')')) {
            throw std::runtime_error("Expected ')' to close expression");
        }
        return first;
    }

    std::unique_ptr<ASTNode> parseSetLiteral() {
        advance(); // consume '{'
        std::vector<std::unique_ptr<ASTNode>> elements;

        skipWhitespace();
        if (peek() != '}') {
            elements.push_back(parseExpr());
            while (matchChar(',')) {
                elements.push_back(parseExpr());
            }
        }
        skipWhitespace();
        if (!matchChar('}')) {
            throw std::runtime_error("Expected '}' to close set literal");
        }
        return ASTNode::makeSetLiteral(std::move(elements));
    }

    std::vector<std::unique_ptr<ASTNode>> parseArgList() {
        std::vector<std::unique_ptr<ASTNode>> args;
        skipWhitespace();
        if (peek() == ')') return args;

        args.push_back(parseExpr());
        while (matchChar(',')) {
            args.push_back(parseExpr());
        }
        return args;
    }

    std::string tryParseIdentifier() {
        skipWhitespace();
        if (pos_ >= input_.length() || (!std::isalpha(input_[pos_]) && input_[pos_] != '_')) {
            return "";
        }

        size_t start = pos_;
        while (pos_ < input_.length() &&
               (std::isalnum(input_[pos_]) || input_[pos_] == '_')) {
            pos_++;
        }
        return input_.substr(start, pos_ - start);
    }

    std::vector<std::string> tryParseParamList() {
        std::vector<std::string> params;
        skipWhitespace();
        if (peek() == ')') return params;

        std::string name = tryParseIdentifier();
        if (name.empty()) {
            throw std::runtime_error("Expected parameter name in function definition");
        }
        params.push_back(name);

        while (matchChar(',')) {
            name = tryParseIdentifier();
            if (name.empty()) {
                throw std::runtime_error("Expected parameter name after ','");
            }
            params.push_back(name);
        }
        return params;
    }
};

} // namespace calc
