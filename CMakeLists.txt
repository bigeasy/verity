cmake_minimum_required(VERSION 2.6)

project(verity)

file(GLOB BHO VerityController/*.c VerityController/*.idl VerityController/*.rc)

# In accordance with: http://public.kitware.com/Bug/view.php?id=12909
include_directories(. "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)")
add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
add_library(verity MODULE ${BHO})


file(GLOB RUNNER VerityControllerRunner/*.c)
add_executable(runner ${RUNNER})
