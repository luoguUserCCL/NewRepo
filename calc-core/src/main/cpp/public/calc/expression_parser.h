#ifndef CALC_EXPRESSION_PARSER_H
#define CALC_EXPRESSION_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <calc/expression.h>

namespace calc {

/**
 * Recursive descent expression parser.
 * Converts input strings into AST trees.
 *
 * Operator precedence (low to high):
 *   := (definition)
 *   or (∨)
 *   and (∧)
 *   = ≠ < > ≤ ≥ (comparison)
 *   in (∈) cap (∩) cup (∪) (set operations)
 *   + - (additive)
 *   * / % (multiplicative)
 *   ^ (power, right-associative)
 *   - not (unary)
 */
class ExpressionParser {
public:
    explicit ExpressionParser(const std::string& input);

    /// Parse the input string into an AST
    std::unique_ptr<ASTNode> parse();

private:
    std::string input_;
    size_t pos_;

    // Token peeking
    char peek() const;
    char advance();
    bool match(char expected);
    bool matchKeyword(const std::string& keyword);
    void skipWhitespace();

    // Parsing by precedence
    std::unique_ptr<ASTNode> parseDefinition();
    std::unique_ptr<ASTNode> parseLogicalOr();
    std::unique_ptr<ASTNode> parseLogicalAnd();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseSetOps();
    std::unique_ptr<ASTNode> parseAdditive();
    std::unique_ptr<ASTNode> parseMultiplicative();
    std::unique_ptr<ASTNode> parsePower();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parseNot();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<ASTNode> parseNumber();
    std::unique_ptr<ASTNode> parseIdentifierOrFunction();
    std::unique_ptr<ASTNode> parseParenOrInterval();
    std::unique_ptr<ASTNode> parseSetLiteral();

    // Internal helpers
    std::vector<std::unique_ptr<ASTNode>> parseArgList();
    std::string tryParseIdentifier();

    /// Result of attempting to parse a function-definition parameter list.
    /// `valid` is false if the content inside parens is NOT a valid param
    /// list (e.g. it's actually a function-call argument list like `-5` or
    /// `2, 3`). This makes tryParseParamList non-throwing, so the caller can
    /// backtrack cleanly.
    struct ParamListResult {
        std::vector<std::string> names;
        bool valid = false;
    };
    ParamListResult tryParseParamList();
};

} // namespace calc

#endif // CALC_EXPRESSION_PARSER_H
