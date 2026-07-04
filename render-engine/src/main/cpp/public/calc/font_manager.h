#ifndef CALC_FONT_MANAGER_H
#define CALC_FONT_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>

namespace calc {

/**
 * Font manager — loads and manages embedded fonts (Latin Modern Math, Noto Sans SC).
 * Handles glyph lookup, caching, and rendering preparation.
 */
class FontManager {
public:
    /// Font identifiers
    enum class FontId {
        LATIN_MODERN_MATH,   ///< Latin Modern Math (formula rendering)
        NOTO_SANS_SC_REGULAR,///< Noto Sans SC Regular (Chinese text)
        NOTO_SANS_SC_BOLD    ///< Noto Sans SC Bold (Chinese headings)
    };

    /// Glyph information for a single character
    struct GlyphInfo {
        uint32_t codepoint = 0;
        float advanceX = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float bearingX = 0.0f;
        float bearingY = 0.0f;
        int atlasX = 0;
        int atlasY = 0;
        int atlasW = 0;
        int atlasH = 0;
    };

    FontManager();
    ~FontManager();

    /// Initialize fonts (loads embedded font data)
    bool initialize();

    /// Get glyph info for a codepoint in a given font at given size
    const GlyphInfo& getGlyph(FontId font, uint32_t codepoint, float fontSize);

    /// Get the line height for a font at given size
    float getLineHeight(FontId font, float fontSize) const;

    /// Get the ascent for a font at given size
    float getAscent(FontId font, float fontSize) const;

    /// Get the descent for a font at given size
    float getDescent(FontId font, float fontSize) const;

    /// Check if a codepoint exists in a given font
    bool hasGlyph(FontId font, uint32_t codepoint) const;

    /// Map a Unicode codepoint to its glyph name (for special math symbols)
    static std::string codepointToGlyphName(uint32_t cp);

    /// Common math Unicode codepoints used in rendering
    struct MathCodepoints {
        static constexpr uint32_t SUM = 0x2211;         ///< Σ
        static constexpr uint32_t PRODUCT = 0x220F;     ///< Π
        static constexpr uint32_t FLOOR_LEFT = 0x230A;  ///< ⌊
        static constexpr uint32_t FLOOR_RIGHT = 0x230B; ///< ⌋
        static constexpr uint32_t CEIL_LEFT = 0x2308;   ///< ⌈
        static constexpr uint32_t CEIL_RIGHT = 0x2309;  ///< ⌉
        static constexpr uint32_t IN = 0x2208;           ///< ∈
        static constexpr uint32_t CAP = 0x2229;          ///< ∩
        static constexpr uint32_t CUP = 0x222A;          ///< ∪
        static constexpr uint32_t AND = 0x2227;          ///< ∧
        static constexpr uint32_t OR = 0x2228;           ///< ∨
        static constexpr uint32_t NOT = 0x00AC;          ///< ¬
        static constexpr uint32_t IVERSON = 0x1D546;     ///< 𝕀
        static constexpr uint32_t LEQ = 0x2264;          ///< ≤
        static constexpr uint32_t GEQ = 0x2265;          ///< ≥
        static constexpr uint32_t NEQ = 0x2260;          ///< ≠
        static constexpr uint32_t MOD = 0x1D45A;         ///< 𝑚 (italic m for mod)
        static constexpr uint32_t RADICAL = 0x221A;      ///< √
        static constexpr uint32_t PARTIAL_DIFF = 0x2202; ///< ∂
        static constexpr uint32_t INFINITY = 0x221E;     ///< ∞
    };

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace calc

#endif // CALC_FONT_MANAGER_H
