#!/bin/bash
# =====================================================================
# build.sh — Pure Gradle-oriented build script for scientific-calculator
# Builds 4 target variants:
#   1. Linux x86_64 native   (system GCC)
#   2. Windows x86_64        (MinGW-w64 cross-compile)
#   3. macOS x86_64          (Zig c++ cross-compile)
#   4. JAR                   (JNI bridge + Java wrapper)
#
# This script reads the same source/layout structure defined in the
# Gradle build.gradle files but invokes the compilers directly since
# Gradle's cpp-application plugin has limited cross-compile support.
# =====================================================================

set -euo pipefail

# ==================== Project Paths ====================
ROOT="$(cd "$(dirname "$0")" && pwd)"
LOCAL_PREFIX="$ROOT/.local"
THIRD_PARTY="$ROOT/third_party"
IMGUI_DIR="$THIRD_PARTY/imgui"
BUILD_DIR="$ROOT/build"

# ==================== Compiler Settings ====================
CXX_COMMON="-std=c++17 -Wall -Wextra -O2 -fPIC"

# Include paths
GMP_INC="/usr/include/x86_64-linux-gnu"
MPFR_INC="$LOCAL_PREFIX/include"
LOCAL_INC="$LOCAL_PREFIX/include"
CALC_CORE_PUBLIC="$ROOT/calc-core/src/main/cpp/public"
RENDER_ENGINE_PUBLIC="$ROOT/render-engine/src/main/cpp/public"
JNI_BRIDGE_PUBLIC="$ROOT/jni-bridge/src/main/cpp/public"
NATIVE_APP_DIR="$ROOT/native-app/src/main/cpp"
EMBEDDED_DIR="$NATIVE_APP_DIR/embedded"

# ==================== Source Files ====================
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

# Convert source paths to absolute
abs_sources() {
    local arr=("$@")
    local result=()
    for s in "${arr[@]}"; do
        if [[ "$s" == /* ]]; then
            result+=("$s")
        else
            result+=("$ROOT/$s")
        fi
    done
    echo "${result[@]}"
}

# ==================== Build Functions ====================

build_linux() {
    info "========== Building Linux x86_64 Native =========="
    local OUT="$BUILD_DIR/linux"
    mkdir -p "$OUT/obj"

    local CXX="g++"
    local INCLUDES="-I$GMP_INC -I$MPFR_INC -I$LOCAL_INC -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$NATIVE_APP_DIR -I$EMBEDDED_DIR"
    local FLAGS="$CXX_COMMON -DPLATFORM_LINUX -DEMBED_FONTS"

    # Compile calc-core (static library)
    info "  Compiling calc-core..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        CORE_OBJS+=("$obj")
    done
    ar rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core static library built"

    # Compile render-engine (static library)
    info "  Compiling render-engine..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        RENDER_OBJS+=("$obj")
    done
    ar rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine static library built"

    # Compile ImGui sources
    info "  Compiling Dear ImGui..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src"
        IMGUI_OBJS+=("$obj")
    done
    ok "  Dear ImGui compiled"

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
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" \
        -L"$OUT" -L"$LOCAL_PREFIX/lib" -L/usr/lib/x86_64-linux-gnu \
        -lrender-engine -lcalc-core \
        -lmpfr -lgmp -lglfw -lGL -ldl -lpthread \
        || fail "Linking failed"

    ok "Linux build: $OUT/scientific-calculator"
    file "$OUT/scientific-calculator"
}

build_windows() {
    info "========== Building Windows x86_64 (MinGW-w64) =========="
    local OUT="$BUILD_DIR/windows"
    mkdir -p "$OUT/obj"

    # MinGW paths
    local MINGW_BIN="$LOCAL_PREFIX/bin"
    local MINGW_GPP="$MINGW_BIN/x86_64-w64-mingw32-g++"
    local MINGW_AR="$MINGW_BIN/x86_64-w64-mingw32-ar"
    local MINGW_SYSROOT="$LOCAL_PREFIX/x86_64-w64-mingw32"

    if [[ ! -x "$MINGW_GPP" ]]; then
        fail "MinGW-w64 not found at $MINGW_GPP"
    fi

    export PATH="$MINGW_BIN:$PATH"
    local CXX="x86_64-w64-mingw32-g++"
    local AR="x86_64-w64-mingw32-ar"

    # For Windows cross-compile, we won't have GMP/MPFR/GLFW built for MinGW
    # So we compile what we can (calc-core + render-engine without external deps)
    # and note that a full Windows build requires cross-compiled GMP/MPFR/GLFW
    local INCLUDES="-I$MINGW_SYSROOT/include -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$NATIVE_APP_DIR -I$EMBEDDED_DIR"
    local FLAGS="-std=c++17 -Wall -Wextra -O2 -DPLATFORM_WINDOWS -DWIN32_LEAN_AND_MEAN -D__USE_MINGW_ANSI_STDIO=1 -pipe -B$MINGW_BIN/"

    # Compile calc-core
    info "  Compiling calc-core for Windows..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -I$GMP_INC -I$MPFR_INC -c "$src" -o "$obj" 2>&1 || {
            # If GMP/MPFR headers fail, try with stub defines
            info "  Retrying $src with compatibility flags..."
            $CXX $FLAGS $INCLUDES -DGMP_HEADER_INCLUDE -DMPFR_HEADER_INCLUDE -c "$src" -o "$obj" 2>&1 || fail "Failed to compile $src for Windows"
        }
        CORE_OBJS+=("$obj")
    done
    $AR rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core static library built for Windows"

    # Compile render-engine
    info "  Compiling render-engine for Windows..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        RENDER_OBJS+=("$obj")
    done
    $AR rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine static library built for Windows"

    # Compile ImGui
    info "  Compiling Dear ImGui for Windows..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        IMGUI_OBJS+=("$obj")
    done
    ok "  Dear ImGui compiled for Windows"

    # Compile native-app
    info "  Compiling native-app for Windows..."
    local APP_OBJS=()
    for src in $(abs_sources "${NATIVE_APP_SOURCES[@]}"); do
        local obj="$OUT/obj/app_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for Windows"
        APP_OBJS+=("$obj")
    done
    ok "  native-app compiled for Windows"

    # Link — note: GMP/MPFR/GLFW for Windows need to be cross-compiled first
    # For now, link with what we have
    info "  Linking scientific-calculator.exe..."
    $CXX -o "$OUT/scientific-calculator.exe" \
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" \
        -L"$OUT" -L"$MINGW_SYSROOT/lib" \
        -lrender-engine -lcalc-core \
        -lopengl32 -lgdi32 -lshell32 -luser32 -lkernel32 -ladvapi32 \
        -lmingw32 -lmoldname -lmingwex -lmsvcrt \
        -lpthread \
        -static \
        2>&1 || {
        info "  Note: Windows link requires cross-compiled GMP/MPFR/GLFW"
        info "  Building partial Windows object archive instead..."
        $AR rcs "$OUT/scientific-calculator-objs.a" \
            "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" \
            "${CORE_OBJS[@]}" "${RENDER_OBJS[@]}"
        ok "  Windows partial build: $OUT/scientific-calculator-objs.a (needs GMP/MPFR/GLFW for full link)"
        return 0
    }

    ok "Windows build: $OUT/scientific-calculator.exe"
    file "$OUT/scientific-calculator.exe"
}

build_macos() {
    info "========== Building macOS x86_64 (Zig c++) =========="
    local OUT="$BUILD_DIR/macos"
    mkdir -p "$OUT/obj"

    local ZIG="$LOCAL_PREFIX/bin/zig"
    if [[ ! -x "$ZIG" ]]; then
        fail "Zig not found at $ZIG"
    fi

    local CXX="$ZIG c++"
    local INCLUDES="-I$GMP_INC -I$MPFR_INC -I$LOCAL_INC -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$IMGUI_DIR -I$IMGUI_DIR/backends -I$NATIVE_APP_DIR -I$EMBEDDED_DIR"
    local FLAGS="-target x86_64-macos -std=c++17 -Wall -Wextra -O2 -DPLATFORM_MACOS -DEMBED_FONTS"

    # Compile calc-core
    info "  Compiling calc-core for macOS..."
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" 2>&1 || fail "Failed to compile $src for macOS"
        CORE_OBJS+=("$obj")
    done
    # Use zig to create archive
    $ZIG ar rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}" 2>/dev/null || ar rcs "$OUT/libcalc-core.a" "${CORE_OBJS[@]}"
    ok "  calc-core static library built for macOS"

    # Compile render-engine
    info "  Compiling render-engine for macOS..."
    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/re_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        RENDER_OBJS+=("$obj")
    done
    $ZIG ar rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}" 2>/dev/null || ar rcs "$OUT/librender-engine.a" "${RENDER_OBJS[@]}"
    ok "  render-engine static library built for macOS"

    # Compile ImGui
    info "  Compiling Dear ImGui for macOS..."
    local IMGUI_OBJS=()
    for src in $(abs_sources "${IMGUI_SOURCES[@]}"); do
        local obj="$OUT/obj/imgui_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        IMGUI_OBJS+=("$obj")
    done
    ok "  Dear ImGui compiled for macOS"

    # Compile native-app
    info "  Compiling native-app for macOS..."
    local APP_OBJS=()
    for src in $(abs_sources "${NATIVE_APP_SOURCES[@]}"); do
        local obj="$OUT/obj/app_$(basename "$src" .cpp).o"
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for macOS"
        APP_OBJS+=("$obj")
    done
    ok "  native-app compiled for macOS"

    # Link — Zig bundles macOS libc++ but not GMP/MPFR/GLFW
    info "  Linking scientific-calculator for macOS..."
    $CXX -target x86_64-macos -o "$OUT/scientific-calculator" \
        "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" \
        -L"$OUT" \
        -lrender-engine -lcalc-core \
        -framework OpenGL -framework Cocoa -framework IOKit \
        2>&1 || {
        info "  Note: macOS link requires cross-compiled GMP/MPFR/GLFW"
        info "  Building partial macOS object archive instead..."
        ar rcs "$OUT/scientific-calculator-objs.a" \
            "${APP_OBJS[@]}" "${IMGUI_OBJS[@]}" \
            "${CORE_OBJS[@]}" "${RENDER_OBJS[@]}"
        ok "  macOS partial build: $OUT/scientific-calculator-objs.a (needs GMP/MPFR/GLFW for full link)"
        return 0
    }

    ok "macOS build: $OUT/scientific-calculator"
    file "$OUT/scientific-calculator"
}

build_jar() {
    info "========== Building JAR (Java Wrapper + JNI Bridge) =========="
    local OUT="$BUILD_DIR/jar"
    mkdir -p "$OUT"

    local JAVA_HOME="${JAVA_HOME:-$(dirname $(dirname $(readlink -f $(which java))))}"

    # Compile JNI bridge as shared library (Linux)
    info "  Compiling JNI bridge shared library..."
    local CXX="g++"
    local JNI_INCLUDES="-I$JAVA_HOME/include -I$JAVA_HOME/include/linux"
    local INCLUDES="$JNI_INCLUDES -I$GMP_INC -I$MPFR_INC -I$CALC_CORE_PUBLIC -I$RENDER_ENGINE_PUBLIC -I$JNI_BRIDGE_PUBLIC"

    # First build calc-core + render-engine for JNI
    local CORE_OBJS=()
    for src in $(abs_sources "${CALC_CORE_SOURCES[@]}"); do
        local obj="$OUT/obj/jni_core_$(basename "$src" .cpp).o"
        $CXX $CXX_COMMON $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI"
        CORE_OBJS+=("$obj")
    done

    local RENDER_OBJS=()
    for src in $(abs_sources "${RENDER_ENGINE_SOURCES[@]}"); do
        local obj="$OUT/obj/jni_re_$(basename "$src" .cpp).o"
        $CXX $CXX_COMMON $INCLUDES -c "$src" -o "$obj" || fail "Failed to compile $src for JNI"
        RENDER_OBJS+=("$obj")
    done

    # JNI bridge source
    for src in $(abs_sources "${JNI_BRIDGE_SOURCES[@]}"); do
        local obj="$OUT/obj/jni_bridge_$(basename "$src" .cpp).o"
        $CXX $CXX_COMMON $INCLUDES -fPIC -c "$src" -o "$obj" || fail "Failed to compile $src for JNI"
        # Link shared library
        $CXX -shared -o "$OUT/libcalc-bridge.so" \
            "$obj" "${CORE_OBJS[@]}" "${RENDER_OBJS[@]}" \
            -L"$LOCAL_PREFIX/lib" -lgmp -lmpfr -ldl \
            || fail "Failed to link JNI bridge"
    done
    ok "  JNI bridge: $OUT/libcalc-bridge.so"

    # Compile Java wrapper
    info "  Compiling Java wrapper classes..."
    local JAVA_SRC="$ROOT/java-wrapper/src/main/java"
    mkdir -p "$OUT/classes"
    $JAVA_HOME/bin/javac -d "$OUT/classes" \
        "$JAVA_SRC/com/calc/CalcEngine.java" \
        "$JAVA_SRC/com/calc/CalcResult.java" \
        "$JAVA_SRC/com/calc/Calculator.java" \
        "$JAVA_SRC/com/calc/NativeLoader.java" \
        2>&1 || fail "Java compilation failed"
    ok "  Java classes compiled"

    # Package JAR
    info "  Packaging JAR..."
    mkdir -p "$OUT/classes/native"
    cp "$OUT/libcalc-bridge.so" "$OUT/classes/native/"
    cd "$OUT/classes"
    $JAVA_HOME/bin/jar cf "$OUT/scientific-calculator.jar" \
        -C . . \
        || fail "JAR packaging failed"
    ok "  JAR: $OUT/scientific-calculator.jar"

    cd "$ROOT"
    info "  JAR contents:"
    $JAVA_HOME/bin/jar tf "$OUT/scientific-calculator.jar" | head -20
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
        windows) ls -lh "$BUILD_DIR/windows/scientific-calculator.exe" "$BUILD_DIR/windows/scientific-calculator-objs.a" 2>/dev/null ;;
        macos)   ls -lh "$BUILD_DIR/macos/scientific-calculator" "$BUILD_DIR/macos/scientific-calculator-objs.a" 2>/dev/null ;;
        jar)     ls -lh "$BUILD_DIR/jar/scientific-calculator.jar" 2>/dev/null ;;
        all)
            ls -lh "$BUILD_DIR/linux/scientific-calculator" 2>/dev/null
            ls -lh "$BUILD_DIR/windows/scientific-calculator.exe" "$BUILD_DIR/windows/scientific-calculator-objs.a" 2>/dev/null
            ls -lh "$BUILD_DIR/macos/scientific-calculator" "$BUILD_DIR/macos/scientific-calculator-objs.a" 2>/dev/null
            ls -lh "$BUILD_DIR/jar/scientific-calculator.jar" 2>/dev/null
            ;;
    esac
done

ok "Build complete!"
