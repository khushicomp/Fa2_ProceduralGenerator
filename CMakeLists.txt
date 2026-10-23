cmake_minimum_required(VERSION 3.20)
project(ProceduralCityGenerator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
# GLFW is pinned; disable unrelated targets to keep the first build small.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL https://codeload.github.com/glfw/glfw/zip/refs/tags/3.4
    URL_HASH SHA256=a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a
)
FetchContent_MakeAvailable(glfw)
find_package(OpenGL REQUIRED)

# Official unmodified Dear ImGui core and the backends matching OpenGL 2.1.
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/zip/refs/tags/v1.91.9b
    URL_HASH SHA256=fd37507c8476a6d14cc7c4b352401f31bcbd0f0d995d35390811e968c466f46e)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Generated GLAD sources are checked in: users do not need Python.
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

add_executable(ProceduralCityGenerator
    src/main.cpp
    src/CandidateApplication.cpp
    src/ApplicationInterface.cpp
    src/SmokeValidation.cpp
    src/CityGenerator.cpp
    src/Renderer.cpp
    src/TextureManager.cpp
    src/VisualAssets.cpp
    src/VisualComposition.cpp
    src/RoadGraphBuilder.cpp
    src/Emergency.cpp
    src/AccessibilityAnalyzer.cpp
    src/AccessibilityScorer.cpp
    src/CityOptimizer.cpp
)
target_include_directories(ProceduralCityGenerator PRIVATE include)
target_compile_definitions(ProceduralCityGenerator PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(ProceduralCityGenerator PRIVATE glad glfw OpenGL::GL imgui)
# Windows Imaging Component is part of Windows; no new third-party PNG loader.
if(WIN32)
    target_link_libraries(ProceduralCityGenerator PRIVATE windowscodecs ole32)
endif()
# Assets travel with the executable, independent of the launch working directory.
add_custom_target(CityVisualAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:ProceduralCityGenerator>/assets")
add_dependencies(ProceduralCityGenerator CityVisualAssets)

if(MSVC)
    target_compile_options(ProceduralCityGenerator PRIVATE /W4 /permissive-)
else()
    target_compile_options(ProceduralCityGenerator PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(CityGeneratorTests tests/CityGeneratorTests.cpp src/CityGenerator.cpp)
    target_include_directories(CityGeneratorTests PRIVATE include)
    add_test(NAME CityGeneratorTests COMMAND CityGeneratorTests)
    add_executable(CityBuildingTests tests/CityBuildingTests.cpp src/CityGenerator.cpp)
    target_include_directories(CityBuildingTests PRIVATE include)
    add_test(NAME CityBuildingTests COMMAND CityBuildingTests)
    add_executable(RoadGraphTests tests/RoadGraphTests.cpp src/RoadGraphBuilder.cpp src/CityGenerator.cpp)
    target_include_directories(RoadGraphTests PRIVATE include)
    add_test(NAME RoadGraphTests COMMAND RoadGraphTests)
    add_executable(AccessibilityTests tests/AccessibilityTests.cpp src/AccessibilityAnalyzer.cpp
        src/Emergency.cpp src/RoadGraphBuilder.cpp src/CityGenerator.cpp)
    target_include_directories(AccessibilityTests PRIVATE include)
    add_test(NAME AccessibilityTests COMMAND AccessibilityTests)
    add_executable(AccessibilityScoreTests tests/AccessibilityScoreTests.cpp
        src/AccessibilityScorer.cpp src/AccessibilityAnalyzer.cpp src/Emergency.cpp
        src/RoadGraphBuilder.cpp src/CityGenerator.cpp)
    target_include_directories(AccessibilityScoreTests PRIVATE include)
    add_test(NAME AccessibilityScoreTests COMMAND AccessibilityScoreTests)
    add_executable(CityOptimizerTests tests/CityOptimizerTests.cpp src/CityOptimizer.cpp
        src/AccessibilityScorer.cpp src/AccessibilityAnalyzer.cpp src/Emergency.cpp
        src/RoadGraphBuilder.cpp src/CityGenerator.cpp)
    target_include_directories(CityOptimizerTests PRIVATE include)
    add_test(NAME CityOptimizerTests COMMAND CityOptimizerTests)
    add_executable(CandidateApplicationTests tests/CandidateApplicationTests.cpp src/CandidateApplication.cpp
        src/CityOptimizer.cpp src/AccessibilityScorer.cpp src/AccessibilityAnalyzer.cpp src/Emergency.cpp
        src/RoadGraphBuilder.cpp src/CityGenerator.cpp)
    target_include_directories(CandidateApplicationTests PRIVATE include)
    add_test(NAME CandidateApplicationTests COMMAND CandidateApplicationTests)
    add_test(NAME GraphicsSmokeTest COMMAND ProceduralCityGenerator --smoke-test)
    add_test(NAME SeedDemoSmokeTest COMMAND ProceduralCityGenerator --seed 42 --smoke-test)
    add_test(NAME DemoSmokeTest COMMAND ProceduralCityGenerator --demo --smoke-test)
    add_test(NAME InvalidSeedTest COMMAND ProceduralCityGenerator --seed invalid)
    set_tests_properties(InvalidSeedTest PROPERTIES WILL_FAIL TRUE)
endif()

if(BUILD_TESTING AND WIN32)
    add_executable(VisualAssetTests tests/VisualAssetTests.cpp src/TextureManager.cpp src/VisualAssets.cpp)
    target_include_directories(VisualAssetTests PRIVATE include)
    target_compile_definitions(VisualAssetTests PRIVATE GLFW_INCLUDE_NONE
        VISUAL_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    target_link_libraries(VisualAssetTests PRIVATE glad glfw OpenGL::GL windowscodecs ole32)
    add_dependencies(VisualAssetTests CityVisualAssets)
    add_test(NAME VisualAssetTests COMMAND VisualAssetTests)
endif()
