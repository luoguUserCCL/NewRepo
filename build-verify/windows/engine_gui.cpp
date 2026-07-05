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
