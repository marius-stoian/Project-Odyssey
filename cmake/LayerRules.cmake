# Each target exports only its own headers. Dependencies add the permitted lower layers.
function(odysseus_register_layer target layer directory)
    set(include_root "${CMAKE_BINARY_DIR}/layer-includes/${layer}")
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/src/${directory}/*.h"
        "${PROJECT_SOURCE_DIR}/src/${directory}/*.hpp"
        "${PROJECT_SOURCE_DIR}/src/${directory}/*.hh"
        "${PROJECT_SOURCE_DIR}/src/${directory}/*.hxx")
    foreach(header IN LISTS headers)
        file(RELATIVE_PATH spelling "${PROJECT_SOURCE_DIR}/src" "${header}")
        # Forwarding headers preserve readable includes while relative includes inside
        # the real header still resolve beside that header and encounter its guard.
        get_filename_component(parent "${include_root}/${spelling}" DIRECTORY)
        file(MAKE_DIRECTORY "${parent}")
        file(WRITE "${include_root}/${spelling}" "#include \"${header}\"\n")
    endforeach()
    target_include_directories(${target} PUBLIC "${include_root}")
    string(TOUPPER "${layer}" identity)
    target_compile_definitions(${target} PRIVATE "ODYSSEUS_LAYER_${identity}")
    add_dependencies(${target} odysseus_layer_rules)
endfunction()

# ALL runs even when no C++ source changed: edits after configure are checked too.
add_custom_target(odysseus_layer_rules ALL
    COMMAND "${CMAKE_COMMAND}" "-DODYSSEUS_SOURCE_ROOT=${PROJECT_SOURCE_DIR}/src"
            -P "${CMAKE_CURRENT_LIST_DIR}/ValidateLayerIncludes.cmake"
    COMMENT "Checking architecture include rules"
    VERBATIM)
