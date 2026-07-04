#include <calc/math_renderer.h>
#include <calc/font_manager.h>
#include <sstream>

namespace calc {

// ==================== LayoutNode Factory Methods ====================

std::unique_ptr<LayoutNode> LayoutNode::makeGlyph(uint32_t codepoint, float fontSize,
                                                     const std::string& font) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::GLYPH;
    node->codepoint = codepoint;
    node->fontSize = fontSize;
    node->fontFamily = font;
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeHorizontal(std::vector<std::unique_ptr<LayoutNode>> children) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::HORIZONTAL;
    node->children = std::move(children);
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeVertical(std::vector<std::unique_ptr<LayoutNode>> children) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::VERTICAL;
    node->children = std::move(children);
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeFraction(std::unique_ptr<LayoutNode> numerator,
                                                        std::unique_ptr<LayoutNode> denominator,
                                                        float ruleThickness) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::FRACTION;
    node->ruleThickness = ruleThickness;
    node->children.push_back(std::move(numerator));
    node->children.push_back(std::move(denominator));
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeRadical(std::unique_ptr<LayoutNode> radicand,
                                                       std::unique_ptr<LayoutNode> index) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::RADICAL;
    node->children.push_back(std::move(radicand));
    if (index) {
        node->children.push_back(std::move(index));
    }
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeSubSuperscript(std::unique_ptr<LayoutNode> base,
                                                              std::unique_ptr<LayoutNode> sub,
                                                              std::unique_ptr<LayoutNode> sup) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::SUB_SUPERSCRIPT;
    node->children.push_back(std::move(base));
    if (sub) node->children.push_back(std::move(sub));
    if (sup) node->children.push_back(std::move(sup));
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeUnderOver(std::unique_ptr<LayoutNode> base,
                                                         std::unique_ptr<LayoutNode> under,
                                                         std::unique_ptr<LayoutNode> over) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::UNDER_OVER;
    node->children.push_back(std::move(base));
    if (under) node->children.push_back(std::move(under));
    if (over) node->children.push_back(std::move(over));
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeDelimiter(uint32_t open, uint32_t close,
                                                         std::unique_ptr<LayoutNode> content) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::DELIMITER;
    node->openDelim = open;
    node->closeDelim = close;
    node->children.push_back(std::move(content));
    return node;
}

std::unique_ptr<LayoutNode> LayoutNode::makeSpace(float width) {
    auto node = std::make_unique<LayoutNode>();
    node->type = LayoutType::SPACE;
    node->spaceWidth = width;
    return node;
}

// ==================== Layout Computation ====================

void LayoutNode::computeLayout(float fontSize) {
    switch (type) {
        case LayoutType::GLYPH: {
            // Approximate glyph dimensions
            float scale = fontSize / 16.0f;
            bbox.width = fontSize * 0.6f;
            bbox.height = fontSize;
            bbox.ascent = fontSize * 0.8f;
            bbox.descent = fontSize * 0.2f;
            break;
        }
        case LayoutType::HORIZONTAL: {
            float maxAscent = 0, maxDescent = 0;
            float totalWidth = 0;
            for (auto& child : children) {
                child->computeLayout(fontSize);
                totalWidth += child->bbox.width;
                maxAscent = std::max(maxAscent, child->bbox.ascent);
                maxDescent = std::max(maxDescent, child->bbox.descent);
            }
            bbox.width = totalWidth;
            bbox.height = maxAscent + maxDescent;
            bbox.ascent = maxAscent;
            bbox.descent = maxDescent;
            break;
        }
        case LayoutType::VERTICAL: {
            float maxWidth = 0;
            float totalHeight = 0;
            for (auto& child : children) {
                child->computeLayout(fontSize);
                maxWidth = std::max(maxWidth, child->bbox.width);
                totalHeight += child->bbox.height;
            }
            bbox.width = maxWidth;
            bbox.height = totalHeight;
            bbox.ascent = children.empty() ? 0 : children[0]->bbox.ascent;
            bbox.descent = totalHeight - bbox.ascent;
            break;
        }
        case LayoutType::FRACTION: {
            if (children.size() >= 2) {
                children[0]->computeLayout(fontSize * 0.7f);
                children[1]->computeLayout(fontSize * 0.7f);
            }
            float numWidth = children.size() >= 1 ? children[0]->bbox.width : 0;
            float denWidth = children.size() >= 2 ? children[1]->bbox.width : 0;
            float numH = children.size() >= 1 ? children[0]->bbox.height : 0;
            float denH = children.size() >= 2 ? children[1]->bbox.height : 0;

            bbox.width = std::max(numWidth, denWidth) + 4.0f;
            bbox.height = numH + denH + ruleThickness + 4.0f;
            bbox.ascent = numH + ruleThickness / 2 + 2.0f;
            bbox.descent = denH + ruleThickness / 2 + 2.0f;
            break;
        }
        case LayoutType::RADICAL: {
            if (!children.empty()) {
                children[0]->computeLayout(fontSize);
            }
            float contentW = children.empty() ? 0 : children[0]->bbox.width;
            float contentH = children.empty() ? 0 : children[0]->bbox.height;

            bbox.width = contentW + fontSize * 0.8f;
            bbox.height = contentH + fontSize * 0.2f;
            bbox.ascent = (children.empty() ? fontSize * 0.8f : children[0]->bbox.ascent) + fontSize * 0.1f;
            bbox.descent = (children.empty() ? fontSize * 0.2f : children[0]->bbox.descent) + fontSize * 0.1f;
            break;
        }
        case LayoutType::SUB_SUPERSCRIPT: {
            if (!children.empty()) children[0]->computeLayout(fontSize);
            float scriptSize = fontSize * 0.7f;
            for (size_t i = 1; i < children.size(); i++) {
                children[i]->computeLayout(scriptSize);
            }
            float baseW = children.empty() ? 0 : children[0]->bbox.width;
            float baseA = children.empty() ? 0 : children[0]->bbox.ascent;
            float baseD = children.empty() ? 0 : children[0]->bbox.descent;

            float scriptW = 0;
            for (size_t i = 1; i < children.size(); i++) {
                scriptW = std::max(scriptW, children[i]->bbox.width);
            }

            bbox.width = baseW + scriptW;
            bbox.ascent = baseA;
            bbox.descent = baseD;
            bbox.height = bbox.ascent + bbox.descent;
            break;
        }
        case LayoutType::UNDER_OVER: {
            for (auto& child : children) {
                child->computeLayout(child == children[0] ? fontSize : fontSize * 0.7f);
            }
            float maxW = 0;
            for (const auto& child : children) {
                maxW = std::max(maxW, child->bbox.width);
            }
            float baseA = children.empty() ? 0 : children[0]->bbox.ascent;
            float baseD = children.empty() ? 0 : children[0]->bbox.descent;
            float underH = children.size() > 1 ? children[1]->bbox.height : 0;
            float overH = children.size() > 2 ? children[2]->bbox.height : 0;

            bbox.width = maxW;
            bbox.ascent = baseA + overH;
            bbox.descent = baseD + underH;
            bbox.height = bbox.ascent + bbox.descent;
            break;
        }
        case LayoutType::DELIMITER: {
            if (!children.empty()) children[0]->computeLayout(fontSize);
            float contentW = children.empty() ? 0 : children[0]->bbox.width;
            float contentH = children.empty() ? 0 : children[0]->bbox.height;
            float contentA = children.empty() ? 0 : children[0]->bbox.ascent;
            float contentD = children.empty() ? 0 : children[0]->bbox.descent;

            float delimW = fontSize * 0.4f;
            bbox.width = contentW + 2 * delimW;
            bbox.ascent = contentA;
            bbox.descent = contentD;
            bbox.height = contentA + contentD;
            break;
        }
        case LayoutType::SPACE: {
            bbox.width = spaceWidth;
            bbox.height = 0;
            bbox.ascent = 0;
            bbox.descent = 0;
            break;
        }
        default:
            break;
    }
}

// ==================== MathRenderer ====================

MathRenderer::MathRenderer() : defaultFontSize_(18.0f) {}

std::unique_ptr<LayoutNode> MathRenderer::astToLayout(const ASTNode& ast, float fontSize) {
    switch (ast.type) {
        case NodeType::NUMBER_LIT:   return convertNumber(ast, fontSize);
        case NodeType::VARIABLE_REF: return convertVariable(ast, fontSize);
        case NodeType::BINARY_OP:    return convertBinaryOp(ast, fontSize);
        case NodeType::UNARY_OP:     return convertUnaryOp(ast, fontSize);
        case NodeType::FUNCTION_CALL:return convertFunction(ast, fontSize);
        case NodeType::IVERSON:      return convertIverson(ast, fontSize);
        case NodeType::INTERVAL:     return convertInterval(ast, fontSize);
        case NodeType::SET_LITERAL:  return convertSetLiteral(ast, fontSize);
        case NodeType::DEFINE_VAR:   return astToLayout(*ast.children[0], fontSize);
        case NodeType::DEFINE_FUNC:  return astToLayout(*ast.children[0], fontSize);
        default:
            return LayoutNode::makeGlyph('?', fontSize);
    }
}

void MathRenderer::layout(std::unique_ptr<LayoutNode>& root) {
    if (root) root->computeLayout(defaultFontSize_);
}

Vec2 MathRenderer::getSize(const LayoutNode& root) const {
    return Vec2(root.bbox.width, root.bbox.height);
}

// ==================== AST → Layout Conversion ====================

std::unique_ptr<LayoutNode> MathRenderer::convertNumber(const ASTNode& node, float fontSize) {
    std::string str = node.numberValue.toString();
    std::vector<std::unique_ptr<LayoutNode>> glyphs;
    for (char c : str) {
        glyphs.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
    }
    return LayoutNode::makeHorizontal(std::move(glyphs));
}

std::unique_ptr<LayoutNode> MathRenderer::convertVariable(const ASTNode& node, float fontSize) {
    std::vector<std::unique_ptr<LayoutNode>> glyphs;
    for (char c : node.identifier) {
        // Use italic for variable names (Latin Modern Math italic)
        glyphs.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
    }
    return LayoutNode::makeHorizontal(std::move(glyphs));
}

std::unique_ptr<LayoutNode> MathRenderer::convertBinaryOp(const ASTNode& node, float fontSize) {
    auto left = astToLayout(*node.children[0], fontSize);
    auto right = astToLayout(*node.children[1], fontSize);

    uint32_t symbol = 0;
    std::string text;
    float spacing = fontSize * 0.3f;

    switch (node.binaryOp) {
        case BinaryOp::ADD:    symbol = '+'; break;
        case BinaryOp::SUB:    symbol = '-'; break;
        case BinaryOp::MUL:    symbol = 0x00D7; break;  // ×
        case BinaryOp::DIV:    return LayoutNode::makeFraction(std::move(left), std::move(right));
        case BinaryOp::MOD:    text = "mod"; break;
        case BinaryOp::POW:    return LayoutNode::makeSubSuperscript(
                                    std::move(left), nullptr, std::move(right));
        case BinaryOp::EQ:     symbol = '='; break;
        case BinaryOp::NEQ:    symbol = FontManager::MathCodepoints::NEQ; break;
        case BinaryOp::LT:     symbol = '<'; break;
        case BinaryOp::GT:     symbol = '>'; break;
        case BinaryOp::LEQ:    symbol = FontManager::MathCodepoints::LEQ; break;
        case BinaryOp::GEQ:    symbol = FontManager::MathCodepoints::GEQ; break;
        case BinaryOp::AND:    symbol = FontManager::MathCodepoints::AND; break;
        case BinaryOp::OR:     symbol = FontManager::MathCodepoints::OR; break;
        case BinaryOp::IN:     symbol = FontManager::MathCodepoints::IN; break;
        case BinaryOp::CAP:    symbol = FontManager::MathCodepoints::CAP; break;
        case BinaryOp::CUP:    symbol = FontManager::MathCodepoints::CUP; break;
        default:               symbol = '?'; break;
    }

    std::vector<std::unique_ptr<LayoutNode>> parts;
    parts.push_back(std::move(left));
    parts.push_back(LayoutNode::makeSpace(spacing));

    if (symbol) {
        parts.push_back(LayoutNode::makeGlyph(symbol, fontSize));
    } else {
        for (char c : text) {
            parts.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
        }
    }
    parts.push_back(LayoutNode::makeSpace(spacing));
    parts.push_back(std::move(right));

    return LayoutNode::makeHorizontal(std::move(parts));
}

std::unique_ptr<LayoutNode> MathRenderer::convertUnaryOp(const ASTNode& node, float fontSize) {
    auto operand = astToLayout(*node.children[0], fontSize);

    switch (node.unaryOp) {
        case UnaryOp::NEGATE: {
            std::vector<std::unique_ptr<LayoutNode>> parts;
            parts.push_back(LayoutNode::makeGlyph('-', fontSize));
            parts.push_back(std::move(operand));
            return LayoutNode::makeHorizontal(std::move(parts));
        }
        case UnaryOp::NOT: {
            std::vector<std::unique_ptr<LayoutNode>> parts;
            parts.push_back(LayoutNode::makeGlyph(FontManager::MathCodepoints::NOT, fontSize));
            parts.push_back(LayoutNode::makeSpace(fontSize * 0.15f));
            parts.push_back(std::move(operand));
            return LayoutNode::makeHorizontal(std::move(parts));
        }
        default:
            return operand;
    }
}

std::unique_ptr<LayoutNode> MathRenderer::convertFunction(const ASTNode& node, float fontSize) {
    const std::string& name = node.funcName;

    // abs(x) → |x|
    if (name == "abs" && node.children.size() == 1) {
        auto content = astToLayout(*node.children[0], fontSize);
        return LayoutNode::makeDelimiter('|', '|', std::move(content));
    }

    // floor(x) → ⌊x⌋
    if (name == "floor" && node.children.size() == 1) {
        auto content = astToLayout(*node.children[0], fontSize);
        return LayoutNode::makeDelimiter(
            FontManager::MathCodepoints::FLOOR_LEFT,
            FontManager::MathCodepoints::FLOOR_RIGHT,
            std::move(content));
    }

    // ceil(x) → ⌈x⌉
    if (name == "ceil" && node.children.size() == 1) {
        auto content = astToLayout(*node.children[0], fontSize);
        return LayoutNode::makeDelimiter(
            FontManager::MathCodepoints::CEIL_LEFT,
            FontManager::MathCodepoints::CEIL_RIGHT,
            std::move(content));
    }

    // sum(i,s,e,expr) → Σ_{i=s}^{e}(expr)
    if (name == "sum" && node.children.size() == 4) {
        auto expr = astToLayout(*node.children[3], fontSize);
        auto under = astToLayout(*node.children[0], fontSize * 0.7f);
        auto over = astToLayout(*node.children[2], fontSize * 0.7f);
        auto sigma = LayoutNode::makeGlyph(FontManager::MathCodepoints::SUM, fontSize * 1.4f);
        auto bigOp = LayoutNode::makeUnderOver(std::move(sigma), std::move(under), std::move(over));
        std::vector<std::unique_ptr<LayoutNode>> parts;
        parts.push_back(std::move(bigOp));
        parts.push_back(std::move(expr));
        return LayoutNode::makeHorizontal(std::move(parts));
    }

    // prod(i,s,e,expr) → Π_{i=s}^{e}(expr)
    if (name == "prod" && node.children.size() == 4) {
        auto expr = astToLayout(*node.children[3], fontSize);
        auto under = astToLayout(*node.children[0], fontSize * 0.7f);
        auto over = astToLayout(*node.children[2], fontSize * 0.7f);
        auto pi = LayoutNode::makeGlyph(FontManager::MathCodepoints::PRODUCT, fontSize * 1.4f);
        auto bigOp = LayoutNode::makeUnderOver(std::move(pi), std::move(under), std::move(over));
        std::vector<std::unique_ptr<LayoutNode>> parts;
        parts.push_back(std::move(bigOp));
        parts.push_back(std::move(expr));
        return LayoutNode::makeHorizontal(std::move(parts));
    }

    // sqrt(a,b) → ᵃ√b, sqrt(b) → √b
    if (name == "sqrt") {
        if (node.children.size() == 1) {
            auto content = astToLayout(*node.children[0], fontSize);
            return LayoutNode::makeRadical(std::move(content));
        } else if (node.children.size() == 2) {
            auto content = astToLayout(*node.children[1], fontSize);
            auto index = astToLayout(*node.children[0], fontSize * 0.6f);
            return LayoutNode::makeRadical(std::move(content), std::move(index));
        }
    }

    // log(a,b) → log_a(b), log(b) → ln(b)
    if (name == "log") {
        std::vector<std::unique_ptr<LayoutNode>> parts;
        // "log" text
        for (char c : std::string("log")) {
            parts.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
        }
        auto logText = LayoutNode::makeHorizontal(std::move(parts));

        if (node.children.size() == 1) {
            // Natural log → ln(b)
            std::vector<std::unique_ptr<LayoutNode>> lnParts;
            for (char c : std::string("ln")) {
                lnParts.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
            }
            auto lnText = LayoutNode::makeHorizontal(std::move(lnParts));
            auto arg = astToLayout(*node.children[0], fontSize);
            std::vector<std::unique_ptr<LayoutNode>> result;
            result.push_back(std::move(lnText));
            result.push_back(LayoutNode::makeDelimiter('(', ')', std::move(arg)));
            return LayoutNode::makeHorizontal(std::move(result));
        } else if (node.children.size() == 2) {
            auto sub = astToLayout(*node.children[0], fontSize * 0.7f);
            auto arg = astToLayout(*node.children[1], fontSize);
            auto baseSub = LayoutNode::makeSubSuperscript(std::move(logText), std::move(sub), nullptr);
            std::vector<std::unique_ptr<LayoutNode>> result;
            result.push_back(std::move(baseSub));
            result.push_back(LayoutNode::makeDelimiter('(', ')', std::move(arg)));
            return LayoutNode::makeHorizontal(std::move(result));
        }
    }

    // pow(a,b) → a^b
    if (name == "pow" && node.children.size() == 2) {
        auto base = astToLayout(*node.children[0], fontSize);
        auto exp = astToLayout(*node.children[1], fontSize * 0.7f);
        return LayoutNode::makeSubSuperscript(std::move(base), nullptr, std::move(exp));
    }

    // rand(a,b) → Rand(a,b)
    if (name == "rand" && node.children.size() == 2) {
        std::vector<std::unique_ptr<LayoutNode>> parts;
        for (char c : std::string("Rand")) {
            parts.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
        }
        auto min = astToLayout(*node.children[0], fontSize);
        auto max = astToLayout(*node.children[1], fontSize);
        std::vector<std::unique_ptr<LayoutNode>> args;
        args.push_back(std::move(min));
        args.push_back(LayoutNode::makeGlyph(',', fontSize));
        args.push_back(std::move(max));
        auto argsLayout = LayoutNode::makeHorizontal(std::move(args));
        auto content = LayoutNode::makeDelimiter('(', ')', std::move(argsLayout));
        parts.push_back(std::move(content));
        return LayoutNode::makeHorizontal(std::move(parts));
    }

    // Default: render as name(arg1, arg2, ...)
    std::vector<std::unique_ptr<LayoutNode>> result;
    for (char c : name) {
        result.push_back(LayoutNode::makeGlyph(static_cast<uint32_t>(c), fontSize));
    }
    result.push_back(LayoutNode::makeGlyph('(', fontSize));
    for (size_t i = 0; i < node.children.size(); i++) {
        if (i > 0) result.push_back(LayoutNode::makeGlyph(',', fontSize));
        result.push_back(astToLayout(*node.children[i], fontSize));
    }
    result.push_back(LayoutNode::makeGlyph(')', fontSize));
    return LayoutNode::makeHorizontal(std::move(result));
}

std::unique_ptr<LayoutNode> MathRenderer::convertIverson(const ASTNode& node, float fontSize) {
    auto predicate = astToLayout(*node.children[0], fontSize);
    auto I = LayoutNode::makeGlyph(FontManager::MathCodepoints::IVERSON, fontSize);
    auto content = LayoutNode::makeDelimiter('(', ')', std::move(predicate));
    std::vector<std::unique_ptr<LayoutNode>> parts;
    parts.push_back(std::move(I));
    parts.push_back(std::move(content));
    return LayoutNode::makeHorizontal(std::move(parts));
}

std::unique_ptr<LayoutNode> MathRenderer::convertInterval(const ASTNode& node, float fontSize) {
    uint32_t open = (node.leftBound == BoundType::CLOSED) ? '[' : '(';
    uint32_t close = (node.rightBound == BoundType::CLOSED) ? ']' : ')';

    auto low = astToLayout(*node.children[0], fontSize);
    auto high = astToLayout(*node.children[1], fontSize);

    std::vector<std::unique_ptr<LayoutNode>> parts;
    parts.push_back(LayoutNode::makeGlyph(open, fontSize));
    parts.push_back(std::move(low));
    parts.push_back(LayoutNode::makeGlyph(',', fontSize));
    parts.push_back(LayoutNode::makeSpace(fontSize * 0.15f));
    parts.push_back(std::move(high));
    parts.push_back(LayoutNode::makeGlyph(close, fontSize));

    return LayoutNode::makeHorizontal(std::move(parts));
}

std::unique_ptr<LayoutNode> MathRenderer::convertSetLiteral(const ASTNode& node, float fontSize) {
    std::vector<std::unique_ptr<LayoutNode>> parts;
    parts.push_back(LayoutNode::makeGlyph('{', fontSize));
    for (size_t i = 0; i < node.children.size(); i++) {
        if (i > 0) {
            parts.push_back(LayoutNode::makeGlyph(',', fontSize));
            parts.push_back(LayoutNode::makeSpace(fontSize * 0.15f));
        }
        parts.push_back(astToLayout(*node.children[i], fontSize));
    }
    parts.push_back(LayoutNode::makeGlyph('}', fontSize));
    return LayoutNode::makeHorizontal(std::move(parts));
}

} // namespace calc
