#!/usr/bin/env bash
# ============================================================================
# build-release.sh — build all release artifacts for sci-calc
# ============================================================================
set -euo pipefail

ROOT="/home/z/sci-calc"
CACHE="$ROOT/.scicalc-cache"
MPFR_INC="$CACHE/native-deps/usr/include"
LOCAL_LIB="$CACHE/native-deps/usr/lib"
PUB="$ROOT/calc-core/src/main/cpp/public"
SRC="$ROOT/calc-core/src/main/cpp/src"
CLI="$ROOT/cli/main.cpp"
DIST="$ROOT/dist"
MAC_PREFIX="$CACHE/macdeps"
BUILDLOG="$ROOT/build-release.log"
mkdir -p "$DIST"
: > "$BUILDLOG"

CXXFLAGS="-std=c++17 -O2 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-vexing-parse"
LINUX_INC="-I$PUB -I$MPFR_INC"
MINGW_SYSROOT="$CACHE/toolchains/mingw/usr/x86_64-w64-mingw32ucrt"
MINGW_INC="-I$PUB -I$MINGW_SYSROOT/include"
MINGW_LIB="-L$MINGW_SYSROOT/lib -lmpfr -lgmp"
ZIG="$CACHE/toolchains/zig/zig"
JDK="$CACHE/jdk"

ok() { echo "  ✅ $*"; }
fail() { echo "  ❌ $*"; echo "  See $BUILDLOG for details"; exit 1; }

# ============================================================================
# 1. LINUX x86_64
# ============================================================================
echo "=== 1/4 Linux x86_64 ==="
if g++ $CXXFLAGS $LINUX_INC "$CLI" $SRC/*.cpp \
    -L"$LOCAL_LIB" -lmpfr -lgmp -static-libgcc -static-libstdc++ \
    -o "$DIST/sci-calc-linux-x86_64" >>"$BUILDLOG" 2>&1; then
    ok "Linux: $(ls -lh $DIST/sci-calc-linux-x86_64 | awk '{print $5}')"
    file "$DIST/sci-calc-linux-x86_64" | grep -q "ELF" && ok "ELF format"
    # Smoke tests
    "$DIST/sci-calc-linux-x86_64" "2+3*4" 2>/dev/null
    "$DIST/sci-calc-linux-x86_64" "sqrt(2)" 2>/dev/null | head -c 40; echo "..."
    "$DIST/sci-calc-linux-x86_64" --version 2>/dev/null
else
    fail "Linux build failed"
fi

# ============================================================================
# 2. WINDOWS x86_64 (-mwindows GUI + console)
# ============================================================================
echo ""
echo "=== 2/4 Windows x86_64 ==="
MINGW_GXX="$CACHE/toolchains/mingw/usr/bin/x86_64-w64-mingw32ucrt-g++"

# GUI version (-mwindows)
if "$MINGW_GXX" $CXXFLAGS $MINGW_INC -mwindows "$CLI" $SRC/*.cpp $MINGW_LIB \
    -static -static-libgcc -static-libstdc++ \
    -o "$DIST/sci-calc-windows-x86_64.exe" >>"$BUILDLOG" 2>&1; then
    ok "Windows GUI (-mwindows): $(ls -lh $DIST/sci-calc-windows-x86_64.exe | awk '{print $5}')"
    file "$DIST/sci-calc-windows-x86_64.exe" | grep -q "PE32+" && ok "PE32+ format"
else
    fail "Windows GUI build failed"
fi

# Console version
if "$MINGW_GXX" $CXXFLAGS $MINGW_INC "$CLI" $SRC/*.cpp $MINGW_LIB \
    -static -static-libgcc -static-libstdc++ \
    -o "$DIST/sci-calc-windows-x86_64-console.exe" >>"$BUILDLOG" 2>&1; then
    ok "Windows console: $(ls -lh $DIST/sci-calc-windows-x86_64-console.exe | awk '{print $5}')"
else
    fail "Windows console build failed"
fi

# ============================================================================
# 3. macOS x86_64 (Zig cross-compile GMP/MPFR + link)
# ============================================================================
echo ""
echo "=== 3/4 macOS x86_64 (Zig) ==="

# Build macOS GMP if not cached
if [[ ! -f "$MAC_PREFIX/lib/libgmp.a" ]]; then
    echo "  Building GMP for macOS (this takes a few minutes)..."
    mkdir -p "$MAC_PREFIX" "$ROOT/.scicalc-cache/src"
    cd "$ROOT/.scicalc-cache/src"
    [[ -d gmp-6.3.0 ]] || {
        [[ -f gmp-6.3.0.tar.xz ]] || curl -fsSL -o gmp-6.3.0.tar.xz \
            "https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz"
        tar -xJf gmp-6.3.0.tar.xz
    }
    cd gmp-6.3.0
    CC="$ZIG cc -target x86_64-macos.11.0" \
    CXX="$ZIG c++ -target x86_64-macos.11.0" \
    AR="$ZIG ar" RANLIB="$ZIG ranlib" \
    ./configure --host=x86_64-apple-darwin \
        --prefix="$MAC_PREFIX" --disable-shared --enable-static --with-pic \
        >>"$BUILDLOG" 2>&1
    make -j2 >>"$BUILDLOG" 2>&1
    make install >>"$BUILDLOG" 2>&1
    cd "$ROOT"
fi
[[ -f "$MAC_PREFIX/lib/libgmp.a" ]] && ok "macOS GMP ready" || fail "macOS GMP build failed"

# Build macOS MPFR if not cached
if [[ ! -f "$MAC_PREFIX/lib/libmpfr.a" ]]; then
    echo "  Building MPFR for macOS..."
    cd "$ROOT/.scicalc-cache/src"
    [[ -d mpfr-4.2.2 ]] || {
        [[ -f mpfr-4.2.2.tar.xz ]] || curl -fsSL -o mpfr-4.2.2.tar.xz \
            "https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.xz"
        tar -xJf mpfr-4.2.2.tar.xz
    }
    cd mpfr-4.2.2
    CC="$ZIG cc -target x86_64-macos.11.0" \
    CXX="$ZIG c++ -target x86_64-macos.11.0" \
    AR="$ZIG ar" RANLIB="$ZIG ranlib" \
    ./configure --host=x86_64-apple-darwin \
        --prefix="$MAC_PREFIX" --disable-shared --enable-static --with-pic \
        --with-gmp="$MAC_PREFIX" >>"$BUILDLOG" 2>&1
    make -j2 >>"$BUILDLOG" 2>&1
    make install >>"$BUILDLOG" 2>&1
    cd "$ROOT"
fi
[[ -f "$MAC_PREFIX/lib/libmpfr.a" ]] && ok "macOS MPFR ready" || fail "macOS MPFR build failed"

# Link macOS executable
if "$ZIG" c++ -target x86_64-macos.11.0 $CXXFLAGS \
    -I"$PUB" -I"$MAC_PREFIX/include" \
    "$CLI" $SRC/*.cpp \
    -L"$MAC_PREFIX/lib" -lmpfr -lgmp \
    -o "$DIST/sci-calc-macos-x86_64" >>"$BUILDLOG" 2>&1; then
    ok "macOS: $(ls -lh $DIST/sci-calc-macos-x86_64 | awk '{print $5}')"
    file "$DIST/sci-calc-macos-x86_64" | grep -q "Mach-O" && ok "Mach-O format"
else
    fail "macOS link failed"
fi

# ============================================================================
# 4. JAR (cross-platform, with JNI shared libs)
# ============================================================================
echo ""
echo "=== 4/4 JAR ==="
JNIDIR="$ROOT/jni-bridge/src/main/cpp/src"
JNIPUB="$ROOT/jni-bridge/src/main/cpp/public"
JAVADIR="$ROOT/java-wrapper/src/main/java"
JNI_INC="-I$JDK/include -I$JDK/include/linux -I$JDK/include/win32 -I$JDK/include/darwin"
JNILIBS="$ROOT/build-jni"
mkdir -p "$JNILIBS/linux" "$JNILIBS/windows" "$JNILIBS/macos"

echo "  [4a] JNI shared libraries..."
# Linux .so
if g++ $CXXFLAGS $LINUX_INC $JNI_INC -I"$JNIPUB" -fPIC -shared \
    "$JNIDIR/calc_jni.cpp" $SRC/*.cpp \
    -L"$LOCAL_LIB" -lmpfr -lgmp -static-libgcc -static-libstdc++ \
    -o "$JNILIBS/linux/libscicalc-native.so" >>"$BUILDLOG" 2>&1; then
    ok "linux .so: $(ls -lh $JNILIBS/linux/libscicalc-native.so | awk '{print $5}')"
else
    fail "linux JNI .so failed"
fi

# Windows .dll
if "$MINGW_GXX" $CXXFLAGS $MINGW_INC $JNI_INC -I"$JNIPUB" -shared \
    "$JNIDIR/calc_jni.cpp" $SRC/*.cpp $MINGW_LIB \
    -static -static-libgcc -static-libstdc++ \
    -o "$JNILIBS/windows/scicalc-native.dll" >>"$BUILDLOG" 2>&1; then
    ok "windows .dll: $(ls -lh $JNILIBS/windows/scicalc-native.dll | awk '{print $5}')"
else
    fail "windows JNI .dll failed"
fi

# macOS .dylib
if "$ZIG" c++ -target x86_64-macos.11.0 $CXXFLAGS \
    -I"$PUB" -I"$MAC_PREFIX/include" $JNI_INC -I"$JNIPUB" -shared \
    "$JNIDIR/calc_jni.cpp" $SRC/*.cpp \
    -L"$MAC_PREFIX/lib" -lmpfr -lgmp \
    -o "$JNILIBS/macos/libscicalc-native.dylib" >>"$BUILDLOG" 2>&1; then
    ok "macos .dylib: $(ls -lh $JNILIBS/macos/libscicalc-native.dylib | awk '{print $5}')"
else
    fail "macOS JNI .dylib failed"
fi

echo "  [4b] Stage JAR resources..."
JAVACLASSES="$ROOT/build-jar/classes"
mkdir -p "$JAVACLASSES/jni/linux/x86_64" "$JAVACLASSES/jni/windows/x86_64" "$JAVACLASSES/jni/macos/x86_64"
cp "$JNILIBS/linux/libscicalc-native.so" "$JAVACLASSES/jni/linux/x86_64/"
cp "$JNILIBS/windows/scicalc-native.dll" "$JAVACLASSES/jni/windows/x86_64/"
cp "$JNILIBS/macos/libscicalc-native.dylib" "$JAVACLASSES/jni/macos/x86_64/"

echo "  [4c] Compile Java..."
if "$JDK/bin/javac" -d "$JAVACLASSES" \
    $(find "$JAVADIR" -name '*.java') >>"$BUILDLOG" 2>&1; then
    ok "Java compiled"
else
    fail "Java compile failed"
fi

echo "  [4d] Package JAR..."
if "$JDK/bin/jar" cfe "$DIST/sci-calc.jar" com.calc.Calculator \
    -C "$JAVACLASSES" . >>"$BUILDLOG" 2>&1; then
    ok "JAR: $(ls -lh $DIST/sci-calc.jar | awk '{print $5}')"
else
    fail "JAR package failed"
fi

# ============================================================================
echo ""
echo "=== RELEASE ARTIFACTS ==="
ls -lh "$DIST/"
echo ""
echo "=== JAR contents (JNI libs) ==="
"$JDK/bin/jar" tf "$DIST/sci-calc.jar" 2>/dev/null | grep -E 'jni/|CalcEngine' | head
echo ""
echo "Build log: $BUILDLOG"