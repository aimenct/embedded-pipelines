# Check required dependencies.
find_package( open62541 ${OPEN62541_VERSION} REQUIRED )
find_package( yaml-cpp ${YAMLCPP_VERSION} REQUIRED )

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filters/opcua_server.cpp
    filters/opcua_client.cpp
    filters/opcua_xml_server.cpp
    libs/opcua_data.cpp
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
    open62541::open62541
    yaml-cpp
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/opcua_data.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/libs/"
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/opcua_client.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/opcua.h"
    DESTINATION
    ${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/
)