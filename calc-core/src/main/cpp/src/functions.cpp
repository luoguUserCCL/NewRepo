#include <calc/function_registry.h>
#include <calc/crypto_random.h>
#include <mpfr.h>
#include <stdexcept>

namespace calc {

RenderSpec FunctionRegistry::defaultRenderSpec_;

// Helper: get mpfr_t pointer from BigDecimal
static const mpfr_t& getMpfr(const BigDecimal& bd) {
    auto access = bd.getMpfrAccess();
    return *static_cast<const mpfr_t*>(access.mpfrPtr);
}

// Helper: call an MPFR unary function and return BigDecimal
static BigDecimal mpfrUnaryOp(const BigDecimal& arg, int precision,
                                std::function<void(mpfr_t, const mpfr_t, mpfr_rnd_t)> mpfrFunc) {
    return BigDecimal::fromMpfrResult([&](mpfr_t result) {
        mpfrFunc(result, getMpfr(arg), MPFR_RNDN);
    }, precision);
}

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

    // ===== rand(min, max) → Rand(min,max) (crypto-safe, continuous uniform) =====
    builtins_["rand"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return CryptoRandom::uniform(args[0], args[1]);
        },
        RenderSpec{RenderSpec::Type::RAND, "Rand"},
        2, 2
    };

    // ===== sum(i, start, end, expr) → Σ =====
    // sum/prod need AST-level iteration (handled in ExpressionEvaluator).
    // Registered here for lookup/rendering only.
    builtins_["sum"] = BuiltinEntry{
        [](const std::vector<BigDecimal>&) -> BigDecimal {
            return BigDecimal(0); // Placeholder; real eval in ExpressionEvaluator
        },
        RenderSpec{RenderSpec::Type::SUM, "Σ"},
        4, 4
    };

    // ===== prod(i, start, end, expr) → Π =====
    builtins_["prod"] = BuiltinEntry{
        [](const std::vector<BigDecimal>&) -> BigDecimal {
            return BigDecimal(0); // Placeholder; real eval in ExpressionEvaluator
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

    // ===== gcd(a, b) — Euclidean algorithm =====
    builtins_["gcd"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
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

    // ===== lcm(a, b) = |a*b| / gcd(a,b) =====
    builtins_["lcm"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal a = args[0].abs();
            BigDecimal b = args[1].abs();
            if (a.isZero() || b.isZero()) return BigDecimal(0);
            // Compute gcd inline to avoid recursive lookup
            BigDecimal ga = a, gb = b;
            while (!gb.isZero()) {
                BigDecimal r = ga.mod(gb);
                ga = gb;
                gb = r;
            }
            return a.mul(b).div(ga);
        },
        RenderSpec{RenderSpec::Type::PLAIN, "lcm"},
        2, 2
    };

    // ===== log(b) → ln(b), log(a, b) → log_a(b) =====
    // High-precision using MPFR mpfr_log
    builtins_["log"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            int prec = args[0].getPrecision();
            if (args.size() == 1) {
                // Natural logarithm ln(b) via mpfr_log
                return mpfrUnaryOp(args[0], prec,
                    [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                        mpfr_log(result, arg, rnd);
                    });
            } else {
                // log_a(b) = ln(b) / ln(a)
                BigDecimal lnB = mpfrUnaryOp(args[1], prec,
                    [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                        mpfr_log(result, arg, rnd);
                    });
                BigDecimal lnA = mpfrUnaryOp(args[0], prec,
                    [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                        mpfr_log(result, arg, rnd);
                    });
                return lnB.div(lnA);
            }
        },
        RenderSpec{RenderSpec::Type::SUBSCRIPT, "log"},
        1, 2
    };

    // ===== sqrt(b) → √b, sqrt(a, b) → ᵃ√b =====
    builtins_["sqrt"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            if (args.size() == 1) {
                return args[0].nthRoot(2);
            } else {
                // a-th root of b = b^(1/a)
                BigDecimal reciprocal = BigDecimal(int64_t(1), args[0].getPrecision()).div(args[0]);
                return args[1].pow(reciprocal);
            }
        },
        RenderSpec{RenderSpec::Type::RADICAL, "√"},
        1, 2
    };

    // ===== exp(x) — high-precision via mpfr_exp =====
    builtins_["exp"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_exp(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::SUPERSCRIPT, "e"},
        1, 1
    };

    // ===== Trigonometric functions (high-precision via MPFR) =====

    // sin
    builtins_["sin"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_sin(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "sin"},
        1, 1
    };

    // cos
    builtins_["cos"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_cos(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "cos"},
        1, 1
    };

    // tan
    builtins_["tan"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_tan(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "tan"},
        1, 1
    };

    // cot = cos/sin
    builtins_["cot"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal cosVal = mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_cos(result, arg, rnd);
                });
            BigDecimal sinVal = mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_sin(result, arg, rnd);
                });
            return cosVal.div(sinVal);
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "cot"},
        1, 1
    };

    // sec = 1/cos
    builtins_["sec"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal cosVal = mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_cos(result, arg, rnd);
                });
            return BigDecimal(int64_t(1), args[0].getPrecision()).div(cosVal);
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "sec"},
        1, 1
    };

    // csc = 1/sin
    builtins_["csc"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal sinVal = mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_sin(result, arg, rnd);
                });
            return BigDecimal(int64_t(1), args[0].getPrecision()).div(sinVal);
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "csc"},
        1, 1
    };

    // arcsin
    builtins_["arcsin"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_asin(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arcsin"},
        1, 1
    };

    // arccos
    builtins_["arccos"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_acos(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arccos"},
        1, 1
    };

    // arctan
    builtins_["arctan"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_atan(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arctan"},
        1, 1
    };

    // arccot = arctan(1/x)
    builtins_["arccot"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal recip = BigDecimal(int64_t(1), args[0].getPrecision()).div(args[0]);
            return mpfrUnaryOp(recip, recip.getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_atan(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arccot"},
        1, 1
    };

    // arcsec = arccos(1/x)
    builtins_["arcsec"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal recip = BigDecimal(int64_t(1), args[0].getPrecision()).div(args[0]);
            return mpfrUnaryOp(recip, recip.getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_acos(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arcsec"},
        1, 1
    };

    // arccsc = arcsin(1/x)
    builtins_["arccsc"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            BigDecimal recip = BigDecimal(int64_t(1), args[0].getPrecision()).div(args[0]);
            return mpfrUnaryOp(recip, recip.getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_asin(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "arccsc"},
        1, 1
    };

    // ===== log2(x) — base-2 logarithm =====
    builtins_["log2"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_log2(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::SUBSCRIPT, "log₂"},
        1, 1
    };

    // ===== log10(x) — base-10 logarithm =====
    builtins_["log10"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_log10(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::SUBSCRIPT, "log₁₀"},
        1, 1
    };

    // ===== sinh(x) — hyperbolic sine =====
    builtins_["sinh"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_sinh(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "sinh"},
        1, 1
    };

    // ===== cosh(x) — hyperbolic cosine =====
    builtins_["cosh"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_cosh(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "cosh"},
        1, 1
    };

    // ===== tanh(x) — hyperbolic tangent =====
    builtins_["tanh"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_tanh(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "tanh"},
        1, 1
    };

    // ===== gamma(x) — gamma function =====
    builtins_["gamma"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [](mpfr_t result, const mpfr_t arg, mpfr_rnd_t rnd) {
                    mpfr_gamma(result, arg, rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "Γ"},
        1, 1
    };

    // ===== factorial(n) = n! =====
    builtins_["factorial"] = BuiltinEntry{
        [](const std::vector<BigDecimal>& args) -> BigDecimal {
            int64_t n = args[0].toInt64();
            if (n < 0) throw std::domain_error("Factorial of negative number");
            return mpfrUnaryOp(args[0], args[0].getPrecision(),
                [n](mpfr_t result, const mpfr_t, mpfr_rnd_t rnd) {
                    mpfr_fac_ui(result, static_cast<unsigned long>(n), rnd);
                });
        },
        RenderSpec{RenderSpec::Type::PARENTHESIS, "!"},
        1, 1
    };
}

// ==================== Registry Management ====================

bool FunctionRegistry::registerUserFunction(const std::string& name,
                                              std::vector<std::string> params,
                                              std::unique_ptr<ASTNode> body) {
    if (builtins_.count(name)) return false;
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
