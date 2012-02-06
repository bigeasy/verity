cmake_minimum_required (VERSION 2.6)

project(verity)

if (UNIX)
    set(gecko_defs "-DXP_UNIX")
    if(APPLE)
        set(gecko_defs "${gecko_defs} -DXP_MACOSX")
    endif()
endif()

set(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist plugin.c)
add_library(verity MODULE ${SOURCES})
set_target_properties(verity PROPERTIES
    BUNDLE 1
    BUNDLE_EXTENSION plugin
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION plugin
    XCODE_ATTRIBUTE_MACH_O_TYPE mh_bundle
    XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    LINK_FLAGS "-W1,-exported_symbols_list,\"${CMAKE_CURRENT_BINARY_DIR}/exports.txt\"")
