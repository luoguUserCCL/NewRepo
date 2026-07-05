#!/usr/bin/env bash
# ============================================================================
# build-verify.sh — cross-platform build verification for sci-calc
#
# Builds calc-core (the C++ engine) for three targets and runs tests where
# possible:
#   1. Linux   x86_64  (native g++)          — compile + link + run tests
#   2. Windows x86_64  (MinGW UCRT64 cross)  — compile + link (-mwindows for GUI)
#   3. macOS   x86_64  (Zig as clang)        — compile + link
#
# Usage: ./build-verify.sh
# ============================================================================
set -euo pipefail

ROOT="/home/z/sci-calc"
CACHE="$ROOT/.scicalc-cache"
MPFR_INC="$CACHE/native-deps/usr/include"
LOCAL_LIB="$CACHE/native-deps/usr/lib"
PUB="$ROOT/calc-core/src/main/cpp/public"
SRC="$ROOT/calc-core/src/main/cpp/src"
BUILD="$ROOT/build-verify"
mkdir -p "$BUILD"

# Common compiler flags
COMMON_CXXFLAGS="-std=c++17 -O2 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-vexing-parse"
MATH_INC="-I$PUB -I$MPFR_INC"

pass() { echo "  ✅ $*"; }
fail() { echo "  ❌ $*"; FAIL=1; }
FAIL=0

# ============================================================================
# 1. LINUX (native)
# ============================================================================
echo "========================================"
echo "1. LINUX x86_64 (native g++)"
echo "========================================"

LINUX_OUT="$BUILD/linux"
mkdir -p "$LINUX_OUT"

echo "  [1a] Compile calc-core → static library"
OBJS=""
for f in $SRC/*.cpp; do
  base=$(basename "$f" .cpp)
  if g++ $COMMON_CXXFLAGS $MATH_INC -c "$f" -o "$LINUX_OUT/$base.o" 2>/dev/null; then
    OBJS="$OBJS $LINUX_OUT/$base.o"
  else
    fail "compile $base.cpp"
    g++ $COMMON_CXXFLAGS $MATH_INC -c "$f" -o "$LINUX_OUT/$base.o" 2>&1 | head -3
  fi
done
if [ -n "$OBJS" ]; then
  ar rcs "$LINUX_OUT/libcalc-core.a" $OBJS 2>/dev/null && pass "static lib: $(ls -lh $LINUX_OUT/libcalc-core.a | awk '{print $5}')" || fail "ar"
fi

echo "  [1b] Compile + link test program"
cat > "$LINUX_OUT/test.cpp" <<'CPP'
#include <calc/calc_engine.h>
#include <iostream>
int main() {
    calc::CalcEngine engine;
    engine.setOutputMode(calc::OutputMode::MATH);
    const char* tests[] = {
        "2+3*4", "10/4", "sqrt(2)", "1/2+1/3", "2*sqrt(2)",
        "sum(i,1,5,i)", "factorial(5)", "Iverson(3>2)",
        "Iverson(5 in Integer)", "Iverson(2 in {1,2,3}\\{2})",
        "x:=42", "x*2", "f(n):=n*n+1", "f(5)"
    };
    int ok = 0;
    for (auto e : tests) {
        auto r = engine.evaluate(e);
        if (engine.lastError().empty()) { ok++; }
    }
    std::cout << "Linux test: " << ok << "/" << (sizeof(tests)/sizeof(tests[0])) << " passed\n";
    return ok == (sizeof(tests)/sizeof(tests[0])) ? 0 : 1;
}
CPP
if g++ $COMMON_CXXFLAGS $MATH_INC "$LINUX_OUT/test.cpp" $SRC/*.cpp \
    -L"$LOCAL_LIB" -lmpfr -lgmp -o "$LINUX_OUT/test" 2>/dev/null; then
  pass "link test program"
  echo "  [1c] Run tests"
  if "$LINUX_OUT/test" 2>/dev/null; then
    pass "all Linux tests passed"
  else
    fail "Linux test execution"
    "$LINUX_OUT/test" 2>&1 | head -5
  fi
else
  fail "Linux link"
  g++ $COMMON_CXXFLAGS $MATH_INC "$LINUX_OUT/test.cpp" $SRC/*.cpp \
      -L"$LOCAL_LIB" -lmpfr -lgmp -o "$LINUX_OUT/test" 2>&1 | head -5
fi

# ============================================================================
# 2. WINDOWS (MinGW UCRT64 cross)
# ============================================================================
echo ""
echo "========================================"
echo "2. WINDOWS x86_64 (MinGW-w64 UCRT64 cross)"
echo "========================================"

MINGW_PREFIX="$CACHE/toolchains/mingw/usr/bin"
MINGW_GXX="$MINGW_PREFIX/x86_64-w64-mingw32ucrt-g++"
MINGW_AR="$MINGW_PREFIX/x86_64-w64-mingw32ucrt-ar"
WIN_OUT="$BUILD/windows"
mkdir -p "$WIN_OUT"

# MinGW needs its own MPFR/GMP headers+libs. The cross-compiler ships with
# them in its sysroot. Check:
MINGW_SYSROOT="$CACHE/toolchains/mingw/usr/x86_64-w64-mingw32ucrt"
echo "  [2a] Check MinGW sysroot for mpfr/gmp"
if ls "$MINGW_SYSROOT/include/mpfr.h" 2>/dev/null && ls "$MINGW_SYSROOT/lib/libmpfr.a" 2>/dev/null; then
  pass "MinGW sysroot has mpfr/gmp (headers + static libs)"
  MINGW_MATH_INC="-I$PUB -I$MINGW_SYSROOT/include"
  MINGW_MATH_LIB="-L$MINGW_SYSROOT/lib -lmpfr -lgmp"
else
  echo "  ⚠️  MinGW sysroot missing mpfr/gmp — will compile-only (no link)"
  MINGW_MATH_INC="-I$PUB -I$MPFR_INC"
  MINGW_MATH_LIB=""
fi

echo "  [2b] Compile calc-core → object files"
WIN_OBJS=""
for f in $SRC/*.cpp; do
  base=$(basename "$f" .cpp)
  if "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC -c "$f" -o "$WIN_OUT/$base.o" 2>/dev/null; then
    WIN_OBJS="$WIN_OBJS $WIN_OUT/$base.o"
  else
    fail "mingw compile $base.cpp"
    "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC -c "$f" -o "$WIN_OUT/$base.o" 2>&1 | head -3
  fi
done

if [ -n "$WIN_OBJS" ] && [ -x "$MINGW_AR" ]; then
  "$MINGW_AR" rcs "$WIN_OUT/libcalc-core.a" $WIN_OBJS 2>/dev/null && \
    pass "mingw static lib: $(ls -lh $WIN_OUT/libcalc-core.a | awk '{print $5}')" || fail "mingw ar"
fi

echo "  [2c] Compile + link test program (console, without -mwindows)"
cat > "$WIN_OUT/test.cpp" <<'CPP'
#include <calc/calc_engine.h>
#include <iostream>
int main() {
    calc::CalcEngine engine;
    auto r = engine.evaluate("2+3*4");
    std::cout << "2+3*4 = " << engine.formatResult(r) << "\n";
    r = engine.evaluate("sqrt(2)");
    engine.setOutputMode(calc::OutputMode::MATH);
    r = engine.evaluate("sqrt(2)");
    std::cout << "sqrt(2) [MATH] = " << engine.formatResult(r) << "\n";
    return 0;
}
CPP
if [ -n "$MINGW_MATH_LIB" ]; then
  if "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC "$WIN_OUT/test.cpp" $SRC/*.cpp \
      $MINGW_MATH_LIB -o "$WIN_OUT/test.exe" 2>/dev/null; then
    pass "mingw link test.exe: $(ls -lh $WIN_OUT/test.exe | awk '{print $5}')"
    file "$WIN_OUT/test.exe" | grep -q "PE32+" && pass "PE32+ Windows executable" || fail "not PE32+"
  else
    fail "mingw link"
    "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC "$WIN_OUT/test.cpp" $SRC/*.cpp \
        $MINGW_MATH_LIB -o "$WIN_OUT/test.exe" 2>&1 | head -5
  fi
else
  echo "  ⏭️  skipping link (no mingw mpfr/gmp)"
fi

echo "  [2d] Link test with -mwindows (GUI subsystem, no console)"
# -mwindows tells the MinGW linker to produce a GUI-subsystem executable
# (no console window). This is REQUIRED for the ImGui GUI app per spec.
# We link a minimal main that uses MessageBoxA to avoid needing GLFW/OpenGL.
cat > "$WIN_OUT/gui_test.cpp" <<'CPP'
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    MessageBoxA(NULL, "sci-calc Windows build OK", "Verify", MB_OK);
    return 0;
}
CPP
if "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC -mwindows \
    "$WIN_OUT/gui_test.cpp" -o "$WIN_OUT/gui_test.exe" 2>/dev/null; then
  pass "-mwindows link: $(ls -lh $WIN_OUT/gui_test.exe | awk '{print $5}')"
  file "$WIN_OUT/gui_test.exe" | grep -q "PE32+" && pass "PE32+ GUI executable" || fail "not PE32+"
  # Verify subsystem is GUI (not console) via objdump if available
  if command -v objdump >/dev/null 2>&1; then
    sub=$(objdump -f "$WIN_OUT/gui_test.exe" 2>/dev/null | grep -oE 'gui|console' || echo "?")
    pass "subsystem: $sub"
  fi
else
  fail "-mwindows link"
fi

echo "  [2e] Link calc-core test with -mwindows (full engine, GUI subsystem)"
# NOTE: <windows.h> defines `IN` as a macro (SAL annotation), which clashes
# with BinaryOp::IN in expression.h. Include calc headers BEFORE windows.h
# so the enum is already parsed when the macro is defined.
cat > "$WIN_OUT/engine_gui.cpp" <<'CPP'
#include <calc/calc_engine.h>
#include <string>
// windows.h must come AFTER calc headers to avoid the IN/OUT macro clash.
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    calc::CalcEngine engine;
    engine.setOutputMode(calc::OutputMode::MATH);
    auto r = engine.evaluate("2+3*4");
    std::string msg = "2+3*4 = " + engine.formatResult(r);
    r = engine.evaluate("sqrt(2)");
    msg += "\nsqrt(2) = " + engine.formatResult(r);
    msg += "\n10/4 = ";
    r = engine.evaluate("10/4");
    msg += engine.formatResult(r);
    MessageBoxA(NULL, msg.c_str(), "sci-calc Engine Verify", MB_OK);
    return 0;
}
CPP
if "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC -mwindows \
    "$WIN_OUT/engine_gui.cpp" $SRC/*.cpp $MINGW_MATH_LIB \
    -static -o "$WIN_OUT/sci-calc.exe" 2>/dev/null; then
  pass "full engine -mwindows: $(ls -lh $WIN_OUT/sci-calc.exe | awk '{print $5}')"
  file "$WIN_OUT/sci-calc.exe" | grep -q "PE32+" && pass "PE32+ executable" || fail "not PE32+"
else
  fail "full engine -mwindows link"
  "$MINGW_GXX" $COMMON_CXXFLAGS $MINGW_MATH_INC -mwindows \
      "$WIN_OUT/engine_gui.cpp" $SRC/*.cpp $MINGW_MATH_LIB \
      -static -o "$WIN_OUT/sci-calc.exe" 2>&1 | head -8
fi

# ============================================================================
# 3. macOS (Zig as clang)
# ============================================================================
echo ""
echo "========================================"
echo "3. macOS x86_64 (Zig 0.17.0-dev as clang)"
echo "========================================"

ZIG="$CACHE/toolchains/zig/zig"
MAC_OUT="$BUILD/macos"
mkdir -p "$MAC_OUT"

# Zig provides its own libc but NOT GMP/MPFR. For macOS cross we can only
# compile (not link) unless we build GMP/MPFR for macOS too. Verify compile.
echo "  [3a] Compile calc-core → object files (macOS target)"
MAC_OBJS=""
MAC_FAIL=0
for f in $SRC/*.cpp; do
  base=$(basename "$f" .cpp)
  if "$ZIG" c++ -target x86_64-macos.11.0 $COMMON_CXXFLAGS $MATH_INC \
      -c "$f" -o "$MAC_OUT/$base.o" 2>/dev/null; then
    MAC_OBJS="$MAC_OBJS $MAC_OUT/$base.o"
  else
    # retry showing errors for the first failure
    if [ $MAC_FAIL -eq 0 ]; then
      echo "  first failure details:"
      "$ZIG" c++ -target x86_64-macos.11.0 $COMMON_CXXFLAGS $MATH_INC \
          -c "$f" -o "$MAC_OUT/$base.o" 2>&1 | head -5
    fi
    MAC_FAIL=$((MAC_FAIL+1))
    fail "zig compile $base.cpp"
  fi
done
if [ -n "$MAC_OBJS" ]; then
  pass "compiled $(echo $MAC_OBJS | wc -w) objects for macOS"
  # Verify they are Mach-O
  first_obj=$(echo $MAC_OBJS | awk '{print $1}')
  file "$first_obj" | grep -q "Mach-O" && pass "Mach-O object format verified" || fail "not Mach-O"
fi

echo ""
echo "========================================"
echo "BUILD SUMMARY"
echo "========================================"
if [ $FAIL -eq 0 ]; then
  echo "  ✅ ALL TARGETS BUILT SUCCESSFULLY"
else
  echo "  ❌ $FAIL failure(s) — see above"
fi
exit $FAIL