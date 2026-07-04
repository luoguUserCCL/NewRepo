#include <calc/font_manager.h>

namespace calc {

struct FontManager::Impl {
    bool initialized = false;
    // Font metrics cache (per font, per size)
    struct FontMetrics {
        float ascentRatio = 0.8f;
        float descentRatio = 0.2f;
        float lineHeightRatio = 1.2f;
        float avgGlyphWidthRatio = 0.6f;
    };
    FontMetrics metrics[3]; // indexed by FontId
};

FontManager::FontManager() : impl_(std::make_unique<Impl>()) {
    // Set up default font metrics
    // Latin Modern Math: taller ascent for math symbols
    impl_->metrics[0] = {0.82f, 0.22f, 1.25f, 0.55f};
    // Noto Sans SC Regular: typical CJK metrics
    impl_->metrics[1] = {0.88f, 0.12f, 1.15f, 0.6f};
    // Noto Sans SC Bold: similar to regular
    impl_->metrics[2] = {0.88f, 0.12f, 1.15f, 0.65f};
}

FontManager::~FontManager() = default;

bool FontManager::initialize() {
    // Font initialization happens at the application level via ImGui's font atlas.
    // The FontManager itself doesn't load fonts — it provides metric information
    // and glyph lookup for the layout algorithm.
    //
    // The actual font loading sequence is:
    // 1. main.cpp loads embedded Latin Modern Math via AddFontFromMemoryTTF()
    // 2. main.cpp loads system Noto Sans SC via AddFontFromFileTTF()
    // 3. ImGuiRenderer uses ImFont pointers from the atlas for rendering
    // 4. FontManager provides metric estimates for layout computation
    impl_->initialized = true;
    return true;
}

const FontManager::GlyphInfo& FontManager::getGlyph(FontId font, uint32_t codepoint, float fontSize) {
    static GlyphInfo defaultGlyph;
    int idx = static_cast<int>(font);
    if (idx < 0 || idx > 2) idx = 0;

    const auto& m = impl_->metrics[idx];
    defaultGlyph.codepoint = codepoint;
    defaultGlyph.advanceX = fontSize * m.avgGlyphWidthRatio;
    defaultGlyph.width = fontSize * m.avgGlyphWidthRatio;
    defaultGlyph.height = fontSize;
    defaultGlyph.bearingY = fontSize * m.ascentRatio;

    // Adjust metrics for specific character types
    // CJK characters are wider
    if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
        defaultGlyph.advanceX = fontSize * 1.0f;
        defaultGlyph.width = fontSize * 1.0f;
    }
    // Fullwidth forms
    if (codepoint >= 0xFF00 && codepoint <= 0xFFEF) {
        defaultGlyph.advanceX = fontSize * 1.0f;
        defaultGlyph.width = fontSize * 1.0f;
    }
    // CJK punctuation
    if (codepoint >= 0x3000 && codepoint <= 0x303F) {
        defaultGlyph.advanceX = fontSize * 0.8f;
        defaultGlyph.width = fontSize * 0.8f;
    }
    // Mathematical operators (wider in Latin Modern Math)
    if (codepoint >= 0x2200 && codepoint <= 0x22FF) {
        defaultGlyph.advanceX = fontSize * 0.7f;
        defaultGlyph.width = fontSize * 0.7f;
    }
    // Big operators (sum, product) are wider
    if (codepoint == MathCodepoints::SUM || codepoint == MathCodepoints::PRODUCT) {
        defaultGlyph.advanceX = fontSize * 1.0f;
        defaultGlyph.width = fontSize * 1.0f;
    }
    // Delimiters
    if (codepoint == MathCodepoints::FLOOR_LEFT || codepoint == MathCodepoints::FLOOR_RIGHT ||
        codepoint == MathCodepoints::CEIL_LEFT || codepoint == MathCodepoints::CEIL_RIGHT) {
        defaultGlyph.advanceX = fontSize * 0.35f;
        defaultGlyph.width = fontSize * 0.35f;
    }

    return defaultGlyph;
}

float FontManager::getLineHeight(FontId font, float fontSize) const {
    int idx = static_cast<int>(font);
    if (idx < 0 || idx > 2) idx = 0;
    return fontSize * impl_->metrics[idx].lineHeightRatio;
}

float FontManager::getAscent(FontId font, float fontSize) const {
    int idx = static_cast<int>(font);
    if (idx < 0 || idx > 2) idx = 0;
    return fontSize * impl_->metrics[idx].ascentRatio;
}

float FontManager::getDescent(FontId font, float fontSize) const {
    int idx = static_cast<int>(font);
    if (idx < 0 || idx > 2) idx = 0;
    return fontSize * impl_->metrics[idx].descentRatio;
}

bool FontManager::hasGlyph(FontId font, uint32_t codepoint) const {
    // Latin Modern Math: covers Latin, math symbols, and common operators
    if (font == FontId::LATIN_MODERN_MATH) {
        if (codepoint < 0x4E00) return true;  // All non-CJK
        // Some math symbols in higher planes
        if (codepoint >= 0x1D400 && codepoint <= 0x1D7FF) return true;
        return false;
    }
    // Noto Sans SC: covers CJK and also Latin as fallback
    if (font == FontId::NOTO_SANS_SC_REGULAR || font == FontId::NOTO_SANS_SC_BOLD) {
        return true;  // Noto Sans SC has comprehensive coverage
    }
    return false;
}

std::string FontManager::codepointToGlyphName(uint32_t cp) {
    // Map common codepoints to glyph names (used for debugging and rendering hints)
    switch (cp) {
        case MathCodepoints::SUM: return "summation";
        case MathCodepoints::PRODUCT: return "product";
        case MathCodepoints::FLOOR_LEFT: return "floorleft";
        case MathCodepoints::FLOOR_RIGHT: return "floorright";
        case MathCodepoints::CEIL_LEFT: return "ceilleft";
        case MathCodepoints::CEIL_RIGHT: return "ceilright";
        case MathCodepoints::IN: return "element";
        case MathCodepoints::CAP: return "intersection";
        case MathCodepoints::CUP: return "union";
        case MathCodepoints::AND: return "logicaland";
        case MathCodepoints::OR: return "logicalor";
        case MathCodepoints::NOT: return "logicalnot";
        case MathCodepoints::RADICAL: return "radical";
        case MathCodepoints::LEQ: return "lessequal";
        case MathCodepoints::GEQ: return "greaterequal";
        case MathCodepoints::NEQ: return "notequal";
        case MathCodepoints::IVERSON: return "doublestruck_I";
        case MathCodepoints::INFINITY: return "infinity";
        case MathCodepoints::PARTIAL_DIFF: return "partialdiff";
        case MathCodepoints::MOD: return "italic_m";
        default: return "glyph_" + std::to_string(cp);
    }
}

} // namespace calc
