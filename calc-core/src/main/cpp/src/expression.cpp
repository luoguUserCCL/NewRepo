#include <calc/expression.h>
#include <sstream>
#include <algorithm>

namespace calc {

// ==================== AST Node Factory Methods ====================

std::unique_ptr<ASTNode> ASTNode::makeNumber(const BigDecimal& value) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::NUMBER_LIT;
    node->numberValue = value;
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeVariable(const std::string& name) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::VARIABLE_REF;
    node->identifier = name;
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeBinary(BinaryOp op,
                                               std::unique_ptr<ASTNode> left,
                                               std::unique_ptr<ASTNode> right) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::BINARY_OP;
    node->binaryOp = op;
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeUnary(UnaryOp op,
                                              std::unique_ptr<ASTNode> operand) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::UNARY_OP;
    node->unaryOp = op;
    node->children.push_back(std::move(operand));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeFunction(const std::string& name,
                                                 std::vector<std::unique_ptr<ASTNode>> args) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::FUNCTION_CALL;
    node->funcName = name;
    node->children = std::move(args);
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeDefineVar(const std::string& name,
                                                  std::unique_ptr<ASTNode> value) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::DEFINE_VAR;
    node->identifier = name;
    node->children.push_back(std::move(value));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeDefineFunc(const std::string& name,
                                                   std::vector<std::string> params,
                                                   std::unique_ptr<ASTNode> body) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::DEFINE_FUNC;
    node->identifier = name;
    node->paramNames = std::move(params);
    node->children.push_back(std::move(body));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeSetLiteral(std::vector<std::unique_ptr<ASTNode>> elements) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::SET_LITERAL;
    node->children = std::move(elements);
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeInterval(BoundType left, BoundType right,
                                                 std::unique_ptr<ASTNode> low,
                                                 std::unique_ptr<ASTNode> high) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::INTERVAL;
    node->leftBound = left;
    node->rightBound = right;
    node->children.push_back(std::move(low));
    node->children.push_back(std::move(high));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeIverson(std::unique_ptr<ASTNode> predicate) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::IVERSON;
    node->children.push_back(std::move(predicate));
    return node;
}

std::unique_ptr<ASTNode> ASTNode::clone() const {
    auto node = std::make_unique<ASTNode>();
    node->type = type;
    node->numberValue = numberValue;
    node->identifier = identifier;
    node->binaryOp = binaryOp;
    node->unaryOp = unaryOp;
    node->funcName = funcName;
    node->leftBound = leftBound;
    node->rightBound = rightBound;
    node->paramNames = paramNames;
    for (const auto& child : children) {
        node->children.push_back(child->clone());
    }
    return node;
}

// ==================== Debug String ====================

static std::string opToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::DEFINE: return ":=";
        case BinaryOp::OR:     return "or";
        case BinaryOp::AND:    return "and";
        case BinaryOp::EQ:     return "=";
        case BinaryOp::NEQ:    return "≠";
        case BinaryOp::LT:     return "<";
        case BinaryOp::GT:     return ">";
        case BinaryOp::LEQ:    return "≤";
        case BinaryOp::GEQ:    return "≥";
        case BinaryOp::IN:     return "in";
        case BinaryOp::CAP:    return "cap";
        case BinaryOp::CUP:    return "cup";
        case BinaryOp::SET_DIFF: return "\\";
        case BinaryOp::ADD:    return "+";
        case BinaryOp::SUB:    return "-";
        case BinaryOp::MUL:    return "*";
        case BinaryOp::DIV:    return "/";
        case BinaryOp::MOD:    return "%";
        case BinaryOp::POW:    return "^";
        default:               return "?";
    }
}

std::string ASTNode::debugString(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;

    switch (type) {
        case NodeType::NUMBER_LIT:
            ss << pad << "NUMBER(" << numberValue.toString() << ")";
            break;
        case NodeType::VARIABLE_REF:
            ss << pad << "VAR(" << identifier << ")";
            break;
        case NodeType::BINARY_OP:
            ss << pad << "BINARY(" << opToString(binaryOp) << ")";
            break;
        case NodeType::UNARY_OP:
            ss << pad << "UNARY(" << (unaryOp == UnaryOp::NEGATE ? "-" : "not") << ")";
            break;
        case NodeType::FUNCTION_CALL:
            ss << pad << "CALL(" << funcName << ", " << children.size() << " args)";
            break;
        case NodeType::DEFINE_VAR:
            ss << pad << "DEF_VAR(" << identifier << ")";
            break;
        case NodeType::DEFINE_FUNC:
            ss << pad << "DEF_FUNC(" << identifier << "(";
            for (size_t i = 0; i < paramNames.size(); i++) {
                if (i > 0) ss << ",";
                ss << paramNames[i];
            }
            ss << "))";
            break;
        case NodeType::SET_LITERAL:
            ss << pad << "SET(" << children.size() << " elements)";
            break;
        case NodeType::INTERVAL:
            ss << pad << "INTERVAL("
               << (leftBound == BoundType::CLOSED ? "[" : "(") << ".."
               << (rightBound == BoundType::CLOSED ? "]" : ")") << ")";
            break;
        case NodeType::IVERSON:
            ss << pad << "IVERSON";
            break;
        default:
            ss << pad << "UNKNOWN";
            break;
    }

    for (const auto& child : children) {
        ss << "\n" << child->debugString(indent + 2);
    }
    return ss.str();
}

// ==================== EvalResult ====================

EvalResult EvalResult::makeNumber(const BigDecimal& value) {
    EvalResult r;
    r.type = Type::NUMBER;
    r.numberValue = value;
    return r;
}

EvalResult EvalResult::makeBoolean(bool value) {
    EvalResult r;
    r.type = Type::BOOLEAN;
    r.boolValue = value;
    r.numberValue = BigDecimal(int64_t(value ? 1 : 0));
    return r;
}

} // namespace calc
