#ifndef CALC_EXPRESSION_H
#define CALC_EXPRESSION_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <calc/big_decimal.h>

namespace calc {

/**
 * AST node types for the expression parser.
 * Each node represents a construct in the calculator's input language.
 */
enum class NodeType {
    NUMBER_LIT,       ///< Numeric literal
    VARIABLE_REF,     ///< Variable reference
    BINARY_OP,        ///< Binary operator (+,-,*,/,%,^,=,<,>,and,or,cap,cup,in)
    UNARY_OP,         ///< Unary operator (-,not)
    FUNCTION_CALL,    ///< Built-in or user-defined function call
    DEFINE_VAR,       ///< Variable definition (x := expr)
    DEFINE_FUNC,      ///< Function definition (f(x,y) := expr)
    SET_LITERAL,      ///< Enumerated set {1,2,3}
    INTERVAL,         ///< Interval [a,b], (a,b], [a,b), (a,b)
    IVERSON,          ///< Iverson bracket Iverson(P)
    CONDITIONAL       ///< Ternary / if-expression (future)
};

/**
 * Binary operator kinds.
 * Ordered by precedence (low to high) for the parser.
 */
enum class BinaryOp {
    DEFINE,           ///< :=
    OR,               ///< or  → ∨
    AND,              ///< and → ∧
    EQ,               ///< =
    NEQ,              ///< ≠
    LT,               ///< <
    GT,               ///< >
    LEQ,              ///< ≤
    GEQ,              ///< ≥
    IN,               ///< in → ∈
    CAP,              ///< cap → ∩
    CUP,              ///< cup → ∪
    SET_DIFF,         ///< \ → set difference (A \ B)
    ADD,              ///< +
    SUB,              ///< -
    MUL,              ///< *
    DIV,              ///< /
    MOD,              ///< %  → mod
    POW               ///< ^  (right-associative)
};

/**
 * Unary operator kinds.
 */
enum class UnaryOp {
    NEGATE,           ///< -x
    NOT               ///< not x → ¬x
};

/**
 * Interval boundary types.
 */
enum class BoundType {
    CLOSED,           ///< [ or ]
    OPEN              ///< ( or )
};

/**
 * AST node for parsed expressions.
 * Uses a tagged-union style with std::variant for payload data.
 */
struct ASTNode {
    NodeType type;

    // Payload data (only one is active depending on type)
    BigDecimal numberValue;              ///< NUMBER_LIT
    std::string identifier;              ///< VARIABLE_REF, DEFINE_VAR, DEFINE_FUNC name
    BinaryOp binaryOp = BinaryOp::ADD;   ///< BINARY_OP
    UnaryOp unaryOp = UnaryOp::NEGATE;   ///< UNARY_OP
    std::string funcName;                ///< FUNCTION_CALL

    // Child nodes
    std::vector<std::unique_ptr<ASTNode>> children;

    // Interval-specific data
    BoundType leftBound = BoundType::OPEN;   ///< INTERVAL
    BoundType rightBound = BoundType::OPEN;  ///< INTERVAL

    // Function definition parameter names
    std::vector<std::string> paramNames;     ///< DEFINE_FUNC

    // Constructor helpers
    static std::unique_ptr<ASTNode> makeNumber(const BigDecimal& value);
    static std::unique_ptr<ASTNode> makeVariable(const std::string& name);
    static std::unique_ptr<ASTNode> makeBinary(BinaryOp op,
                                                std::unique_ptr<ASTNode> left,
                                                std::unique_ptr<ASTNode> right);
    static std::unique_ptr<ASTNode> makeUnary(UnaryOp op,
                                               std::unique_ptr<ASTNode> operand);
    static std::unique_ptr<ASTNode> makeFunction(const std::string& name,
                                                  std::vector<std::unique_ptr<ASTNode>> args);
    static std::unique_ptr<ASTNode> makeDefineVar(const std::string& name,
                                                   std::unique_ptr<ASTNode> value);
    static std::unique_ptr<ASTNode> makeDefineFunc(const std::string& name,
                                                    std::vector<std::string> params,
                                                    std::unique_ptr<ASTNode> body);
    static std::unique_ptr<ASTNode> makeSetLiteral(std::vector<std::unique_ptr<ASTNode>> elements);
    static std::unique_ptr<ASTNode> makeInterval(BoundType left, BoundType right,
                                                  std::unique_ptr<ASTNode> low,
                                                  std::unique_ptr<ASTNode> high);
    static std::unique_ptr<ASTNode> makeIverson(std::unique_ptr<ASTNode> predicate);

    /// Deep clone
    std::unique_ptr<ASTNode> clone() const;

    /// Pretty-print for debugging
    std::string debugString(int indent = 0) const;
};

/**
 * Result of evaluating an expression.
 * Can be a BigDecimal value, a set, or a boolean (from Iverson).
 */
struct EvalResult {
    enum class Type { NUMBER, SET, BOOLEAN };

    Type type;
    BigDecimal numberValue;
    bool boolValue = false;
    // Set representation stored as string for now; full CalcSet in set_operations.h

    static EvalResult makeNumber(const BigDecimal& value);
    static EvalResult makeBoolean(bool value);

    bool isNumber() const { return type == Type::NUMBER; }
    bool isBoolean() const { return type == Type::BOOLEAN; }
    const BigDecimal& asNumber() const { return numberValue; }
    bool asBoolean() const { return boolValue; }
};

} // namespace calc

#endif // CALC_EXPRESSION_H
