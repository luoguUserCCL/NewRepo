#ifndef CALC_APP_STATE_H
#define CALC_APP_STATE_H

#include <cstring>
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

    // Char buffers for ImGui::InputText (ImGui requires char*, not std::string)
    char inputBuf[1024] = {};
    char newFuncNameBuf[256] = {};
    char newFuncParamsBuf[256] = {};
    char newFuncBodyBuf[1024] = {};
    char newVarNameBuf[256] = {};
    char newVarValueBuf[1024] = {};

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

    // Function/Variable definition UI state (std::string counterparts for logic)
    std::string newFuncName;
    std::string newFuncParams;
    std::string newFuncBody;
    std::string newVarName;
    std::string newVarValue;

    /// Sync char buffers to std::strings (call before logic that reads the strings)
    void syncBuffersToStrings() {
        inputText = inputBuf;
        newFuncName = newFuncNameBuf;
        newFuncParams = newFuncParamsBuf;
        newFuncBody = newFuncBodyBuf;
        newVarName = newVarNameBuf;
        newVarValue = newVarValueBuf;
    }

    /// Sync std::strings back to char buffers (call after logic that modifies the strings)
    void syncStringsToBuffers() {
        std::strncpy(inputBuf, inputText.c_str(), sizeof(inputBuf) - 1);
        inputBuf[sizeof(inputBuf) - 1] = '\0';
        std::strncpy(newFuncNameBuf, newFuncName.c_str(), sizeof(newFuncNameBuf) - 1);
        newFuncNameBuf[sizeof(newFuncNameBuf) - 1] = '\0';
        std::strncpy(newFuncParamsBuf, newFuncParams.c_str(), sizeof(newFuncParamsBuf) - 1);
        newFuncParamsBuf[sizeof(newFuncParamsBuf) - 1] = '\0';
        std::strncpy(newFuncBodyBuf, newFuncBody.c_str(), sizeof(newFuncBodyBuf) - 1);
        newFuncBodyBuf[sizeof(newFuncBodyBuf) - 1] = '\0';
        std::strncpy(newVarNameBuf, newVarName.c_str(), sizeof(newVarNameBuf) - 1);
        newVarNameBuf[sizeof(newVarNameBuf) - 1] = '\0';
        std::strncpy(newVarValueBuf, newVarValue.c_str(), sizeof(newVarValueBuf) - 1);
        newVarValueBuf[sizeof(newVarValueBuf) - 1] = '\0';
    }

    /// Execute the current input
    void execute() {
        syncBuffersToStrings();
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
        inputBuf[0] = '\0';
    }

    /// Apply format config to engine
    void applyFormat() {
        engine.formatConfig() = formatConfig;
    }
};

} // namespace calc

#endif // CALC_APP_STATE_H
