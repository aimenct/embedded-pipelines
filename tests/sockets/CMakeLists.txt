#-----------test_socket_client----------#
  # Add the output executable for the test
  add_executable(test_socket_client)

  # Add sources specific for the test
  target_sources(
  test_socket_client
    PRIVATE
      test_socket_client.cpp
    )

  # Link against the library itself.
  target_link_libraries(
  test_socket_client
  PUBLIC
    ${PROJECT_NAME}
  )

  # Add targets to the global "tests" target
  add_dependencies(tests test_socket_client)


