# FindRKAIQRuntime.cmake
# Find the Rockchip AIQ (ISP) runtime library

find_path(RKAIQRuntime_INCLUDE_DIR
    NAMES rk_aiq.h rk_aiq_user_api2_sysctl.h
    HINTS
        "$ENV{RKAIQ_INCLUDE_DIR}"
        "${CMAKE_SYSROOT}/usr/include"
        "${CMAKE_SYSROOT}/usr/include/rkaiq"
    PATH_SUFFIXES rkaiq
)

find_library(RKAIQRuntime_LIBRARY
    NAMES rkaiq
    HINTS
        "$ENV{RKAIQ_LIB_DIR}"
        "${CMAKE_SYSROOT}/usr/lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RKAIQRuntime
    REQUIRED_VARS RKAIQRuntime_LIBRARY RKAIQRuntime_INCLUDE_DIR
)

if(RKAIQRuntime_FOUND AND NOT TARGET RKAIQRuntime::RKAIQRuntime)
    add_library(RKAIQRuntime::RKAIQRuntime SHARED IMPORTED)
    set_target_properties(RKAIQRuntime::RKAIQRuntime PROPERTIES
        IMPORTED_LOCATION "${RKAIQRuntime_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${RKAIQRuntime_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(RKAIQRuntime_INCLUDE_DIR RKAIQRuntime_LIBRARY)
