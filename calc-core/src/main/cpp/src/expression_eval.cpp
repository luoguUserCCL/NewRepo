#include <calc/calc_engine.h>
#include <calc/set_operations.h>
#include <calc/expression_parser.h>
#include <mpfr.h>
#include <stdexcept>
#include <cmath>

namespace calc {

/**
 * Expression evaluator.
 * Walks the AST and computes numerical results.
 */
class ExpressionEvaluator {
public:
    explicit ExpressionEvaluator(CalcEngine& engine) : engine_(engine) {}

    EvalResult evaluate(const ASTNode& node) {
        switch (node.type) {
            case NodeType::NUMBER_LIT:
                return EvalResult::makeNumber(node.numberValue);

            case NodeType::VARIABLE_REF: {
                auto* val = engine_.variables().get(node.identifier);
                if (!val) {
                    throw std::runtime_error("Undefined variable: " + node.identifier);
                }
                return EvalResult::makeNumber(*val);
            }

            case NodeType::BINARY_OP:
                return evaluateBinary(node);

            case NodeType::UNARY_OP:
                return evaluateUnary(node);

            case NodeType::FUNCTION_CALL:
                return evaluateFunction(node);

            case NodeType::DEFINE_VAR: {
                auto val = evaluate(*node.children[0]);
                engine_.variables().set(node.identifier, val.asNumber());
                return val;
            }

            case NodeType::DEFINE_FUNC: {
                auto body = node.children[0]->clone();
                engine_.functions().registerUserFunction(
                    node.identifier, node.paramNames, std::move(body));
                return EvalResult::makeNumber(BigDecimal(int64_t(0)));
            }

            case NodeType::SET_LITERAL:
            case NodeType::INTERVAL:
                // Sets are primarily used with 'in', 'cap', 'cup' operators.
                // When evaluated directly as a value, return 0.
                return EvalResult::makeNumber(BigDecimal(int64_t(0)));

            case NodeType::IVERSON:
                // The IVERSON node stores its predicate as children[0].
                // evaluateIverson evaluates the predicate and wraps as 0/1.
                // Pass the CHILD, not the node itself, to avoid infinite
                // recursion (evaluate(node) on an IVERSON node re-enters
                // this case).
                if (node.children.empty()) {
                    throw std::runtime_error("Iverson bracket missing predicate");
                }
                return evaluateIverson(*node.children[0]);

            default:
                throw std::runtime_error("Unknown node type in evaluation");
        }
    }

private:
    CalcEngine& engine_;

    EvalResult evaluateBinary(const ASTNode& node) {
        // Short-circuit for logical operators
        if (node.binaryOp == BinaryOp::AND) {
            auto left = evaluate(*node.children[0]);
            if (!IversonEvaluator::toBool(left.asNumber())) {
                return EvalResult::makeNumber(BigDecimal(int64_t(0)));
            }
            auto right = evaluate(*node.children[1]);
            return EvalResult::makeNumber(IversonEvaluator::fromBool(
                IversonEvaluator::toBool(right.asNumber())));
        }

        if (node.binaryOp == BinaryOp::OR) {
            auto left = evaluate(*node.children[0]);
            if (IversonEvaluator::toBool(left.asNumber())) {
                return EvalResult::makeNumber(BigDecimal(int64_t(1)));
            }
            auto right = evaluate(*node.children[1]);
            return EvalResult::makeNumber(IversonEvaluator::fromBool(
                IversonEvaluator::toBool(right.asNumber())));
        }

        // Set membership: x in S — must be handled BEFORE the generic
        // left/right number evaluation, because S may be a set expression
        // (predefined set, interval, cap/cup/diff) that cannot be evaluated
        // as a BigDecimal.
        if (node.binaryOp == BinaryOp::IN) {
            auto leftVal = evaluate(*node.children[0]).asNumber();
            CalcSet rightSet = evaluateToSet(*node.children[1]);
            return EvalResult::makeNumber(
                IversonEvaluator::evaluateMembership(leftVal, rightSet));
        }

        // Set operations (cap → ∩, cup → ∪, \ → difference) — same reason:
        // operands are sets, not numbers.
        if (node.binaryOp == BinaryOp::CAP ||
            node.binaryOp == BinaryOp::CUP ||
            node.binaryOp == BinaryOp::SET_DIFF) {
            CalcSet leftSet = evaluateToSet(*node.children[0]);
            CalcSet rightSet = evaluateToSet(*node.children[1]);

            CalcSet resultSet = (node.binaryOp == BinaryOp::CAP)
                ? leftSet.intersection(rightSet)
                : (node.binaryOp == BinaryOp::CUP)
                    ? leftSet.union_(rightSet)
                    : leftSet.difference(rightSet);

            (void)resultSet;
            return EvalResult::makeNumber(BigDecimal(int64_t(0)));
        }

        auto left = evaluate(*node.children[0]);
        auto right = evaluate(*node.children[1]);

        // Comparison operators → Iverson-style (1 or 0)
        switch (node.binaryOp) {
            case BinaryOp::EQ:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::EQ, left.asNumber(), right.asNumber()));
            case BinaryOp::NEQ:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::NEQ, left.asNumber(), right.asNumber()));
            case BinaryOp::LT:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::LT, left.asNumber(), right.asNumber()));
            case BinaryOp::GT:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::GT, left.asNumber(), right.asNumber()));
            case BinaryOp::LEQ:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::LEQ, left.asNumber(), right.asNumber()));
            case BinaryOp::GEQ:
                return EvalResult::makeNumber(IversonEvaluator::evaluateComparison(
                    BinaryOp::GEQ, left.asNumber(), right.asNumber()));
            default:
                break;
        }

        // Arithmetic operators
        BigDecimal result(int64_t(0));
        switch (node.binaryOp) {
            case BinaryOp::ADD: result = left.asNumber().add(right.asNumber()); break;
            case BinaryOp::SUB: result = left.asNumber().sub(right.asNumber()); break;
            case BinaryOp::MUL: result = left.asNumber().mul(right.asNumber()); break;
            case BinaryOp::DIV: result = left.asNumber().div(right.asNumber()); break;
            case BinaryOp::MOD: result = left.asNumber().mod(right.asNumber()); break;
            case BinaryOp::POW: result = left.asNumber().pow(right.asNumber()); break;
            default:
                throw std::runtime_error("Unsupported binary operator in evaluation");
        }
        return EvalResult::makeNumber(result);
    }

    EvalResult evaluateUnary(const ASTNode& node) {
        auto operand = evaluate(*node.children[0]);
        switch (node.unaryOp) {
            case UnaryOp::NEGATE:
                return EvalResult::makeNumber(operand.asNumber().neg());
            case UnaryOp::NOT:
                return EvalResult::makeNumber(
                    IversonEvaluator::evaluateNot(operand.asNumber()));
            default:
                throw std::runtime_error("Unknown unary operator");
        }
    }

    EvalResult evaluateFunction(const ASTNode& node) {
        const std::string& name = node.funcName;

        // ===== Special handling for sum/prod — need AST iteration =====
        if (name == "sum" && node.children.size() == 4) {
            return evaluateSum(node);
        }
        if (name == "prod" && node.children.size() == 4) {
            return evaluateProd(node);
        }

        // ===== Iverson bracket with AST access =====
        if (name == "Iverson" && node.children.size() == 1) {
            return evaluateIverson(*node.children[0]);
        }

        // Evaluate arguments
        std::vector<BigDecimal> args;
        for (const auto& child : node.children) {
            args.push_back(evaluate(*child).asNumber());
        }

        // Check built-in first
        if (engine_.functions().isBuiltin(name)) {
            auto result = engine_.functions().evalBuiltin(name, args);
            return EvalResult::makeNumber(result);
        }

        // Check user-defined
        const auto* userFunc = engine_.functions().getUserFunction(name);
        if (userFunc) {
            return evaluateUserFunction(*userFunc, args);
        }

        throw std::runtime_error("Undefined function: " + name);
    }

    /**
     * Evaluate sum(i, start, end, expr)
     * Iterates i from start to end (inclusive), evaluates expr for each i,
     * and accumulates the sum with full precision.
     */
    EvalResult evaluateSum(const ASTNode& node) {
        // node.children: [i_name_as_var, start, end, expr]
        // i_name is a VARIABLE_REF node
        const std::string& varName = node.children[0]->identifier;
        BigDecimal startVal = evaluate(*node.children[1]).asNumber();
        BigDecimal endVal = evaluate(*node.children[2]).asNumber();

        int64_t start = startVal.toInt64();
        int64_t end = endVal.toInt64();

        BigDecimal result(int64_t(0));
        for (int64_t i = start; i <= end; i++) {
            engine_.variables().set(varName, BigDecimal(int64_t(i)));
            auto termResult = evaluate(*node.children[3]);
            result = result.add(termResult.asNumber());
        }

        return EvalResult::makeNumber(result);
    }

    /**
     * Evaluate prod(i, start, end, expr)
     * Iterates i from start to end (inclusive), evaluates expr for each i,
     * and accumulates the product with full precision.
     */
    EvalResult evaluateProd(const ASTNode& node) {
        const std::string& varName = node.children[0]->identifier;
        BigDecimal startVal = evaluate(*node.children[1]).asNumber();
        BigDecimal endVal = evaluate(*node.children[2]).asNumber();

        int64_t start = startVal.toInt64();
        int64_t end = endVal.toInt64();

        BigDecimal result(int64_t(1));
        for (int64_t i = start; i <= end; i++) {
            engine_.variables().set(varName, BigDecimal(int64_t(i)));
            auto termResult = evaluate(*node.children[3]);
            result = result.mul(termResult.asNumber());
        }

        return EvalResult::makeNumber(result);
    }

    EvalResult evaluateUserFunction(const UserFunction& func,
                                      const std::vector<BigDecimal>& args) {
        if (args.size() != func.paramNames.size()) {
            throw std::runtime_error("Function " + func.name + " expects " +
                std::to_string(func.paramNames.size()) + " arguments, got " +
                std::to_string(args.size()));
        }

        // Save variables that will be shadowed by parameters
        std::vector<std::pair<std::string, BigDecimal>> savedVars;
        for (const auto& paramName : func.paramNames) {
            auto* existing = engine_.variables().get(paramName);
            if (existing) {
                savedVars.emplace_back(paramName, *existing);
            }
        }

        // Bind parameters
        for (size_t i = 0; i < func.paramNames.size(); i++) {
            engine_.variables().set(func.paramNames[i], args[i]);
        }

        // Evaluate function body
        auto result = evaluate(*func.body);

        // Restore shadowed variables (remove params, restore originals)
        for (const auto& paramName : func.paramNames) {
            engine_.variables().remove(paramName);
        }
        for (const auto& [name, value] : savedVars) {
            engine_.variables().set(name, value);
        }

        return result;
    }

    EvalResult evaluateIverson(const ASTNode& node) {
        auto predicate = evaluate(node);
        return EvalResult::makeNumber(predicate.asNumber());
    }

    /**
     * Evaluate an AST node to a CalcSet.
     * Handles SET_LITERAL, INTERVAL, cap/cup/diff binary operations, and
     * predefined set constants (Real, Rational, Quotient, Integer, Zahlen).
     */
    CalcSet evaluateToSet(const ASTNode& node) {
        switch (node.type) {
            case NodeType::SET_LITERAL: {
                std::vector<BigDecimal> elements;
                for (const auto& child : node.children) {
                    elements.push_back(evaluate(*child).asNumber());
                }
                return CalcSet::makeEnumerated(std::move(elements));
            }
            case NodeType::INTERVAL: {
                auto low = evaluate(*node.children[0]).asNumber();
                auto high = evaluate(*node.children[1]).asNumber();
                auto toSetBound = [](BoundType bt) -> CalcSet::BoundType {
                    return bt == BoundType::CLOSED
                        ? CalcSet::BoundType::CLOSED
                        : CalcSet::BoundType::OPEN;
                };
                return CalcSet::makeInterval(toSetBound(node.leftBound),
                    toSetBound(node.rightBound),
                    std::move(low), std::move(high));
            }
            case NodeType::VARIABLE_REF: {
                // Predefined set constants: Real (ℝ), Rational/Quotient (ℚ),
                // Integer/Zahlen (ℤ). These are recognized by name, not via
                // the variable store (which holds BigDecimal values).
                const std::string& name = node.identifier;
                if (name == "Real")     return CalcSet::makePredefined(CalcSet::PredefinedSet::REAL);
                if (name == "Rational" || name == "Quotient")
                    return CalcSet::makePredefined(CalcSet::PredefinedSet::RATIONAL);
                if (name == "Integer" || name == "Zahlen")
                    return CalcSet::makePredefined(CalcSet::PredefinedSet::INTEGER);
                break;
            }
            case NodeType::BINARY_OP: {
                if (node.binaryOp == BinaryOp::CAP) {
                    auto left = evaluateToSet(*node.children[0]);
                    auto right = evaluateToSet(*node.children[1]);
                    return left.intersection(right);
                }
                if (node.binaryOp == BinaryOp::CUP) {
                    auto left = evaluateToSet(*node.children[0]);
                    auto right = evaluateToSet(*node.children[1]);
                    return left.union_(right);
                }
                if (node.binaryOp == BinaryOp::SET_DIFF) {
                    auto left = evaluateToSet(*node.children[0]);
                    auto right = evaluateToSet(*node.children[1]);
                    return left.difference(right);
                }
                break;
            }
            default:
                break;
        }
        // Fallback: return empty set
        return CalcSet::makeEnumerated({});
    }
};

// ==================== CalcEngine Implementation ====================

struct CalcEngine::Impl {
    FunctionRegistry functionRegistry;
    VariableStore variableStore;
    NumberFormatConfig formatConfig;
    std::string lastError;
};

CalcEngine::CalcEngine() : impl_(std::make_unique<Impl>()) {
    impl_->functionRegistry.registerBuiltins();
    impl_->variableStore.registerConstants();
}

CalcEngine::~CalcEngine() = default;

EvalResult CalcEngine::evaluate(const std::string& input) {
    try {
        impl_->lastError.clear();
        auto ast = parse(input);
        ExpressionEvaluator evaluator(*this);
        return evaluator.evaluate(*ast);
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        return EvalResult::makeNumber(BigDecimal(int64_t(0)));
    }
}

std::unique_ptr<ASTNode> CalcEngine::parse(const std::string& input) {
    ExpressionParser parser(input);
    return parser.parse();
}

EvalResult CalcEngine::evaluateAST(const ASTNode& node) {
    try {
        ExpressionEvaluator evaluator(*this);
        return evaluator.evaluate(node);
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        return EvalResult::makeNumber(BigDecimal(int64_t(0)));
    }
}

FunctionRegistry& CalcEngine::functions() { return impl_->functionRegistry; }
const FunctionRegistry& CalcEngine::functions() const { return impl_->functionRegistry; }

VariableStore& CalcEngine::variables() { return impl_->variableStore; }
const VariableStore& CalcEngine::variables() const { return impl_->variableStore; }

NumberFormatConfig& CalcEngine::formatConfig() { return impl_->formatConfig; }
const NumberFormatConfig& CalcEngine::formatConfig() const { return impl_->formatConfig; }

std::string CalcEngine::formatResult(const EvalResult& result) const {
    return NumberFormatter::format(result.asNumber(), impl_->formatConfig);
}

const std::string& CalcEngine::lastError() const { return impl_->lastError; }

void CalcEngine::reset() {
    impl_->variableStore.clear();
    impl_->variableStore.registerConstants();
    // Don't clear built-in functions, only user-defined
}

} // namespace calc
