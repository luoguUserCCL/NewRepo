#include "app_state.h"
#include "i18n.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <imgui.h>
#include <imgui_internal.h>

namespace calc {

/**
 * Main calculator UI — expression input, result display, and history.
 */
class UICalculator {
public:
    void draw(AppState& state) {
        auto& io = ImGui::GetIO();

        // Input area
        ImGui::SeparatorText(I18n::get("tab.calculator").c_str());

        // Expression input
        float inputWidth = ImGui::GetContentRegionAvail().x - 120.0f;
        ImGui::PushItemWidth(inputWidth);
        ImGui::InputText("##input", &state.inputText,
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button(I18n::get("input.evaluate").c_str())) {
            state.execute();
        }
        ImGui::SameLine();
        if (ImGui::Button(I18n::get("input.clear").c_str())) {
            state.clearInput();
        }

        // Result display
        if (state.hasError) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::Text("%s: %s", I18n::get("error").c_str(), state.lastError.c_str());
            ImGui::PopStyleColor();
        } else if (!state.lastResult.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::Text("%s: %s", I18n::get("result").c_str(), state.lastResult.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Format controls (inline)
        drawFormatControls(state);

        ImGui::Spacing();
        ImGui::Separator();

        // History
        ImGui::SeparatorText(I18n::get("history").c_str());
        if (state.history.empty()) {
            ImGui::TextDisabled("%s", I18n::get("history.empty").c_str());
        } else {
            // Show history in reverse order (newest first)
            for (int i = static_cast<int>(state.history.size()) - 1; i >= 0; i--) {
                const auto& entry = state.history[i];
                ImGui::PushID(i);
                if (entry.isError) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                }
                ImGui::Text(">> %s", entry.input.c_str());
                ImGui::Text("   %s", entry.result.c_str());
                if (entry.isError) {
                    ImGui::PopStyleColor();
                }

                // Click to re-use expression
                if (ImGui::IsItemClicked()) {
                    state.inputText = entry.input;
                }
                ImGui::PopID();
                ImGui::Spacing();
            }
        }
    }

private:
    void drawFormatControls(AppState& state) {
        // Format mode
        const char* formatModes[] = {
            I18n::get("mode.floating").c_str(),
            I18n::get("mode.scientific").c_str(),
            I18n::get("mode.fixed").c_str(),
            I18n::get("mode.significant").c_str()
        };
        int currentFormat = static_cast<int>(state.formatConfig.mode);
        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo(I18n::get("settings.format").c_str(), &currentFormat, formatModes, 4)) {
            state.formatConfig.mode = static_cast<FormatMode>(currentFormat);
            state.applyFormat();
        }

        ImGui::SameLine();

        // Base selection
        const char* bases[] = {
            I18n::get("base.decimal").c_str(),
            I18n::get("base.binary").c_str(),
            I18n::get("base.octal").c_str(),
            I18n::get("base.hex").c_str()
        };
        int baseValues[] = {10, 2, 8, 16};
        int currentBaseIdx = 0;
        for (int i = 0; i < 4; i++) {
            if (baseValues[i] == state.formatConfig.base) {
                currentBaseIdx = i;
                break;
            }
        }
        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo(I18n::get("settings.base").c_str(), &currentBaseIdx, bases, 4)) {
            state.formatConfig.base = baseValues[currentBaseIdx];
            state.applyFormat();
        }

        // Base prefix toggle
        ImGui::SameLine();
        if (ImGui::Checkbox(I18n::get("base.prefix").c_str(), &state.formatConfig.showBasePrefix)) {
            state.applyFormat();
        }
    }
};

} // namespace calc
