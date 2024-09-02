set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#Add a global "examples" target to compile all tests from enabled subdirectories
add_custom_target(examples)

#-- -- -- -- -- - tutorial_01 -- -- -- -- -- #
#Add the output executable for the test
add_executable(tutorial_01)

#Add sources specific for the test
target_sources(
  tutorial_01
  PRIVATE
  tutorial_01.cpp
  filters/src_template.cpp
  )

#Link against the library itself.
target_link_libraries(
  tutorial_01
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
add_dependencies(examples tutorial_01)

#-- -- -- -- -- - tutorial_02 -- -- -- -- -- #
#Add the output executable for the test
add_executable(tutorial_02)

#Add sources specific for the test
target_sources(
  tutorial_02
  PRIVATE
  tutorial_02.cpp
  filters/sink_template.cpp
  filters/filter_template.cpp
  filters/src_template.cpp
  )

#Link against the library itself.
target_link_libraries(
  tutorial_02
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
add_dependencies(examples tutorial_02)

#-- -- -- -- -- - tutorial_03 -- -- -- -- -- #
#Add the output executable for the test
add_executable(tutorial_03)

#Add sources specific for the test
target_sources(
  tutorial_03
  PRIVATE
  tutorial_03.cpp
  filters/sink_template.cpp
  filters/sim_sdk_src.cpp
  libs/sensor_sdk.cpp
  )

#Link against the library itself.
target_link_libraries(
  tutorial_03
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
add_dependencies(examples tutorial_03)

#-- -- -- -- -- - image_display -- -- -- -- -- #
#Add your source files
add_executable(image_display)

#Find LibTIFF package
find_package(TIFF REQUIRED)
#Include LibTIFF headers
include_directories(${TIFF_INCLUDE_DIRS})

#Add sources specific for the test
target_sources(
  image_display
  PRIVATE
  image_display.cpp
  filters/image_src.cpp
#filters / postproc.cpp
)

#Link against the library itself.
target_link_libraries(
  image_display
  PRIVATE
  Threads::Threads
  ${TIFF_LIBRARIES}
#${OpenCV_LIBS }
  PUBLIC
    ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
add_dependencies(examples image_display)

#-- -- -- -- -- - camera_pipeline -- -- -- -- -- #
if ( ENABLE_CAMERA AND ENABLE_OPCUA )
#-- -- -- -- -- - camera_pipeline -- -- -- -- -- #
#Add the output executable for the test
add_executable(camera_pipeline)

#Add sources specific for the test
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

#Add targets to the global "examples" target
  add_dependencies(examples camera_pipeline)

#-- -- -- -- -- - sim_sensor_pipeline -- -- -- -- -- #
#Add the output executable for the test
  add_executable(sim_sensor_pipeline)

#Add sources specific for the test
  target_sources(
    sim_sensor_pipeline
  PRIVATE
  sim_sensor_pipeline.cpp
  filters/src_template.cpp
  )

#Link against the library itself.
  target_link_libraries(
    sim_sensor_pipeline
  PUBLIC
  ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
  add_dependencies(examples sim_sensor_pipeline)

else()
  message(STATUS "To compile camera_pipeline enable BASIC_IO, CAMERA and OPCUA.")
endif ()

#-- -- -- -- -- - test sensor_sdk -- -- -- -- -- #
#Add the output executable for the test
add_executable(test_sensor_sdk)

#Add sources specific for the test
target_sources(
  test_sensor_sdk
  PRIVATE
  libs/test_sensor_sdk.cpp
  libs/sensor_sdk.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_sensor_sdk
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#Add targets to the global "examples" target
add_dependencies(examples test_sensor_sdk)
