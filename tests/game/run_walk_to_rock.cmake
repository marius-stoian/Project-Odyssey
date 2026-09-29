# End-to-end US-024 in the real game window: hold Move Right for 3 seconds from the start.
# The boulder on the east path (test map) must stop the hero flush against it:
# boulder column 36 -> left edge x = 1152; feet box 20 px wide -> feet centre x = 1142.
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "run_walk_to_rock.cmake needs GAME and WORK_DIR")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
execute_process(COMMAND "${GAME}" --quit-after 4 --hold MoveRight:0.2:3.2 --log-dir "${WORK_DIR}"
                        --screenshot "${WORK_DIR}/walked-to-rock.bmp"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
file(GLOB logs "${WORK_DIR}/session-*.log")
file(READ "${logs}" log)
message(STATUS "Game log:\n${log}")
if(NOT result EQUAL 0 OR log MATCHES "\\[ERROR\\]")
    message(FATAL_ERROR "The game failed (exit ${result})")
endif()
if(NOT log MATCHES "Hero at \\(([0-9.]+), ([0-9.]+)\\) facing ([A-Za-z]+), (idle|walking)")
    message(FATAL_ERROR "The log does not report the hero")
endif()
set(x "${CMAKE_MATCH_1}")
set(facing "${CMAKE_MATCH_3}")
set(state "${CMAKE_MATCH_4}")
if(NOT x STREQUAL "1142.0" OR NOT facing STREQUAL "East" OR NOT state STREQUAL "idle")
    message(FATAL_ERROR "Expected the hero idle at x 1142.0 facing East, got x ${x} facing ${facing}, ${state}")
endif()
message(STATUS "US-024 end to end: walked right, stopped at the boulder (x ${x}), idle facing ${facing}")
