#
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was epfConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

#get_filename_component(epf_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

#include(CMakeFindDependencyMacro)

#find_dependency(yaml-cpp REQUIRED)

#if(ON)
#  find_dependency(open62541 REQUIRED)
#endif()

#if(NOT TARGET epf::epf)
#  include("${epf_CMAKE_DIR}/epfTargets.cmake")
#  set_and_check(epf_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
#  set(EMBEDDED_PIPELINES_LIBRARY epf::epf)
#endif()


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was epfConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

find_dependency(yaml-cpp REQUIRED)

set(epf_CAMERA_AVAILABLE ON)
set(epf_OPCUA_AVAILABLE  ON)

if(epf_CAMERA_AVAILABLE)
  find_dependency(PkgConfig REQUIRED)
  pkg_check_modules(ARAVIS aravis-0.10)
  if(NOT ARAVIS_FOUND)
    pkg_check_modules(ARAVIS REQUIRED aravis-0.8)
  endif()
  find_dependency(LibXml2 REQUIRED)
endif()

if(epf_OPCUA_AVAILABLE)
  find_dependency(open62541 REQUIRED)
endif()

if(NOT TARGET epf::epf)
  include("${CMAKE_CURRENT_LIST_DIR}/epfTargets.cmake")
endif()

set_and_check(epf_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
set(EMBEDDED_PIPELINES_LIBRARY epf::epf)

# Base package is always available
set(epf_FOUND TRUE)
set(epf_core_FOUND TRUE)

# Optional components
if(TARGET epf::epf_camera)
  set(epf_camera_FOUND TRUE)
else()
  set(epf_camera_FOUND FALSE)
endif()

if(TARGET epf::epf_opcua)
  set(epf_opcua_FOUND TRUE)
else()
  set(epf_opcua_FOUND FALSE)
endif()

check_required_components(epf)
