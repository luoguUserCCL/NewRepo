#include <calc/expression.h>
#include <calc/calc_engine.h>
#include <calc/set_operations.h>
#include <calc/expression_parser.h>
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

namespace calc {

// ==================== ExpressionParser Implementation ====================

ExpressionParser::ExpressionParser(const std::string& input)
    : input_(input), pos_(0) {}

std::unique_ptr<ASTNode> ExpressionParser::parse() {
    skipWhitespace();
    auto result = parseDefinition();
    skipWhitespace();
    if (pos_ < input_.length()) {
        throw std::runtime_error("Unexpected character at position " +
            std::to_string(pos_) + ": '" + std::string(1, input_[pos_]) + "'");
    }
    return result;
}

// ==================== Token helpers ====================

char ExpressionParser::peek() const {
    if (pos_ >= input_.length()) return '\0';
    return input_[pos_];
}

char ExpressionParser::advance() {
    return input_[pos_++];
}

void ExpressionParser::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
    }
}

bool ExpressionParser::match(char expected) {
    skipWhitespace();
    if (pos_ < input_.length() && input_[pos_] == expected) {
        pos_++;
        return true;
    }
    return false;
}

bool ExpressionParser::matchKeyword(const std::string& token) {
    skipWhitespace();
    if (input_.compare(pos_, token.length(), token) == 0) {
        // Ensure it's not part of a longer identifier
        if (std::isalpha(static_cast<unsigned char>(token[0])) || token[0] == '_') {
            size_t endPos = pos_ + token.length();
            if (endPos < input_.length() &&
                (std::isalnum(static_cast<unsigned char>(input_[endPos])) || input_[endPos] == '_')) {
                return false;
            }
        }
        pos_ += token.length();
        return true;
    }
    return false;
}

// ==================== Parser rules ====================

std::unique_ptr<ASTNode> ExpressionParser::parseDefinition() {
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
            if (matchKeyword(":=")) {
                // Function definition
                auto body = parseLogicalOr();
                return ASTNode::makeDefineFunc(name, std::move(params), std::move(body));
            }
            // Not a definition, restore position
            pos_ = savePos;
        }

        // Check for variable definition: name := expr
        if (matchKeyword(":=")) {
            auto value = parseLogicalOr();
            return ASTNode::makeDefineVar(name, std::move(value));
        }

        // Not a definition, restore position
        pos_ = savePos;
    }

    return parseLogicalOr();
}

std::unique_ptr<ASTNode> ExpressionParser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (matchKeyword("or")) {
        auto right = parseLogicalAnd();
        left = ASTNode::makeBinary(BinaryOp::OR, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd() {
    auto left = parseNot();
    while (matchKeyword("and")) {
        auto right = parseNot();
        left = ASTNode::makeBinary(BinaryOp::AND, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> ExpressionParser::parseNot() {
    if (matchKeyword("not")) {
        auto operand = parseNot();
        return ASTNode::makeUnary(UnaryOp::NOT, std::move(operand));
    }
    return parseComparison();
}

std::unique_ptr<ASTNode> ExpressionParser::parseComparison() {
    auto left = parseSetOps();
    skipWhitespace();

    BinaryOp op;
    if (matchKeyword("!=") || matchKeyword("\xe2\x89\xa0"))       op = BinaryOp::NEQ;  // ≠
    else if (matchKeyword("<=") || matchKeyword("\xe2\x89\xa4"))   op = BinaryOp::LEQ;  // ≤
    else if (matchKeyword(">=") || matchKeyword("\xe2\x89\xa5"))   op = BinaryOp::GEQ;  // ≥
    else if (match('='))              op = BinaryOp::EQ;
    else if (match('<'))              op = BinaryOp::LT;
    else if (match('>'))              op = BinaryOp::GT;
    else return left;

    auto right = parseSetOps();
    return ASTNode::makeBinary(op, std::move(left), std::move(right));
}

std::unique_ptr<ASTNode> ExpressionParser::parseSetOps() {
    auto left = parseAdditive();
    skipWhitespace();

    while (true) {
        if (matchKeyword("cap")) {
            auto right = parseAdditive();
            left = ASTNode::makeBinary(BinaryOp::CAP, std::move(left), std::move(right));
        } else if (matchKeyword("cup")) {
            auto right = parseAdditive();
            left = ASTNode::makeBinary(BinaryOp::CUP, std::move(left), std::move(right));
        } else if (matchKeyword("in")) {
            auto right = parseAdditive();
            left = ASTNode::makeBinary(BinaryOp::IN, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> ExpressionParser::parseAdditive() {
    auto left = parseMultiplicative();

    while (true) {
        skipWhitespace();
        if (match('+')) {
            auto right = parseMultiplicative();
            left = ASTNode::makeBinary(BinaryOp::ADD, std::move(left), std::move(right));
        } else if (match('-')) {
            auto right = parseMultiplicative();
            left = ASTNode::makeBinary(BinaryOp::SUB, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative() {
    auto left = parseUnary();

    while (true) {
        skipWhitespace();
        if (match('*')) {
            auto right = parseUnary();
            left = ASTNode::makeBinary(BinaryOp::MUL, std::move(left), std::move(right));
        } else if (match('/')) {
            auto right = parseUnary();
            left = ASTNode::makeBinary(BinaryOp::DIV, std::move(left), std::move(right));
        } else if (match('%')) {
            auto right = parseUnary();
            left = ASTNode::makeBinary(BinaryOp::MOD, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> ExpressionParser::parseUnary() {
    skipWhitespace();
    if (match('-')) {
        auto operand = parseUnary();
        return ASTNode::makeUnary(UnaryOp::NEGATE, std::move(operand));
    }
    return parsePower();
}

std::unique_ptr<ASTNode> ExpressionParser::parsePower() {
    auto base = parsePrimary();
    skipWhitespace();
    if (match('^')) {
        auto exp = parsePower();  // Right-associative recursion
        return ASTNode::makeBinary(BinaryOp::POW, std::move(base), std::move(exp));
    }
    return base;
}

std::unique_ptr<ASTNode> ExpressionParser::parsePrimary() {
    skipWhitespace();

    // Number literal (including base-prefixed: 0b, 0o, 0x)
    if (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.') {
        return parseNumber();
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
    if (matchKeyword("Iverson")) {
        skipWhitespace();
        if (!match('(')) {
            throw std::runtime_error("Expected '(' after Iverson");
        }
        auto predicate = parseLogicalOr();
        skipWhitespace();
        if (!match(')')) {
            throw std::runtime_error("Expected ')' after Iverson predicate");
        }
        return ASTNode::makeIverson(std::move(predicate));
    }

    // Identifier (variable or function call)
    return parseIdentifierOrFunction();
}

// ==================== Primary helpers ====================

std::unique_ptr<ASTNode> ExpressionParser::parseNumber() {
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
                   std::isxdigit(static_cast<unsigned char>(input_[pos_]))) {
                numStr += advance();
            }
            return ASTNode::makeNumber(BigDecimal::fromBase(numStr, 16));
        }
        // Could be just 0 or 0.xxx
    }

    // Regular decimal number
    while (pos_ < input_.length() &&
           (std::isdigit(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '.')) {
        numStr += advance();
    }

    // Scientific notation
    if (pos_ < input_.length() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
        numStr += advance();
        if (pos_ < input_.length() && (input_[pos_] == '+' || input_[pos_] == '-')) {
            numStr += advance();
        }
        while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            numStr += advance();
        }
    }

    if (numStr.empty() || numStr == ".") {
        throw std::runtime_error("Invalid number literal at position " + std::to_string(pos_));
    }

    return ASTNode::makeNumber(BigDecimal(numStr));
}

std::unique_ptr<ASTNode> ExpressionParser::parseIdentifierOrFunction() {
    std::string name = tryParseIdentifier();
    if (name.empty()) {
        throw std::runtime_error("Unexpected character at position " +
            std::to_string(pos_) + ": '" + std::string(1, peek()) + "'");
    }
    skipWhitespace();
    // Function call
    if (peek() == '(') {
        pos_++; // skip '('
        auto args = parseArgList();
        skipWhitespace();
        if (!match(')')) {
            throw std::runtime_error("Expected ')' after function arguments");
        }
        return ASTNode::makeFunction(name, std::move(args));
    }
    return ASTNode::makeVariable(name);
}

std::unique_ptr<ASTNode> ExpressionParser::parseParenOrInterval() {
    bool leftClosed = (peek() == '[');
    char openChar = advance(); // consume '(' or '['

    auto first = parseLogicalOr();
    skipWhitespace();

    // Check if this is an interval: (expr, expr) or [expr, expr]
    if (match(',')) {
        auto second = parseLogicalOr();
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
    if (!match(')')) {
        throw std::runtime_error("Expected ')' to close expression");
    }
    return first;
}

std::unique_ptr<ASTNode> ExpressionParser::parseSetLiteral() {
    advance(); // consume '{'
    std::vector<std::unique_ptr<ASTNode>> elements;

    skipWhitespace();
    if (peek() != '}') {
        elements.push_back(parseLogicalOr());
        while (match(',')) {
            elements.push_back(parseLogicalOr());
        }
    }
    skipWhitespace();
    if (!match('}')) {
        throw std::runtime_error("Expected '}' to close set literal");
    }
    return ASTNode::makeSetLiteral(std::move(elements));
}

// ==================== Internal helpers ====================

std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArgList() {
    std::vector<std::unique_ptr<ASTNode>> args;
    skipWhitespace();
    if (peek() == ')') return args;

    args.push_back(parseLogicalOr());
    while (match(',')) {
        args.push_back(parseLogicalOr());
    }
    return args;
}

std::string ExpressionParser::tryParseIdentifier() {
    skipWhitespace();
    if (pos_ >= input_.length() || (!std::isalpha(static_cast<unsigned char>(input_[pos_])) && input_[pos_] != '_')) {
        return "";
    }

    size_t start = pos_;
    while (pos_ < input_.length() &&
           (std::isalnum(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '_')) {
        pos_++;
    }
    return input_.substr(start, pos_ - start);
}

std::vector<std::string> ExpressionParser::tryParseParamList() {
    std::vector<std::string> params;
    skipWhitespace();
    if (peek() == ')') return params;

    std::string name = tryParseIdentifier();
    if (name.empty()) {
        throw std::runtime_error("Expected parameter name in function definition");
    }
    params.push_back(name);

    while (match(',')) {
        name = tryParseIdentifier();
        if (name.empty()) {
            throw std::runtime_error("Expected parameter name after ','");
        }
        params.push_back(name);
    }
    return params;
}

} // namespace calc
