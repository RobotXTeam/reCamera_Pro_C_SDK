# FindRKNNRuntime.cmake
# Find the RKNN Runtime library (librknnrt.so)
#
# Exported target: RKNNRuntime::RKNNRuntime
# Variables:
#   RKNNRuntime_FOUND
#   RKNNRuntime_INCLUDE_DIRS
#   RKNNRuntime_LIBRARIES

find_path(RKNNRuntime_INCLUDE_DIR
    NAMES rknn_api.h
    HINTS
        "$ENV{RKNN_INCLUDE_DIR}"
        "${CMAKE_SYSROOT}/usr/include"
        "${CMAKE_SYSROOT}/usr/include/rknn"
    PATH_SUFFIXES rknn
)

find_library(RKNNRuntime_LIBRARY
    NAMES rknnrt rknn_api
    HINTS
        "$ENV{RKNN_LIB_DIR}"
        "${CMAKE_SYSROOT}/usr/lib"
        "${CMAKE_SYSROOT}/usr/lib/rknn"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RKNNRuntime
    REQUIRED_VARS RKNNRuntime_LIBRARY RKNNRuntime_INCLUDE_DIR
)

if(RKNNRuntime_FOUND AND NOT TARGET RKNNRuntime::RKNNRuntime)
    add_library(RKNNRuntime::RKNNRuntime SHARED IMPORTED)
    set_target_properties(RKNNRuntime::RKNNRuntime PROPERTIES
        IMPORTED_LOCATION "${RKNNRuntime_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${RKNNRuntime_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(RKNNRuntime_INCLUDE_DIR RKNNRuntime_LIBRARY)
