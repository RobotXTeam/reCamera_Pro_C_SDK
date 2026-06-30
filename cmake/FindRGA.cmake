# FindRGA.cmake
# Find the Rockchip RGA (Raster Graphics Accelerator) library

find_path(RGA_INCLUDE_DIR
    NAMES rga/RgaApi.h im2d.hpp
    HINTS
        "$ENV{RGA_INCLUDE_DIR}"
        "${CMAKE_SYSROOT}/usr/include"
)

find_library(RGA_LIBRARY
    NAMES rga
    HINTS
        "$ENV{RGA_LIB_DIR}"
        "${CMAKE_SYSROOT}/usr/lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RGA
    REQUIRED_VARS RGA_LIBRARY RGA_INCLUDE_DIR
)

if(RGA_FOUND AND NOT TARGET RGA::RGA)
    add_library(RGA::RGA SHARED IMPORTED)
    set_target_properties(RGA::RGA PROPERTIES
        IMPORTED_LOCATION "${RGA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${RGA_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(RGA_INCLUDE_DIR RGA_LIBRARY)
