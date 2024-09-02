# Add a new shared library with the project name.
add_library(${PROJECT_NAME} SHARED)

# Add installation rules for the library
if ( INSTALL_LOCAL ) # Install lib and headers locally in the repo main folder
  # Tell CMake where to install the library when using make install
  install(TARGETS ${PROJECT_NAME}
          DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
  # Create a variable to store headers install folder (set to system folder)
  set(HEADERS_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  # Message so user knows where files will be installed before installing
  message(STATUS "Local install selected. Target library file will installed in ${CMAKE_CURRENT_SOURCE_DIR}/../lib and headers in ${CMAKE_CURRENT_SOURCE_DIR}/../include.")
else () # Install lib and headers in the system
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
  	VERSION ${VERSION}
  	COMPATIBILITY ExactVersion
  )
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME}Config.cmake.in
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
  include(${CMAKE_CURRENT_SOURCE_DIR}/../CPackConfig.cmake)
  include(CPack)
endif ()

# Add source for different parts of the library
add_subdirectory(core)
if ( ENABLE_CAMERA )
  add_subdirectory( camera )
endif ()
if ( ENABLE_MODBUS )
  add_subdirectory( modbus )
endif ()
if ( ENABLE_OPCUA )
  add_subdirectory( opcua )
endif ()
if ( ENABLE_SOCKETS )
  add_subdirectory( sockets )
endif ()
if ( ENABLE_RTSP )
  add_subdirectory( rtsp )
endif ()
