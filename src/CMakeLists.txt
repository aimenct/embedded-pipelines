# Add a new shared library with the project name.
add_library(${PROJECT_NAME} SHARED)

# Optional interface component targets.
# These do not create extra library files; they only expose feature availability.
if(ENABLE_CAMERA)
#  add_library(${PROJECT_NAME}_camera INTERFACE)
#  target_link_libraries(${PROJECT_NAME}_camera INTERFACE ${PROJECT_NAME})
  add_library(${PROJECT_NAME}_camera INTERFACE)
  add_library(${PROJECT_NAME}::${PROJECT_NAME}_camera ALIAS ${PROJECT_NAME}_camera)
  target_link_libraries(${PROJECT_NAME}_camera INTERFACE ${PROJECT_NAME})
endif()

if(ENABLE_OPCUA)
#  add_library(${PROJECT_NAME}_opcua INTERFACE)
#  target_link_libraries(${PROJECT_NAME}_opcua INTERFACE ${PROJECT_NAME})
  add_library(${PROJECT_NAME}_opcua INTERFACE)
  add_library(${PROJECT_NAME}::${PROJECT_NAME}_opcua ALIAS ${PROJECT_NAME}_opcua)
  target_link_libraries(${PROJECT_NAME}_opcua INTERFACE ${PROJECT_NAME})
endif()

# Add installation rules for the library
if(INSTALL_LOCAL) # Install lib and headers locally in the repo main folder
  # Tell CMake where to install the library when using make install
  install(TARGETS ${PROJECT_NAME}
          DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../lib")

  # Create a variable to store headers install folder (set to system folder)
  set(HEADERS_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")

  # Message so user knows where files will be installed before installing
  message(STATUS "Local install selected. Target library file will installed in ${CMAKE_CURRENT_SOURCE_DIR}/../lib and headers in ${CMAKE_CURRENT_SOURCE_DIR}/../include.")
else() # Install lib and headers in the system
  # Define install directory
  include(GNUInstallDirs)
  set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME})

  # Tell CMake where to install the library when using make install
  install(
    TARGETS
      ${PROJECT_NAME}
    EXPORT
      ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  if(TARGET ${PROJECT_NAME}_camera)
    install(
      TARGETS
        ${PROJECT_NAME}_camera
      EXPORT
        ${PROJECT_NAME}Targets
    )
  endif()

  if(TARGET ${PROJECT_NAME}_opcua)
    install(
      TARGETS
        ${PROJECT_NAME}_opcua
      EXPORT
        ${PROJECT_NAME}Targets
    )
  endif()

  # Install targets file
  install(
    EXPORT
      ${PROJECT_NAME}Targets
    NAMESPACE
      ${PROJECT_NAME}::
    DESTINATION
      ${CMAKE_INSTALL_CONFIGDIR}
  )

  # Create a variable to store headers install folder (set to main repo folder)
  set(HEADERS_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

  # CMake configuration files to make package available to other libraries
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
  )
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_CONFIGDIR}"
  )
  install(
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}Config.cmake"
      "${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION
      ${CMAKE_INSTALL_CONFIGDIR}
  )

  # CPack
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CPackConfig.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/../CPackConfig.cmake)
    include(CPack)
  endif()
endif()

# Add source for different parts of the library
add_subdirectory(core)
if(ENABLE_CAMERA)
  add_subdirectory(camera)
endif()
if(ENABLE_OPCUA)
  add_subdirectory(opcua)
endif()
