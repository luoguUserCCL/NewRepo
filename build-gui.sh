#!/usr/bin/env bash
# ============================================================================
# build-gui.sh — build GUI executables for sci-calc (Dear ImGui + GLFW + OpenGL)
#
# Produces:
#   dist/sci-calc-gui-linux-x86_64       — Linux GUI (GLFW + GLX)
#   dist/sci-calc-gui-windows-x86_64.exe — Windows GUI (-mwindows, GLFW + WGL)
#   dist/sci-calc-gui-macos-x86_64       — macOS GUI (GLFW + NSGL)
# ============================================================================
set -euo pipefail

ROOT="/home/z/sci-calc"
CACHE="$ROOT/.scicalc-cache"
MPFR_INC="$CACHE/native-deps/usr/include"
LOCAL_LIB="$CACHE/native-deps/usr/lib/x86_64-linux-gnu"
PUB="$ROOT/calc-core/src/main/cpp/public"
CORE_SRC="$ROOT/calc-core/src/main/cpp/src"
RENDER_SRC="$ROOT/render-engine/src/main/cpp/src"
APP_SRC="$ROOT/native-app/src/main/cpp"
IMGUI="$ROOT/third_party/imgui"
DIST="$ROOT/dist"
MAC_PREFIX="$CACHE/macdeps"
MINGW_SYSROOT="$CACHE/toolchains/mingw/usr/x86_64-w64-mingw32ucrt"
ZIG="$CACHE/toolchains/zig/zig"
JDK="$CACHE/jdk"
BUILDLOG="$ROOT/build-gui.log"
mkdir -p "$DIST"
: > "$BUILDLOG"

CXXFLAGS="-std=c++17 -O2 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-vexing-parse -Wno-deprecated-declarations"

# ImGui core sources
IMGUI_SRCS="$IMGUI/imgui.cpp $IMGUI/imgui_draw.cpp $IMGUI/imgui_tables.cpp $IMGUI/imgui_widgets.cpp"
# ImGui backends
IMGUI_BACKEND_SRCS="$IMGUI/backends/imgui_impl_glfw.cpp $IMGUI/backends/imgui_impl_opengl3.cpp"

# All app sources (calc-core + render-engine + native-app)
ALL_APP_SRCS="$CORE_SRC/*.cpp $RENDER_SRC/*.cpp $APP_SRC/main.cpp $APP_SRC/i18n.cpp $APP_SRC/embedded/fonts.cpp $IMGUI_SRCS $IMGUI_BACKEND_SRCS"

ok() { echo "  ✅ $*"; }
fail() { echo "  ❌ $*"; echo "  See $BUILDLOG"; exit 1; }

# ============================================================================
# 1. LINUX GUI (GLFW + GLX)
# ============================================================================
echo "=== 1/3 Linux GUI ==="
LINUX_INC="-I$PUB -I$MPFR_INC -I$APP_SRC -I$IMGUI -I$IMGUI/backends -I$CACHE/native-deps/usr/include"
LINUX_LIBS="-L$LOCAL_LIB -lglfw -lGL -lmpfr -lgmp -ldl -lpthread"

if g++ $CXXFLAGS $LINUX_INC \
    $APP_SRC/main.cpp $APP_SRC/i18n.cpp $APP_SRC/embedded/fonts.cpp \
    $CORE_SRC/*.cpp $RENDER_SRC/*.cpp \
    $IMGUI_SRCS $IMGUI_BACKEND_SRCS \
    $LINUX_LIBS -static-libgcc -static-libstdc++ \
    -o "$DIST/sci-calc-gui-linux-x86_64" >>"$BUILDLOG" 2>&1; then
    ok "Linux GUI: $(ls -lh $DIST/sci-calc-gui-linux-x86_64 | awk '{print $5}')"
    file "$DIST/sci-calc-gui-linux-x86_64" | grep -q "ELF" && ok "ELF format"
else
    fail "Linux GUI build failed"
fi

# ============================================================================
# 2. WINDOWS GUI (-mwindows, GLFW + WGL)
# ============================================================================
echo ""
echo "=== 2/3 Windows GUI (-mwindows) ==="
MINGW_GXX="$CACHE/toolchains/mingw/usr/bin/x86_64-w64-mingw32ucrt-g++"

# MinGW needs GLFW for Windows. Check if we have it; if not, download MSYS2 pkg.
WIN_GLFW_INC="$MINGW_SYSROOT/include"
WIN_GLFW_LIB="$MINGW_SYSROOT/lib"
if [[ ! -f "$WIN_GLFW_LIB/libglfw3.a" ]]; then
    echo "  Downloading GLFW for Windows (MSYS2)..."
    cd /tmp
    DL=/tmp/win-glfw
    rm -rf $DL; mkdir -p $DL; cd $DL
    URL=$(curl -fsSL "https://mirror.msys2.org/mingw/mingw64/" 2>/dev/null | \
          grep -oE 'mingw-w64-x86_64-glfw[^"]*pkg\.tar\.zst' | sort -V | tail -1)
    if [[ -n "$URL" ]]; then
        curl -fSL -o glfw.pkg.tar.zst "https://mirror.msys2.org/mingw/mingw64/$URL" 2>/dev/null
        /usr/bin/python3 - <<'PY'
import zstandard, tarfile
with open('glfw.pkg.tar.zst','rb') as f:
    dctx = zstandard.ZstdDecompressor()
    with dctx.stream_reader(f) as reader:
        with tarfile.open(fileobj=reader, mode='r|') as tar:
            tar.extractall('.')
PY
        cp mingw64/include/GLFW/glfw3.h "$MINGW_SYSROOT/include/GLFW/" 2>/dev/null || \
            { mkdir -p "$MINGW_SYSROOT/include/GLFW"; cp mingw64/include/GLFW/glfw3.h "$MINGW_SYSROOT/include/GLFW/"; }
        cp mingw64/lib/libglfw3.a "$MINGW_SYSROOT/lib/" 2>/dev/null
        cp mingw64/lib/libglfw3dll.a "$MINGW_SYSROOT/lib/" 2>/dev/null
        cp mingw64/bin/libglfw3.dll "$MINGW_SYSROOT/bin/" 2>/dev/null
        echo "  GLFW for Windows installed"
    fi
    cd "$ROOT"
fi

WIN_INC="-I$PUB -I$MINGW_SYSROOT/include -I$APP_SRC -I$IMGUI -I$IMGUI/backends"
# Windows links: glfw3, gdi32, opengl32 (all in MinGW sysroot)
WIN_LIBS="-L$MINGW_SYSROOT/lib -lglfw3 -lopengl32 -lgdi32 -lmpfr -lgmp"

if "$MINGW_GXX" $CXXFLAGS $WIN_INC -mwindows \
    $APP_SRC/main.cpp $APP_SRC/i18n.cpp $APP_SRC/embedded/fonts.cpp \
    $CORE_SRC/*.cpp $RENDER_SRC/*.cpp \
    $IMGUI_SRCS $IMGUI_BACKEND_SRCS \
    $WIN_LIBS -static -static-libgcc -static-libstdc++ \
    -o "$DIST/sci-calc-gui-windows-x86_64.exe" >>"$BUILDLOG" 2>&1; then
    ok "Windows GUI: $(ls -lh $DIST/sci-calc-gui-windows-x86_64.exe | awk '{print $5}')"
    file "$DIST/sci-calc-gui-windows-x86_64.exe" | grep -q "PE32+" && ok "PE32+ format"
else
    fail "Windows GUI build failed"
fi

# ============================================================================
# 3. macOS GUI (GLFW + NSGL)
# ============================================================================
echo ""
echo "=== 3/3 macOS GUI ==="

# macOS needs GLFW built for macOS. Build from source via Zig cross-compile.
MAC_GLFW="$CACHE/macglfw"
if [[ ! -f "$MAC_GLFW/lib/libglfw3.a" ]]; then
    echo "  Building GLFW for macOS from source..."
    cd "$CACHE/src"
    [[ -d glfw-3.4 ]] || {
        [[ -f glfw-3.4.zip ]] || curl -fsSL -o glfw-3.4.zip \
            "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip"
        unzip -q glfw-3.4.zip
    }
    mkdir -p glfw-3.4/build-mac && cd glfw-3.4/build-mac
    cmake -DCMAKE_C_COMPILER="$ZIG" \
          -DCMAKE_C_FLAGS="-target x86_64-macos.11.0" \
          -DCMAKE_INSTALL_PREFIX="$MAC_GLFW" \
          -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF \
          -DBUILD_SHARED_LIBS=OFF \
          .. >>"$BUILDLOG" 2>&1 2>&1 || true
    # cmake may not find zig properly; try manual compile of GLFW native files
    cd "$ROOT"
    # If cmake failed, manually compile GLFW native (cocoa, nsgl, etc.)
    if [[ ! -f "$MAC_GLFW/lib/libglfw3.a" ]]; then
        echo "  cmake failed, trying manual GLFW compile..."
        mkdir -p "$MAC_GLFW/lib" "$MAC_GLFW/include"
        cp -r "$CACHE/src/glfw-3.4/include/GLFW" "$MAC_GLFW/include/"
        GLFW_NATIVE="$CACHE/src/glfw-3.4/src"
        # Gather all GLFW source files (non-platform-specific + macOS-specific)
        GLFW_SRCS="$GLFW_NATIVE/init.c $GLFW_NATIVE/input.c $GLFW_NATIVE/monitor.c \
                   $GLFW_NATIVE/vulkan.c $GLFW_NATIVE/window.c $GLFW_NATIVE/context.c \
                   $GLFW_NATIVE/egl_context.c $GLFW_NATIVE/osmesa_context.c \
                   $GLFW_NATIVE/null_joystick.c $GLFW_NATIVE/null_init.c \
                   $CACHE/src/glfw-3.4/src/cocoa_init.m \
                   $CACHE/src/glfw-3.4/src/cocoa_joystick.m \
                   $CACHE/src/glfw-3.4/src/cocoa_monitor.m \
                   $CACHE/src/glfw-3.4/src/cocoa_window.m \
                   $CACHE/src/glfw-3.4/src/cocoa_time.c \
                   $CACHE/src/glfw-3.4/src/nsgl_context.m \
                   $CACHE/src/glfw-3.4/src/posix_thread.c \
                   $CACHE/src/glfw-3.4/src/posix_module.c"
        OBJDIR="$MAC_GLFW/obj"
        mkdir -p "$OBJDIR"
        OBJS=""
        for src in $GLFW_SRCS; do
            [[ -f "$src" ]] || continue
            base=$(basename "$src" | sed 's/\.[cm]$/\.o/')
            if "$ZIG" cc -target x86_64-macos.11.0 -O2 -fPIC \
                -I"$CACHE/src/glfw-3.4/include" \
                -D_GLFW_COCOA -D_GLFW_USE_OPENGL \
                -c "$src" -o "$OBJDIR/$base" 2>>"$BUILDLOG"; then
                OBJS="$OBJS $OBJDIR/$base"
            fi
        done
        if [[ -n "$OBJS" ]]; then
            "$ZIG" ar rcs "$MAC_GLFW/lib/libglfw3.a" $OBJS 2>>"$BUILDLOG"
        fi
    fi
fi
[[ -f "$MAC_GLFW/lib/libglfw3.a" ]] && ok "macOS GLFW ready" || { echo "  ⚠️ GLFW not built, will link without (may fail)"; }

MAC_INC="-I$PUB -I$MAC_PREFIX/include -I$MAC_GLFW/include -I$APP_SRC -I$IMGUI -I$IMGUI/backends"
# macOS frameworks: Cocoa, IOKit, CoreVideo, OpenGL (provided by Zig target)
MAC_LIBS="-L$MAC_PREFIX/lib -L$MAC_GLFW/lib -lglfw3 -lmpfr -lgmp \
          -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL"

if "$ZIG" c++ -target x86_64-macos.11.0 $CXXFLAGS $MAC_INC \
    $APP_SRC/main.cpp $APP_SRC/i18n.cpp $APP_SRC/embedded/fonts.cpp \
    $CORE_SRC/*.cpp $RENDER_SRC/*.cpp \
    $IMGUI_SRCS $IMGUI_BACKEND_SRCS \
    $MAC_LIBS \
    -o "$DIST/sci-calc-gui-macos-x86_64" >>"$BUILDLOG" 2>&1; then
    ok "macOS GUI: $(ls -lh $DIST/sci-calc-gui-macos-x86_64 | awk '{print $5}')"
    file "$DIST/sci-calc-gui-macos-x86_64" | grep -q "Mach-O" && ok "Mach-O format"
else
    fail "macOS GUI build failed"
fi

# ============================================================================
echo ""
echo "=== GUI ARTIFACTS ==="
ls -lh "$DIST/"sci-calc-gui-* 2>/dev/null
echo ""
echo "Build log: $BUILDLOG"