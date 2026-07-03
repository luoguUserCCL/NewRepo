#include <calc/font_manager.h>

namespace calc {

struct FontManager::Impl {
    bool initialized = false;
};

FontManager::FontManager() : impl_(std::make_unique<Impl>()) {}

FontManager::~FontManager() = default;

bool FontManager::initialize() {
    // Font initialization will use FreeType + ImGui font atlas
    // This is a placeholder for the initialization sequence
    impl_->initialized = true;
    return true;
}

const FontManager::GlyphInfo& FontManager::getGlyph(FontId font, uint32_t codepoint, float fontSize) {
    static GlyphInfo defaultGlyph;
    // Placeholder: actual implementation uses FreeType to load glyphs from embedded fonts
    defaultGlyph.codepoint = codepoint;
    defaultGlyph.advanceX = fontSize * 0.6f;
    defaultGlyph.width = fontSize * 0.5f;
    defaultGlyph.height = fontSize;
    defaultGlyph.bearingY = fontSize * 0.8f;
    return defaultGlyph;
}

float FontManager::getLineHeight(FontId font, float fontSize) const {
    return fontSize * 1.2f;
}

float FontManager::getAscent(FontId font, float fontSize) const {
    return fontSize * 0.8f;
}

float FontManager::getDescent(FontId font, float fontSize) const {
    return fontSize * 0.2f;
}

bool FontManager::hasGlyph(FontId font, uint32_t codepoint) const {
    // Latin Modern Math covers math symbols, Noto Sans SC covers CJK
    return true;
}

std::string FontManager::codepointToGlyphName(uint32_t cp) {
    // Map common codepoints to glyph names
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
        default: return "glyph_" + std::to_string(cp);
    }
}

} // namespace calc
