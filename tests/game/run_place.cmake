# End-to-end US-125 in the real game window, on a copy of the demo level: in the Editor choose
# Place and the goblin, click next to the hero's start, save, play (F1), take the sword and strike.
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR OR NOT DEFINED LEVEL)
    message(FATAL_ERROR "run_place.cmake needs GAME, WORK_DIR and LEVEL")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}/logs")
file(COPY "${LEVEL}" DESTINATION "${WORK_DIR}")
get_filename_component(name "${LEVEL}" NAME)
set(copy "${WORK_DIR}/${name}")
file(READ "${copy}" original)
string(JSON placed_id GET "${original}" nextId)

execute_process(COMMAND "${GAME}" --level "${copy}" --editor --quit-after 2.8
                        --click 170:11:0.4 --click 18:74:0.7 --click 512:270:1.0 --hold Save:1.3:1.4
                        --hold ModeGame:1.6:1.7 --hold SwitchWeapon:1.9:2.0 --hold Interact:2.2:2.3
                        --log-dir "${WORK_DIR}/logs" --screenshot "${WORK_DIR}/struck.bmp"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
file(GLOB logs "${WORK_DIR}/logs/session-*.log")
file(READ "${logs}" log)
message(STATUS "Game log:\n${log}")
if(NOT result EQUAL 0 OR log MATCHES "\\[ERROR\\]")
    message(FATAL_ERROR "The game failed (exit ${result})")
endif()
foreach(expected "Editor: place goblin #${placed_id}" "Editor: Saved ${name}" "Mode: Game" "Switched to Sword" "Goblin took 5 damage, HP 55 / 60")
    if(NOT log MATCHES "${expected}")
        message(FATAL_ERROR "Missing from the log: ${expected}")
    endif()
endforeach()
file(READ "${copy}" saved)
string(JSON saved_id GET "${saved}" characters 1 id)
string(JSON saved_kind GET "${saved}" characters 1 kind)
if(NOT saved_id EQUAL placed_id OR NOT saved_kind STREQUAL "goblin")
    message(FATAL_ERROR "The placed goblin is not in the saved level")
endif()
message(STATUS "US-125 end to end: placed, saved, played and struck")
