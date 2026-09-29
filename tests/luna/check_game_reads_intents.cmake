# US-021 "Given the Game layer code, when it is reviewed, then it only reads intents,
# never key codes": an automated review of every Game source and the game's main.
# Usage: cmake -DSOURCE_DIR=<repo> -P check_game_reads_intents.cmake
if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "check_game_reads_intents.cmake needs SOURCE_DIR")
endif()
file(GLOB_RECURSE files "${SOURCE_DIR}/src/game/*.h" "${SOURCE_DIR}/src/game/*.cpp" "${SOURCE_DIR}/apps/odysseus/*.cpp")
# Device-level names that belong to Platform and InputMap only.
set(forbidden "luna/platform/events\\.h" "luna/platform/sdl_events\\.h" "SDL" "Key::" "GamepadButton" "GamepadAxis"
              "KeyDown" "KeyUp" "Scancode" "Keycode")
set(problems "")
foreach(file IN LISTS files)
    file(READ "${file}" content)
    foreach(pattern IN LISTS forbidden)
        if(content MATCHES "${pattern}")
            string(APPEND problems "  ${file}: uses ${pattern}\n")
        endif()
    endforeach()
endforeach()
list(LENGTH files count)
if(problems)
    message(FATAL_ERROR "Game code must read intents only (ARC-03):\n${problems}")
endif()
message(STATUS "US-021: ${count} Game files reviewed, only intents used")
