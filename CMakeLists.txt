cmake_minimum_required (VERSION 2.6)

project(cubby)

set(SOURCES cubby.c mongoose.c)
add_executable(cubby ${SOURCES})
