# Check required dependencies.
# For aravis the pkgconfig is used inside CMake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ARAVIS aravis-0.10)
if(NOT ARAVIS_FOUND)
	pkg_check_modules(ARAVIS REQUIRED aravis-0.8)
	find_library(ARAVIS_LIBRARIES aravis-0.8 REQUIRED)
else()
	find_library(ARAVIS_LIBRARIES aravis-0.10 REQUIRED)
endif()

# pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDENCIES})
find_package(LibXml2 REQUIRED)

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filters/arv_cam.cpp
    #libs/parse_genicam_xml.cpp
)

# Add include search directories.
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:${HEADERS_INSTALL_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${LIBXML2_INCLUDE_DIR}
    ${ARAVIS_INCLUDE_DIRS}
)

# Add shared libraries to link with during linking process.
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    ${ARAVIS_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${ARAVIS_LIBRARIES}
    ${GLIB2_LIBRARIES}
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/arv_cam.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/camera/filters/"
)

# Tell CMake where to install public headers when using Cmake install
# install(
#   FILES
#     "${CMAKE_CURRENT_SOURCE_DIR}/libs/parse_genicam_xml.h"
#   DESTINATION
#     "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/camera/libs/"
# )

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/camera.h"
    DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/camera/"
)
