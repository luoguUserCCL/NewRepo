#ifndef CALC_MATH_RENDERER_H
#define CALC_MATH_RENDERER_H

#include <string>
#include <vector>
#include <memory>
#include <calc/expression.h>

namespace calc {

/**
 * A 2D point/size for layout computation.
 */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
};

/**
 * Bounding box for a rendered element.
 */
struct BBox {
    float width = 0.0f;
    float height = 0.0f;
    float ascent = 0.0f;    ///< Distance from baseline to top
    float descent = 0.0f;   ///< Distance from baseline to bottom
};

/**
 * Layout tree node types — each corresponds to a visual element.
 */
enum class LayoutType {
    GLYPH,           ///< Single character/glyph
    HORIZONTAL,      ///< Horizontal row of children
    VERTICAL,        ///< Vertical stack of children
    FRACTION,        ///< Fraction with numerator/denominator and rule line
    RADICAL,         ///< Radical sign with overline and optional index
    SUB_SUPERSCRIPT, ///< Base with subscript and/or superscript
    UNDER_OVER,      ///< Base with under/over decorations (Σ, Π)
    DELIMITER,       ///< Auto-sizing delimiters (|x|, ⌊x⌋, ⌈x⌉, brackets)
    ACCENT,          ///< Accent/hat over a symbol
    SPACE,           ///< Horizontal or vertical space
    COLOR            ///< Colored group
};

/**
 * A node in the layout tree — the intermediate representation
 * between AST and final ImGui drawing commands.
 */
struct LayoutNode {
    LayoutType type;

    // Glyph data
    uint32_t codepoint = 0;    ///< Unicode codepoint for GLYPH type
    float fontSize = 16.0f;    ///< Font size in pixels
    std::string fontFamily;    ///< "Latin Modern Math" or "Noto Sans SC"

    // Bounding box (computed during layout)
    BBox bbox;

    // Children
    std::vector<std::unique_ptr<LayoutNode>> children;

    // Type-specific data
    float ruleThickness = 1.0f;  ///< FRACTION: line thickness
    float spaceWidth = 0.0f;     ///< SPACE: width
    uint32_t openDelim = 0;      ///< DELIMITER: opening delimiter codepoint
    uint32_t closeDelim = 0;     ///< DELIMITER: closing delimiter codepoint

    // Factory methods
    static std::unique_ptr<LayoutNode> makeGlyph(uint32_t codepoint, float fontSize,
                                                   const std::string& font = "Latin Modern Math");
    static std::unique_ptr<LayoutNode> makeHorizontal(std::vector<std::unique_ptr<LayoutNode>> children);
    static std::unique_ptr<LayoutNode> makeVertical(std::vector<std::unique_ptr<LayoutNode>> children);
    static std::unique_ptr<LayoutNode> makeFraction(std::unique_ptr<LayoutNode> numerator,
                                                      std::unique_ptr<LayoutNode> denominator,
                                                      float ruleThickness = 1.0f);
    static std::unique_ptr<LayoutNode> makeRadical(std::unique_ptr<LayoutNode> radicand,
                                                     std::unique_ptr<LayoutNode> index = nullptr);
    static std::unique_ptr<LayoutNode> makeSubSuperscript(std::unique_ptr<LayoutNode> base,
                                                            std::unique_ptr<LayoutNode> sub = nullptr,
                                                            std::unique_ptr<LayoutNode> super = nullptr);
    static std::unique_ptr<LayoutNode> makeUnderOver(std::unique_ptr<LayoutNode> base,
                                                       std::unique_ptr<LayoutNode> under = nullptr,
                                                       std::unique_ptr<LayoutNode> over = nullptr);
    static std::unique_ptr<LayoutNode> makeDelimiter(uint32_t open, uint32_t close,
                                                       std::unique_ptr<LayoutNode> content);
    static std::unique_ptr<LayoutNode> makeSpace(float width);

    /// Compute layout (bounding boxes) for this subtree
    void computeLayout(float fontSize);
};

/**
 * Math renderer — converts AST to layout tree and then to ImGui draw commands.
 */
class MathRenderer {
public:
    MathRenderer();

    /// Convert an AST to a layout tree
    std::unique_ptr<LayoutNode> astToLayout(const ASTNode& ast, float fontSize = 16.0f);

    /// Compute layout for the entire tree
    void layout(std::unique_ptr<LayoutNode>& root);

    /// Get the total size of the rendered expression
    Vec2 getSize(const LayoutNode& root) const;

private:
    float defaultFontSize_;

    // AST-to-layout conversion methods for each node type
    std::unique_ptr<LayoutNode> convertNumber(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertVariable(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertBinaryOp(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertUnaryOp(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertFunction(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertIverson(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertInterval(const ASTNode& node, float fontSize);
    std::unique_ptr<LayoutNode> convertSetLiteral(const ASTNode& node, float fontSize);
};

} // namespace calc

#endif // CALC_MATH_RENDERER_H
