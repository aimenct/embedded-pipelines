#-----------test_modbus_client----------#

# Add the output executable for the test
add_executable(test_modbus_client)

# Add sources specific for the test
target_sources(
test_modbus_client
PRIVATE
  test_modbus_client.cpp
)

  # Link against the library itself.
target_link_libraries(
test_modbus_client
PUBLIC
  ${PROJECT_NAME}
)

  # Add targets to the global "tests" target
add_dependencies(tests test_modbus_client)


#-----------test_random_server----------#
# Add the output executable for the test
add_executable(test_random_server)

# Add sources specific for the test
target_sources(
  test_random_server
  PRIVATE
    test_random_server.cpp
)

# Link against the library itself.
target_link_libraries(
  test_random_server
  PUBLIC
    ${PROJECT_NAME}
)

# Add targets to the global "tests" target
add_dependencies(tests test_random_server)