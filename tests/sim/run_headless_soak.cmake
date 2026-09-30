# US-015 end to end: the headless runner from the command line.
#   "Run":       odysseus_headless --seed 7 --years 100 finishes without crashing and prints population,
#                deaths by cause, average needs and tick time.
#   "Bad input": --years -5 (and other wrong values) print a usage message and exit with an error code.
# Usage: cmake -DHEADLESS=<odysseus_headless.exe> -DCHECK=run|bad -P run_headless_soak.cmake
if(NOT DEFINED HEADLESS OR NOT DEFINED CHECK)
    message(FATAL_ERROR "run_headless_soak.cmake needs HEADLESS and CHECK")
endif()

if(CHECK STREQUAL "run")
    execute_process(COMMAND "${HEADLESS}" --seed 7 --years 100
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 600)
    message(STATUS "Output:\n${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "The 100-year run failed (exit ${result})")
    endif()
    foreach(expected "Seed 7, 2800 days \\(100 years" "Population: [0-9]+ alive" "Deaths by cause: starvation [0-9]+"
                     "Average needs of the living: Hunger [0-9]+, Energy [0-9]+, Warmth [0-9]+, Social [0-9]+"
                     "Tick time: [0-9.]+ microseconds per tick" "World hash: [0-9]+")
        if(NOT output MATCHES "${expected}")
            message(FATAL_ERROR "The report is missing: ${expected}")
        endif()
    endforeach()
    message(STATUS "US-015 Run: 100 years finished and reported")
elseif(CHECK STREQUAL "bad")
    foreach(arguments "--years;-5" "--years;abc" "--years;0" "--years;12x" "--seed;-1" "--frobnicate" "--years")
        execute_process(COMMAND "${HEADLESS}" ${arguments}
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
        if(result EQUAL 0)
            message(FATAL_ERROR "'${arguments}' was accepted (exit 0)")
        endif()
        if(NOT error MATCHES "Usage: odysseus_headless")
            message(FATAL_ERROR "'${arguments}' did not print the usage message:\n${output}${error}")
        endif()
        message(STATUS "'${arguments}' -> exit ${result}, usage printed")
    endforeach()
else()
    message(FATAL_ERROR "CHECK must be run or bad")
endif()
