#!/bin/bash
# Setup script — downloads third-party dependencies
# Run this once before building

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRD_PARTY="$SCRIPT_DIR/third_party"

echo "=== Setting up third-party dependencies ==="

# 1. Dear ImGui
IMGUI_DIR="$THIRD_PARTY/imgui"
if [ ! -d "$IMGUI_DIR" ]; then
    echo "Cloning Dear ImGui..."
    git clone --depth 1 --branch v1.90.4 https://github.com/ocornut/imgui.git "$IMGUI_DIR"
    echo "Dear ImGui cloned to $IMGUI_DIR"
else
    echo "Dear ImGui already present at $IMGUI_DIR"
fi

# 2. Fonts
FONTS_DIR="$SCRIPT_DIR/fonts"
mkdir -p "$FONTS_DIR"

if [ ! -f "$FONTS_DIR/latinmodern-math.otf" ]; then
    echo "Downloading Latin Modern Math font..."
    curl -L -o "$FONTS_DIR/latinmodern-math.otf" \
        "https://github.com/aliftype/xits/releases/download/v1.200/XITSMath-Regular.otf" 2>/dev/null || \
    echo "Warning: Could not download Latin Modern Math. Please place latinmodern-math.otf in fonts/"
else
    echo "Latin Modern Math font already present"
fi

if [ ! -f "$FONTS_DIR/NotoSansSC-Regular.otf" ]; then
    echo "Downloading Noto Sans SC..."
    curl -L -o "$FONTS_DIR/NotoSansSC-Regular.otf" \
        "https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansSC-Regular.otf" 2>/dev/null || \
    echo "Warning: Could not download Noto Sans SC. Please place NotoSansSC-Regular.otf in fonts/"
else
    echo "Noto Sans SC font already present"
fi

echo ""
echo "=== Setup complete ==="
echo "To build with CMake:"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build"
echo ""
echo "To build with Gradle:"
echo "  ./gradlew build"
