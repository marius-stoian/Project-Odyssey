# End-to-end US-029 in the real game window: tap Move Left to face the straw target 8 tiles
# west, then press Interact half a second after the start; the log must report the hit.
# A second, shorter run saves a screenshot while the spear is still in the air.
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR OR NOT DEFINED LEVEL)
    message(FATAL_ERROR "run_throw_spear.cmake needs GAME, WORK_DIR and LEVEL")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}/hit" "${WORK_DIR}/flight")

function(run_game folder quit_after screenshot)
    execute_process(COMMAND "${GAME}" --level "${LEVEL}" --quit-after ${quit_after} --aim 10:135:0:4 --hold MoveLeft:0.25:0.35 --hold Interact:0.5:0.6 --log-dir "${WORK_DIR}/${folder}"
                            --screenshot "${WORK_DIR}/${screenshot}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
    file(GLOB logs "${WORK_DIR}/${folder}/session-*.log")
    file(READ "${logs}" log)
    message(STATUS "Game log (${folder}):\n${log}")
    if(NOT result EQUAL 0 OR log MATCHES "\\[ERROR\\]")
        message(FATAL_ERROR "The game failed (exit ${result})")
    endif()
    set(log "${log}" PARENT_SCOPE)
endfunction()

run_game(hit 3 spear-hit.bmp)
if(NOT log MATCHES "Threw a flint spear facing West")
    message(FATAL_ERROR "The log does not report the throw")
endif()
if(NOT log MATCHES "Spear \\(flint\\) hit the straw target at \\(([0-9.]+), ([0-9.]+), ([0-9.]+)\\) m, ([0-9.]+) m/s, ([0-9.]+) damage")
    message(FATAL_ERROR "The spear did not hit the straw target")
endif()
set(damage "${CMAKE_MATCH_5}")
if(NOT log MATCHES "Straw target at \\(24.50, 32.75\\) m: 1 hits")
    message(FATAL_ERROR "The open straw target does not report exactly one hit")
endif()

run_game(flight 0.85 spear-in-flight.bmp)
if(NOT log MATCHES "Threw a flint spear" OR log MATCHES "Spear \\(flint\\) hit")
    message(FATAL_ERROR "The flight screenshot run should end with the spear still in the air")
endif()
message(STATUS "US-029 end to end: the flint spear hit the straw target for ${damage} damage")
