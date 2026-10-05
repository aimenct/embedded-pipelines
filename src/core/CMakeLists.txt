# Check required dependencies.
if (NOT TARGET yaml-cpp)
  find_package(yaml-cpp ${YAMLCPP_MIN_VERSION} REQUIRED)
endif ()
#find_package(LibXml2 REQUIRED)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Add compilation instructions for this part of the library.
set(EPF_CORE_SOURCES
  libs/epf_types.cpp
  libs/filter.cpp
  libs/filter_templates.cpp
  libs/filter_impl.cpp
  libs/queue.cpp
  libs/queue_handlers.cpp
  libs/node.cpp
  libs/node_tree.cpp
  libs/message.cpp
  libs/settings.cpp
  libs/settings_templates.cpp
  libs/settings_impl.cpp
  libs/pipeline.cpp
  libs/image_formats.cpp
  libs/image_processing.cpp
  libs/image_object.cpp
  libs/compressed_image_object.cpp
  libs/serialization.cpp
  libs/error_logger.cpp
  filters/file_sink.cpp
  filters/file_src.cpp
  libs/glut_window.cpp
  filters/glut_display.cpp
)


target_sources(
  ${PROJECT_NAME}
  PRIVATE
    ${EPF_CORE_SOURCES}
)

# Add include search directories.
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:${HEADERS_INSTALL_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#  ${LIBXML2_INCLUDE_DIR}
)

# Add shared libraries to link with during linking process.
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    yaml-cpp
#    ${LIBXML2_LIBRARIES}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${GLUT_INCLUDE_DIRS}
)
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/epf_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/filter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/queue.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/queue_handlers.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/node.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/node_tree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/message.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/settings.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/pipeline.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/image_formats.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/image_processing.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/time_utils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/image_object.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/compressed_image_object.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/serialization.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/signals.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/glut_window.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/error_logger.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/core/libs/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/file_sink.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/file_src.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/glut_display.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/core/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/core.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/core/"
)
