#include "app_state.h"
#include "ui_calculator.h"
#include "ui_function_def.h"
#include "ui_settings.h"
#include "i18n.h"

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

    // Load fonts
    // TODO: Load Latin Modern Math and Noto Sans SC from embedded data
    // For now use default font + CJK support
    // ImFont* fontLatin = io.Fonts->AddFontFromFileTTF("fonts/latinmodern-math.otf", 18.0f);
    // ImFont* fontCJK = io.Fonts->AddFontFromFileTTF("fonts/NotoSansSC-Regular.otf", 18.0f);

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
