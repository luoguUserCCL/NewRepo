#ifndef CALC_IMGUI_RENDERER_H
#define CALC_IMGUI_RENDERER_H

#include <calc/math_renderer.h>
#include <imgui.h>

namespace calc {

/**
 * Renders a layout tree to ImGui DrawList.
 * Walks the layout tree and emits draw commands for each node type.
 * Supports Latin Modern Math and Noto Sans SC fonts.
 */
class ImGuiRenderer {
public:
    /**
     * Render a layout tree at the given position.
     * @param node The root layout node to render
     * @param pos The top-left position to start rendering
     * @param drawList The ImGui draw list to add commands to
     * @param fontMath The font for Latin Modern Math
     * @param fontCJK The font for Noto Sans SC
     * @param color Text color (default: white)
     */
    static void render(const LayoutNode& node,
                       const Vec2& pos,
                       ImDrawList* drawList,
                       ImFont* fontMath = nullptr,
                       ImFont* fontCJK = nullptr,
                       uint32_t color = IM_COL32(255, 255, 255, 255));

private:
    static void renderGlyph(const LayoutNode& node, const Vec2& pos,
                             ImDrawList* drawList, ImFont* fontMath,
                             ImFont* fontCJK, uint32_t color);
    static void renderHorizontal(const LayoutNode& node, const Vec2& pos,
                                   ImDrawList* drawList, ImFont* fontMath,
                                   ImFont* fontCJK, uint32_t color);
    static void renderVertical(const LayoutNode& node, const Vec2& pos,
                                 ImDrawList* drawList, ImFont* fontMath,
                                 ImFont* fontCJK, uint32_t color);
    static void renderFraction(const LayoutNode& node, const Vec2& pos,
                                 ImDrawList* drawList, ImFont* fontMath,
                                 ImFont* fontCJK, uint32_t color);
    static void renderRadical(const LayoutNode& node, const Vec2& pos,
                                ImDrawList* drawList, ImFont* fontMath,
                                ImFont* fontCJK, uint32_t color);
    static void renderSubSuperscript(const LayoutNode& node, const Vec2& pos,
                                       ImDrawList* drawList, ImFont* fontMath,
                                       ImFont* fontCJK, uint32_t color);
    static void renderUnderOver(const LayoutNode& node, const Vec2& pos,
                                  ImDrawList* drawList, ImFont* fontMath,
                                  ImFont* fontCJK, uint32_t color);
    static void renderDelimiter(const LayoutNode& node, const Vec2& pos,
                                  ImDrawList* drawList, ImFont* fontMath,
                                  ImFont* fontCJK, uint32_t color);

    /// Choose the appropriate font for a glyph
    static ImFont* selectFont(const LayoutNode& node, ImFont* fontMath, ImFont* fontCJK);
};

} // namespace calc

#endif // CALC_IMGUI_RENDERER_H
