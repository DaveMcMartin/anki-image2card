cmake_minimum_required(VERSION 3.25)

project(AnkiImage2Card VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG preview-3.1.3
)

set(SDL_SHARED ON CACHE BOOL "Build shared SDL library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build static SDL library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable installing SDL" FORCE)

FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(cpp-httplib)

add_library(imgui_sdl3 STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui_sdl3 PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)

target_link_libraries(imgui_sdl3 PUBLIC SDL3::SDL3)

# Force re-scan of source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

if(APPLE)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/assets/logo.icns")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/assets/logo.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif(WIN32)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/assets/windows_icon.rc")
endif()

add_executable(AnkiImage2Card MACOSX_BUNDLE ${SOURCES})

target_include_directories(AnkiImage2Card PRIVATE
    src
    src/core
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/assets
)

target_link_libraries(AnkiImage2Card PRIVATE
    imgui_sdl3
    SDL3::SDL3
    nlohmann_json::nlohmann_json
    httplib::httplib
)

if(APPLE)
    add_custom_command(TARGET AnkiImage2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AnkiImage2Card>/../Resources/assets
    )
else()
    add_custom_command(TARGET AnkiImage2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AnkiImage2Card>/assets
    )
endif()

if(WIN32)
    set_target_properties(AnkiImage2Card PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
elseif(APPLE)
    set_target_properties(AnkiImage2Card PROPERTIES
        MACOSX_BUNDLE_ICON_FILE logo.icns
        MACOSX_BUNDLE_BUNDLE_NAME "Anki Image2Card"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ankiimage2card.app"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
    )
endif()
