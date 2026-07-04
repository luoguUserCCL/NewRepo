#include "app_state.h"
#include "i18n.h"
#include <imgui.h>

namespace calc {

/**
 * Settings panel — language, format, precision configuration.
 */
class UISettings {
public:
    void draw(AppState& state) {
        if (!state.settingsOpen) return;

        ImGui::OpenPopup(I18n::get("settings.title").c_str());
        if (ImGui::BeginPopupModal(I18n::get("settings.title").c_str(), &state.settingsOpen)) {
            // Language selection
            const char* languages[] = {
                I18n::get("lang.english").c_str(),
                I18n::get("lang.chinese").c_str()
            };
            int currentLang = static_cast<int>(I18n::getLanguage());
            if (ImGui::Combo(I18n::get("settings.language").c_str(), &currentLang, languages, 2)) {
                I18n::setLanguage(static_cast<I18n::Language>(currentLang));
            }

            ImGui::Spacing();

            // Precision
            static int precision = BigDecimal::DEFAULT_PRECISION;
            if (ImGui::SliderInt(I18n::get("precision").c_str(), &precision, 32, 1024)) {
                BigDecimal::setDefaultPrecision(precision);
            }

            ImGui::Spacing();

            // Significant digits
            static int sigDigits = 0;
            if (ImGui::InputInt(I18n::get("digits").c_str(), &sigDigits, 1, 10)) {
                if (sigDigits < 0) sigDigits = 0;
                state.formatConfig.significantDigits = sigDigits;
                state.applyFormat();
            }

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::Button(I18n::get("btn.reset").c_str())) {
                state.engine.reset();
                state.history.clear();
            }

            ImGui::EndPopup();
        }
    }
};

} // namespace calc
