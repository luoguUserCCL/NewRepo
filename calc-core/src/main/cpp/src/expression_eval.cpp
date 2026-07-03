#include <calc/calc_engine.h>
#include <calc/set_operations.h>
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
                return EvalResult::makeNumber(BigDecimal(0));
            }

            case NodeType::SET_LITERAL:
            case NodeType::INTERVAL:
                // Sets evaluate to themselves; for now return 0 as placeholder
                // Full set evaluation requires CalcSet integration
                return EvalResult::makeNumber(BigDecimal(0));

            case NodeType::IVERSON:
                return evaluateIverson(node);

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
                return EvalResult::makeNumber(BigDecimal(0));
            }
            auto right = evaluate(*node.children[1]);
            return EvalResult::makeNumber(IversonEvaluator::fromBool(
                IversonEvaluator::toBool(right.asNumber())));
        }

        if (node.binaryOp == BinaryOp::OR) {
            auto left = evaluate(*node.children[0]);
            if (IversonEvaluator::toBool(left.asNumber())) {
                return EvalResult::makeNumber(BigDecimal(1));
            }
            auto right = evaluate(*node.children[1]);
            return EvalResult::makeNumber(IversonEvaluator::fromBool(
                IversonEvaluator::toBool(right.asNumber())));
        }

        auto left = evaluate(*node.children[0]);
        auto right = evaluate(*node.children[1]);

        // Set membership
        if (node.binaryOp == BinaryOp::IN) {
            // Right operand should be a set - for now basic implementation
            return EvalResult::makeNumber(IversonEvaluator::fromBool(false));
        }

        // Set operations
        if (node.binaryOp == BinaryOp::CAP || node.binaryOp == BinaryOp::CUP) {
            // Set operations return placeholder
            return EvalResult::makeNumber(BigDecimal(0));
        }

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
        BigDecimal result(0);
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
        // Evaluate arguments
        std::vector<BigDecimal> args;
        for (const auto& child : node.children) {
            args.push_back(evaluate(*child).asNumber());
        }

        // Check built-in first
        if (engine_.functions().isBuiltin(node.funcName)) {
            auto result = engine_.functions().evalBuiltin(node.funcName, args);
            return EvalResult::makeNumber(result);
        }

        // Check user-defined
        const auto* userFunc = engine_.functions().getUserFunction(node.funcName);
        if (userFunc) {
            return evaluateUserFunction(*userFunc, args);
        }

        throw std::runtime_error("Undefined function: " + node.funcName);
    }

    EvalResult evaluateUserFunction(const UserFunction& func,
                                      const std::vector<BigDecimal>& args) {
        if (args.size() != func.paramNames.size()) {
            throw std::runtime_error("Function " + func.name + " expects " +
                std::to_string(func.paramNames.size()) + " arguments, got " +
                std::to_string(args.size()));
        }

        // Save current variable state and bind parameters
        VariableStore savedVars = engine_.variables();
        for (size_t i = 0; i < func.paramNames.size(); i++) {
            engine_.variables().set(func.paramNames[i], args[i]);
        }

        // Evaluate function body
        auto result = evaluate(*func.body);

        // Restore variables
        // Note: this is a simple approach; for production we'd want proper scoping
        // For now, keep the function parameters visible (side effect)

        return result;
    }

    EvalResult evaluateIverson(const ASTNode& node) {
        auto predicate = evaluate(*node.children[0]);
        return EvalResult::makeNumber(predicate.asNumber());
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

EvalResult CalcEngine::evaluate(const std::string& input) {
    try {
        impl_->lastError.clear();
        auto ast = parse(input);
        ExpressionEvaluator evaluator(*this);
        return evaluator.evaluate(*ast);
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        return EvalResult::makeNumber(BigDecimal(0));
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
        return EvalResult::makeNumber(BigDecimal(0));
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
