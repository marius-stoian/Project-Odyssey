# Project Odyssey build. Start here: cmake --preset windows-x64-debug
cmake_minimum_required(VERSION 3.28)

# The version number lives only here; Core reads it through ODYSSEUS_VERSION.
project(Odysseus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# All programs land in one folder per configuration: build/windows-x64/bin/Debug/...
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NOT MSVC)
    message(FATAL_ERROR "Odysseus builds with MSVC (ADR-001). Use a windows-x64 preset.")
endif()

# AddressSanitizer (ADR-015) cannot run together with MSVC's own runtime checks (/RTC1)
# or incremental linking, so Debug drops both.
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
foreach(flags_var CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_DEBUG)
    string(REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" ${flags_var} "${${flags_var}}")
endforeach()

# Libraries come from vcpkg.json. If one is missing there, find_package stops here
# with a message that names the library.
find_package(doctest CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

# The AddressSanitizer runtime DLL sits next to the compiler. Debug programs need it
# beside them to start outside Visual Studio (double-click, ctest).
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH msvc_bin_dir)
set(ODYSSEUS_ASAN_DLL "${msvc_bin_dir}/clang_rt.asan_dynamic-x86_64.dll")
if(NOT EXISTS "${ODYSSEUS_ASAN_DLL}")
    message(FATAL_ERROR "AddressSanitizer runtime not found at ${ODYSSEUS_ASAN_DLL}. "
                        "Install the Visual Studio component 'C++ AddressSanitizer'.")
endif()

# Settings every target we write gets: strict warnings that fail the build,
# and AddressSanitizer in Debug. Library code from vcpkg is not affected.
function(odysseus_apply_settings target)
    target_compile_options(${target} PRIVATE
        /W4 /WX /permissive- /utf-8
        $<$<CONFIG:Debug>:/fsanitize=address>)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "EXECUTABLE")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E $<IF:$<CONFIG:Debug>,copy_if_different,true>
                    "${ODYSSEUS_ASAN_DLL}" "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()

# --- Five layers: dependency arrows point down only (ADR-004, ADR-016). ---
include(cmake/LayerRules.cmake)
add_library(odysseus_core STATIC
    src/core/version.cpp
    src/core/log.cpp
    src/core/assertions.cpp)
odysseus_register_layer(odysseus_core core core)
target_compile_definitions(odysseus_core PRIVATE ODYSSEUS_VERSION="${PROJECT_VERSION}")
odysseus_apply_settings(odysseus_core)

add_library(luna_platform STATIC
    src/luna/platform/layer.cpp
    src/luna/platform/sdl_events.cpp
    src/luna/platform/system.cpp
    src/luna/platform/user_paths.cpp
    src/luna/platform/window.cpp)
target_link_libraries(luna_platform PUBLIC odysseus_core)
# PRIVATE: only Platform sees SDL3 headers (Charter rule 2); callers still link it.
target_link_libraries(luna_platform PRIVATE SDL3::SDL3)
odysseus_register_layer(luna_platform platform luna/platform)
odysseus_apply_settings(luna_platform)

add_library(luna_engine STATIC
    src/luna/engine/layer.cpp
    src/luna/engine/application.cpp
    src/luna/engine/fixed_step_clock.cpp
    src/luna/engine/frame_stats.cpp
    src/luna/engine/input.cpp)
target_link_libraries(luna_engine PUBLIC luna_platform)
odysseus_register_layer(luna_engine engine luna/engine)
odysseus_apply_settings(luna_engine)

add_library(odysseus_sim STATIC src/sim/layer.cpp)
target_link_libraries(odysseus_sim PUBLIC odysseus_core)
odysseus_register_layer(odysseus_sim sim sim)
odysseus_apply_settings(odysseus_sim)

add_library(odysseus_game STATIC
    src/game/layer.cpp
    src/game/odyssey_game.cpp)
target_link_libraries(odysseus_game PUBLIC luna_engine odysseus_sim)
odysseus_register_layer(odysseus_game game game)
odysseus_apply_settings(odysseus_game)

# --- Programs ---
add_executable(odysseus apps/odysseus/main.cpp)
target_link_libraries(odysseus PRIVATE odysseus_game)
target_compile_definitions(odysseus PRIVATE ODYSSEUS_LAYER_GAME)
odysseus_apply_settings(odysseus)

add_executable(odysseus_headless apps/headless/main.cpp)
target_link_libraries(odysseus_headless PRIVATE odysseus_sim)
target_compile_definitions(odysseus_headless PRIVATE ODYSSEUS_LAYER_SIM)
odysseus_apply_settings(odysseus_headless)

# Visual Studio: F5 starts the game.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT odysseus)

# --- Tests ---
enable_testing()
add_executable(odysseus_tests
    tests/main.cpp
    tests/core/version_test.cpp
    tests/core/log_test.cpp
    tests/architecture/layer_rules_test.cpp)
target_link_libraries(odysseus_tests PRIVATE odysseus_core doctest::doctest)
target_compile_definitions(odysseus_tests PRIVATE
    ODYSSEUS_LAYER_CORE
    ODYSSEUS_BUILD_DIR="${CMAKE_BINARY_DIR}"
    ODYSSEUS_TEST_CONFIGURATION="$<CONFIG>"
    ODYSSEUS_PROBE_SCRIPT="${PROJECT_SOURCE_DIR}/tests/architecture/run_probe.cmake"
    ODYSSEUS_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    ODYSSEUS_VALIDATOR_SCRIPT="${PROJECT_SOURCE_DIR}/tests/architecture/run_validator.cmake"
    ODYSSEUS_ASSERT_PROBE="$<TARGET_FILE:us004_assert_probe>"
    ODYSSEUS_ASSERT_PROBE_LINE=13)
odysseus_apply_settings(odysseus_tests)

# US-004: a tiny program that fires a false assert on purpose (line 13 of assert_probe.cpp).
add_executable(us004_assert_probe tests/core/assert_probe.cpp)
target_link_libraries(us004_assert_probe PRIVATE odysseus_core)
target_compile_definitions(us004_assert_probe PRIVATE ODYSSEUS_LAYER_CORE)
odysseus_apply_settings(us004_assert_probe)
add_dependencies(odysseus_tests us004_assert_probe)
add_subdirectory(tests/architecture)
# Architecture scenarios invoke the build, so serialize them even under ctest -j.
add_test(NAME odysseus_tests COMMAND odysseus_tests "--test-case-exclude=US-003*")
foreach(scenario IN ITEMS "Allowed use" "Forbidden use" "Luna stays game-agnostic" "Guard completeness")
    set(test_name "US-003 ${scenario}")
    add_test(NAME "${test_name}" COMMAND odysseus_tests "--test-case=${test_name}")
    set_tests_properties("${test_name}" PROPERTIES RESOURCE_LOCK architecture_build)
endforeach()

# --- Luna tests: headless unit tests with the Engine identity (Engine, Platform, Core) ---
add_executable(luna_tests
    tests/main.cpp
    tests/luna/input_test.cpp
    tests/luna/loop_test.cpp)
target_link_libraries(luna_tests PRIVATE luna_engine doctest::doctest)
target_compile_definitions(luna_tests PRIVATE ODYSSEUS_LAYER_ENGINE)
odysseus_apply_settings(luna_tests)
add_test(NAME luna_tests COMMAND luna_tests)

# --- End-to-end: the real game window (US-020 Open and Close) ---
add_test(NAME "US-020 Open and close"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us020 -DQUIT_AFTER=3
            -P "${PROJECT_SOURCE_DIR}/tests/luna/run_game_window.cmake")
set_tests_properties("US-020 Open and close" PROPERTIES LABELS window TIMEOUT 120)

# --- Platform tests: Platform identity, may use SDL3 (US-021 event translation) ---
add_executable(luna_platform_tests
    tests/main.cpp
    tests/luna/sdl_events_test.cpp)
target_link_libraries(luna_platform_tests PRIVATE luna_platform SDL3::SDL3 doctest::doctest)
target_compile_definitions(luna_platform_tests PRIVATE ODYSSEUS_LAYER_PLATFORM)
odysseus_apply_settings(luna_platform_tests)
add_test(NAME luna_platform_tests COMMAND luna_platform_tests)

# --- US-021: Game code reads intents only (ARC-03) ---
add_test(NAME "US-021 Game reads only intents"
    COMMAND "${CMAKE_COMMAND}" -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -P "${PROJECT_SOURCE_DIR}/tests/luna/check_game_reads_intents.cmake")
