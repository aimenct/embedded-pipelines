# Check required dependencies.
find_package(PkgConfig REQUIRED)
set(DEPENDENCIES gobject-2.0
                 glib-2.0
                 gstreamer-rtsp-server-1.0>=${GSTREAMER_VERSION}
                 gstreamer-1.0>=${GSTREAMER_VERSION})
pkg_check_modules(GSTLIBS REQUIRED IMPORTED_TARGET ${DEPENDENCIES})

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filters/rtsp_sink.cpp
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
    PkgConfig::GSTLIBS
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/filters/rtsp_sink.h"
  DESTINATION
  "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/rtsp.h"
  DESTINATION
    ${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/
)