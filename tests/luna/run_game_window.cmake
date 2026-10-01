# End-to-end check for US-020 "Open" and "Close": start the real game, let it close itself
# after QUIT_AFTER seconds through the same path as the close button, then read its log.
# Usage: cmake -DGAME=<odysseus.exe> -DWORK_DIR=<folder> -DQUIT_AFTER=3 -DCONFIG=<Debug|Release> -P run_game_window.cmake
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "run_game_window.cmake needs GAME and WORK_DIR")
endif()
if(NOT DEFINED QUIT_AFTER)
    set(QUIT_AFTER 3)
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

execute_process(COMMAND "${GAME}" --quit-after ${QUIT_AFTER} --log-dir "${WORK_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
file(GLOB logs "${WORK_DIR}/session-*.log")
list(LENGTH logs count)
if(NOT count EQUAL 1)
    message(FATAL_ERROR "Expected one session log, found ${count}. Output:\n${output}\n${error}")
endif()
file(READ "${logs}" log)
message(STATUS "Game log:\n${log}")

if(NOT result EQUAL 0)
    message(FATAL_ERROR "The game exited with ${result}")
endif()
if(log MATCHES "\\[ERROR\\]")
    message(FATAL_ERROR "The log contains an error")
endif()
foreach(expected "Window opened: 1280x720" "Average frame rate" "Window closed by the player" "Shutting down"
                 "Log session ended")
    string(FIND "${log}" "${expected}" at)
    if(at EQUAL -1)
        message(FATAL_ERROR "Missing from the log: ${expected}")
    endif()
endforeach()
# "Open": the window must show its first frame within 3 seconds of starting, in the build a
# player runs (Release). The Debug build runs under AddressSanitizer, which on shared CI
# machines can take several seconds just to create the window, so it gets 15 seconds (CI-006).
# GitHub's shared runners have no GPU and sometimes need more than 3 seconds even in Release, so
# there Release gets 10 seconds; the 3-second check is run on the owner's PC at each milestone exit
# (`pwsh tools/verify.ps1 -Config Release`, D-46).
if(NOT log MATCHES "First frame after ([0-9]+) ms")
    message(FATAL_ERROR "The log does not report the first frame")
endif()
set(first_frame_ms "${CMAKE_MATCH_1}")
if(CONFIG STREQUAL "Debug")
    set(limit_ms 15000)
elseif(DEFINED ENV{GITHUB_ACTIONS})
    set(limit_ms 10000)
else()
    set(limit_ms 3000)
endif()
if(first_frame_ms GREATER_EQUAL limit_ms)
    message(FATAL_ERROR "First frame took ${first_frame_ms} ms (limit ${limit_ms} ms in ${CONFIG})")
endif()
set(CMAKE_MATCH_1 "${first_frame_ms}")
message(STATUS "US-020 Open: first frame after ${CMAKE_MATCH_1} ms; Close: clean shutdown")
