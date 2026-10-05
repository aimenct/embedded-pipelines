set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add a global "examples" target to compile all tests from enabled subdirectories
add_custom_target(examples)

#-- -- -- -- -- - camera_pipeline -- -- -- -- -- #
if ( ENABLE_CAMERA AND ENABLE_OPCUA )
#-- -- -- -- -- - camera_pipeline -- -- -- -- -- ##Add the output executable for the test
add_executable(camera_pipeline)
# Add sources specific for the test
  target_sources(
    camera_pipeline
  PRIVATE
  camera_pipeline.cpp
  )

target_include_directories( camera_pipeline PRIVATE ../tests/core/)

#Link against the library itself.
  target_link_libraries(
    camera_pipeline
  PUBLIC
  ${PROJECT_NAME}
  )

# Add targets to the global "examples" target
add_dependencies(examples camera_pipeline)
endif()


