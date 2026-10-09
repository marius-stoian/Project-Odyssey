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
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED) # PNG reading and writing (ADR-018)

# Content (JSON, sprites) lives in assets/; programs find it here during development.
set(ODYSSEUS_DATA_DIR "${PROJECT_SOURCE_DIR}/assets/data")

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

# --- Six layers: dependency arrows point down only (ADR-004, ADR-016, ARC-10). ---
include(cmake/LayerRules.cmake)
add_library(odysseus_core STATIC
    src/core/version.cpp
    src/core/log.cpp
    src/core/assertions.cpp
    src/core/presentation.cpp
    src/core/random.cpp
    src/core/text.cpp)
odysseus_register_layer(odysseus_core core core)
target_compile_definitions(odysseus_core PRIVATE ODYSSEUS_VERSION="${PROJECT_VERSION}")
odysseus_apply_settings(odysseus_core)

# The GPU renderer (US-230, ADR-021) draws with shaders written in HLSL and compiled at build time to DXIL with the Windows SDK's dxc.exe, then built into
# the program as byte arrays (no shader files to ship). Without dxc.exe the GPU backend is left out and the game uses SDL_Renderer.
set(LUNA_SHADER_DIR "${CMAKE_BINARY_DIR}/luna_shaders")
set(LUNA_SHADER_HEADERS "")
option(LUNA_GPU "Build the GPU renderer (needs dxc.exe from the Windows SDK)" ON)
if(NOT LUNA_GPU)
    set(LUNA_DXC "")
    message(STATUS "GPU renderer: switched off by -DLUNA_GPU=OFF; the game will use SDL_Renderer only")
elseif(NOT LUNA_DXC)
    set(_luna_sdk_roots "$ENV{WindowsSdkDir}" "$ENV{ProgramFiles\(x86\)}/Windows Kits/10" "C:/Program Files (x86)/Windows Kits/10")
    foreach(_root IN LISTS _luna_sdk_roots)
        if(_root AND NOT LUNA_DXC)
            file(GLOB _versions "${_root}/bin/10.*")
            list(SORT _versions ORDER DESCENDING)
            foreach(_version IN LISTS _versions)
                if(NOT LUNA_DXC AND EXISTS "${_version}/x64/dxc.exe")
                    set(LUNA_DXC "${_version}/x64/dxc.exe")
                endif()
            endforeach()
        endif()
    endforeach()
endif()
if(LUNA_GPU AND LUNA_DXC)
    message(STATUS "GPU renderer: shaders are compiled with ${LUNA_DXC}")
    foreach(_shader sprite.vert sprite.frag sprite_lit.frag blit.vert blit.frag)
        if(_shader MATCHES "[.]vert$")
            set(_profile vs_6_0)
        else()
            set(_profile ps_6_0)
        endif()
        string(REPLACE "." "_" _symbol "g_${_shader}")
        set(_source "${CMAKE_SOURCE_DIR}/src/luna/platform/shaders/${_shader}.hlsl")
        set(_header "${LUNA_SHADER_DIR}/${_shader}.h")
        add_custom_command(OUTPUT "${_header}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${LUNA_SHADER_DIR}"
            COMMAND "${LUNA_DXC}" -T ${_profile} -E main -O3 -Fh "${_header}" -Vn ${_symbol} "${_source}"
            DEPENDS "${_source}"
            COMMENT "Compiling shader ${_shader}.hlsl"
            VERBATIM)
        list(APPEND LUNA_SHADER_HEADERS "${_header}")
    endforeach()
elseif(LUNA_GPU)
    message(STATUS "GPU renderer: dxc.exe was not found (Windows SDK); the game will use SDL_Renderer only. Set -DLUNA_DXC=<path> to change that.")
endif()

add_library(luna_platform STATIC
    src/luna/platform/layer.cpp
    src/luna/platform/sdl_events.cpp
    src/luna/platform/crash.cpp
    src/luna/platform/system.cpp
    src/luna/platform/user_paths.cpp
    src/luna/platform/sdl_renderer_backend.cpp
    src/luna/platform/gpu_backend.cpp
    src/luna/platform/window.cpp
    ${LUNA_SHADER_HEADERS})
target_include_directories(luna_platform PRIVATE "${LUNA_SHADER_DIR}")
if(LUNA_GPU AND LUNA_DXC)
    target_compile_definitions(luna_platform PRIVATE LUNA_GPU=1)
endif()
target_link_libraries(luna_platform PUBLIC odysseus_core)
# PRIVATE: only Platform sees SDL3 headers (Charter rule 2); callers still link it.
target_link_libraries(luna_platform PRIVATE SDL3::SDL3)
odysseus_register_layer(luna_platform platform luna/platform)
odysseus_apply_settings(luna_platform)

# Luna Physics (ADR-017): deterministic fixed-point 3D, headless, Core only.
add_library(luna_physics STATIC
    src/luna/physics/fixed.cpp
    src/luna/physics/vec3.cpp
    src/luna/physics/quat.cpp
    src/luna/physics/shapes.cpp
    src/luna/physics/spatial_grid.cpp
    src/luna/physics/ballistics.cpp
    src/luna/physics/rigid_body.cpp
    src/luna/physics/material.cpp)
target_link_libraries(luna_physics PUBLIC odysseus_core)
odysseus_register_layer(luna_physics physics luna/physics)
odysseus_apply_settings(luna_physics)

add_library(luna_engine STATIC
    src/luna/engine/layer.cpp
    src/luna/engine/application.cpp
    src/luna/engine/fixed_step_clock.cpp
    src/luna/engine/frame_stats.cpp
    src/luna/engine/input.cpp
    src/luna/engine/renderer.cpp
    src/luna/engine/camera.cpp
    src/luna/engine/scaled_renderer.cpp
    src/luna/engine/tile_map.cpp
    src/luna/engine/collision.cpp
    src/luna/engine/physics_view.cpp
    src/luna/engine/image_ops.cpp
    src/luna/engine/shadow_draw.cpp
    src/luna/engine/sprite_layers.cpp
    src/luna/engine/chunk_streamer.cpp
    src/luna/engine/image_io.cpp
    src/luna/engine/ui.cpp
    src/luna/engine/node_graph.cpp
    src/luna/engine/effects.cpp
    src/luna/engine/minimap.cpp)
target_link_libraries(luna_engine PUBLIC luna_platform luna_physics)
# PRIVATE and SYSTEM: only image_io.cpp includes the stb headers, and their warnings are not ours.
target_include_directories(luna_engine SYSTEM PRIVATE ${Stb_INCLUDE_DIR})
odysseus_register_layer(luna_engine engine luna/engine)
odysseus_apply_settings(luna_engine)

add_library(odysseus_sim STATIC
    src/sim/layer.cpp
    src/sim/calendar.cpp
    src/sim/game_clock.cpp
    src/sim/json_data.cpp
    src/sim/json_text.cpp
    src/sim/json_patch.cpp
    src/sim/data_document.cpp
    src/sim/data_form.cpp
    src/sim/data_refs.cpp
    src/sim/quick_check.cpp
    src/sim/play_rules.cpp
    src/sim/schema.cpp
    src/sim/schema_index.cpp
    src/sim/rule_json.cpp
    src/sim/rule_expr.cpp
    src/sim/rule_effect.cpp
    src/sim/interaction.cpp
    src/sim/npc_class.cpp
    src/sim/opinion.cpp
    src/sim/economy.cpp
    src/sim/npc_extras.cpp
    src/sim/trade_market.cpp
    src/sim/npc_schedule.cpp
    src/sim/partner_types.cpp
    src/sim/npc_events.cpp
    src/sim/npc_context.cpp
    src/sim/npc_actions.cpp
    src/sim/npc_director.cpp
    src/sim/npc_kind.cpp
    src/sim/npc_population.cpp
    src/sim/action_runner.cpp
    src/sim/npc_chooser.cpp
    src/sim/dialogue_script.cpp
    src/sim/dialogue_select.cpp
    src/sim/conversation.cpp
    src/sim/smalltalk.cpp
    src/sim/flag_store.cpp
    src/sim/world.cpp
    src/sim/world_story.cpp
    src/sim/world_care.cpp
    src/sim/world_love.cpp
    src/sim/world_hunt.cpp
    src/sim/episodes.cpp
    src/sim/needs.cpp
    src/sim/person.cpp
    src/sim/clan.cpp
    src/sim/chronicle.cpp
    src/sim/actions.cpp
    src/sim/ai.cpp
    src/sim/memory.cpp
    src/sim/life.cpp
    src/sim/story.cpp
    src/sim/report.cpp
    src/sim/save.cpp
    src/sim/region.cpp
    src/sim/rivals.cpp
    src/sim/region_edits.cpp
    src/sim/region_shapes.cpp
    src/sim/world_file.cpp
    src/sim/world_places.cpp
    src/sim/world_setup.cpp
    src/sim/region_save.cpp
    src/sim/hero_data.cpp
    src/sim/hero_life.cpp
    src/sim/building_data.cpp
    src/sim/building_store.cpp
    src/sim/building_life.cpp
    src/sim/graph_check.cpp
    src/sim/test_play.cpp
    src/sim/quest_data.cpp
    src/sim/quest_book.cpp
    src/sim/quest_check.cpp)
target_link_libraries(odysseus_sim PUBLIC odysseus_core luna_physics)
# PRIVATE: JSON is how the simulation reads content; nobody above it sees JSON.
target_link_libraries(odysseus_sim PRIVATE nlohmann_json::nlohmann_json)
odysseus_register_layer(odysseus_sim sim sim)
odysseus_apply_settings(odysseus_sim)

add_library(odysseus_game STATIC
    src/game/layer.cpp
    src/game/odyssey_game.cpp
    src/game/odyssey_reload.cpp
    src/game/placeholder_art.cpp
    src/game/test_map.cpp
    src/game/hero.cpp
    src/game/materials.cpp
    src/game/spear_range.cpp
    src/game/sword.cpp
    src/game/enemy.cpp
    src/game/status.cpp
    src/game/art.cpp
    src/game/content_art.cpp
    src/game/catalogs.cpp
    src/game/tags.cpp
    src/game/game_rules.cpp
    src/game/quest_ui.cpp
    src/game/quest_graph.cpp
    src/game/story_event_editor.cpp
    src/game/quest_debug.cpp
    src/game/object_art.cpp
    src/game/npc_life.cpp
    src/game/builtin_actions.cpp
    src/game/weapons.cpp
    src/game/arc_shots.cpp
    src/game/pickups.cpp
    src/game/plants.cpp
    src/game/animals.cpp
    src/game/weather.cpp
    src/game/lighting.cpp
    src/game/world_lights.cpp
    src/game/shadows.cpp
    src/game/sky.cpp
    src/game/celestial.cpp
    src/game/normal_art.cpp
    src/game/clan_art.cpp
    src/game/clan_view.cpp
    src/game/region_level.cpp
    src/game/generator_panel.cpp
    src/game/world_history.cpp
    src/game/region_view.cpp
    src/game/bubbles.cpp
    src/game/run_flow.cpp
    src/game/settings.cpp
    src/game/tutorial.cpp
    src/game/session_stats.cpp
    src/game/level.cpp
    src/game/level_baseline.cpp
    src/game/editor.cpp
    src/game/npc_class_book.cpp
    src/game/npc_marker.cpp
    src/game/npc_people.cpp
    src/game/building_layer.cpp
    src/game/building_editor.cpp
    src/game/data_reload.cpp
    src/game/editor_help.cpp
    src/game/editor_history.cpp
    src/game/dialogue_graph.cpp
    src/game/graph_editor.cpp
    src/game/data_editor.cpp
    src/game/timeline_view.cpp
    src/game/atlas_cuts.cpp
    src/game/picture_tool.cpp
    src/game/interaction_graph.cpp)
target_link_libraries(odysseus_game PUBLIC luna_engine odysseus_sim)
# PUBLIC since the Data tab (US-191): the Editor's header holds a data document, which is JSON, so the programs that include the Editor need the library's headers.
target_link_libraries(odysseus_game PUBLIC nlohmann_json::nlohmann_json)
odysseus_register_layer(odysseus_game game game)
odysseus_apply_settings(odysseus_game)

# --- Programs ---
add_executable(odysseus apps/odysseus/main.cpp)
target_link_libraries(odysseus PRIVATE odysseus_game)
target_compile_definitions(odysseus PRIVATE ODYSSEUS_LAYER_GAME ODYSSEUS_DATA_DIR="${ODYSSEUS_DATA_DIR}")
odysseus_apply_settings(odysseus)

add_executable(odysseus_headless apps/headless/main.cpp)
target_link_libraries(odysseus_headless PRIVATE odysseus_sim)
target_compile_definitions(odysseus_headless PRIVATE ODYSSEUS_LAYER_SIM ODYSSEUS_DATA_DIR="${ODYSSEUS_DATA_DIR}")
odysseus_apply_settings(odysseus_headless)

# Visual Studio: F5 starts the game.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT odysseus)

# --- Tests ---
enable_testing()
add_executable(odysseus_tests
    tests/main.cpp
    tests/core/version_test.cpp
    tests/core/log_test.cpp
    tests/core/presentation_test.cpp
    tests/core/text_test.cpp
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
add_test(NAME odysseus_tests COMMAND odysseus_tests "--test-case-exclude=US-003*,US-025*")
foreach(test_name IN ITEMS "US-003 Allowed use" "US-003 Forbidden use" "US-003 Luna stays game-agnostic"
                           "US-003 Guard completeness" "US-025 Physics layer rules")
    add_test(NAME "${test_name}" COMMAND odysseus_tests "--test-case=${test_name}")
    set_tests_properties("${test_name}" PROPERTIES RESOURCE_LOCK architecture_build)
endforeach()

# --- Luna Physics tests: Physics identity, headless (M1b) ---
add_executable(luna_physics_tests
    tests/main.cpp
    tests/physics/fixed_test.cpp
    tests/physics/rotation_test.cpp
    tests/physics/determinism_test.cpp
    tests/physics/shapes_test.cpp
    tests/physics/ballistics_test.cpp
    tests/physics/rigid_body_test.cpp)
target_link_libraries(luna_physics_tests PRIVATE luna_physics doctest::doctest)
target_compile_definitions(luna_physics_tests PRIVATE ODYSSEUS_LAYER_PHYSICS)
odysseus_apply_settings(luna_physics_tests)
add_test(NAME luna_physics_tests COMMAND luna_physics_tests)

# --- Charter rule 10: no float or double inside Luna Physics ---
add_test(NAME "US-025 Physics uses no floating point"
    COMMAND "${CMAKE_COMMAND}" -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -P "${PROJECT_SOURCE_DIR}/tests/physics/check_no_floating_point.cmake")

# --- Luna tests: headless unit tests with the Engine identity (Engine, Platform, Core) ---
add_executable(luna_tests
    tests/main.cpp
    tests/luna/input_test.cpp
    tests/luna/loop_test.cpp
    tests/luna/pixel_scale_test.cpp
    tests/luna/tile_map_test.cpp
    tests/luna/collision_test.cpp
    tests/luna/ui_test.cpp
    tests/luna/node_graph_test.cpp
    tests/luna/effects_test.cpp
    tests/luna/zoom_test.cpp
    tests/luna/minimap_test.cpp
    tests/luna/normals_test.cpp)
target_link_libraries(luna_tests PRIVATE luna_engine doctest::doctest)
target_compile_definitions(luna_tests PRIVATE ODYSSEUS_LAYER_ENGINE)
odysseus_apply_settings(luna_tests)
add_test(NAME luna_tests COMMAND luna_tests)

# --- End-to-end: the real game window (US-020 Open and Close) ---
add_test(NAME "US-020 Open and close"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us020 -DQUIT_AFTER=3
            -DCONFIG=$<CONFIG> -P "${PROJECT_SOURCE_DIR}/tests/luna/run_game_window.cmake")
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

# --- Luna window tests: a real hidden window, reading back pixels (US-022) ---
add_executable(luna_window_tests
    tests/main.cpp
    tests/luna/pixels_window_test.cpp
    tests/luna/lighting_window_test.cpp)
target_link_libraries(luna_window_tests PRIVATE luna_engine doctest::doctest)
target_compile_definitions(luna_window_tests PRIVATE ODYSSEUS_LAYER_ENGINE)
odysseus_apply_settings(luna_window_tests)
add_test(NAME luna_window_tests COMMAND luna_window_tests --success=false)
set_tests_properties(luna_window_tests PROPERTIES LABELS window TIMEOUT 120)

# --- Game tests: Game identity (US-024 the hero) ---
add_executable(odysseus_game_tests
    tests/main.cpp
    tests/game/hero_test.cpp
    tests/game/m6_test.cpp
    tests/game/spear_test.cpp
    tests/game/art_test.cpp
    tests/game/lighting_test.cpp
    tests/game/normals_test.cpp
    tests/game/sky_test.cpp
    tests/game/celestial_test.cpp
    tests/game/shadow_test.cpp
    tests/game/world_lights_test.cpp
    tests/game/level_test.cpp
    tests/game/modes_test.cpp
    tests/game/paint_test.cpp
    tests/game/place_test.cpp
    tests/game/settings_test.cpp
    tests/game/presentation_settings_test.cpp
    tests/game/content_test.cpp
    tests/game/combat_test.cpp
    tests/game/weapons_test.cpp
    tests/game/elements_test.cpp
    tests/game/aiming_test.cpp
    tests/game/arc_test.cpp
    tests/game/ranged_test.cpp
    tests/game/plants_test.cpp
    tests/game/animals_test.cpp
    tests/game/weather_test.cpp
    tests/game/weather_light_test.cpp
    tests/game/lighting_editor_test.cpp
    tests/game/npc_class_editor_test.cpp
    tests/game/npc_kind_test.cpp
    tests/game/npc_people_test.cpp
    tests/game/npc_attitude_test.cpp
    tests/game/npc_talk_test.cpp
    tests/game/npc_confront_test.cpp
    tests/game/npc_actions_test.cpp
    tests/game/npc_editor_test.cpp
    tests/game/npc_kinds_tab_test.cpp
    tests/game/npc_test_level_test.cpp
    tests/game/economy_editor_test.cpp
    tests/game/editor_help_test.cpp
    tests/game/editor_suggest_test.cpp
    tests/game/data_reload_test.cpp
    tests/game/data_watch_test.cpp
    tests/game/level_merge_test.cpp
    tests/game/trade_stock_test.cpp
    tests/game/trade_gate_test.cpp
    tests/game/trade_screen_test.cpp
    tests/game/trade_editor_test.cpp
    tests/game/schedule_editor_test.cpp
    tests/game/npc_life_game_test.cpp
    tests/game/npc_dealings_game_test.cpp
    tests/game/npc_defaults_game_test.cpp
    tests/game/living_level_test.cpp
    tests/game/clan_test.cpp
    tests/game/m4_test.cpp
    tests/game/pickups_test.cpp
    tests/game/interactions_test.cpp
    tests/game/tags_test.cpp
    tests/game/menu_test.cpp
    tests/game/npc_test.cpp
    tests/game/conversation_test.cpp
    tests/game/greeting_test.cpp
    tests/game/exchange_test.cpp
    tests/game/renderer_test.cpp
    tests/game/graph_command_test.cpp
    tests/game/dialogue_graph_test.cpp
    tests/game/graph_editor_test.cpp
    tests/game/interaction_graph_test.cpp
    tests/game/quest_graph_test.cpp
    tests/game/story_event_test.cpp
    tests/game/thing_overrides_test.cpp
    tests/game/building_game_test.cpp
    tests/game/building_life_game_test.cpp
    tests/game/building_editor_test.cpp
    tests/game/schema_game_test.cpp
    tests/game/data_editor_test.cpp
    tests/game/picture_tool_test.cpp
    tests/game/live_catalog_test.cpp
    tests/game/region_view_test.cpp
    tests/game/place_tool_test.cpp
    tests/game/generator_panel_test.cpp
    tests/game/world_edit_test.cpp
    tests/game/water_tools_test.cpp)
target_link_libraries(odysseus_game_tests PRIVATE odysseus_game doctest::doctest nlohmann_json::nlohmann_json)
target_compile_definitions(odysseus_game_tests PRIVATE ODYSSEUS_LAYER_GAME ODYSSEUS_DATA_DIR="${ODYSSEUS_DATA_DIR}"
    ODYSSEUS_DEMO_LEVEL="${PROJECT_SOURCE_DIR}/assets/levels/demo.json")
odysseus_apply_settings(odysseus_game_tests)
# The game tests are split into six runs so CTest can run them side by side (CI and verify.ps1 use -j). A to E name their files (balanced by
# measured time, about two to three minutes each in Debug); F runs every other file, so a new test file is never left out. "*game?" anchors
# the name at the folder, so level_test.cpp does not also pick living_level_test.cpp.
set(ODYSSEUS_SHARD_A "*game?aiming_test.cpp,*game?weapons_test.cpp,*game?combat_test.cpp")
set(ODYSSEUS_SHARD_B "*game?plants_test.cpp,*game?ranged_test.cpp,*game?elements_test.cpp,*game?settings_test.cpp")
set(ODYSSEUS_SHARD_C "*game?shadow_test.cpp,*game?pickups_test.cpp,*game?world_lights_test.cpp,*game?paint_test.cpp,*game?celestial_test.cpp,*game?trade_editor_test.cpp,*game?thing_overrides_test.cpp,*game?lighting_editor_test.cpp,*game?npc_kind_test.cpp,*game?clan_test.cpp,*game?npc_class_editor_test.cpp,*game?npc_test_level_test.cpp,*game?arc_test.cpp,*game?interactions_test.cpp,*game?quest_graph_test.cpp,*game?presentation_settings_test.cpp,*game?hero_test.cpp,*game?dialogue_graph_test.cpp")
set(ODYSSEUS_SHARD_D "*game?menu_test.cpp,*game?animals_test.cpp,*game?npc_test.cpp,*game?data_watch_test.cpp,*game?building_life_game_test.cpp,*game?weather_light_test.cpp,*game?trade_stock_test.cpp,*game?modes_test.cpp,*game?exchange_test.cpp,*game?npc_actions_test.cpp,*game?building_editor_test.cpp,*game?trade_gate_test.cpp,*game?npc_attitude_test.cpp,*game?graph_editor_test.cpp,*game?art_test.cpp")
set(ODYSSEUS_SHARD_E "*game?normals_test.cpp,*game?conversation_test.cpp,*game?weather_test.cpp,*game?renderer_test.cpp,*game?trade_screen_test.cpp,*game?level_test.cpp,*game?schedule_editor_test.cpp,*game?npc_editor_test.cpp,*game?economy_editor_test.cpp,*game?greeting_test.cpp,*game?npc_kinds_tab_test.cpp,*game?npc_defaults_game_test.cpp,*game?living_level_test.cpp,*game?sky_test.cpp,*game?lighting_test.cpp,*game?interaction_graph_test.cpp")
foreach(_shard A B C D E)
    string(TOLOWER ${_shard} _name)
    add_test(NAME odysseus_game_tests_${_name} COMMAND odysseus_game_tests "--source-file=${ODYSSEUS_SHARD_${_shard}}")
endforeach()
add_test(NAME odysseus_game_tests_f COMMAND odysseus_game_tests
    "--source-file-exclude=${ODYSSEUS_SHARD_A},${ODYSSEUS_SHARD_B},${ODYSSEUS_SHARD_C},${ODYSSEUS_SHARD_D},${ODYSSEUS_SHARD_E}")

# --- End-to-end: walk right into the boulder in the real window (US-024) ---
add_test(NAME "US-024 Walk to the rock"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us024 -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json
            -P "${PROJECT_SOURCE_DIR}/tests/game/run_walk_to_rock.cmake")
set_tests_properties("US-024 Walk to the rock" PROPERTIES LABELS window TIMEOUT 120)

# --- End-to-end: throw a spear at the straw target in the real window (US-029) ---
add_test(NAME "US-029 Throw in the game"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us029 -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json
            -P "${PROJECT_SOURCE_DIR}/tests/game/run_throw_spear.cmake")
set_tests_properties("US-029 Throw in the game" PROPERTIES LABELS window TIMEOUT 120)

# --- Simulation tests: Simulation identity, headless (M2) ---
add_executable(odysseus_sim_tests
    tests/main.cpp
    tests/sim/world_test.cpp
    tests/sim/needs_test.cpp
    tests/sim/ai_test.cpp
    tests/sim/memory_test.cpp
    tests/sim/chronicle_test.cpp
    tests/sim/report_test.cpp
    tests/sim/save_test.cpp
    tests/sim/story_test.cpp
    tests/sim/story_quarrel_test.cpp
    tests/sim/story_care_test.cpp
    tests/sim/story_love_test.cpp
    tests/sim/story_hunt_test.cpp
    tests/sim/story_episode_test.cpp
    tests/sim/region_test.cpp
    tests/sim/region_edits_test.cpp
    tests/sim/world_file_test.cpp
    tests/sim/world_places_test.cpp
    tests/sim/world_setup_test.cpp
    tests/sim/region_shapes_test.cpp
    tests/sim/hero_test.cpp
    tests/sim/rules_test.cpp
    tests/sim/npc_class_test.cpp
    tests/sim/opinion_test.cpp
    tests/sim/economy_test.cpp
    tests/sim/trade_market_test.cpp
    tests/sim/trade_price_test.cpp
    tests/sim/trade_deal_test.cpp
    tests/sim/npc_schedule_test.cpp
    tests/sim/npc_actions_test.cpp
    tests/sim/npc_interact_test.cpp
    tests/sim/npc_defaults_test.cpp
    tests/sim/npc_kind_test.cpp
    tests/sim/npc_population_test.cpp
    tests/sim/npc_scale_test.cpp
    tests/sim/runner_test.cpp
    tests/sim/dialogue_test.cpp
    tests/sim/quest_test.cpp
    tests/sim/conversation_test.cpp
    tests/sim/selection_test.cpp
    tests/sim/smalltalk_test.cpp
    tests/sim/memory_talk_test.cpp
    tests/sim/building_data_test.cpp
    tests/sim/building_store_test.cpp
    tests/sim/building_life_test.cpp
    tests/sim/graph_check_test.cpp
    tests/sim/test_play_test.cpp
    tests/sim/schema_test.cpp
    tests/sim/json_patch_test.cpp
    tests/sim/data_document_test.cpp
    tests/sim/data_form_test.cpp
    tests/sim/data_refs_test.cpp
    tests/sim/quick_check_test.cpp
    tests/sim/play_rules_test.cpp
    tests/sim/routine_test.cpp)
target_link_libraries(odysseus_sim_tests PRIVATE odysseus_sim doctest::doctest nlohmann_json::nlohmann_json)
target_compile_definitions(odysseus_sim_tests PRIVATE ODYSSEUS_LAYER_SIM ODYSSEUS_DATA_DIR="${ODYSSEUS_DATA_DIR}")
odysseus_apply_settings(odysseus_sim_tests)
add_test(NAME odysseus_sim_tests COMMAND odysseus_sim_tests)

# --- US-294: the soak (100,000 persons, 30 in-game days, twice) in its own executable so the slow run stays apart from the quick tests ---
add_executable(odysseus_sim_soak
    tests/main.cpp
    tests/sim/npc_soak_test.cpp)
target_link_libraries(odysseus_sim_soak PRIVATE odysseus_sim doctest::doctest nlohmann_json::nlohmann_json)
target_compile_definitions(odysseus_sim_soak PRIVATE ODYSSEUS_LAYER_SIM ODYSSEUS_DATA_DIR="${ODYSSEUS_DATA_DIR}")
odysseus_apply_settings(odysseus_sim_soak)
add_test(NAME odysseus_sim_soak COMMAND odysseus_sim_soak)
set_tests_properties(odysseus_sim_soak PROPERTIES TIMEOUT 3000 LABELS soak) # skipped by owner in verify and CI (D-54, 2026-10-05); run by hand: ctest -L soak

# --- US-015: the headless runner from the command line (100 years; wrong input) ---
add_test(NAME "US-015 Run"
    COMMAND "${CMAKE_COMMAND}" -DHEADLESS=$<TARGET_FILE:odysseus_headless> -DCHECK=run
            -P "${PROJECT_SOURCE_DIR}/tests/sim/run_headless_soak.cmake")
set_tests_properties("US-015 Run" PROPERTIES TIMEOUT 600)
add_test(NAME "US-015 Bad input"
    COMMAND "${CMAKE_COMMAND}" -DHEADLESS=$<TARGET_FILE:odysseus_headless> -DCHECK=bad
            -P "${PROJECT_SOURCE_DIR}/tests/sim/run_headless_soak.cmake")
add_test(NAME "US-110 Traceable"
    COMMAND "${CMAKE_COMMAND}" -DHEADLESS=$<TARGET_FILE:odysseus_headless> -DCHECK=why
            -P "${PROJECT_SOURCE_DIR}/tests/sim/run_headless_soak.cmake")
add_test(NAME "US-115 Both views"
    COMMAND "${CMAKE_COMMAND}" -DHEADLESS=$<TARGET_FILE:odysseus_headless> -DCHECK=story
            -P "${PROJECT_SOURCE_DIR}/tests/sim/run_headless_soak.cmake")
set_tests_properties("US-115 Both views" PROPERTIES TIMEOUT 600)

# --- US-120: the atlas cutter, turning the owner's sprite sheets into game-ready atlases ---
add_executable(odysseus_atlas apps/atlas/main.cpp)
target_link_libraries(odysseus_atlas PRIVATE odysseus_game)
target_compile_definitions(odysseus_atlas PRIVATE ODYSSEUS_LAYER_GAME ODYSSEUS_ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets")
odysseus_apply_settings(odysseus_atlas)

# --- End-to-end: Game mode and Editor mode in the real window (US-123) ---
add_test(NAME "US-123 Modes in the game"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us123 -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json
            -P "${PROJECT_SOURCE_DIR}/tests/game/run_modes.cmake")
set_tests_properties("US-123 Modes in the game" PROPERTIES LABELS window TIMEOUT 120)

# --- End-to-end: paint a tile and save, in the real window, on a copy of the valley (US-124) ---
add_test(NAME "US-124 Paint in the game"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us124
            -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json -P "${PROJECT_SOURCE_DIR}/tests/game/run_paint.cmake")
set_tests_properties("US-124 Paint in the game" PROPERTIES LABELS window TIMEOUT 120)

# --- End-to-end: place a character, save, play and strike it, in the real window (US-125) ---
add_test(NAME "US-125 Place in the game"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/us125
            -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json -P "${PROJECT_SOURCE_DIR}/tests/game/run_place.cmake")
set_tests_properties("US-125 Place in the game" PROPERTIES LABELS window TIMEOUT 120)

# --- Scripted input: two holds of one intent are exactly two presses (found at X-M2c) ---
add_test(NAME "X-M2c Two scripted presses"
    COMMAND "${CMAKE_COMMAND}" -DGAME=$<TARGET_FILE:odysseus> -DWORK_DIR=${CMAKE_BINARY_DIR}/e2e/twopresses
            -DLEVEL=${PROJECT_SOURCE_DIR}/assets/levels/demo.json -P "${PROJECT_SOURCE_DIR}/tests/game/run_two_presses.cmake")
set_tests_properties("X-M2c Two scripted presses" PROPERTIES LABELS window TIMEOUT 120)

# Playtest package (US-091): `cmake --build --preset windows-x64-release --target package` makes a zip that unpacks and runs on a
# Windows 10/11 x64 PC without developer tools: odysseus.exe, the SDL3 library it needs, and the assets folder it reads at start.
install(TARGETS odysseus RUNTIME DESTINATION .)
install(FILES $<TARGET_RUNTIME_DLLS:odysseus> DESTINATION . OPTIONAL)
install(DIRECTORY assets/ DESTINATION assets PATTERN "saves" EXCLUDE PATTERN "*.bak" EXCLUDE)
install(FILES README.md DESTINATION . OPTIONAL)
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME "ProjectOdyssey-playtest")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
