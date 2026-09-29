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
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "EXECUTABLE")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E $<IF:$<CONFIG:Debug>,copy_if_different,true>
                    "${ODYSSEUS_ASAN_DLL}" "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()

# --- Core: logging, asserts, math, random, ids (layer rules arrive in US-003) ---
add_library(odysseus_core STATIC
    src/core/version.cpp)
target_include_directories(odysseus_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_definitions(odysseus_core PRIVATE ODYSSEUS_VERSION="${PROJECT_VERSION}")
odysseus_apply_settings(odysseus_core)

# --- Programs ---
add_executable(odysseus apps/odysseus/main.cpp)
target_link_libraries(odysseus PRIVATE odysseus_core)
odysseus_apply_settings(odysseus)

add_executable(odysseus_headless apps/headless/main.cpp)
target_link_libraries(odysseus_headless PRIVATE odysseus_core)
odysseus_apply_settings(odysseus_headless)

# Visual Studio: F5 starts the game.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT odysseus)

# --- Tests ---
enable_testing()
add_executable(odysseus_tests
    tests/main.cpp
    tests/core/version_test.cpp)
target_link_libraries(odysseus_tests PRIVATE odysseus_core doctest::doctest)
odysseus_apply_settings(odysseus_tests)
add_test(NAME odysseus_tests COMMAND odysseus_tests)
