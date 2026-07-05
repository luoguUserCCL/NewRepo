// ============================================================================
//  sci-calc CLI — interactive scientific calculator REPL
//
//  Builds into a standalone executable for each platform:
//    Linux:   ELF, statically linked GMP/MPFR
//    Windows: PE32+ console (or -mwindows GUI), statically linked
//    macOS:   Mach-O, statically linked
//
//  No JVM required at runtime — pure native C++.
// ============================================================================
#include <calc/calc_engine.h>
#include <calc/number_format.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
// windows.h defines IN/OUT macros; calc headers already included above so the
// BinaryOp::IN enum is safe.
#endif

using namespace calc;

static void printHelp() {
    std::cout <<
        "sci-calc — high-precision scientific calculator\n"
        "\n"
        "Commands:\n"
        "  <expression>    Evaluate and print the result\n"
        "  :mode <mode>    Set output mode: decimal | math\n"
        "  :format <mode>  Set number format: float | scientific | fixed | sig\n"
        "  :base <n>       Set display base: 2 | 8 | 10 | 16\n"
        "  :precision <n>  Set MPFR precision in bits (default 256)\n"
        "  :vars           List defined variables\n"
        "  :funcs          List defined functions\n"
        "  :reset          Clear all user variables and functions\n"
        "  :help           Show this help\n"
        "  :quit           Exit\n"
        "\n"
        "Examples:\n"
        "  > 2 + 3 * 4\n"
        "  > sqrt(2)\n"
        "  > sum(i, 1, 100, i)        # 5050\n"
        "  > x := 42\n"
        "  > f(n) := n * n + 1\n"
        "  > f(5)                     # 26\n"
        "  > Iverson(3 > 2)           # 1\n"
        "  > :mode math\n"
        "  > 10 / 4                   # 5/2\n"
        "  > sqrt(8)                  # 2√2\n"
        "\n";
}

static std::string versionString() {
    return "sci-calc 0.1.0 (calc-core, MPFR-backed, 256-bit default precision)";
}

int main(int argc, char** argv) {
    CalcEngine engine;

    // Batch mode: evaluate expressions passed as arguments
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--version" || arg == "-v") {
            std::cout << versionString() << "\n";
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
        // Join all args as a single expression
        std::ostringstream ss;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) ss << " ";
            ss << argv[i];
        }
        auto r = engine.evaluate(ss.str());
        if (engine.lastError().empty()) {
            std::cout << engine.formatResult(r) << "\n";
            return 0;
        } else {
            std::cerr << "Error: " << engine.lastError() << "\n";
            return 1;
        }
    }

    // Interactive REPL
    std::cout << versionString() << "\n";
    std::cout << "Type :help for commands, :quit to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        // Command processing
        if (line[0] == ':') {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            cmd = cmd.substr(1);  // strip ':'

            if (cmd == "quit" || cmd == "exit") break;
            if (cmd == "help") { printHelp(); continue; }
            if (cmd == "reset") {
                engine.reset();
                std::cout << "Cleared user definitions.\n";
                continue;
            }
            if (cmd == "vars") {
                auto vars = engine.variables().listVariables();
                if (vars.empty()) {
                    std::cout << "(no user variables)\n";
                } else {
                    for (const auto& name : vars) {
                        std::cout << "  " << name << " = "
                                  << engine.variables().get(name)->toString() << "\n";
                    }
                }
                continue;
            }
            if (cmd == "funcs") {
                auto funcs = engine.functions().listUserFunctions();
                if (funcs.empty()) {
                    std::cout << "(no user functions)\n";
                } else {
                    for (const auto& f : funcs) {
                        std::cout << "  " << f << "\n";
                    }
                }
                continue;
            }
            if (cmd == "mode") {
                std::string m; iss >> m;
                if (m == "decimal") {
                    engine.setOutputMode(OutputMode::DECIMAL);
                    std::cout << "Output mode: DECIMAL\n";
                } else if (m == "math") {
                    engine.setOutputMode(OutputMode::MATH);
                    std::cout << "Output mode: MATH (fractions, radicals)\n";
                } else {
                    std::cout << "Usage: :mode decimal|math\n";
                }
                continue;
            }
            if (cmd == "format") {
                std::string m; iss >> m;
                auto& fmt = engine.formatConfig();
                if (m == "float")        fmt.mode = FormatMode::FLOATING_POINT;
                else if (m == "scientific") fmt.mode = FormatMode::SCIENTIFIC;
                else if (m == "fixed")      fmt.mode = FormatMode::FIXED_POINT;
                else if (m == "sig")        fmt.mode = FormatMode::SIGNIFICANT_DIGITS;
                else { std::cout << "Usage: :format float|scientific|fixed|sig\n"; continue; }
                std::cout << "Format: " << m << "\n";
                continue;
            }
            if (cmd == "base") {
                int b; iss >> b;
                if (b == 2 || b == 8 || b == 10 || b == 16) {
                    engine.formatConfig().base = b;
                    std::cout << "Base: " << b << "\n";
                } else {
                    std::cout << "Usage: :base 2|8|10|16\n";
                }
                continue;
            }
            if (cmd == "precision") {
                int p; iss >> p;
                if (p >= 32 && p <= 4096) {
                    BigDecimal::setDefaultPrecision(p);
                    std::cout << "Precision: " << p << " bits\n";
                } else {
                    std::cout << "Usage: :precision <32-4096>\n";
                }
                continue;
            }
            std::cout << "Unknown command: :" << cmd << " (try :help)\n";
            continue;
        }

        // Expression evaluation
        try {
            auto r = engine.evaluate(line);
            if (engine.lastError().empty()) {
                std::cout << "= " << engine.formatResult(r) << "\n";
            } else {
                std::cout << "Error: " << engine.lastError() << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    std::cout << "Goodbye.\n";
    return 0;
}
