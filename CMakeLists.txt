cmake_minimum_required (VERSION 2.6)

include(BundleUtilities)
project(verity)

if (UNIX)
    set(gecko_defs "-DXP_UNIX")
    if(APPLE)
        set(gecko_defs "${gecko_defs} -DXP_MACOSX")
    endif()
endif()

set(SOURCES Info.plist
            plugin.c
            vendor/attendant/attendant_posix.c
            vendor/attendant/errors.c
            )
add_library(verity MODULE ${SOURCES})
add_executable(relay vendor/attendant/relay_posix.c
                     vendor/attendant/errors.c)
set_target_properties(verity PROPERTIES
    BUNDLE 1
    BUNDLE_EXTENSION plugin
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION plugin
    XCODE_ATTRIBUTE_MACH_O_TYPE mh_bundle
    XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    LINK_FLAGS "-lcurl -fpcc-struct-return -W1,-exported_symbols_list,\"${CMAKE_CURRENT_BINARY_DIR}/exports.txt\"")

if (XCODE)
copy_resolved_item_into_bundle(${CMAKE_CURRENT_SOURCE_DIR}/vendor/cubby/build/cubby gerg/bluarg)
else()
copy_resolved_item_into_bundle(${CMAKE_CURRENT_SOURCE_DIR}/vendor/cubby/build/cubby verity.plugin/Contents/MacOS/cubby)
copy_resolved_item_into_bundle(${CMAKE_CURRENT_BINARY_DIR}/relay verity.plugin/Contents/MacOS/relay)
endif()
