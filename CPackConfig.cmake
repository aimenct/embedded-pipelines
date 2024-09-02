# Output directory
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/packages)

# Package information
set(CPACK_PACKAGE_VENDOR "Aimen")
set(CPACK_PACKAGE_CONTACT "Roi Mendez <roi.mendez@aimen.es>")

# Package version
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Try to use branch name to set the package version suffix
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE BRANCH_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (BRANCH_NAME STREQUAL "master")
    set(PACKAGE_SUFFIX "")
  elseif (BRANCH_NAME STREQUAL "dev")
    set(PACKAGE_SUFFIX "~dev")
  elseif (BRANCH_NAME MATCHES "^feature/")
    string (REGEX REPLACE "^feature/" "~feature-" PACKAGE_SUFFIX ${BRANCH_NAME})
  elseif (BRANCH_NAME MATCHES "^bugfix/")
    string (REGEX REPLACE "^bugfix/" "~fix-" PACKAGE_SUFFIX ${BRANCH_NAME})
  elseif (BRANCH_NAME MATCHES "^release/")
    string (REGEX REPLACE "^release/" "~rc" PACKAGE_SUFFIX ${BRANCH_NAME})
  endif()
endif()

set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}${PACKAGE_SUFFIX}")

# Package generator
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

