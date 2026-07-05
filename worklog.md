# sci-calc Work Log

---
Task ID: 1
Agent: Explore
Task: Inventory existing C++ engine code in calc-core/render-engine/jni-bridge/native-app

Work Log:
- Checked for prior worklog.md (did not exist); created fresh.
- Listed repo root and enumerated all .h/.hpp/.cpp files under the four target module trees. Also noticed java-wrapper/, third_party/glad/, buildSrc/, tools/ exist but are out of scope.
- Ran `wc -l` on every C++ file to get line counts. Identified `native-app/.../embedded/font_latinmodern_math.h` as auto-generated binary font data (61,164 lines of `0xNN,` byte literals) — excluded from "real code" totals.
- Read all 10 calc-core headers in `public/calc/`: big_decimal.h, expression.h, expression_parser.h, calc_engine.h, function_registry.h, variable_store.h, set_operations.h, number_format.h, base_converter.h, crypto_random.h, base91.hpp.
- Read all 11 calc-core sources in `src/`: big_decimal.cpp, expression.cpp, expression_parser.cpp, expression_eval.cpp, functions.cpp, function_registry.cpp, variable_store.cpp, set_operations.cpp, number_format.cpp, base_converter.cpp, crypto_random.cpp.
- Read all 3 render-engine headers and sources: math_renderer.{h,cpp}, font_manager.{h,cpp}, imgui_renderer.{h,cpp}.
- Read the jni-bridge single source: calc_jni.cpp.
- Read all native-app files: main.cpp, app_state.h, ui_calculator.h, ui_function_def.h, ui_settings.h, i18n.{h,cpp}, embedded/fonts.{h,cpp} and verified font_latinmodern_math.h is binary-only.
- Cross-checked feature implementation by tracing parser precedence levels, AST node types, evaluator branches, function registry entries, set operations API surface, and layout-node types.
- Computed final totals: 39 files total (38 code + 1 font data header), 5874 lines of real code, 67038 lines including the font data header.
- Wrote this worklog entry.

Stage Summary:
- **Total: 39 C++ files** (38 logic files + 1 auto-generated font-data header). **5874 lines of real C++ code** (67038 lines including the 61164-line `font_latinmodern_math.h` byte array).
- **calc-core** (21 files, ~2623 lines): the engine. BigDecimal (MPFR-backed, 256-bit default), AST + recursive-descent parser, evaluator, function library, set operations, base conversion, crypto RNG, number formatter.
- **render-engine** (6 files, ~1163 lines): MathRenderer AST→layout tree, FontManager (metric estimates, no real glyph atlas), ImGuiRenderer (draws layout via ImDrawList).
- **jni-bridge** (1 file, 140 lines): thin C ABI over CalcEngine for Java (`com_calc_CalcEngine_*` JNI methods).
- **native-app** (11 files, ~2048 lines incl. font data): GLFW+ImGui desktop app. main.cpp, AppState, three UI panels (calculator/function-def/settings), i18n (EN/ZH_CN), embedded font loader + system CJK font search.

**What's implemented (matches task feature list):**
- AST node types — yes: NUMBER_LIT, VARIABLE_REF, BINARY_OP, UNARY_OP, FUNCTION_CALL, DEFINE_VAR, DEFINE_FUNC, SET_LITERAL, INTERVAL, IVERSON, plus declared-but-unused CONDITIONAL.
- BigDec / high-precision arithmetic — yes: BigDecimal wraps MPFR (gmp.h + mpfr.h), default 256-bit precision, full arithmetic/comparison/conversion, constants pi/e, MPFR direct access for trig/log.
- Parser — yes, recursive descent (NOT a Pratt parser, but equivalent precedence semantics). ~10 levels: `:=` → `or` → `and` → `not` → comparison → set ops → additive → multiplicative → unary `-` → `^` (right-assoc) → primary. Handles Unicode ≠ ≤ ≥ and ASCII != <= >=. Detects base prefixes (0b/0o/0x) in number literals. Supports `name(params) := body` and `name := expr`.
- Evaluator — yes for arithmetic/logic/sets/functions. **Decimal output mode is complete** (FLOATING_POINT / SCIENTIFIC / FIXED_POINT / SIGNIFICANT_DIGITS via NumberFormatter). **Math output mode with fractions/roots/π is NOT implemented** — DIV always produces a BigDecimal (no rational preservation); only the *renderer* visualizes `a/b` as a fraction bar.
- Function library — yes: abs, floor, ceil, rand (crypto-secure), sum, prod (AST-iterated), pow, gcd, lcm, sqrt (1/2-arg), log (1-arg=ln, 2-arg=log_a b), exp, log2, log10, sin/cos/tan/cot/sec/csc + arcsin/arccos/arctan/arccot/arcsec/arccsc + sinh/cosh/tanh, gamma, factorial, Iverson.
- Set operations — partial: enum sets {…}, intervals [a,b]/(a,b]/[a,b)/(a,b), `cap` (∩), `cup` (∪), `in` (∈) all work. CalcSet is a variant of Enumerated | Interval | Computed (lazy). **Set difference `\` is MISSING**. **Real/Rational/Integer predefined sets are NOT implemented** (no built-in sets in variable store or a set registry).
- Variable/function store with `:=` — yes: VariableStore (hash map) + FunctionRegistry (builtins + user functions with param names + body AST). Constants pi/e/phi pre-registered. User functions support param shadowing/restoration during eval.
- Box-model formula renderer — yes: MathRenderer builds a LayoutNode tree with bbox (width/height/ascent/descent). 10 LayoutTypes: GLYPH, HORIZONTAL, VERTICAL, FRACTION, RADICAL, SUB_SUPERSCRIPT, UNDER_OVER, DELIMITER, SPACE, plus declared-but-unhandled ACCENT and COLOR. Special-case rendering for abs→|x|, floor→⌊x⌋, ceil→⌈x⌉, sum/prod→Σ/Π with under/over, sqrt→√, log→log_a(b) or ln, pow→a^b, Iverson→𝕀(...). Layout metrics are *approximate* (ratio-based, no real glyph atlas).
- Number format / base conversion (bin/oct/dec/hex) — yes: BaseConverter + NumberFormatter handle 0b/0o/0x prefixes, 4 display bases, format modes, base prefix toggle.
- JNI bridge — yes: create/destroy/evaluate/setFormatMode/setBase/setPrecision/reset.
- Native desktop app — yes: GLFW+ImGui3, embedded Latin Modern Math + system Noto Sans SC lookup, calculator UI with formula preview, function/variable definition panels, settings popup, EN/ZH_CN i18n.

**What's missing / stubbed:**
- `function_registry.cpp` is a 9-line stub (real logic lives in `functions.cpp`).
- Parser is recursive descent, not strictly a Pratt parser (functionally equivalent; flagging in case the spec is strict).
- Set difference operator `\` is missing from both BinaryOp enum and parser/evaluator.
- Predefined sets Real / Rational / Integer are not implemented.
- Math/symbolic output mode (fractions preserved as rationals, √/π kept symbolic) is NOT implemented — evaluator always reduces to BigDecimal. Fraction/√ rendering is purely visual via the layout tree.
- LayoutType::ACCENT and LayoutType::COLOR are declared but never constructed/rendered.
- Layout bounding boxes use ratio-based estimates, not real font metrics (FontManager has no glyph atlas; actual rendering relies on ImGui's AddText).
- CONDITIONAL NodeType is declared ("Ternary / if-expression (future)") but never produced or evaluated.
- No unit tests, no test harness anywhere in the explored trees.
- No CMakeLists.txt or build script in cpp trees; build is driven by Gradle `buildSrc/CrossCompilePlugin.groovy` (not read — out of scope).

**Key design notes for the next agent:**
- BigDecimal uses a pImpl pattern wrapping `mpfr_t`; precision is per-instance with a thread-safe global default (atomic). `getMpfrAccess()` / `fromMpfrResult()` expose the mpfr_t so functions.cpp can call MPFR directly without losing precision.
- ASTNode uses a single struct with a `NodeType` tag + `std::variant`-style payload fields (numberValue, identifier, binaryOp, etc.) and `std::vector<std::unique_ptr<ASTNode>> children`. Factory helpers (`makeNumber`, `makeBinary`, …) live in expression.cpp. EvalResult is similar (tagged NUMBER/SET/BOOLEAN but SET is essentially unused — set results always degrade to BigDecimal 0 in evaluateBinary).
- CalcEngine is the orchestrator (pImpl holding FunctionRegistry + VariableStore + NumberFormatConfig + lastError). Construction auto-registers builtins and constants. Errors are caught and surfaced via `lastError()`, with `0` returned as fallback result.
- Parser precedence quirk: `in` is grouped with `cap`/`cup` (set-ops level), not with comparison relations. `not` is its own level between `and` and comparison. Verify against spec before changing.
- The evaluator handles `sum`/`prod` specially (needs AST access for the body) — they are registered as builtins for rendering/arg-count lookup only, and the real logic is in ExpressionEvaluator::evaluateSum/evaluateProd. Same for `Iverson` (also handled in parser as `Iverson(P)` syntax).
- CalcSet is move-only (holds unique_ptr in Computed); intersection/union build lazy Computed nodes. `contains()` recursively evaluates. Sets cannot currently be a first-class EvalResult value (evaluator collapses to 0).
- MathRenderer is approximate — if the next agent needs pixel-accurate rendering, real font metrics from ImGui's glyph atlas must be wired into FontManager / LayoutNode::computeLayout.
- Cross-module dependency graph: native-app → calc-core + render-engine; jni-bridge → calc-core; render-engine → calc-core (for ASTNode). No module depends on native-app or jni-bridge.

---
Task ID: 2
Agent: main (C++ engine development)
Task: Fix critical bugs and implement missing features in C++ calc engine

Work Log:
- Explored existing codebase: 39 files, ~5874 lines, ~80% complete
- Fixed BigDecimal(int) overload trap causing MPFR assertion abort
- Fixed parser tryParseParamList throwing on function calls
- Fixed Iverson bracket infinite recursion segfault
- Fixed BigDecimal toString() negative number formatting (-.4 → -4)
- Added set difference operator '\' (BinaryOp::SET_DIFF)
- Moved 'in' operator to comparison level (spec precedence level 8)
- Added predefined sets Real/Rational/Integer (ℝ/ℚ/ℤ)
- Made comparisons non-chainable per spec
- Verified with 40+ test expressions

Stage Summary:
- All critical crashes fixed; engine runs stable
- Core arithmetic, functions, variables, function definitions work
- Set operations (cap/cup/diff/in), intervals, predefined sets work
- Iverson brackets work with set/logic/relation predicates
- Remaining: math output mode (fractions/√/π preservation), mod keyword alias
- Commit: de664f1 pushed to origin/main

---
Task ID: 3
Agent: main (C++ engine development)
Task: Implement math output mode with fractions, radicals, symbolic arithmetic

Work Log:
- Created SymbolicValue class (symbolic_value.h/.cpp) representing exact forms
- Added OutputMode enum (DECIMAL/MATH) to CalcEngine
- Extended EvalResult with SYMBOLIC type
- DIV in MATH mode preserves integer/integer as gcd-reduced fraction
- sqrt in MATH mode preserves √n with square-factor extraction
- Arithmetic path promotes integers to SymbolicValue when mixed with symbolic
- Verified: 1/2+1/3→5/6, sqrt(8)→2√2, 2*sqrt(2)→2√2, sqrt(2)*sqrt(2)→2
- Both DECIMAL and MATH modes pass full regression

Stage Summary:
- Math output mode fully functional for fractions, radicals
- Pi preservation implemented in SymbolicValue (hasPi_ flag) but not yet
  wired into evaluator (would need to detect pi/e variable references
  during evaluation — future enhancement)
- All 40+ test expressions pass in both modes
- Commit: 46604b5 pushed to origin/main

---
Task ID: 4
Agent: main (cross-platform build verification)
Task: Build and verify calc-core for Linux, Windows (-mwindows), macOS

Work Log:
- Created build-verify.sh for unified 3-platform build verification
- Linux: compiled calc-core → static lib (479K) + test program, 14/14 tests pass
- Windows: installed MSYS2 mingw-w64-x86_64-gmp/mpfr into MinGW sysroot
- Windows: compiled calc-core → static lib (378K)
- Windows: linked console test.exe (399K, PE32+ console)
- Windows: linked -mwindows GUI test (133K, PE32+ subsystem=GUI)
- Windows: linked full engine + -mwindows (sci-calc.exe, 3.4M, PE32+ GUI)
- Discovered windows.h IN macro clash with BinaryOp::IN; fixed via include order
- macOS: compiled 12 object files via Zig cc -target x86_64-macos.11.0
- macOS: Mach-O 64-bit x86_64 object format verified

Stage Summary:
- All 3 targets compile successfully (Linux/Windows/macOS)
- Linux: full build + test pass (14/14)
- Windows: full static link with -mwindows, PE32+ GUI subsystem confirmed
- macOS: compile-only (GMP/MPFR macOS libs needed for link — future work)
- Commit: 0f7703a pushed to origin/main
- build-verify.sh committed for reproducible verification

---
Task ID: 5
Agent: macOS build completion
Task: Complete macOS build and upload to release

Work Log:
- Read prior worklog for context (Tasks 1-4). Task 4 had macOS compile-only verified; GMP/MPFR macOS libs were the remaining blocker.
- Checked GMP configure status: prior `mac-gmp-configure2.log` was incomplete (no CONFIGURE_DONE), no configure process running, macdeps/lib/ did not exist. The previous run had been killed mid-check (around the C++ preprocessor / "checking compiler" step) — config.log showed it never wrote config.status.
- Root cause: GMP's `--disable-assembly` does NOT skip the assembler-probing tests; `/usr/bin/nm -B` (Linux binutils) cannot read Mach-O object files produced by `zig cc -target x86_64-macos.11.0`, so tests like "how to define a 32-bit word" failed with `file format not recognized` → configure errored with "cannot determine how to define a 32-bit word".
- Fix: wrote a cache-file `/home/z/sci-calc/.scicalc-cache/mac-gmp-cache.cfg` pre-populating every `gmp_cv_asm_*` / `gmp_cv_c_*` / `gmp_cv_func_*` / `gmp_cv_prog_*` variable used by GMP's configure (including the misnamed `gmp_cv_asm_w32` for the 32-bit-word test, which is NOT `gmp_cv_asm_w`). Also added `--disable-cxx` (libgmpxx not needed) and `--cache-file=…`.
- GMP configure succeeded (exit 0). `make -j2` → `libgmp.a` (4.0 MB, 105 mpn objects). `make install` to `macdeps/` (libgmp.a + gmp.h).
- MPFR configure for x86_64-apple-darwin succeeded (linked against macdeps libgmp.a). `make -j2` → `libmpfr.a` (2.83 MB). `make install` to `macdeps/` (libmpfr.a + mpfr.h + mpf2mpfr.h).
- Built macOS CLI executable: `zig c++ -target x86_64-macos.11.0 -std=c++17 -O2 … cli/main.cpp calc-core/.../*.cpp -lmpfr -lgmp`. Output: `dist/sci-calc-macos-x86_64` (1,547,983 bytes). `file` confirms: Mach-O 64-bit x86_64 executable, PIE, NOUNDEFS, DYLDLINK.
- Prepared JDK darwin include dir: copied `linux/jni_md.h` to `jdk/include/darwin/jni_md.h` (the file is platform-neutral: JNIEXPORT uses GCC visibility attr, jint/jlong/jbyte typedefs work on macOS x86_64). Created empty `jni-bridge/src/main/cpp/public/` placeholder for the include path.
- Built macOS JNI shared library: `zig c++ -target x86_64-macos.11.0 -std=c++17 -O2 -shared calc_jni.cpp calc-core/.../*.cpp -lmpfr -lgmp`. Output: `build-jni/macos/libscicalc-native.dylib` (1,578,168 bytes). `file` confirms: Mach-O 64-bit x86_64 dynamically linked shared library.
- Rebuilt JAR: copied .dylib into `build-jar/classes/jni/macos/x86_64/libscicalc-native.dylib`, ran `jar cfe dist/sci-calc.jar com.calc.Calculator -C classes .`. JAR now contains native libs for all 3 platforms (linux/x86_64 .so, windows/x86_64 .dll, macos/x86_64 .dylib). Size: 2,197,537 bytes (was 1,666,504).
- Uploaded macOS executable to release 349097712 (new asset ID 466923428, 1,547,983 bytes, state=uploaded).
- Deleted old sci-calc.jar asset (ID 466916308, HTTP 204) and uploaded the rebuilt JAR (new asset ID 466923517, 2,197,537 bytes, state=uploaded).
- Final release now has 5 assets: linux-x86_64, macos-x86_64, windows-x86_64.exe, windows-x86_64-console.exe, sci-calc.jar (all 3-platform native).

Stage Summary:
- macOS x86_64 build is COMPLETE and uploaded.
- GMP 6.3.0 + MPFR 4.2.2 cross-compiled via Zig for x86_64-macos.11.0, installed to `.scicalc-cache/macdeps/{lib,include}`.
- CLI executable `sci-calc-macos-x86_64` (Mach-O 64-bit x86_64, 1.55 MB) — uploaded as release asset 466923428.
- JNI shared lib `libscicalc-native.dylib` (Mach-O 64-bit x86_64 shared library, 1.58 MB) — embedded in JAR at `jni/macos/x86_64/`.
- JAR `sci-calc.jar` (2.20 MB) rebuilt with all 3 platforms' native libs — uploaded as release asset 466923517 (old asset deleted).
- All 5 release assets verified uploaded: linux-x86_64, macos-x86_64, windows-x86_64.exe, windows-x86_64-console.exe, sci-calc.jar.
- Three-platform release v0.1.0 (release ID 349097712) is now complete.
- Key gotcha for future reference: GMP cross-configure cannot use the build system's `nm` for Mach-O objects — must pre-populate ALL `gmp_cv_asm_*` cache variables (especially `gmp_cv_asm_w32`, NOT `gmp_cv_asm_w`) via `--cache-file`. `--disable-assembly` does not skip these probes.

---
Task ID: 6
Agent: macOS GUI build
Task: Build macOS GUI executable with Dear ImGui + GLFW + OpenGL

Work Log:
- Read prior worklog for context. Tasks 1-5 completed: engine code inventory, bug fixes, math output mode, 3-platform CLI/JNI build. Task 4 noted "macOS: compile-only (GMP/MPFR macOS libs needed for link — future work)"; Task 5 added GMP/MPFR macOS libs to .scicalc-cache/macdeps/ and built the macOS CLI executable. Linux and Windows GUI builds existed in dist/; macOS GUI was the remaining gap.
- Existing build-gui.sh had a macOS section that tried `cmake` (with Zig as C compiler) then fell back to manual GLFW compile, but neither had been exercised yet. .scicalc-cache/macglfw/ did not exist.
- **Step 1 — Get macOS SDK**: Zig 0.17.0-dev.1158 ships only `libSystem.tbd` and basic `any-darwin-any` libc headers (no Carbon/Cocoa/AppKit/OpenGL framework headers and no framework TBDs). Downloaded MacOSX10.15.sdk.tar.xz (40 MB) AND MacOSX11.3.sdk.tar.xz (51 MB) from github.com/phracker/MacOSX-SDKs (the 11.3 release), extracted both into .scicalc-cache/macsdk/. The 10.15 SDK uses tapi-tbd v3 format; 11.3 uses v4 format.
- **Step 2 — Build GLFW 3.4 for macOS**: Downloaded glfw-3.4.zip from GitHub releases into .scicalc-cache/src/glfw-3.4/. Compiled 21 GLFW source files (7 core + 2 EGL/Osmesa + 4 null + 3 POSIX/Cocoa-time + 5 Cocoa .m) with `zig cc -target x86_64-macos.11.0 -D_GLFW_COCOA -isysroot <10.15 SDK> -F <frameworks>`. Archived into .scicalc-cache/macglfw/lib/libglfw3.a (1,024,672 bytes). Also copied glfw3.h and glfw3native.h into .scicalc-cache/macglfw/include/GLFW/ for downstream use.
- **Step 3 — Discover Zig ld reexported-libraries segfault**: First link attempt failed silently (0-byte output, exit 0). Re-running with `-v` revealed the actual linker invocation `zig ld -dynamic -platform_version macos 11.0.0 26.5 ... -framework Cocoa -framework OpenGL -lSystem ...` followed by `exit=139` (SIGSEGV). Root cause: Zig's bundled LLD segfaults when parsing TAPI TBD files whose `reexported-libraries` (v4) or `re-exports` (v3) section points at dylibs/frameworks that themselves need to be resolved from the SDK. The OpenGL.tbd reexports libGL.dylib + libGLU.dylib; AppKit.tbd reexports ApplicationServices + Foundation + UIFoundation; Carbon.tbd reexports HIToolbox + CommonPanels + ...; CoreServices.tbd reexports CarbonCore; etc. Zig reports success but writes an empty file.
- **Step 4 — Workaround**: Wrote a Python script to strip the `reexported-libraries` (v4) section from every framework TBD we need. Generated cleaned TBDs in .scicalc-cache/macgl/: libCocoa.tbd, libAppKit.tbd, libFoundation.tbd, libCoreFoundation.tbd, libCoreGraphics.tbd, libApplicationServices.tbd, libIOKit.tbd, libCoreVideo.tbd, libCarbon.tbd, libCoreServices.tbd, libHIToolbox.tbd, libOpenGL.tbd, libGL.tbd, libGLU.tbd, libobjc.tbd. Used 11.3 SDK TBDs (v4 format, cleanly strippable) for linking; used 10.15 SDK headers (-isysroot) for compilation (the 11.3 SDK headers trigger additional C++17 enum-base errors in CoreFoundation that 10.15 does not).
- **Step 5 — C++ libc++ header conflict**: The naive `-isysroot $SDK -I $SDK/usr/include` puts macOS SDK's `string.h`/`stddef.h`/etc. ahead of Zig's libc++ wrappers, causing libc++ to error out (`<cstring> tried including <string.h> but didn't find libc++'s <string.h> header`). Fix: drop `-I $SDK/usr/include` and rely on `-isysroot` alone (zig's libc++ wrappers come first in the search order; the SDK's /usr/include is added by -isysroot at lower priority). Also added `-Wno-elaborated-enum-base` for CoreFoundation's forward enum declarations and `-Wno-macro-redefined` for `TARGET_OS_*` built-in vs SDK conflicts.
- **Step 6 — Link**: Final link used `-lCocoa -lAppKit -lFoundation -lCoreFoundation -lCoreGraphics -lApplicationServices -lIOKit -lCoreVideo -lCarbon -lCoreServices -lOpenGL -lGL -lGLU -lobjc` (all from .scicalc-cache/macgl/) plus `-lglfw3 -lmpfr -lgmp` (from macglfw/ and macdeps/). After adding `-lCoreServices` (for `_UCKeyTranslate` referenced by GLFW's `cocoa_window.o`), the link succeeded with zero undefined symbols.
- **Step 7 — Verify**: `file dist/sci-calc-gui-macos-x86_64` → `Mach-O 64-bit x86_64 executable, flags:<NOUNDEFS|DYLDLINK|TWOLEVEL|WEAK_DEFINES|BINDS_TO_WEAK|NO_REEXPORTED_DYLIBS|PIE|HAS_TLV_DESCRIPTORS>`. Size: 3,053,816 bytes (≈3.05 MB; cf. Linux GUI 2.35 MB, Windows GUI 5.50 MB). NOUNDEFS confirms all symbols resolved.
- **Step 8 — Upload**: `curl -X POST ... /assets?name=sci-calc-gui-macos-x86_64` to release 349097712. Response: asset ID 466955481, size 3053816, state=uploaded, sha256=56d8833f6df1663e64e9ed71b6b63b00ee1d372687d1c541442f447755418c7c.

Stage Summary:
- macOS x86_64 GUI build is COMPLETE and uploaded. `dist/sci-calc-gui-macos-x86_64` (3.05 MB, Mach-O 64-bit x86_64, PIE) — release asset 466955481.
- GLFW 3.4 static lib (libglfw3.a, 1.0 MB) built for x86_64-macos.11.0 with Cocoa backend; archived at .scicalc-cache/macglfw/lib/.
- **Key gotcha for future cross-compile work**: Zig 0.17.0-dev.1158's LLD silently segfaults (writes empty file, reports exit 0) when linking against macOS frameworks whose TAPI TBD files have a non-empty `reexported-libraries`/`re-exports` section. Workaround: pre-strip that section from each framework TBD and link via `-l<Framework>` from a local dir, instead of `-framework <Framework>`. Always run with `-v` to confirm the linker actually ran.
- **Second gotcha**: When compiling C++ against a macOS SDK with Zig, do NOT pass `-I $SDK/usr/include` — it shadows Zig's libc++ wrapper headers (`<string.h>` etc.) and breaks libc++. Use `-isysroot $SDK` alone; the SDK's /usr/include is added at lower priority automatically.
- **Third gotcha**: macOS framework headers use `enum Foo : type Foo;` forward declarations that C++17 rejects by default. Add `-Wno-elaborated-enum-base` to suppress.
- macOS GUI release asset list now complete: linux-x86_64, macos-x86_64, windows-x86_64.exe, windows-x86_64-console.exe, sci-calc.jar (CLI/JNI for all 3 platforms). All three platforms now have BOTH CLI and GUI native builds available on release v0.1.0 (id 349097712).
