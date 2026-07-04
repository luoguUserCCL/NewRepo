#!/bin/bash
# Setup script — downloads third-party dependencies and fonts
# Run this once before building

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRD_PARTY="$SCRIPT_DIR/third_party"

echo "=== Setting up third-party dependencies ==="

# 1. Dear ImGui
IMGUI_DIR="$THIRD_PARTY/imgui"
if [ ! -d "$IMGUI_DIR" ]; then
    echo "Cloning Dear ImGui (v1.90.4)..."
    git clone --depth 1 --branch v1.90.4 https://github.com/ocornut/imgui.git "$IMGUI_DIR"
    echo "Dear ImGui cloned to $IMGUI_DIR"
else
    echo "Dear ImGui already present at $IMGUI_DIR"
fi

# 2. System dependencies
echo ""
echo "=== Checking system dependencies ==="

# Check for GMP + MPFR
if ! pkg-config --exists gmp 2>/dev/null; then
    echo "GMP not found. Install with:"
    echo "  Debian/Ubuntu: sudo apt install libgmp-dev"
    echo "  macOS: brew install gmp"
    echo "  Windows (MSYS2): pacman -S mingw-w64-x86_64-gmp"
fi

if ! pkg-config --exists mpfr 2>/dev/null; then
    echo "MPFR not found. Install with:"
    echo "  Debian/Ubuntu: sudo apt install libmpfr-dev"
    echo "  macOS: brew install mpfr"
    echo "  Windows (MSYS2): pacman -S mingw-w64-x86_64-mpfr"
fi

# Check for GLFW
if ! pkg-config --exists glfw3 2>/dev/null; then
    echo "GLFW not found. Install with:"
    echo "  Debian/Ubuntu: sudo apt install libglfw3-dev"
    echo "  macOS: brew install glfw"
fi

# 3. Font information
echo ""
echo "=== Font setup ==="
echo "Latin Modern Math is embedded in the binary — no external font files needed."
echo "For Chinese (CJK) text rendering, the app searches for Noto Sans SC on the system."
echo "If not found, install it:"
echo "  Debian/Ubuntu: sudo apt install fonts-noto-cjk"
echo "  macOS: brew install font-noto-sans-cjk-sc"
echo ""
echo "To update the embedded Latin Modern Math font:"
echo "  1. Place latinmodern-math.otf in fonts/"
echo "  2. Run: cd fonts && python3 -c \""
echo "       data = open('latinmodern-math.otf','rb').read()"
echo "       with open('../native-app/src/main/cpp/embedded/font_latinmodern_math.h','w') as f:"
echo "           f.write('// Auto-generated - DO NOT EDIT\\n')"
echo "           f.write('#ifndef EMBEDDED_FONT_LATINMODERN_MATH_H\\n')"
echo "           f.write('#define EMBEDDED_FONT_LATINMODERN_MATH_H\\n\\n')"
echo "           f.write('#include <cstddef>\\n#include <cstdint>\\n\\n')"
echo "           f.write('namespace calc { namespace embedded {\\n\\n')"
echo "           f.write(f'constexpr unsigned int latinmodern_math_len = {len(data)};\\n')"
echo "           f.write('constexpr unsigned char latinmodern_math[] = {\\n')"
echo "           for i in range(0, len(data), 12):"
echo "               chunk = data[i:i+12]"
echo "               f.write('  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\\n')"
echo "           f.write('};\\n\\n}} // namespace calc::embedded\\n\\n#endif\\n')"
echo "       \""

echo ""
echo "=== Setup complete ==="
echo ""
echo "To build with CMake:"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build -j$(nproc)"
echo ""
echo "To build with Gradle:"
echo "  ./gradlew build"
