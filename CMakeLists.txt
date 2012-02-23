cmake_minimum_required(VERSION 2.6)

project(verity)

file(GLOB BHO VerityController/*.c
              VerityController/*.idl
              VerityController/*.rc
              VerityController/*.def)


# In accordance with: http://public.kitware.com/Bug/view.php?id=12909
include_directories(. "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)")
add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
add_library(verity MODULE ${BHO})

macro(copy_javascript SOURCE)
    get_filename_component(BASENAME ${SOURCE} NAME)
    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/${SOURCE}" SOURCE_JS)
    add_custom_command(TARGET verity POST_BUILD
        COMMAND    ${CMAKE_COMMAND}
        ARGS       -E copy_if_different "${SOURCE_JS}" "$(OutDir)${BASENAME}")
endmacro()

copy_javascript("json2.js")
copy_javascript("extension/boilerplate.js")
copy_javascript("extension/background.js")
copy_javascript("ie.js")

file(GLOB RUNNER VerityControllerRunner/*.c)
add_executable(runner ${RUNNER})
