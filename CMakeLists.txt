cmake_minimum_required(VERSION 3.20)
project(scientific-calculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==================== Options ====================
option(BUILD_NATIVE_APP "Build the native desktop application" ON)
option(BUILD_JNI_BRIDGE "Build the JNI bridge library" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(EMBED_FONTS "Embed font files into the binary" ON)

# ==================== Dependencies ====================
find_package(PkgConfig REQUIRED)
pkg_check_modules(GMP REQUIRED gmp)
pkg_check_modules(MPFR REQUIRED mpfr)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Dear ImGui (header-only, included as source)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")

# ==================== calc-core (static library) ====================
add_library(calc-core STATIC
    calc-core/src/main/cpp/src/big_decimal.cpp
    calc-core/src/main/cpp/src/expression.cpp
    calc-core/src/main/cpp/src/expression_parser.cpp
    calc-core/src/main/cpp/src/expression_eval.cpp
    calc-core/src/main/cpp/src/functions.cpp
    calc-core/src/main/cpp/src/function_registry.cpp
    calc-core/src/main/cpp/src/variable_store.cpp
    calc-core/src/main/cpp/src/set_operations.cpp
    calc-core/src/main/cpp/src/number_format.cpp
    calc-core/src/main/cpp/src/base_converter.cpp
    calc-core/src/main/cpp/src/crypto_random.cpp
)
target_include_directories(calc-core PUBLIC
    calc-core/src/main/cpp/public
    ${GMP_INCLUDE_DIRS}
    ${MPFR_INCLUDE_DIRS}
)
target_link_libraries(calc-core PUBLIC ${GMP_LIBRARIES} ${MPFR_LIBRARIES})
target_compile_options(calc-core PRIVATE -Wall -Wextra -O2 -fPIC)

# ==================== render-engine (static library) ====================
add_library(render-engine STATIC
    render-engine/src/main/cpp/src/math_renderer.cpp
    render-engine/src/main/cpp/src/font_manager.cpp
    render-engine/src/main/cpp/src/imgui_renderer.cpp
)
target_include_directories(render-engine PUBLIC
    render-engine/src/main/cpp/public
)
target_link_libraries(render-engine PUBLIC calc-core)
target_compile_options(render-engine PRIVATE -Wall -Wextra -O2 -fPIC)

# ==================== native-app (executable) ====================
if(BUILD_NATIVE_APP)
    # Dear ImGui sources
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    add_executable(calc
        native-app/src/main/cpp/main.cpp
        native-app/src/main/cpp/i18n.cpp
        native-app/src/main/cpp/embedded/fonts.cpp
        ${IMGUI_SOURCES}
    )
    target_include_directories(calc PRIVATE
        native-app/src/main/cpp
        native-app/src/main/cpp/embedded
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(calc PRIVATE
        calc-core
        render-engine
        OpenGL::GL
        glfw
        ${CMAKE_DL_LIBS}
    )

    # Platform-specific libraries
    if(UNIX AND NOT APPLE)
        target_link_libraries(calc PRIVATE pthread)
    endif()

    # Font embedding
    if(EMBED_FONTS)
        # Embed font files as binary resources using objcopy or incbin
        # For now, fonts are loaded at runtime from the fonts/ directory
        target_compile_definitions(calc PRIVATE FONTS_DIR="${CMAKE_SOURCE_DIR}/fonts")
    endif()

    # Install
    install(TARGETS calc RUNTIME DESTINATION bin)
endif()

# ==================== jni-bridge (shared library) ====================
if(BUILD_JNI_BRIDGE)
    find_package(JNI REQUIRED)

    add_library(calc-bridge SHARED
        jni-bridge/src/main/cpp/src/calc_jni.cpp
    )
    target_include_directories(calc-bridge PUBLIC
        jni-bridge/src/main/cpp/public
        ${JNI_INCLUDE_DIRS}
    )
    target_link_libraries(calc-bridge PUBLIC calc-core render-engine ${GMP_LIBRARIES} ${MPFR_LIBRARIES})
    set_target_properties(calc-bridge PROPERTIES
        PREFIX ""
        OUTPUT_NAME "calc-bridge"
    )

    install(TARGETS calc-bridge LIBRARY DESTINATION lib)
endif()

# ==================== Cross-compilation presets ====================
# Linux (native): cmake -B build-linux
# Windows (MinGW): cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
# macOS (Zig): cmake -B build-mac -DCMAKE_TOOLCHAIN_FILE=cmake/zig-macos.cmake
