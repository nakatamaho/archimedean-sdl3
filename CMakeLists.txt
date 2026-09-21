cmake_minimum_required(VERSION 3.20)

project(archimedean_sdl3 VERSION 1.2.0 LANGUAGES CXX)

option(ARCHVIEW_USE_SYSTEM_DEPS
       "Use system-installed SDL3 and nlohmann_json packages"
       OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

set(ARCHVIEW_EMBEDDED_MODEL_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_model.cpp"
)
add_custom_command(
    OUTPUT "${ARCHVIEW_EMBEDDED_MODEL_SOURCE}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/data/archimedean.json
            -DOUTPUT=${ARCHVIEW_EMBEDDED_MODEL_SOURCE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_json.cmake
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/data/archimedean.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_json.cmake"
    VERBATIM
)

if(NOT ARCHVIEW_USE_SYSTEM_DEPS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt")
        message(FATAL_ERROR
                "SDL submodule is missing; run git submodule update --init --recursive")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json/CMakeLists.txt")
        message(FATAL_ERROR
                "nlohmann/json submodule is missing; run git submodule update --init --recursive")
    endif()

    set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "Disable SDL examples" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL test library" FORCE)
    set(SDL_INSTALL_TESTS OFF CACHE BOOL "Disable SDL test installation" FORCE)
    set(SDL_X11_XTEST OFF CACHE BOOL "Disable optional XTEST backend" FORCE)
    set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "Disable optional XScreenSaver backend" FORCE)
    if(MINGW)
        set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
        set(SDL_STATIC ON CACHE BOOL "Build SDL3 as a static library" FORCE)
    endif()
    add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(external/json EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
endif()

add_library(archview_core
    "${ARCHVIEW_EMBEDDED_MODEL_SOURCE}"
    src/math3d.cpp
    src/polyhedron.cpp
)
target_include_directories(archview_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(archview_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_library(archview_renderer
    src/renderer.cpp
)
target_include_directories(archview_renderer
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(archview_renderer
    PUBLIC
        archview_core
        SDL3::SDL3
)

add_executable(archimedean_viewer
    src/main.cpp
)
if(APPLE)
    set_target_properties(archimedean_viewer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.archview.archimedean-sdl3"
        MACOSX_BUNDLE_BUNDLE_NAME "Archimedean SDL3 Viewer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
target_link_libraries(archimedean_viewer
    PRIVATE
        archview_renderer
)

add_executable(archview_tests
    tests/test_json.cpp
    tests/test_math.cpp
    tests/test_renderer.cpp
    tests/test_topology.cpp
    tests/test_viewer_state.cpp
)
target_link_libraries(archview_tests
    PRIVATE
        archview_renderer
)
target_compile_definitions(archview_tests
    PRIVATE
        ARCHVIEW_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(archview_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(archview_renderer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(archimedean_viewer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(archview_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME archview_tests COMMAND archview_tests)
    add_test(
        NAME archview_embedded_selftest
        COMMAND $<TARGET_FILE:archimedean_viewer> --selftest
    )
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME archview_validator
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_archimedean.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/data/archimedean.json"
    )
    add_test(
        NAME archview_validator_negative
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_validate_archimedean.py"
    )
endif()

if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(
        TARGET archimedean_viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3-shared>"
                "$<TARGET_FILE_DIR:archimedean_viewer>"
        VERBATIM
    )
endif()
