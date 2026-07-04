#include "app_state.h"
#include "i18n.h"
#include <imgui.h>

namespace calc {

/**
 * Function and Variable definition UI.
 */
class UIFunctionDef {
public:
    void draw(AppState& state) {
        if (ImGui::BeginTabBar("DefTabs")) {
            // ===== Function Definition Tab =====
            if (ImGui::BeginTabItem(I18n::get("func.define").c_str())) {
                ImGui::Spacing();

                ImGui::Text("%s:", I18n::get("func.name").c_str());
                ImGui::SameLine();
                ImGui::PushItemWidth(120);
                ImGui::InputText("##funcname", state.newFuncNameBuf, sizeof(state.newFuncNameBuf));
                ImGui::PopItemWidth();

                ImGui::Text("%s:", I18n::get("func.params").c_str());
                ImGui::SameLine();
                ImGui::PushItemWidth(200);
                ImGui::InputText("##funcparams", state.newFuncParamsBuf, sizeof(state.newFuncParamsBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextDisabled("(e.g. x,y)");

                ImGui::Text("%s:", I18n::get("func.body").c_str());
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##funcbody", state.newFuncBodyBuf, sizeof(state.newFuncBodyBuf));
                ImGui::PopItemWidth();

                if (ImGui::Button(I18n::get("btn.add").c_str())) {
                    // Sync buffers before reading
                    state.syncBuffersToStrings();
                    // Parse parameter list
                    std::vector<std::string> params;
                    std::string param;
                    for (char c : state.newFuncParams) {
                        if (c == ',' || c == ' ') {
                            if (!param.empty()) {
                                params.push_back(param);
                                param.clear();
                            }
                        } else {
                            param += c;
                        }
                    }
                    if (!param.empty()) params.push_back(param);

                    // Define function via engine
                    std::string definition = state.newFuncName + "(" + state.newFuncParams + ") := " + state.newFuncBody;
                    state.engine.evaluate(definition);

                    // Clear inputs
                    state.newFuncName.clear();
                    state.newFuncParams.clear();
                    state.newFuncBody.clear();
                    state.newFuncNameBuf[0] = '\0';
                    state.newFuncParamsBuf[0] = '\0';
                    state.newFuncBodyBuf[0] = '\0';
                }

                ImGui::Spacing();
                ImGui::Separator();

                // List user-defined functions
                auto funcNames = state.engine.functions().listUserFunctions();
                for (const auto& name : funcNames) {
                    ImGui::BulletText("%s", name.c_str());
                    ImGui::SameLine();
                    ImGui::PushID(name.c_str());
                    if (ImGui::SmallButton(I18n::get("btn.remove").c_str())) {
                        state.engine.functions().removeUserFunction(name);
                    }
                    ImGui::PopID();
                }

                ImGui::EndTabItem();
            }

            // ===== Variable Definition Tab =====
            if (ImGui::BeginTabItem(I18n::get("var.define").c_str())) {
                ImGui::Spacing();

                ImGui::Text("%s:", I18n::get("var.name").c_str());
                ImGui::SameLine();
                ImGui::PushItemWidth(120);
                ImGui::InputText("##varname", state.newVarNameBuf, sizeof(state.newVarNameBuf));
                ImGui::PopItemWidth();

                ImGui::Text("%s:", I18n::get("var.value").c_str());
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##varvalue", state.newVarValueBuf, sizeof(state.newVarValueBuf));
                ImGui::PopItemWidth();

                if (ImGui::Button(I18n::get("btn.add").c_str())) {
                    state.syncBuffersToStrings();
                    std::string definition = state.newVarName + " := " + state.newVarValue;
                    state.engine.evaluate(definition);
                    state.newVarName.clear();
                    state.newVarValue.clear();
                    state.newVarNameBuf[0] = '\0';
                    state.newVarValueBuf[0] = '\0';
                }

                ImGui::Spacing();
                ImGui::Separator();

                // List all variables
                auto varNames = state.engine.variables().listVariables();
                for (const auto& name : varNames) {
                    auto* val = state.engine.variables().get(name);
                    if (val) {
                        ImGui::BulletText("%s = %s", name.c_str(),
                            val->toString(state.formatConfig.mode, 0, state.formatConfig.base).c_str());
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
};

} // namespace calc
