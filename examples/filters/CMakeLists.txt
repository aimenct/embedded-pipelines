# Check required dependencies.
# None

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
  src_template.cpp
  sink_template.cpp
  filter_template.cpp
  sim_sdk_src.cpp
  image_src.cpp
)

# Add include search directories.
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:${HEADERS_INSTALL_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Add shared libraries to link with during linking process.
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    #None
)


