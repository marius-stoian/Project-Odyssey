# Scripted input: two holds of the same intent are two presses, no more (found at X-M2c: an
# inactive hold used to release an active one every frame, so one hold pressed on every tick).
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR OR NOT DEFINED LEVEL)
    message(FATAL_ERROR "run_two_presses.cmake needs GAME, WORK_DIR and LEVEL")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
execute_process(COMMAND "${GAME}" --level "${LEVEL}" --quit-after 1.4 --hold Interact:0.3:0.4 --hold Interact:0.9:1.0 --log-dir "${WORK_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
file(GLOB logs "${WORK_DIR}/session-*.log")
file(STRINGS "${logs}" throws REGEX "Threw a")
list(LENGTH throws count)
if(NOT result EQUAL 0 OR NOT count EQUAL 2)
    message(FATAL_ERROR "Two scripted presses threw ${count} spears (exit ${result})")
endif()
message(STATUS "Two scripted presses, two spears")
