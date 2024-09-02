# Check required dependencies.
# For libmodbus the pkgconfig is used inside CMake
find_package(PkgConfig REQUIRED)
set(DEPENDENCIES "libmodbus>=${LIBMODBUS_VERSION}")
pkg_check_modules(MODBUS REQUIRED IMPORTED_TARGET ${DEPENDENCIES})

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filters/modbus_client.cpp
)

# Add include search directories.
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:${HEADERS_INSTALL_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${MODBUS_INCLUDE_DIRS}
)

# Add shared libraries to link with during linking process.
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    ${MODBUS_LIBRARIES}
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/modbus_client.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/modbus.h"
  DESTINATION
    ${HEADERS_INSTALL_DIR}/${PROJECT_NAME}
)
