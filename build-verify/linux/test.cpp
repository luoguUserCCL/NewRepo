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
