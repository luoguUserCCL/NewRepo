# Scientific Calculator

A high-precision scientific calculator with mathematical formula rendering, built with C++/Gradle/JNI.

## Features

- **High-precision arithmetic** — Based on GMP/MPFR, supporting arbitrary precision (default: 256 bits ≈ 77 decimal digits)
- **Multiple number formats** — Scientific notation, fixed-point, significant digits, floating-point
- **Base conversion** — Binary (0b), Octal (0o), Decimal, Hexadecimal (0x)
- **Mathematical formula rendering** — Real-time LaTeX-style rendering using Dear ImGui
- **Bilingual UI** — Chinese / English interface switching
- **Custom functions & variables** — Define with `:=` operator

## Supported Functions

| Input | Rendering | Description |
|-------|-----------|-------------|
| `abs(x)` | \|x\| | Absolute value |
| `floor(x)` | ⌊x⌋ | Floor function |
| `ceil(x)` | ⌈x⌉ | Ceiling function |
| `rand(a,b)` | Rand(a,b) | Crypto-safe uniform random |
| `Iverson(P)` | 𝕀(P) | Iverson bracket |
| `sum(i,s,e,expr)` | Σ | Summation |
| `prod(i,s,e,expr)` | Π | Product |
| `pow(a,b)` | a^b | Power (fast for integers) |
| `gcd(a,b)` | gcd(a,b) | Greatest common divisor |
| `lcm(a,b)` | lcm(a,b) | Least common multiple |
| `log(b)` | ln(b) | Natural logarithm |
| `log(a,b)` | logₐ(b) | Logarithm base a |
| `sqrt(b)` | √b | Square root |
| `sqrt(a,b)` | ᵃ√b | a-th root |
| `sin/cos/tan/cot/sec/csc` | sin/cos/tan/... | Trigonometric (MPFR precision) |
| `arcsin/arccos/arctan/...` | arcsin/... | Inverse trigonometric |
| `sinh/cosh/tanh` | sinh/... | Hyperbolic |
| `exp(x)` | eˣ | Exponential |
| `gamma(x)` | Γ(x) | Gamma function |
| `factorial(n)` | n! | Factorial |
| `x % y` | x mod y | Modulo |
| `x in S` | x ∈ S | Set membership |

### Operators

| Input | Rendering | Precedence |
|-------|-----------|-----------|
| `:=` | := | Definition (lowest) |
| `or` | ∨ | Logical OR |
| `and` | ∧ | Logical AND |
| `not` | ¬ | Logical NOT |
| `= < > ≤ ≥ ≠` | = < > ≤ ≥ ≠ | Comparison (Iverson: 1/0) |
| `in` | ∈ | Set membership |
| `cap` | ∩ | Set intersection |
| `cup` | ∪ | Set union |
| `+ -` | + - | Addition, subtraction |
| `* / %` | × ÷ mod | Multiplication, division, modulo |
| `-` | - | Unary minus |
| `^` | ^ | Power (right-associative) |

### Sets

- Enumerated: `{1, 2, 3}`
- Closed interval: `[0, 1]`
- Open interval: `(0, 1)`
- Half-open: `(0, 1]`, `[0, 1)`
- Intersection: `A cap B` → A ∩ B
- Union: `A cup B` → A ∪ B

### Variable/Function Definition

```
x := 42
f(x,y) := x^2 + y^2
```

## Build System

### Option 1: Gradle (primary)

```bash
# Build all targets
./gradlew build

# Build specific target
./gradlew :calc-core:build
./gradlew :native-app:assemble

# Cross-compile
./gradlew :native-app:crossCompileLinux
./gradlew :native-app:crossCompileWindows
./gradlew :native-app:crossCompileMac
```

### Option 2: CMake (alternative)

```bash
# Linux (native)
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux

# Windows (cross-compile)
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
cmake --build build-win

# macOS (cross-compile via Zig)
cmake -B build-mac -DCMAKE_TOOLCHAIN_FILE=cmake/zig-macos.cmake
cmake --build build-mac

# JNI bridge
cmake -B build-jni -DBUILD_JNI_BRIDGE=ON
cmake --build build-jni
```

### Dependencies

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- GMP (libgmp-dev)
- MPFR (libmpfr-dev)
- GLFW3 (libglfw3-dev)
- OpenGL 3.2+
- Dear ImGui (included as third_party)
- JDK 11+ (for JNI bridge only)

## Project Structure

```
├── calc-core/          # Core calculation engine (cpp-library, static)
├── render-engine/      # Math formula rendering (cpp-library, static)
├── jni-bridge/         # JNI bridge (cpp-library, shared)
├── native-app/         # Desktop GUI app (cpp-application)
├── java-wrapper/       # JAR wrapper with embedded native libs
├── buildSrc/           # Gradle custom toolchain plugins
├── cmake/              # CMake cross-compilation toolchains
├── fonts/              # Latin Modern Math + Noto Sans SC
└── CMakeLists.txt      # Alternative CMake build
```

## Build Outputs

| Target | Output | Dependencies |
|--------|--------|-------------|
| Linux | `calc` (ELF) | None (static linking) |
| Windows | `calc.exe` (PE) | None (static linking) |
| macOS | `calc` (Mach-O) | None (static linking) |
| JVM | `calc-wrapper-1.0.jar` | JDK 11+ |

## License

MIT
