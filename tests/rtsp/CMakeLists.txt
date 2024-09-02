if (NOT ENABLE_CAMERA)
    message( STATUS "To compile all RTSP tests enable CAMERA: some targets were skipped." )
else ()
    #-----------test_rtsp_sink_cam----------#
    # Add the output executable for the test
    add_executable(test_rtsp_sink_cam)

    # Add sources specific for the test
    target_sources(
        test_rtsp_sink_cam
        PRIVATE
        test_rtsp_sink_cam.cpp
    )

    # Link against the library itself.
    target_link_libraries(
        test_rtsp_sink_cam
        PUBLIC
            ${PROJECT_NAME}
    )

    # Add targets to the global "tests" target
    add_dependencies(tests test_rtsp_sink_cam)
endif ()
