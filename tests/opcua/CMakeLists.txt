find_package( OpenCV ${OPENCV_VERSION} REQUIRED )

#-----------test_opcua_server----------#
# Add the output executable for the test
add_executable(test_opcua_server)

# Add sources specific for the test
target_sources(
  test_opcua_server
  PRIVATE
    new_test_opcua_server.cpp
)

# Link against the library itself.
target_link_libraries(
  test_opcua_server
  PUBLIC
    ${PROJECT_NAME}
    ${OpenCV_LIBS}
)

# Add targets to the global "tests" target
add_dependencies(tests test_opcua_server)

#-----------test_opcua_client----------#
# Add the output executable for the test
add_executable(test_opcua_client)

# Add sources specific for the test
target_sources(
  test_opcua_client
  PRIVATE
  test_opcua_client.cpp
)

# Link against the library itself.
target_link_libraries(
  test_opcua_client
  PUBLIC
    ${PROJECT_NAME}
    ${OpenCV_LIBS}
)

# Add targets to the global "tests" target
add_dependencies(tests test_opcua_client)

# #-----------test_opcua_xml2server----------#
# set(UA_NAMESPACE_MAP_DI
#   "2:http://opcfoundation.org/UA/DI/"
# )
# set(UA_NAMESPACE_MAP_I4AAS
#   "2:http://opcfoundation.org/UA/I4AAS/"
# )
# set(UA_SCHEMA_DIR ${PROJECT_SOURCE_DIR}/src/opcua/ua_schema_nodesets)
# set(UA_MODEL_DIR ${PROJECT_SOURCE_DIR}/tests/opcua/xml2server)

# add_executable(test_opcua_xml2server)

# # First build DI Nodeset as Robotics Nodeset depends on it
# ua_generate_nodeset_and_datatypes(
#   NAME di
#   FILE_CSV ${UA_SCHEMA_DIR}/DI/Opc.Ua.Di.NodeIds.csv
#   FILE_BSD ${UA_SCHEMA_DIR}/DI/Opc.Ua.Di.Types.bsd
#   FILE_NS ${UA_SCHEMA_DIR}/DI/Opc.Ua.Di.NodeSet2.xml
#   NAMESPACE_MAP "${UA_NAMESPACE_MAP_DI}"
#   TARGET_PREFIX "${PROJECT_NAME}"
#   INTERNAL
# )

# target_sources(
#   test_opcua_xml2server
#     PRIVATE # cmake-format: sort
#             ${PROJECT_BINARY_DIR}/src_generated/open62541/di_nodeids.h ${UA_NODESET_DI_HEADERS}
#             ${UA_NODESET_DI_SOURCES} ${UA_TYPES_DI_HEADERS} ${UA_TYPES_DI_SOURCES}
# )



# ua_generate_nodeset_and_datatypes(
#   NAME i4aas
#   FILE_CSV ${UA_SCHEMA_DIR}/I4AAS/Opc.Ua.I4AAS.NodeIds.csv
#   FILE_BSD ${UA_SCHEMA_DIR}/I4AAS/Opc.Ua.I4AAS.Types.bsd
#   FILE_NS ${UA_SCHEMA_DIR}/I4AAS/Opc.Ua.I4AAS.NodeSet2.xml
#   NAMESPACE_MAP "${UA_NAMESPACE_MAP_I4AAS}"
#   TARGET_PREFIX "${PROJECT_NAME}"
#   DEPENDS di
#   INTERNAL
# )

# target_sources(
#   test_opcua_xml2server
#    PRIVATE # cmake-format: sort
#            ${PROJECT_BINARY_DIR}/src_generated/open62541/i4aas_nodeids.h
#            ${UA_NODESET_I4AAS_HEADERS}
#            ${UA_NODESET_I4AAS_SOURCES}
#            ${UA_TYPES_I4AAS_HEADERS}
#            ${UA_TYPES_I4AAS_SOURCES}
# )


# ua_generate_nodeset_and_datatypes(
#   NAME aas
#   FILE_NS ${UA_MODEL_DIR}/example_atl_robot.xml
#   TARGET_PREFIX "${PROJECT_NAME}"
#   DEPENDS i4aas
#   INTERNAL
# )
# target_sources(
#   test_opcua_xml2server
#     PRIVATE # cmake-format: sort
#             ${UA_MODEL_DIR}/test_opcua_xml2server.cpp ${UA_NODESET_AAS_HEADERS}
#             ${UA_NODESET_AAS_SOURCES} ${UA_TYPES_AAS_HEADERS} ${UA_TYPES_AAS_SOURCES}
# )

# target_link_libraries(test_opcua_xml2server PRIVATE
#   # Other libraries
#   ${PROJECT_NAME}
#   ${OpenCV_LIBS}
# )

# include_directories(${PROJECT_BINARY_DIR}/src_generated/open62541/)

# add_dependencies(tests test_opcua_xml2server)

#-----------test_opcua_xmlnodesetloader----------#
# Add the output executable for the test
add_executable(test_opcua_xmlnodesetloader)

# Add sources specific for the test
target_sources(
  test_opcua_xmlnodesetloader
  PRIVATE
  xml2server/test_opcua_xmlnodesetloader.cpp
)

# Link against the library itself.
target_link_libraries(
  test_opcua_xmlnodesetloader
  PUBLIC
    ${PROJECT_NAME}
    ${OpenCV_LIBS}
)

# Add targets to the global "tests" target
add_dependencies(tests test_opcua_xmlnodesetloader)