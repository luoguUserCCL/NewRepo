#include "app_state.h"
#include "ui_calculator.h"
#include "ui_function_def.h"
#include "ui_settings.h"
#include "i18n.h"
#include "embedded/fonts.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace calc {

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

/**
 * Main application entry point and main loop.
 * Sets up GLFW window, ImGui context, and runs the event loop.
 */
int runApp(int argc, char** argv) {
    // Setup GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return 1;

    // GL 3.2 + GLSL 150 (compatible with macOS)
    const char* glslVersion = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600,
        I18n::get("app.title").c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;

    // ==================== Font Loading ====================
    // Strategy: Latin Modern Math is embedded in the binary (essential for math rendering).
    // Noto Sans SC is loaded from system fonts (common on CJK-capable systems).
    // Falls back to ImGui default font if system CJK font is unavailable.

    ImFont* fontMath = nullptr;
    ImFont* fontCJK = nullptr;
    ImFont* fontUI = nullptr;

    // 1. Load embedded Latin Modern Math for formula rendering
    {
        size_t mathFontSize = 0;
        const uint8_t* mathFontData = calc::embedded::getLatinModernMath(mathFontSize);
        if (mathFontData && mathFontSize > 0) {
            // Load as the default font (covers ASCII + Latin + math symbols)
            ImFontConfig config;
            config.FontDataOwnedByAtlas = false; // Data is static, don't let ImGui free it
            fontMath = io.Fonts->AddFontFromMemoryTTF(
                const_cast<uint8_t*>(mathFontData),
                static_cast<int>(mathFontSize), 18.0f, &config);
            fontUI = fontMath;
            fprintf(stderr, "[Font] Loaded embedded Latin Modern Math (%zu bytes)\n", mathFontSize);
        } else {
            fprintf(stderr, "[Font] WARNING: Embedded Latin Modern Math not available\n");
        }
    }

    // 2. Load Noto Sans SC (or other CJK font) from system for Chinese UI
    {
        auto result = calc::embedded::findNotoSansSC();
        if (!result.found) {
            result = calc::embedded::findCJKFont();
        }

        if (result.found) {
            // Load CJK font with full CJK glyph range
            static const ImWchar cjkRanges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x3000, 0x30FF, // CJK Symbols, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
                0x4E00, 0x9FFF, // CJK Unified Ideographs
                0x3400, 0x4DBF, // CJK Extension A
                0x2000, 0x206F, // General Punctuation
                0x2070, 0x209F, // Superscripts and Subscripts
                0x20A0, 0x20CF, // Currency Symbols
                0x2100, 0x214F, // Letterlike Symbols
                0x2150, 0x218F, // Number Forms
                0x2190, 0x21FF, // Arrows
                0x2200, 0x22FF, // Mathematical Operators
                0x2300, 0x23FF, // Miscellaneous Technical
                0x2500, 0x257F, // Box Drawing
                0x25A0, 0x25FF, // Geometric Shapes
                0x2600, 0x26FF, // Miscellaneous Symbols
                // Note: Mathematical Alphanumeric Symbols (0x1D400-0x1D7FF) omitted -
                // exceeds 16-bit ImWchar range and causes narrowing conversion error
                0,
            };
            fontCJK = io.Fonts->AddFontFromFileTTF(
                result.path.c_str(), 18.0f, nullptr, cjkRanges);
            if (fontCJK) {
                fprintf(stderr, "[Font] Loaded CJK font from: %s\n", result.path.c_str());
            }
        }

        if (!fontCJK) {
            fprintf(stderr, "[Font] WARNING: No CJK font found. Chinese text may not render.\n");
            // Merge CJK ranges into the math font as a fallback
            if (fontMath) {
                static const ImWchar fallbackCjk[] = {
                    0x3000, 0x30FF, 0xFF00, 0xFFEF,
                    0x4E00, 0x9FFF, 0x3400, 0x4DBF, 0,
                };
                ImFontConfig mergeConfig;
                mergeConfig.MergeMode = true;
                // Try to find any system font for CJK fallback
                auto cjkResult = calc::embedded::findCJKFont();
                if (cjkResult.found) {
                    io.Fonts->AddFontFromFileTTF(
                        cjkResult.path.c_str(), 18.0f, &mergeConfig, fallbackCjk);
                }
            }
        }
    }

    // Build font atlas
    io.Fonts->Build();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    // Application state
    AppState state;
    UICalculator calcUI;
    UIFunctionDef funcUI;
    UISettings settingsUI;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin(I18n::get("app.title").c_str(), nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(I18n::get("menu.file").c_str())) {
                if (ImGui::MenuItem(I18n::get("btn.reset").c_str())) {
                    state.engine.reset();
                    state.history.clear();
                }
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(I18n::get("menu.settings").c_str())) {
                if (ImGui::MenuItem(I18n::get("settings.title").c_str())) {
                    state.settingsOpen = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Tab bar for main content
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem(I18n::get("tab.calculator").c_str())) {
                calcUI.draw(state);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(I18n::get("tab.functions").c_str())) {
                funcUI.draw(state);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(I18n::get("tab.variables").c_str())) {
                funcUI.draw(state); // Shares function def UI
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // Settings popup
        settingsUI.draw(state);

        ImGui::End();

        // Rendering
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

} // namespace calc

int main(int argc, char** argv) {
    return calc::runApp(argc, argv);
}
