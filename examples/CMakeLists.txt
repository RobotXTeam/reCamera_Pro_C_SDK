cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------
# Helper function: register an example executable.
# Subdirectory CMakeLists.txt files call this function to register.
# ----------------------------------------------------------------
function(add_example name)
    # Optional: pass extra source files; default is main.cpp in current dir
    set(extra_sources ${ARGN})

    if(extra_sources)
        add_executable(${name} ${extra_sources})
    else()
        add_executable(${name} main.cpp)
    endif()

    target_link_libraries(${name}
        PRIVATE
            reCamera_pro_sdk
            rknnrt
    )
    if(TARGET MPP::MPP)
        target_link_libraries(${name} PRIVATE MPP::MPP)
    endif()
    if(TARGET RGA::RGA)
        target_link_libraries(${name} PRIVATE RGA::RGA)
    endif()
    
    target_include_directories(${name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/sysroot/oem/usr/include
    )
    target_link_directories(${name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/sysroot/oem/usr/lib
    )

    target_compile_features(${name} PRIVATE cxx_std_17)

    # Set runtime library search path so shared libs are found on device
    set_target_properties(${name} PROPERTIES
        INSTALL_RPATH   "/oem/usr/lib:/usr/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    install(TARGETS ${name}
        RUNTIME DESTINATION bin/examples
    )
endfunction()

# ----------------------------------------------------------------
# Add all 12 example subdirectories
# ----------------------------------------------------------------
add_subdirectory(hello_world)
add_subdirectory(gpio_control)
add_subdirectory(uart_echo)
add_subdirectory(imu_read)
add_subdirectory(camera_preview)
add_subdirectory(capture_image)
add_subdirectory(record_video)
add_subdirectory(rknn_yolo)
add_subdirectory(isp_control)
add_subdirectory(rtsp_server)
add_subdirectory(rtmp_push)
add_subdirectory(onvif_server)
