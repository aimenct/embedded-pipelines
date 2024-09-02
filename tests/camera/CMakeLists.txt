#-----------test_arv_cam----------#
# Add the output executable for the test
add_executable(test_arv_cam)


# Add sources specific for the test
target_sources(
  test_arv_cam
  PRIVATE
  test_arv_cam.cpp
)

# Link against the library itself.
target_link_libraries(
  test_arv_cam
  PUBLIC
  ${PROJECT_NAME}
)

# Add targets to the global "tests" target
add_dependencies(tests test_arv_cam)
