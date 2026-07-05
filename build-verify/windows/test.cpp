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
