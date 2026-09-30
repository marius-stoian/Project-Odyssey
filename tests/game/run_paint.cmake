# End-to-end US-124 in the real game window, on a copy of the valley: open the Editor, choose
# water in the palette, click the map, save with Ctrl+S; the level file is saved with a backup.
if(NOT DEFINED GAME OR NOT DEFINED WORK_DIR OR NOT DEFINED LEVEL)
    message(FATAL_ERROR "run_paint.cmake needs GAME, WORK_DIR and LEVEL")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}/logs")
file(COPY "${LEVEL}" DESTINATION "${WORK_DIR}")
get_filename_component(name "${LEVEL}" NAME)
set(copy "${WORK_DIR}/${name}")

execute_process(COMMAND "${GAME}" --level "${copy}" --editor --quit-after 1.8
                        --click 14:56:0.6 --click 300:200:0.9 --hold Save:1.2:1.3
                        --log-dir "${WORK_DIR}/logs" --screenshot "${WORK_DIR}/painted.bmp"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
file(GLOB logs "${WORK_DIR}/logs/session-*.log")
file(READ "${logs}" log)
message(STATUS "Game log:\n${log}")
if(NOT result EQUAL 0 OR log MATCHES "\\[ERROR\\]")
    message(FATAL_ERROR "The game failed (exit ${result})")
endif()
if(NOT log MATCHES "Editor: paint water \\(1 cells\\)")
    message(FATAL_ERROR "Clicking water in the palette and then the map did not paint one cell")
endif()
if(NOT log MATCHES "Editor: Saved ${name}")
    message(FATAL_ERROR "Ctrl+S did not save the level")
endif()
if(NOT EXISTS "${copy}.bak1")
    message(FATAL_ERROR "The previous level was not kept as a backup")
endif()
message(STATUS "US-124 end to end: painted, saved, backup kept")
