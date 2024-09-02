# Check required dependencies.
find_package(yaml-cpp ${YAMLCPP_VERSION} REQUIRED)

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filters/socket_client.cpp
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
    yaml-cpp
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/socket_client.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/sockets.h"
  DESTINATION
    ${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/
)
