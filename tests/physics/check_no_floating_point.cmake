# Charter rule 10 / ADR-017: all physics state is fixed-point, never float or double, so
# every computer computes the same bits. An automated review of every Luna Physics source.
# Usage: cmake -DSOURCE_DIR=<repo> -P check_no_floating_point.cmake
if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "check_no_floating_point.cmake needs SOURCE_DIR")
endif()
file(GLOB_RECURSE files "${SOURCE_DIR}/src/luna/physics/*.h" "${SOURCE_DIR}/src/luna/physics/*.cpp")
set(problems "")
foreach(file IN LISTS files)
    file(READ "${file}" content)
    # Comments may say "double" in plain English; only code counts.
    string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" "" content "${content}")
    string(REGEX REPLACE "//[^\n]*" "" content "${content}")
    string(REGEX REPLACE "\"[^\"\n]*\"" "\"\"" content "${content}")
    foreach(pattern "(^|[^A-Za-z0-9_])float([^A-Za-z0-9_]|$)" "(^|[^A-Za-z0-9_])double([^A-Za-z0-9_]|$)"
                    "<cmath>" "<math\\.h>" "[0-9]\\.[0-9]*[fF]?([^A-Za-z0-9_]|$)")
        if(content MATCHES "${pattern}")
            string(APPEND problems "  ${file}: matches ${pattern}\n")
        endif()
    endforeach()
endforeach()
list(LENGTH files count)
if(count EQUAL 0)
    message(FATAL_ERROR "No Luna Physics sources found under ${SOURCE_DIR}/src/luna/physics")
endif()
if(problems)
    message(FATAL_ERROR "Luna Physics must not use floating point (Charter rule 10):\n${problems}")
endif()
message(STATUS "US-025: ${count} Luna Physics files reviewed, no floating point")
