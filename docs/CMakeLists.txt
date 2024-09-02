# Add the cmake folder so the FindSphinx module is found
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})


find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)


# Find all the public headers
get_target_property(EP_HEADER_DIR ${PROJECT_NAME} INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE EP_HEADERS $<BUILD_INTERFACE:${EP_HEADER_DIR}/libs/*.h)


set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src/core/libs)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

#Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) #Doxygen won't create this for us
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${EP_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs")

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_BUILD_SRC ${SPHINX_BUILD}/_sources)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)


configure_file("${PROJECT_SOURCE_DIR}/docs/conf.py" "${SPHINX_BUILD_SRC}/conf.py")

# Only regenerate Sphinx when:
#  - Doxygen has rerun 
#  - Our doc files have been updated
#  - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
	COMMAND 
		${SPHINX_EXECUTABLE} -b html
		# Sphinx where the modified config is located
		-c ${SPHINX_BUILD_SRC}
		# Tell Breathe where to find the Doxygen output
		-Dbreathe_projects.embedded-pipelines=${DOXYGEN_OUTPUT_DIR}/xml
		${SPHINX_SOURCE} ${SPHINX_BUILD}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS 
		# Other docs files you want to track should go here (or in some variable)
		${SPHINX_SOURCE}/src/*
		${SPHINX_SOURCE}/index.rst
		${DOXYGEN_INDEX_FILE}
	MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
	COMMENT "Generating documentation with Sphinx"
)

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})