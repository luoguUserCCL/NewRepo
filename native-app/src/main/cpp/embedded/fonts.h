#ifndef CALC_EMBEDDED_FONTS_H
#define CALC_EMBEDDED_FONTS_H

#include <cstddef>
#include <cstdint>

namespace calc { namespace embedded {

// Latin Modern Math — the primary math rendering font.
// Embedded directly into the binary for zero external dependencies.
#include "font_latinmodern_math.h"

/**
 * Get the embedded Latin Modern Math font data.
 * @param size Output: size of the font data in bytes
 * @return Pointer to the font data (valid for the lifetime of the program)
 */
inline const uint8_t* getLatinModernMath(size_t& size) {
    size = latinmodern_math_len;
    return reinterpret_cast<const uint8_t*>(latinmodern_math);
}

/**
 * Try to find Noto Sans SC on the system.
 * Searches common installation paths on Linux, macOS, and Windows.
 * Returns the file path if found, empty string otherwise.
 */
struct SystemFontResult {
    bool found;
    std::string path;
};

SystemFontResult findNotoSansSC();

/**
 * Try to find any suitable CJK font on the system.
 * Used as a fallback if Noto Sans SC is not found.
 */
SystemFontResult findCJKFont();

}} // namespace calc::embedded

#endif // CALC_EMBEDDED_FONTS_H
