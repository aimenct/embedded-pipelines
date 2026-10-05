# ----- test_queue ----- #
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add the output executable for the test
add_executable(test_queue)

# Add sources specific for the test
target_sources(
  test_queue
  PRIVATE
  test_queue.cpp
  )

# Link against the library itself.
target_link_libraries(
  test_queue
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

# ----- test_message ----- #
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(test_message)

# Add sources specific for the test
target_sources(
  test_message
  PRIVATE
  test_message.cpp
  )

# Link against the library itself.
target_link_libraries(
  test_message
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)

# ----- test_display ----- #
# Add your source files
add_executable(test_display)

# Add sources specific for the test
target_sources(
test_display
PRIVATE
test_display.cpp
)

# Link against the library itself.
target_link_libraries(
  test_display
  PRIVATE
  Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)




# ----- test_plot3dwindow_patterns ----- #
add_executable(test_plot3dwindow_patterns)

target_sources(
  test_plot3dwindow_patterns
  PRIVATE
  test_plot3dwindow_patterns.cpp
)

target_compile_options(test_plot3dwindow_patterns PRIVATE -include sstream)

target_link_libraries(
  test_plot3dwindow_patterns
  PRIVATE
  Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)


# ----- test_plot2dwindow_patterns ----- #
add_executable(test_plot2dwindow_patterns)

target_sources(
  test_plot2dwindow_patterns
  PRIVATE
  test_plot2dwindow_patterns.cpp
)

target_compile_options(test_plot2dwindow_patterns PRIVATE -include sstream)

target_link_libraries(
  test_plot2dwindow_patterns
  PRIVATE
  Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)

# ----- test_imagewindow_patterns ----- #
# Add the output executable for the test
add_executable(test_imagewindow_patterns)

# Add sources specific for the test
target_sources(
  test_imagewindow_patterns
  PRIVATE
  test_imagewindow_patterns.cpp
)

# Link against the library itself.
target_link_libraries(
  test_imagewindow_patterns
  PRIVATE
  Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)


add_dependencies(tests test_message test_queue test_display test_imagewindow_patterns test_plot2dwindow_patterns test_plot3dwindow_patterns)

# GTESTS SUITE
find_package(GTest QUIET)
if (GTest_FOUND)
  include(GoogleTest)

  # ----- gtest_settings ----- #
  # Add the output executable for the test
  add_executable(gtest_settings)

  # Add sources specific for the test
  target_sources(
    gtest_settings
    PRIVATE
    gtest_settings.cpp
  )

  # Link against the library itself.
  target_link_libraries(
    gtest_settings
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC
    ${PROJECT_NAME}
  )
  # ----- gtest_ports ----- #
  # Add the output executable for the test

  add_executable(gtest_ports)

  # Add sources specific for the test
  target_sources(
    gtest_ports
    PRIVATE
    gtest_ports.cpp
  )

  # Link against the library itself.
  target_link_libraries(
    gtest_ports
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC
    ${PROJECT_NAME}
  )

  # ----- gtest_filter ----- #
  add_executable(gtest_filter)

  set(EPF_TEST_DATA_DIR "${PROJECT_SOURCE_DIR}/tests")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_vars.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/config_vars.h @ONLY)

  target_include_directories(gtest_filter PRIVATE ${CMAKE_CURRENT_BINARY_DIR})



  target_sources(
    gtest_filter
    PRIVATE
    gtest_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/dummy_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/src_filter.cpp
  )
  target_link_libraries(
    gtest_filter
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_queue ----- #
  add_executable(gtest_queue)
  target_sources(
    gtest_queue
    PRIVATE
    gtest_queue.cpp
  )
  target_link_libraries(
    gtest_queue
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_queue_stresstest ----- #
  add_executable(gtest_queue_stresstest)
  target_sources(
    gtest_queue_stresstest
    PRIVATE
    gtest_queue_stresstest.cpp
  )
  target_link_libraries(
    gtest_queue_stresstest
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_message ----- #
  add_executable(gtest_message)
  target_sources(
    gtest_message
    PRIVATE
    gtest_message.cpp
  )
  target_link_libraries(
    gtest_message
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_serialization ----- #
  add_executable(gtest_serialization)
  target_sources(
    gtest_serialization
    PRIVATE
    gtest_serialization.cpp
  )
  target_link_libraries(
    gtest_serialization
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_node ----- #
  add_executable(gtest_node)
  target_sources(
    gtest_node
    PRIVATE
    gtest_node.cpp
  )
  target_link_libraries(
    gtest_node
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_node_tree ----- #
  add_executable(gtest_node_tree)
  target_sources(
    gtest_node_tree
    PRIVATE
    gtest_node_tree.cpp
  )
  target_link_libraries(
    gtest_node_tree
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_compressed_image ----- #
  add_executable(gtest_compressed_image)
  target_sources(
    gtest_compressed_image
    PRIVATE
    gtest_compressed_image.cpp
  )
  target_link_libraries(
    gtest_compressed_image
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_image_object ----- #
  add_executable(gtest_image_object)
  target_sources(
    gtest_image_object
    PRIVATE
    gtest_image_object.cpp
  )
  target_link_libraries(
    gtest_image_object
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )


  # ----- gtest_image_formats ----- #
  add_executable(gtest_image_formats)
  target_sources(
    gtest_image_formats
    PRIVATE
    gtest_image_formats.cpp
  )
  target_link_libraries(
    gtest_image_formats
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )



  # ----- gtest_plot2d_window ----- #
  add_executable(gtest_plot2d_window)
  target_sources(
    gtest_plot2d_window
    PRIVATE
    gtest_plot2d_window.cpp
  )
  target_compile_options(gtest_plot2d_window PRIVATE -include sstream)
  target_link_libraries(
    gtest_plot2d_window
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC
    ${PROJECT_NAME}
  )

  # ----- gtest_plot3d_window ----- #
  add_executable(gtest_plot3d_window)
  target_sources(
    gtest_plot3d_window
    PRIVATE
    gtest_plot3d_window.cpp
  )
  target_link_libraries(
    gtest_plot3d_window
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC
    ${PROJECT_NAME}
  )

  # ----- gtest_image_window ----- #
  add_executable(gtest_image_window)
  target_sources(
    gtest_image_window
    PRIVATE
    gtest_image_window.cpp
  )
  target_link_libraries(
    gtest_image_window
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_pipeline ----- #
  add_executable(gtest_pipeline)

  target_compile_definitions(gtest_pipeline PRIVATE EPF_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests")

  target_sources(
    gtest_pipeline
    PRIVATE
    gtest_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/src_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/dummy_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/sink_filter.cpp
  )
  target_link_libraries(
    gtest_pipeline
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_signals ----- #
  add_executable(gtest_signals)

  target_sources(
    gtest_signals
    PRIVATE
    gtest_signals.cpp
  )
  target_link_libraries(
    gtest_signals
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  # ----- gtest_file_filters ----- #
  add_executable(gtest_file_filters)
  target_sources(
    gtest_file_filters
    PRIVATE
    gtest_file_filters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/src_filter.cpp
  )
  target_link_libraries(
    gtest_file_filters
    PRIVATE Threads::Threads
    GTest::gtest GTest::gtest_main
    PUBLIC ${PROJECT_NAME}
  )

  add_dependencies(tests gtest_settings gtest_ports gtest_filter gtest_queue gtest_queue_stresstest gtest_message gtest_serialization gtest_node gtest_node_tree gtest_pipeline gtest_compressed_image gtest_image_object gtest_image_formats gtest_image_window gtest_plot2d_window gtest_plot3d_window gtest_signals gtest_file_filters)

  set(EPF_GTEST_TARGETS
    gtest_settings
    gtest_ports
    gtest_filter
    gtest_queue
    gtest_queue_stresstest
    gtest_message
    gtest_serialization
    gtest_node
    gtest_node_tree
    gtest_pipeline
    gtest_compressed_image
    gtest_image_object
    gtest_image_formats
    gtest_image_window
    gtest_plot2d_window
    gtest_plot3d_window
    gtest_signals
    gtest_file_filters
  )

  set(EPF_DISPLAY_GTEST_TARGETS
    gtest_image_window
    gtest_plot2d_window
    gtest_plot3d_window
  )

  foreach(EPF_GTEST_TARGET IN LISTS EPF_GTEST_TARGETS)
    if(EPF_GTEST_TARGET IN_LIST EPF_DISPLAY_GTEST_TARGETS)
      gtest_discover_tests(${EPF_GTEST_TARGET} PROPERTIES LABELS display)
    else()
      gtest_discover_tests(${EPF_GTEST_TARGET})
    endif()
  endforeach()
else()
  message(WARNING "GTest not found — gtest suite will not be built")
endif()
