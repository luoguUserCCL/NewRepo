#ifndef CALC_APP_STATE_H
#define CALC_APP_STATE_H

#include <string>
#include <vector>
#include <calc/calc_engine.h>
#include <calc/number_format.h>

namespace calc {

/**
 * Global application state shared across UI components.
 * Holds the calculator engine, format settings, and history.
 */
struct AppState {
    // Engine
    CalcEngine engine;

    // Input state
    std::string inputText;
    std::string lastResult;
    std::string lastError;
    bool hasError = false;

    // Format configuration
    NumberFormatConfig formatConfig;

    // History
    struct HistoryEntry {
        std::string input;
        std::string result;
        bool isError;
    };
    std::vector<HistoryEntry> history;
    static constexpr size_t MAX_HISTORY = 100;

    // UI state
    int currentTab = 0;         ///< 0=Calculator, 1=Functions, 2=Variables
    bool settingsOpen = false;

    // Function/Variable definition UI state
    std::string newFuncName;
    std::string newFuncParams;
    std::string newFuncBody;
    std::string newVarName;
    std::string newVarValue;

    /// Execute the current input
    void execute() {
        if (inputText.empty()) return;

        try {
            auto result = engine.evaluate(inputText);
            if (!engine.lastError().empty()) {
                lastError = engine.lastError();
                hasError = true;
                lastResult.clear();
            } else {
                lastResult = engine.formatResult(result);
                hasError = false;
                lastError.clear();
            }
        } catch (const std::exception& e) {
            lastError = e.what();
            hasError = true;
            lastResult.clear();
        }

        // Add to history
        if (history.size() >= MAX_HISTORY) {
            history.erase(history.begin());
        }
        history.push_back({inputText, hasError ? lastError : lastResult, hasError});
    }

    /// Clear input and results
    void clearInput() {
        inputText.clear();
        lastResult.clear();
        lastError.clear();
        hasError = false;
    }

    /// Apply format config to engine
    void applyFormat() {
        engine.formatConfig() = formatConfig;
    }
};

} // namespace calc

#endif // CALC_APP_STATE_H
