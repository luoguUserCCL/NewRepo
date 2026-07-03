#ifndef CALC_FUNCTION_REGISTRY_H
#define CALC_FUNCTION_REGISTRY_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <calc/big_decimal.h>
#include <calc/expression.h>

namespace calc {

class CalcEngine;  // forward declaration

/**
 * Render specification for a function — how it should be displayed.
 * Controls the visual rendering in the formula display.
 */
struct RenderSpec {
    enum class Type {
        PARENTHESIS,     ///< f(x)  →  f(x)
        ABS_BARS,        ///< abs(x)  →  |x|
        FLOOR,           ///< floor(x)  →  ⌊x⌋
        CEIL,            ///< ceil(x)  →  ⌈x⌉
        RAND,            ///< rand(a,b)  →  Rand(a,b)
        IVERSON,         ///< Iverson(P)  →  𝕀(P)
        SUM,             ///< sum(i,s,e,expr)  →  Σ
        PRODUCT,         ///< prod(i,s,e,expr)  →  Π
        SUBSCRIPT,       ///< log(a,b)  →  log_a(b)
        RADICAL,         ///< sqrt(a,b)  →  ᵃ√b
        SUPERSCRIPT,     ///< pow(a,b)  →  a^b
        MOD,             ///< a % b  →  a mod b
        PLAIN            ///< gcd(a,b)  →  gcd(a,b)
    };

    Type type = Type::PARENTHESIS;
    std::string symbol;  ///< The rendered symbol (e.g. "log", "gcd", "sin")
};

/**
 * Built-in function implementation.
 * Takes a vector of arguments and returns a result.
 */
using BuiltinFuncImpl = std::function<BigDecimal(const std::vector<BigDecimal>&)>;

/**
 * User-defined function.
 * Stores parameter names and body AST for lazy evaluation.
 */
struct UserFunction {
    std::string name;
    std::vector<std::string> paramNames;
    std::unique_ptr<ASTNode> body;
};

/**
 * Registry of all available functions (built-in and user-defined).
 * Handles function lookup, registration, and evaluation.
 */
class FunctionRegistry {
public:
    FunctionRegistry();

    /// Register all built-in functions
    void registerBuiltins();

    /// Register a user-defined function
    bool registerUserFunction(const std::string& name,
                               std::vector<std::string> params,
                               std::unique_ptr<ASTNode> body);

    /// Remove a user-defined function
    bool removeUserFunction(const std::string& name);

    /// Check if a function exists (built-in or user-defined)
    bool hasFunction(const std::string& name) const;

    /// Check if a function is built-in
    bool isBuiltin(const std::string& name) const;

    /// Get the render specification for a function
    const RenderSpec& getRenderSpec(const std::string& name) const;

    /// Get the argument count range for a function
    std::pair<int, int> getArgCount(const std::string& name) const;

    /// Evaluate a built-in function with given arguments
    BigDecimal evalBuiltin(const std::string& name,
                           const std::vector<BigDecimal>& args) const;

    /// Get a user-defined function (for evaluation with a context)
    const UserFunction* getUserFunction(const std::string& name) const;

    /// List all available function names
    std::vector<std::string> listFunctions() const;

    /// List user-defined function names only
    std::vector<std::string> listUserFunctions() const;

private:
    struct BuiltinEntry {
        BuiltinFuncImpl impl;
        RenderSpec renderSpec;
        int minArgs;
        int maxArgs;
    };

    std::unordered_map<std::string, BuiltinEntry> builtins_;
    std::unordered_map<std::string, UserFunction> userFunctions_;
    static RenderSpec defaultRenderSpec_;
};

} // namespace calc

#endif // CALC_FUNCTION_REGISTRY_H
