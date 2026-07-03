#include <calc/function_registry.h>
#include <calc/crypto_random.h>
#include <mpfr.h>
#include <cmath>
#include <stdexcept>

namespace calc {

RenderSpec FunctionRegistry::defaultRenderSpec_;

FunctionRegistry::FunctionRegistry() {
    registerBuiltins();
}

void FunctionRegistry::registerBuiltins() {
    // ===== abs(x) → |x| =====
    builtins_["abs"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return args[0].abs();
        },
        RenderSpec{RenderSpec::Type::ABS_BARS, ""},
        1, 1
    };

    // ===== floor(x) → ⌊x⌋ =====
    builtins_["floor"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return args[0].floor();
        },
        RenderSpec{RenderSpec::Type::FLOOR, ""},
        1, 1
    };

    // ===== ceil(x) → ⌈x⌉ =====
    builtins_["ceil"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return args[0].ceil();
        },
        RenderSpec{RenderSpec::Type::CEIL, ""},
        1, 1
    };

    // ===== rand(min, max) → Rand(min,max) (crypto-safe) =====
    builtins_["rand"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return CryptoRandom::uniform(args[0], args[1]);
        },
        RenderSpec{RenderSpec::Type::RAND, "Rand"},
        2, 2
    };

    // ===== sum(i, start, end, expr) → Σ =====
    // Note: sum/prod are handled specially in the evaluator since they
    // need a variable binding context. Registered here for lookup/rendering.
    builtins_["sum"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            // Fallback: if called with pre-evaluated args, just return 0
            // Real evaluation happens in ExpressionEvaluator with AST access
            return BigDecimal(0);
        },
        RenderSpec{RenderSpec::Type::SUM, "Σ"},
        4, 4
    };

    // ===== prod(i, start, end, expr) → Π =====
    builtins_["prod"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return BigDecimal(0);
        },
        RenderSpec{RenderSpec::Type::PRODUCT, "Π"},
        4, 4
    };

    // ===== pow(a, b) → a^b (fast power for integer exponents) =====
    builtins_["pow"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return args[0].pow(args[1]);
        },
        RenderSpec{RenderSpec::Type::SUPERSCRIPT, ""},
        2, 2
    };

    // ===== gcd(a, b) =====
    builtins_["gcd"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            // Euclidean algorithm with BigDecimal
            BigDecimal a = args[0].abs();
            BigDecimal b = args[1].abs();
            while (!b.isZero()) {
                BigDecimal r = a.mod(b);
                a = b;
                b = r;
            }
            return a;
        },
        RenderSpec{RenderSpec::Type::PLAIN, "gcd"},
        2, 2
    };

    // ===== lcm(a, b) =====
    builtins_["lcm"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal a = args[0].abs();
            BigDecimal b = args[1].abs();
            if (a.isZero() || b.isZero()) return BigDecimal(0);
            BigDecimal g = builtins_["gcd"].impl({a, b});
            return a.mul(b).div(g);
        },
        RenderSpec{RenderSpec::Type::PLAIN, "lcm"},
        2, 2
    };

    // ===== log(a, b) → log_a(b), log(b) → ln(b) =====
    builtins_["log"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal result(BigDecimal::DEFAULT_PRECISION);
            if (args.size() == 1) {
                // Natural logarithm
                BigDecimal lnArg(BigDecimal::DEFAULT_PRECISION);
                // Use MPFR directly via the implementation
                // For now we use the div approach: log_a(b) = ln(b)/ln(a)
                // ln(b) via mpfr_log
                return args[0]; // Placeholder - will be implemented with MPFR direct access
            } else {
                // log_a(b) = ln(b) / ln(a)
                // Will use MPFR directly
                return args[1]; // Placeholder
            }
        },
        RenderSpec{RenderSpec::Type::SUBSCRIPT, "log"},
        1, 2
    };

    // ===== sqrt(a, b) → ᵃ√b, sqrt(b) → √b =====
    builtins_["sqrt"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            if (args.size() == 1) {
                return args[0].nthRoot(2);
            } else {
                // a-th root of b = b^(1/a)
                BigDecimal reciprocal = BigDecimal(1).div(args[0]);
                return args[1].pow(reciprocal);
            }
        },
        RenderSpec{RenderSpec::Type::RADICAL, "√"},
        1, 2
    };

    // ===== Trigonometric functions =====
    auto registerTrig = [&](const std::string& name, RenderSpec::Type renderType) {
        builtins_[name] = BuiltinEntry{
            [name](const std::vector<BigDecimal>& args) -> BigDecimal {
                // For high-precision trig, we need MPFR trig functions
                // Placeholder implementation
                double val = args[0].toDouble();
                double result = 0;
                if (name == "sin") result = std::sin(val);
                else if (name == "cos") result = std::cos(val);
                else if (name == "tan") result = std::tan(val);
                else if (name == "cot") result = 1.0 / std::tan(val);
                else if (name == "sec") result = 1.0 / std::cos(val);
                else if (name == "csc") result = 1.0 / std::sin(val);
                else if (name == "arcsin") result = std::asin(val);
                else if (name == "arccos") result = std::acos(val);
                else if (name == "arctan") result = std::atan(val);
                else if (name == "arccot") result = std::atan(1.0 / val);
                else if (name == "arcsec") result = std::acos(1.0 / val);
                else if (name == "arccsc") result = std::asin(1.0 / val);
                return BigDecimal(result);
            },
            RenderSpec{renderType, name},
            1, 1
        };
    };

    registerTrig("sin", RenderSpec::Type::PARENTHESIS);
    registerTrig("cos", RenderSpec::Type::PARENTHESIS);
    registerTrig("tan", RenderSpec::Type::PARENTHESIS);
    registerTrig("cot", RenderSpec::Type::PARENTHESIS);
    registerTrig("sec", RenderSpec::Type::PARENTHESIS);
    registerTrig("csc", RenderSpec::Type::PARENTHESIS);
    registerTrig("arcsin", RenderSpec::Type::PARENTHESIS);
    registerTrig("arccos", RenderSpec::Type::PARENTHESIS);
    registerTrig("arctan", RenderSpec::Type::PARENTHESIS);
    registerTrig("arccot", RenderSpec::Type::PARENTHESIS);
    registerTrig("arcsec", RenderSpec::Type::PARENTHESIS);
    registerTrig("arccsc", RenderSpec::Type::PARENTHESIS);

    // ===== exp(x) =====
    builtins_["exp"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return BigDecimal::e(args[0].getPrecision()).pow(args[0]);
        },
        RenderSpec{RenderSpec::Type::SUPERSCRIPT, "e"},
        1, 1
    };
}

bool FunctionRegistry::registerUserFunction(const std::string& name,
                                              std::vector<std::string> params,
                                              std::unique_ptr<ASTNode> body) {
    if (builtins_.count(name)) return false; // Can't override builtins
    UserFunction func{name, std::move(params), std::move(body)};
    userFunctions_[name] = std::move(func);
    return true;
}

bool FunctionRegistry::removeUserFunction(const std::string& name) {
    return userFunctions_.erase(name) > 0;
}

bool FunctionRegistry::hasFunction(const std::string& name) const {
    return builtins_.count(name) > 0 || userFunctions_.count(name) > 0;
}

bool FunctionRegistry::isBuiltin(const std::string& name) const {
    return builtins_.count(name) > 0;
}

const RenderSpec& FunctionRegistry::getRenderSpec(const std::string& name) const {
    auto it = builtins_.find(name);
    if (it != builtins_.end()) return it->second.renderSpec;
    return defaultRenderSpec_;
}

std::pair<int, int> FunctionRegistry::getArgCount(const std::string& name) const {
    auto it = builtins_.find(name);
    if (it != builtins_.end()) return {it->second.minArgs, it->second.maxArgs};
    auto uit = userFunctions_.find(name);
    if (uit != userFunctions_.end()) {
        int n = uit->second.paramNames.size();
        return {n, n};
    }
    return {0, 0};
}

BigDecimal FunctionRegistry::evalBuiltin(const std::string& name,
                                           const std::vector<BigDecimal>& args) const {
    auto it = builtins_.find(name);
    if (it == builtins_.end()) {
        throw std::runtime_error("Not a built-in function: " + name);
    }
    auto [minArgs, maxArgs] = getArgCount(name);
    if ((int)args.size() < minArgs || (int)args.size() > maxArgs) {
        throw std::runtime_error("Function " + name + " expects " +
            std::to_string(minArgs) + "-" + std::to_string(maxArgs) +
            " arguments, got " + std::to_string(args.size()));
    }
    return it->second.impl(args);
}

const UserFunction* FunctionRegistry::getUserFunction(const std::string& name) const {
    auto it = userFunctions_.find(name);
    return it != userFunctions_.end() ? &it->second : nullptr;
}

std::vector<std::string> FunctionRegistry::listFunctions() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : builtins_) names.push_back(name);
    for (const auto& [name, _] : userFunctions_) names.push_back(name);
    return names;
}

std::vector<std::string> FunctionRegistry::listUserFunctions() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : userFunctions_) names.push_back(name);
    return names;
}

} // namespace calc
