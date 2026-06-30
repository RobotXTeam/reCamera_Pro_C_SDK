# FindMPP.cmake
# Find the Rockchip MPP (Media Process Platform) library
#
# Exported target: MPP::MPP
# Variables:
#   MPP_FOUND
#   MPP_INCLUDE_DIRS
#   MPP_LIBRARIES

find_path(MPP_INCLUDE_DIR
    NAMES rockchip/rk_mpi.h mpp_frame.h
    HINTS
        "$ENV{MPP_INCLUDE_DIR}"
        "${CMAKE_SYSROOT}/usr/include"
        "${CMAKE_SYSROOT}/usr/include/rockchip"
    PATH_SUFFIXES rockchip
)

find_library(MPP_LIBRARY
    NAMES rockchip_mpp
    HINTS
        "$ENV{MPP_LIB_DIR}"
        "${CMAKE_SYSROOT}/usr/lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPP
    REQUIRED_VARS MPP_LIBRARY MPP_INCLUDE_DIR
)

if(MPP_FOUND AND NOT TARGET MPP::MPP)
    add_library(MPP::MPP SHARED IMPORTED)
    set_target_properties(MPP::MPP PROPERTIES
        IMPORTED_LOCATION "${MPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPP_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(MPP_INCLUDE_DIR MPP_LIBRARY)
