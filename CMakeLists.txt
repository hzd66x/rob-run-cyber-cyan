cmake_minimum_required(VERSION 3.24)
project(RobRunCyberCyan LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "This project targets Windows 10+ with DirectX 11.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_executable(RobRunCyberCyan WIN32
  src/main.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
  ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp)

target_include_directories(RobRunCyberCyan PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends)
target_link_libraries(RobRunCyberCyan PRIVATE
  nlohmann_json::nlohmann_json
  d3d11 dxgi d3dcompiler dwmapi)
target_compile_definitions(RobRunCyberCyan PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
  target_compile_options(RobRunCyberCyan PRIVATE /W4 /permissive- /EHsc)
endif()
