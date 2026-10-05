# Require a minimum version of CMake for the project
cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

# add_compile_options(-fno-diagnostics-color)

set(EPF_VER_MAJOR 0)
set(EPF_VER_MINOR 1)
set(EPF_VER_PATCH 0)

# Create new project. Library will get name from project name.
project(
  epf
  VERSION
  ${EPF_VER_MAJOR}.${EPF_VER_MINOR}.${EPF_VER_PATCH}
  LANGUAGES
    CXX
    C
  )

include(CTest)

# Add option to install locally (by default it installs in the system)
option( INSTALL_LOCAL OFF )

# Add option for Debug mode (richer error output and enable step-by-step debug but less performance)
option( DEBUG OFF )

# Add options to control compilation of some parts of the library
option( ENABLE_CAMERA OFF )
option( ENABLE_OPCUA OFF )
option( ENABLE_DOCS OFF )

# Add an option to enable all parts of the library
option( ENABLE_ALL OFF )

# Define dependecies version
set( YAMLCPP_VERSION 0.5.2 )
set( ARAVIS_VERSION 0.8.30 )
set( OPEN62541_VERSION 1.4.1 )

# Set all feature options to ON when ENABLE_ALL is requested.
if(ENABLE_ALL)
  set(ENABLE_CAMERA ON CACHE BOOL "Enable camera module" FORCE)
  set(ENABLE_OPCUA  ON CACHE BOOL "Enable OPCUA module" FORCE)
endif()

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

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME}
    PUBLIC
      "-Wno-sign-conversion")
endif ()
