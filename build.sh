#!/bin/bash
# =====================================================================
# build.sh — Full cross-platform build for scientific-calculator
# Builds 4 target variants:
#   1. Linux x86_64 native   (system GCC + shared GLFW)
#   2. Windows x86_64        (MinGW-w64 cross-compile, static)
#   3. macOS aarch64         (Zig c++ cross-compile, static)
#   4. JAR                   (JNI bridge + Java wrapper, 3-platform)
# =====================================================================
set -euo pipefail

# ==================== Project Paths ====================
ROOT="$(cd "$(dirname "$0")" && pwd)"
LOCAL_PREFIX="$ROOT/.local"
THIRD_PARTY="$ROOT/third_party"
IMGUI_DIR="$THIRD_PARTY/imgui"
GLAD_DIR="$THIRD_PARTY/glad"
BUILD_DIR="$ROOT/build"
MACOS_SDK="$LOCAL_PREFIX/MacOSX14.5.sdk"

# ==================== Source Lists ====================
CALC_CORE_SOURCES=(
    calc-core/src/main/cpp/src/big_decimal.cpp
    calc-core/src/main/cpp/src/expression.cpp
    calc-core/src/main/cpp/src/expression_parser.cpp
    calc-core/src/main/cpp/src/expression_eval.cpp
    calc-core/src/main/cpp/src/functions.cpp
    calc-core/src/main/cpp/src/function_registry.cpp
    calc-core/src/main/cpp/src/variable_store.cpp
    calc-core/src/main/cpp/src/set_operations.cpp
    calc-core/src/main/cpp/src/number_format.cpp
    calc-core/src/main/cpp/src/base_converter.cpp
    calc-core/src/main/cpp/src/crypto_random.cpp
)

RENDER_ENGINE_SOURCES=(
    render-engine/src/main/cpp/src/math_renderer.cpp
    render-engine/src/main/cpp/src/font_manager.cpp
    render-engine/src/main/cpp/src/imgui_renderer.cpp
)

IMGUI_SOURCES=(
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

NATIVE_APP_SOURCES=(
    native-app/src/main/cpp/main.cpp
    native-app/src/main/cpp/i18n.cpp
    native-app/src/main/cpp/embedded/fonts.cpp
)

JNI_BRIDGE_SOURCES=(
    jni-bridge/src/main/cpp/src/calc_jni.cpp
)

# ==================== Helper Functions ====================
info() { echo -e "\033[1;34m[INFO]\033[0m $*"; }
ok()   { echo -e "\033[1;32m[ OK ]\033[0m $*"; }
fail() { echo -e "\033[1;31m[FAIL]\033[0m $*"; exit 1; }

abs_sources() {
    local result=()
    for s in "$@"; do
        if [[ "$s" == /* ]]; then result+=("$s"); else result+=("$ROOT/$s"); fi
    done
    echo "${result[@]}"
}

# ==================== Build Linux ====================
build_linux() {
    info "========== Building Linux x86_64 Native =========="
    local OUT="$BUILD_DIR/linux"
    mkdir -p "$OUT/obj"

    local CXX="g++"
    local GMP_INC="$LOCAL_PREFIX/include"
    local MPFR_INC="$LOCAL_PREFIX/include"
    local CALC_CORE_PUBLIC="$ROOT/calc-core/src/main/cpp/public"
    local RENDER_ENGINE_PUBLIC="$ROOT/render-engine/src/main/cpp/public"
    local NATIVE_APP_DIR="$ROOT/native-app/src/main/cpp"
    local EMBEDDED_DIR="$NATIVE_APP_DIR/embedded"

    local INCLUDES="-I$GMP_INC -I$MPFR_INC -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$GLAD_DIR/include -I$NATIVE_APP_DIR -I$EMBEDDED_DIR -I$LOCAL_PREFIX/include"
    local FLAGS="-std=c++17 -Wall -Wextra -O2 -fPIC -DPLATFORM_LINUX -DEMBED_FONTS -DIMGUI_IMPL_OPENGL_LOADER_GLAD"

    # Compile calc-core
    info "  Compiling calc-core..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        CORE_OBJS+=("$obj")
    done
    ar rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core built"

    # Compile render-engine
    info "  Compiling render-engine..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        RENDER_OBJS+=("$obj")
    done
    ar rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine built"

    # Compile ImGui
    info "  Compiling Dear ImGui..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        IMGUI_OBJS+=("$obj")
    done
    # Also compile GLAD
    $CXX $FLAGS $INCLUDES -c "$GLAD_DIR/src/gl.c" -o "$OUT/obj/glad_gl.o" || fail "Failed to compile glad"
    ok "  Dear ImGui + GLAD built"

    # Compile native-app
    info "  Compiling native-app..."
    local APP_OBJS=()
    for src in $(abs_sources "${NATIVE_APP_SOURCES[@]}"); do
        local obj="$OUT/obj/app_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        APP_OBJS+=("$obj")
    done
    ok "  native-app compiled"

    # Link
    info "  Linking scientific-calculator..."
    $CXX -o "$OUT/scientific-calculator" \
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" "$OUT/obj/glad_gl.o" \
        -L"$OUT" -L"$LOCAL_PREFIX/lib" \
        -lrender-engine -lcalc-core \
        -lmpfr -lgmp -lglfw -lGL -ldl -lpthread -lX11 -lXrandr -lXinerama -lXcursor -lXi \
        || fail "Linking failed"

    ok "Linux build: $OUT/scientific-calculator"
    file "$OUT/scientific-calculator"
}

# ==================== Build Windows ====================
build_windows() {
    info "========== Building Windows x86_64 (MinGW-w64) =========="
    local OUT="$BUILD_DIR/windows"
    mkdir -p "$OUT/obj"

    export PATH="$LOCAL_PREFIX/bin:$PATH"
    local CXX="x86_64-w64-mingw32-g++"
    local CC="x86_64-w64-mingw32-gcc"
    local AR="x86_64-w64-mingw32-ar"
    local MINGW_B="-B$LOCAL_PREFIX/bin/"
    local MINGW_SYSROOT="$LOCAL_PREFIX/x86_64-w64-mingw32"
    local MINGW_EXTRA_INC="-I$LOCAL_PREFIX/share/mingw-w64/include"

    local CALC_CORE_PUBLIC="$ROOT/calc-core/src/main/cpp/public"
    local RENDER_ENGINE_PUBLIC="$ROOT/render-engine/src/main/cpp/public"
    local NATIVE_APP_DIR="$ROOT/native-app/src/main/cpp"
    local EMBEDDED_DIR="$NATIVE_APP_DIR/embedded"

    local INCLUDES="$MINGW_EXTRA_INC -I$LOCAL_PREFIX/mingw/include -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$GLAD_DIR/include -I$NATIVE_APP_DIR -I$EMBEDDED_DIR -I$MINGW_SYSROOT/include"
    local FLAGS="-std=c++17 -Wall -Wextra -O2 -DPLATFORM_WINDOWS -DWIN32_LEAN_AND_MEAN -D__USE_MINGW_ANSI_STDIO=1 -DEMBED_FONTS -DIMGUI_IMPL_OPENGL_LOADER_GLAD -pipe $MINGW_B"

    # Compile calc-core
    info "  Compiling calc-core for Windows..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        CORE_OBJS+=("$obj")
    done
    $AR rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core built for Windows"

    # Compile render-engine
    info "  Compiling render-engine for Windows..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        RENDER_OBJS+=("$obj")
    done
    $AR rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine built for Windows"

    # Compile ImGui
    info "  Compiling Dear ImGui for Windows..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        IMGUI_OBJS+=("$obj")
    done
    $CC $FLAGS $INCLUDES -c "$GLAD_DIR/src/gl.c" -o "$OUT/obj/glad_gl.o" || fail "Failed to compile glad for Windows"
    ok "  Dear ImGui + GLAD built for Windows"

    # Compile native-app
    info "  Compiling native-app for Windows..."
    local APP_OBJS=()
    for src in $(abs_sources "${NATIVE_APP_SOURCES[@]}"); do
        local obj="$OUT/obj/app_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        APP_OBJS+=("$obj")
    done
    ok "  native-app compiled for Windows"

    # Link — static linking for Windows
    info "  Linking scientific-calculator.exe..."
    $CXX -o "$OUT/scientific-calculator.exe" \
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" "$OUT/obj/glad_gl.o" \
        -L"$OUT" -L"$LOCAL_PREFIX/mingw/lib" \
        -lrender-engine -lcalc-core -lglfw3 -lmpfr -lgmp \
        -lopengl32 -lgdi32 -lshell32 -luser32 -lkernel32 -ladvapi32 -limm32 -lbcrypt \
        -static -static-libgcc -static-libstdc++ \
        || fail "Linking Windows failed"

    ok "Windows build: $OUT/scientific-calculator.exe"
    file "$OUT/scientific-calculator.exe"
}

# ==================== Build macOS ====================
build_macos() {
    info "========== Building macOS aarch64 (Zig c++) =========="
    local OUT="$BUILD_DIR/macos"
    mkdir -p "$OUT/obj"

    local ZIG="$LOCAL_PREFIX/bin/zig"
    local CXX="$ZIG c++"
    local CC="$ZIG cc"
    local AR="$ZIG ar"
    local TARGET="-target aarch64-macos"

    local CALC_CORE_PUBLIC="$ROOT/calc-core/src/main/cpp/public"
    local RENDER_ENGINE_PUBLIC="$ROOT/render-engine/src/main/cpp/public"
    local NATIVE_APP_DIR="$ROOT/native-app/src/main/cpp"
    local EMBEDDED_DIR="$NATIVE_APP_DIR/embedded"

    local INCLUDES="-I$LOCAL_PREFIX/macos/include -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$GLAD_DIR/include -I$NATIVE_APP_DIR -I$EMBEDDED_DIR -isystem $MACOS_SDK/usr/include -iframework $MACOS_SDK/System/Library/Frameworks"
    local FLAGS="$TARGET -std=c++17 -Wall -Wextra -O2 -DPLATFORM_MACOS -DEMBED_FONTS -DIMGUI_IMPL_OPENGL_LOADER_GLAD -fobjc-arc"

    # Compile calc-core
    info "  Compiling calc-core for macOS..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        CORE_OBJS+=("$obj")
    done
    $AR rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core built for macOS"

    # Compile render-engine
    info "  Compiling render-engine for macOS..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        RENDER_OBJS+=("$obj")
    done
    $AR rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine built for macOS"

    # Compile ImGui
    info "  Compiling Dear ImGui for macOS..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        IMGUI_OBJS+=("$obj")
    done
    $CC $TARGET -O2 $INCLUDES -c "$GLAD_DIR/src/gl.c" -o "$OUT/obj/glad_gl.o" || fail "Failed to compile glad for macOS"
    ok "  Dear ImGui + GLAD built for macOS"

    # Compile native-app
    info "  Compiling native-app for macOS..."
    local APP_OBJS=()
    for src in $(abs_sources "${NATIVE_APP_SOURCES[@]}"); do
        local obj="$OUT/obj/app_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        APP_OBJS+=("$obj")
    done
    ok "  native-app compiled for macOS"

    # Link — use object files directly to avoid archive format issues with Zig's linker
    info "  Linking scientific-calculator for macOS..."
    local FWFLAGS="-F$MACOS_SDK/System/Library/Frameworks"
    local GLFW_OBJS=()
    for obj in "$LOCAL_PREFIX/lib/macos-arm64"/glfw_*.o; do
        GLFW_OBJS+=("$obj")
    done
    $CXX $TARGET -o "$OUT/scientific-calculator" \
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" "$OUT/obj/glad_gl.o" \
        "${CORE_OBJS[@]}" "${RENDER_OBJS[@]}" \
        "${GLFW_OBJS[@]}" \
        -L"$LOCAL_PREFIX/macos/lib" -L"$MACOS_SDK/usr/lib" \
        -lmpfr -lgmp -lobjc \
        $FWFLAGS -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework Carbon \
        -framework Foundation -framework AppKit \
        -lc++ \
        || fail "Linking macOS failed"

    ok "macOS build: $OUT/scientific-calculator"
    file "$OUT/scientific-calculator"
}

# ==================== Build JAR ====================
build_jar() {
    info "========== Building JAR (Java Wrapper + JNI Bridge) =========="
    local OUT="$BUILD_DIR/jar"
    mkdir -p "$OUT"

    export LD_LIBRARY_PATH=/usr/lib/jvm/java-21-openjdk-amd64/lib:${LD_LIBRARY_PATH:-}
    local JAVAC="/home/z/my-project/NewRepo/.local/bin/javac"
    local JAR_TOOL="/home/z/my-project/NewRepo/.local/bin/jar"
    local CALC_CORE_PUBLIC="$ROOT/calc-core/src/main/cpp/public"
    local RENDER_ENGINE_PUBLIC="$ROOT/render-engine/src/main/cpp/public"

    # === Build JNI .so for Linux ===
    info "  Building JNI bridge for Linux..."
    local CXX="g++"
    local JNI_INCLUDES="-I$LOCAL_PREFIX/jdk-include -I$LOCAL_PREFIX/jdk-include/linux"
    local INCLUDES="$JNI_INCLUDES -I$LOCAL_PREFIX/include -I$CALC_CORE_PUBLIC"
    local FLAGS="-std=c++17 -O2 -fPIC"

    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/linux_core_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI"
        CORE_OBJS+=("$obj")
    done

    for src in $(abs_sources "${JNI_BRIDGE_SOURCES[@]}"); do
        local obj="$OUT/obj/linux_bridge_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI"
        $CXX -shared -o "$OUT/libcalc-bridge-linux-x86_64.so" \
            "$obj" "${CORE_OBJS[@]}" \
            -L"$LOCAL_PREFIX/lib" -lmpfr -lgmp -ldl \
            || fail "Failed to link JNI bridge for Linux"
    done
    ok "  JNI bridge Linux: libcalc-bridge-linux-x86_64.so"

    # === Build JNI .dll for Windows ===
    info "  Building JNI bridge for Windows..."
    export PATH="$LOCAL_PREFIX/bin:$PATH"
    local MINGW_CXX="x86_64-w64-mingw32-g++"
    local MINGW_CC="x86_64-w64-mingw32-gcc"
    local MINGW_AR="x86_64-w64-mingw32-ar"
    local MINGW_B="-B$LOCAL_PREFIX/bin/"
    local MINGW_EXTRA_INC="-I$LOCAL_PREFIX/share/mingw-w64/include"
    local WIN_JNI_INCLUDES="-I$LOCAL_PREFIX/jdk-include -I$LOCAL_PREFIX/jdk-include/win32"
    local WIN_INCLUDES="$WIN_JNI_INCLUDES $MINGW_EXTRA_INC -I$LOCAL_PREFIX/mingw/include -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$LOCAL_PREFIX/x86_64-w64-mingw32/include"
    local WIN_FLAGS="-std=c++17 -O2 -D__USE_MINGW_ANSI_STDIO=1 -DWIN32_LEAN_AND_MEAN $MINGW_B"

    local WIN_CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/win_core_$(basename "$src" .cpp).o"
        $MINGW_CXX $WIN_FLAGS $WIN_INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI Windows"
        WIN_CORE_OBJS+=("$obj")
    done

    for src in $(abs_sources "${JNI_BRIDGE_SOURCES[@]}"); do
        local obj="$OUT/obj/win_bridge_$(basename "$src" .cpp).o"
        $MINGW_CXX $WIN_FLAGS $WIN_INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI Windows"
        $MINGW_CXX -shared -o "$OUT/calc-bridge-windows-x86_64.dll" \
            "$obj" "${WIN_CORE_OBJS[@]}" \
            -L"$LOCAL_PREFIX/mingw/lib" -lmpfr -lgmp \
            -static-libgcc -static-libstdc++ \
            || fail "Failed to link JNI bridge for Windows"
    done
    ok "  JNI bridge Windows: calc-bridge-windows-x86_64.dll"

    # === Build JNI .dylib for macOS ===
    info "  Building JNI bridge for macOS..."
    local ZIG="$LOCAL_PREFIX/bin/zig"
    local MAC_CXX="$ZIG c++"
    local MAC_CC="$ZIG cc"
    local MAC_TARGET="-target aarch64-macos"
    local MAC_JNI_INCLUDES="-I$LOCAL_PREFIX/jdk-include -I$LOCAL_PREFIX/jdk-include/darwin"
    local MAC_INCLUDES="$MAC_JNI_INCLUDES -I$LOCAL_PREFIX/macos/include -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -isystem $MACOS_SDK/usr/include -iframework $MACOS_SDK/System/Library/Frameworks"
    local MAC_FLAGS="$MAC_TARGET -std=c++17 -O2 -fPIC"

    local MAC_CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/mac_core_$(basename "$src" .cpp).o"
        $MAC_CXX $MAC_FLAGS $MAC_INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI macOS"
        MAC_CORE_OBJS+=("$obj")
    done

    for src in $(abs_sources "${JNI_BRIDGE_SOURCES[@]}"); do
        local obj="$OUT/obj/mac_bridge_$(basename "$src" .cpp).o"
        $MAC_CXX $MAC_FLAGS $MAC_INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI macOS"
        $MAC_CXX $MAC_TARGET -shared -o "$OUT/libcalc-bridge-macos-arm64.dylib" \
            "$obj" "${MAC_CORE_OBJS[@]}" \
            -L"$LOCAL_PREFIX/macos/lib" -L"$MACOS_SDK/usr/lib" -lgmp -lmpfr -lobjc \
            -lc++ \
            || fail "Failed to link JNI bridge for macOS"
    done
    ok "  JNI bridge macOS: libcalc-bridge-macos-arm64.dylib"

    # === Compile Java wrapper ===
    info "  Compiling Java wrapper classes..."
    local JAVA_SRC="$ROOT/java-wrapper/src/main/java"
    mkdir -p "$OUT/classes"
    $JAVAC -d "$OUT/classes" \
        "$JAVA_SRC/com/calc/CalcEngine.java" \
        "$JAVA_SRC/com/calc/CalcResult.java" \
        "$JAVA_SRC/com/calc/Calculator.java" \
        "$JAVA_SRC/com/calc/NativeLoader.java" \
        2>&1 || fail "Java compilation failed"
    ok "  Java classes compiled"

    # === Package JAR ===
    info "  Packaging JAR with all native libraries..."
    mkdir -p "$OUT/classes/native/linux-x86_64" "$OUT/classes/native/windows-x86_64" "$OUT/classes/native/macos-arm64"
    cp "$OUT/libcalc-bridge-linux-x86_64.so" "$OUT/classes/native/linux-x86_64/"
    cp "$OUT/calc-bridge-windows-x86_64.dll" "$OUT/classes/native/windows-x86_64/"
    cp "$OUT/libcalc-bridge-macos-arm64.dylib" "$OUT/classes/native/macos-arm64/"
    
    cd "$OUT/classes"
    $JAR_TOOL cf "$OUT/scientific-calculator.jar" \
        -C . . \
        || fail "JAR packaging failed"
    cd "$ROOT"
    
    ok "  JAR: $OUT/scientific-calculator.jar"
    info "  JAR contents:"
    $JAR_TOOL tf "$OUT/scientific-calculator.jar" | grep -E "native|calc" | head -10
}

# ==================== Main ====================
TARGETS=()
if [[ $# -eq 0 ]]; then
    TARGETS=(linux windows macos jar)
else
    TARGETS=("$@")
fi

info "Building targets: ${TARGETS[*]}"
info "Project root: $ROOT"

for target in "${TARGETS[@]}"; do
    case "$target" in
        linux)   build_linux ;;
        windows) build_windows ;;
        macos)   build_macos ;;
        jar)     build_jar ;;
        all)     build_linux; build_windows; build_macos; build_jar ;;
        *)       fail "Unknown target: $target (use: linux, windows, macos, jar, all)" ;;
    esac
    echo ""
done

info "========== Build Summary =========="
for target in "${TARGETS[@]}"; do
    case "$target" in
        linux)   ls -lh "$BUILD_DIR/linux/scientific-calculator" 2>/dev/null ;;
        windows) ls -lh "$BUILD_DIR/windows/scientific-calculator.exe" 2>/dev/null ;;
        macos)   ls -lh "$BUILD_DIR/macos/scientific-calculator" 2>/dev/null ;;
        jar)     ls -lh "$BUILD_DIR/jar/scientific-calculator.jar" 2>/dev/null ;;
        all)
            ls -lh "$BUILD_DIR/linux/scientific-calculator" 2>/dev/null
            ls -lh "$BUILD_DIR/windows/scientific-calculator.exe" 2>/dev/null
            ls -lh "$BUILD_DIR/macos/scientific-calculator" 2>/dev/null
            ls -lh "$BUILD_DIR/jar/scientific-calculator.jar" 2>/dev/null
            ;;
    esac
done

ok "Build complete!"
