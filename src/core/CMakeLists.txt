# Check required dependencies.
find_package(yaml-cpp ${YAML_VERSION} REQUIRED)
find_package(LibXml2 REQUIRED)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Add compilation instructions for this part of the library.
target_sources(
  ${PROJECT_NAME}
  PRIVATE
    libs/ep_types.cpp
    libs/filter.cpp
    libs/queue.cpp
    libs/queue_handlers.cpp
    libs/node.cpp
    libs/node_tree.cpp
    libs/message.cpp
    libs/settings.cpp
    libs/settings_templates.cpp
    libs/settings_impl.cpp
    libs/pipeline.cpp
    libs/image_utils.cpp
    libs/image_object.cpp
    libs/serialization.cpp
    filters/file_sink.cpp
    filters/file_src.cpp
    filters/gl_display.cpp
)

# Add include search directories.
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:${HEADERS_INSTALL_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${LIBXML2_INCLUDE_DIR}
  ${GLUT_INCLUDE_DIRS}
)

# Add shared libraries to link with during linking process.
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    yaml-cpp
    ${LIBXML2_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# Tell CMake where to install public headers when using Cmake install
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/ep_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/filter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/queue.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/queue_handlers.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/node.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/node_tree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/message.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/settings.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/pipeline.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/image_utils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/image_object.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/serialization.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/signals.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/libs/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/file_sink.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/file_src.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/gl_display.h"
  DESTINATION
    "${HEADERS_INSTALL_DIR}/${PROJECT_NAME}/filters/"
)

install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/core.h"
  DESTINATION
    ${HEADERS_INSTALL_DIR}/${PROJECT_NAME}
)
