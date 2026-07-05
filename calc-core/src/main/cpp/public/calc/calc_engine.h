#ifndef CALC_CALC_ENGINE_H
#define CALC_CALC_ENGINE_H

#include <string>
#include <memory>
#include <calc/big_decimal.h>
#include <calc/expression.h>
#include <calc/function_registry.h>
#include <calc/variable_store.h>
#include <calc/set_operations.h>
#include <calc/number_format.h>
#include <calc/symbolic_value.h>

namespace calc {

/**
 * Main calculation engine — the central orchestrator.
 * Parses input strings, evaluates expressions, and returns results.
 * Manages the function registry and variable store.
 */
class CalcEngine {
public:
    CalcEngine();
    ~CalcEngine();

    /// Evaluate an input string and return the result
    EvalResult evaluate(const std::string& input);

    /// Parse an input string into an AST (for rendering)
    std::unique_ptr<ASTNode> parse(const std::string& input);

    /// Evaluate a pre-parsed AST
    EvalResult evaluateAST(const ASTNode& node);

    /// Get the function registry
    FunctionRegistry& functions();
    const FunctionRegistry& functions() const;

    /// Get the variable store
    VariableStore& variables();
    const VariableStore& variables() const;

    /// Get/set the number format configuration
    NumberFormatConfig& formatConfig();
    const NumberFormatConfig& formatConfig() const;

    /// Get/set the output mode (DECIMAL vs MATH/symbolic)
    OutputMode outputMode() const;
    void setOutputMode(OutputMode mode);

    /// Format a result for display
    std::string formatResult(const EvalResult& result) const;

    /// Get the last error message (empty if no error)
    const std::string& lastError() const;

    /// Clear all user definitions (variables and functions)
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace calc

#endif // CALC_CALC_ENGINE_H
