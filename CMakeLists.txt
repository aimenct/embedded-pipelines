# Require a minimum version of CMake for the project
cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EP_VER_MAJOR 0)
set(EP_VER_MINOR 4)
set(EP_VER_PATCH 0)

# Create new project. Library will get name from project name.
project(
  embedded-pipelines
  VERSION
  ${EP_VER_MAJOR}.${EP_VER_MINOR}.${EP_VER_PATCH}
  LANGUAGES
    CXX
    C
)

# Add option to install locally (by default it installs in the system)
option( INSTALL_LOCAL OFF )

# Add option for Debug mode (richer error output and enable step-by-step debug but less performance)
option( DEBUG OFF )

# Add options to control compilation of some parts of the library
option( ENABLE_CAMERA OFF )
option( ENABLE_MODBUS OFF )
option( ENABLE_OPCUA OFF )
option( ENABLE_SOCKETS OFF )
option( ENABLE_TEMPLATES OFF )
option( ENABLE_RTSP OFF )
option( ENABLE_DOCS OFF )

# Add an option to enable all parts of the library
option( ENABLE_ALL OFF )

# Define dependecies version
set( YAMLCPP_VERSION 0.5.2 )
set( ARAVIS_VERSION 0.8.21 )
set( OPEN62541_VERSION 1.2.2 )
set( LIBMODBUS_VERSION 3.0.6 )
set( GSTREAMER_VERSION 1.16.2 )

# Set all options to ON just for this execution (not in CACHE)
if ( ENABLE_ALL )
  set( ENABLE_CAMERA ON)
  set( ENABLE_MODBUS OFF)
  set( ENABLE_OPCUA ON)
  set( ENABLE_SOCKETS OFF)
  set( ENABLE_TEMPLATES ON)
  set( ENABLE_RTSP OFF)
endif ()

# Add a subdirectory to compile the library
add_subdirectory(src)

# Add a subdirectories to compile library tests and sample programs.
# EXCLUDE_FROM_ALL makes the tests not to be compiled when doing just make.
add_subdirectory(tests EXCLUDE_FROM_ALL)
add_subdirectory(examples EXCLUDE_FROM_ALL)
if( ENABLE_DOCS )
  add_subdirectory(docs EXCLUDE_FROM_ALL)
endif ()

# Select compiler version.
target_compile_features(${PROJECT_NAME}
  PUBLIC
    cxx_std_17
)

# Add compilation options.
if ( DEBUG )
  target_compile_options(${PROJECT_NAME}
    PUBLIC
      # Enable rich error outputs, debug mode and all the warnings.
      "-O0" "-g" "-Wall" "-pedantic" "-Wextra" "-Wconversion")
else ()
  target_compile_options(${PROJECT_NAME}
    PUBLIC
      # Enable performance optimizations and all the warnings.
      "-O3" "-Wall" "-pedantic" "-Wextra" "-Wconversion")
endif ()
