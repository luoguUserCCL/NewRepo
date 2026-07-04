#include <calc/imgui_renderer.h>
#include <cmath>

namespace calc {

void ImGuiRenderer::render(const LayoutNode& node, const Vec2& pos,
                             ImDrawList* drawList, ImFont* fontMath,
                             ImFont* fontCJK, uint32_t color) {
    switch (node.type) {
        case LayoutType::GLYPH:
            renderGlyph(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::HORIZONTAL:
            renderHorizontal(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::VERTICAL:
            renderVertical(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::FRACTION:
            renderFraction(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::RADICAL:
            renderRadical(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::SUB_SUPERSCRIPT:
            renderSubSuperscript(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::UNDER_OVER:
            renderUnderOver(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::DELIMITER:
            renderDelimiter(node, pos, drawList, fontMath, fontCJK, color);
            break;
        case LayoutType::SPACE:
            // No drawing needed for space nodes
            break;
        default:
            break;
    }
}

void ImGuiRenderer::renderGlyph(const LayoutNode& node, const Vec2& pos,
                                  ImDrawList* drawList, ImFont* fontMath,
                                  ImFont* fontCJK, uint32_t color) {
    ImFont* font = selectFont(node, fontMath, fontCJK);
    if (!font) font = ImGui::GetFont(); // Fallback to default

    // Render the glyph as text
    char utf8[5] = {};
    if (node.codepoint < 0x80) {
        utf8[0] = static_cast<char>(node.codepoint);
    } else if (node.codepoint < 0x800) {
        utf8[0] = static_cast<char>(0xC0 | (node.codepoint >> 6));
        utf8[1] = static_cast<char>(0x80 | (node.codepoint & 0x3F));
    } else if (node.codepoint < 0x10000) {
        utf8[0] = static_cast<char>(0xE0 | (node.codepoint >> 12));
        utf8[1] = static_cast<char>(0x80 | ((node.codepoint >> 6) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | (node.codepoint & 0x3F));
    } else {
        utf8[0] = static_cast<char>(0xF0 | (node.codepoint >> 18));
        utf8[1] = static_cast<char>(0x80 | ((node.codepoint >> 12) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | ((node.codepoint >> 6) & 0x3F));
        utf8[3] = static_cast<char>(0x80 | (node.codepoint & 0x3F));
    }

    // Calculate baseline position (ascent from top)
    float baselineY = pos.y + node.bbox.ascent;
    drawList->AddText(font, node.fontSize, ImVec2(pos.x, baselineY - node.fontSize * 0.8f), color, utf8);
}

void ImGuiRenderer::renderHorizontal(const LayoutNode& node, const Vec2& pos,
                                       ImDrawList* drawList, ImFont* fontMath,
                                       ImFont* fontCJK, uint32_t color) {
    float x = pos.x;
    for (const auto& child : node.children) {
        // Align child to baseline
        float childY = pos.y + node.bbox.ascent - child->bbox.ascent;
        render(*child, Vec2(x, childY), drawList, fontMath, fontCJK, color);
        x += child->bbox.width;
    }
}

void ImGuiRenderer::renderVertical(const LayoutNode& node, const Vec2& pos,
                                     ImDrawList* drawList, ImFont* fontMath,
                                     ImFont* fontCJK, uint32_t color) {
    float y = pos.y;
    for (const auto& child : node.children) {
        float childX = pos.x + (node.bbox.width - child->bbox.width) / 2.0f;
        render(*child, Vec2(childX, y), drawList, fontMath, fontCJK, color);
        y += child->bbox.height;
    }
}

void ImGuiRenderer::renderFraction(const LayoutNode& node, const Vec2& pos,
                                     ImDrawList* drawList, ImFont* fontMath,
                                     ImFont* fontCJK, uint32_t color) {
    if (node.children.size() < 2) return;

    // Center the fraction horizontally
    float centerX = pos.x + node.bbox.width / 2.0f;

    // Render numerator (above the line)
    auto& num = node.children[0];
    float numX = centerX - num->bbox.width / 2.0f;
    float numY = pos.y;
    render(*num, Vec2(numX, numY), drawList, fontMath, fontCJK, color);

    // Draw the fraction line
    float lineY = pos.y + node.bbox.ascent;
    float lineWidth = std::max(num->bbox.width, node.children[1]->bbox.width) + 4.0f;
    drawList->AddLine(
        ImVec2(centerX - lineWidth / 2.0f, lineY),
        ImVec2(centerX + lineWidth / 2.0f, lineY),
        color, node.ruleThickness);

    // Render denominator (below the line)
    auto& den = node.children[1];
    float denX = centerX - den->bbox.width / 2.0f;
    float denY = lineY + node.ruleThickness / 2.0f;
    render(*den, Vec2(denX, denY), drawList, fontMath, fontCJK, color);
}

void ImGuiRenderer::renderRadical(const LayoutNode& node, const Vec2& pos,
                                    ImDrawList* drawList, ImFont* fontMath,
                                    ImFont* fontCJK, uint32_t color) {
    if (node.children.empty()) return;

    auto& radicand = node.children[0];
    float radicandX = pos.x + node.fontSize * 0.8f;
    render(*radicand, Vec2(radicandX, pos.y), drawList, fontMath, fontCJK, color);

    // Draw radical sign (√ shape)
    float baseY = pos.y + node.bbox.height;
    float midY = pos.y + node.bbox.height * 0.6f;
    float topY = pos.y;
    float rightX = radicandX + radicand->bbox.width;

    // √ shape: down-left, up-right, then horizontal overline
    ImVec2 pts[] = {
        ImVec2(pos.x + node.fontSize * 0.3f, midY),       // Left of check
        ImVec2(pos.x + node.fontSize * 0.5f, baseY),      // Bottom of check
        ImVec2(pos.x + node.fontSize * 0.7f, topY),        // Top of check
        ImVec2(rightX, topY),                                // End of overline
    };

    // Draw check mark
    drawList->AddLine(pts[0], pts[1], color, 1.5f);
    drawList->AddLine(pts[1], pts[2], color, 1.5f);
    // Draw overline
    drawList->AddLine(pts[2], pts[3], color, 1.5f);

    // Optional index (e.g., cube root)
    if (node.children.size() > 1) {
        auto& index = node.children[1];
        float indexX = pos.x;
        float indexY = pos.y + node.bbox.height * 0.4f;
        render(*index, Vec2(indexX, indexY), drawList, fontMath, fontCJK, color);
    }
}

void ImGuiRenderer::renderSubSuperscript(const LayoutNode& node, const Vec2& pos,
                                           ImDrawList* drawList, ImFont* fontMath,
                                           ImFont* fontCJK, uint32_t color) {
    if (node.children.empty()) return;

    // Render base
    auto& base = node.children[0];
    render(*base, pos, drawList, fontMath, fontCJK, color);

    float baseWidth = base->bbox.width;
    float scriptOffset = baseWidth * 0.05f; // Small gap

    // Render subscript (child 1)
    if (node.children.size() > 1) {
        auto& sub = node.children[1];
        float subX = pos.x + baseWidth + scriptOffset;
        float subY = pos.y + base->bbox.descent;
        render(*sub, Vec2(subX, subY), drawList, fontMath, fontCJK, color);
    }

    // Render superscript (child 2)
    if (node.children.size() > 2) {
        auto& sup = node.children[2];
        float supX = pos.x + baseWidth + scriptOffset;
        float supY = pos.y - sup->bbox.height * 0.3f;
        render(*sup, Vec2(supX, supY), drawList, fontMath, fontCJK, color);
    }
}

void ImGuiRenderer::renderUnderOver(const LayoutNode& node, const Vec2& pos,
                                      ImDrawList* drawList, ImFont* fontMath,
                                      ImFont* fontCJK, uint32_t color) {
    if (node.children.empty()) return;

    auto& base = node.children[0];
    float centerX = pos.x + node.bbox.width / 2.0f;
    float baseX = centerX - base->bbox.width / 2.0f;

    // Current Y position
    float currentY = pos.y;

    // Render over script first (child 2)
    if (node.children.size() > 2) {
        auto& over = node.children[2];
        float overX = centerX - over->bbox.width / 2.0f;
        render(*over, Vec2(overX, currentY), drawList, fontMath, fontCJK, color);
        currentY += over->bbox.height;
    }

    // Render base (child 0)
    render(*base, Vec2(baseX, currentY), drawList, fontMath, fontCJK, color);
    currentY += base->bbox.height;

    // Render under script (child 1)
    if (node.children.size() > 1) {
        auto& under = node.children[1];
        float underX = centerX - under->bbox.width / 2.0f;
        render(*under, Vec2(underX, currentY), drawList, fontMath, fontCJK, color);
    }
}

void ImGuiRenderer::renderDelimiter(const LayoutNode& node, const Vec2& pos,
                                      ImDrawList* drawList, ImFont* fontMath,
                                      ImFont* fontCJK, uint32_t color) {
    if (node.children.empty()) return;

    auto& content = node.children[0];
    float delimWidth = node.fontSize * 0.4f;

    // Draw left delimiter
    float delimHeight = content->bbox.height + node.fontSize * 0.2f;
    float delimTop = pos.y + node.bbox.ascent - content->bbox.ascent - node.fontSize * 0.1f;
    float delimBottom = delimTop + delimHeight;

    // Render left delimiter as a line or symbol
    if (node.openDelim == '|') {
        // Absolute value bars
        drawList->AddLine(
            ImVec2(pos.x + delimWidth * 0.5f, delimTop),
            ImVec2(pos.x + delimWidth * 0.5f, delimBottom),
            color, 2.0f);
    } else if (node.openDelim == '[' || node.openDelim == (uint32_t)0x230A) {
        // Left bracket or floor
        float x = pos.x + delimWidth * 0.3f;
        drawList->AddLine(ImVec2(x + delimWidth * 0.4f, delimTop),
                           ImVec2(x, delimTop), color, 1.5f);
        drawList->AddLine(ImVec2(x, delimTop),
                           ImVec2(x, delimBottom), color, 1.5f);
        drawList->AddLine(ImVec2(x, delimBottom),
                           ImVec2(x + delimWidth * 0.4f, delimBottom), color, 1.5f);
    } else if (node.openDelim == '(' || node.openDelim == (uint32_t)0x2308) {
        // Left paren or ceil — render as glyph
        char c = (node.openDelim < 128) ? static_cast<char>(node.openDelim) : '(';
        ImFont* font = fontMath ? fontMath : ImGui::GetFont();
        char str[2] = {c, '\0'};
        drawList->AddText(font, node.fontSize,
            ImVec2(pos.x, delimTop), color, str);
    }

    // Render content
    render(*content, Vec2(pos.x + delimWidth, pos.y), drawList, fontMath, fontCJK, color);

    // Draw right delimiter
    float rightX = pos.x + delimWidth + content->bbox.width;
    if (node.closeDelim == '|') {
        drawList->AddLine(
            ImVec2(rightX + delimWidth * 0.5f, delimTop),
            ImVec2(rightX + delimWidth * 0.5f, delimBottom),
            color, 2.0f);
    } else if (node.closeDelim == ']' || node.closeDelim == (uint32_t)0x230B) {
        float x = rightX + delimWidth * 0.3f;
        drawList->AddLine(ImVec2(x, delimTop),
                           ImVec2(x + delimWidth * 0.4f, delimTop), color, 1.5f);
        drawList->AddLine(ImVec2(x + delimWidth * 0.4f, delimTop),
                           ImVec2(x + delimWidth * 0.4f, delimBottom), color, 1.5f);
        drawList->AddLine(ImVec2(x + delimWidth * 0.4f, delimBottom),
                           ImVec2(x, delimBottom), color, 1.5f);
    } else if (node.closeDelim == ')' || node.closeDelim == (uint32_t)0x2309) {
        char c = (node.closeDelim < 128) ? static_cast<char>(node.closeDelim) : ')';
        ImFont* font = fontMath ? fontMath : ImGui::GetFont();
        char str[2] = {c, '\0'};
        drawList->AddText(font, node.fontSize,
            ImVec2(rightX, delimTop), color, str);
    }
}

ImFont* ImGuiRenderer::selectFont(const LayoutNode& node, ImFont* fontMath, ImFont* fontCJK) {
    // Use CJK font for codepoints in CJK ranges
    if (node.codepoint >= 0x4E00 && node.codepoint <= 0x9FFF) return fontCJK;
    if (node.codepoint >= 0x3400 && node.codepoint <= 0x4DBF) return fontCJK;
    if (node.codepoint >= 0x3000 && node.codepoint <= 0x303F) return fontCJK;

    // Use Latin Modern Math for everything else
    return fontMath;
}

} // namespace calc
