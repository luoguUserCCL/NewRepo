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
