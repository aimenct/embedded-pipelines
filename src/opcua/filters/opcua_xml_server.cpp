// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "opcua_xml_server.h"

// #define DEBUG
namespace ep {

OPCUAXmlserver::OPCUAXmlserver(YAML::Node &config, std::string xmlfile)
    : OPCUAserver(config),
      xmlfile_(xmlfile)
{
  addSetting("config_output", config_output_);
}

OPCUAXmlserver::~OPCUAXmlserver()
{
  for (auto &queue_node_ids : queue_node_opcua_id_) {
    for (auto &ua_node : queue_node_ids) {
      UA_NodeId_clear(&ua_node);
    }
  }

  std::cout << "OPCUAXmlserver destructor " << std::endl;
  // delete cfgFile;
}

int32_t OPCUAXmlserver::_open()
{
  server_ = UA_Server_new();
  server_config_ = UA_Server_getConfig(server_);
  UA_UInt16 ua_server_port = 0;

  if (server_port_ >= 0 &&
      server_port_ <= std::numeric_limits<UA_UInt16>::max()) {
    ua_server_port = static_cast<UA_UInt16>(server_port_);
  }
  else {
    std::cerr << "Error: server port is out of the allowable range for "
                 "UA_UInt16. Using default port"
              << std::endl;
    ua_server_port = 4840;
  }
  // UA_ServerConfig_setDefault(serverConfig);
  UA_ServerConfig_setMinimalCustomBuffer(server_config_, ua_server_port, NULL,
                                         524288000, 524288000);

  UA_StatusCode retval = UA_Server_run_startup(server_);
  if (retval != UA_STATUSCODE_GOOD) {
    UA_Server_delete(server_);
    return -1;
  }
  retval = UA_Server_loadNodeset(server_, xmlfile_.c_str(), NULL);
  return 0;
}

int32_t OPCUAXmlserver::_set()
{
  items_to_update_.resize(max_sources_);
  queue_node_opcua_id_.resize(max_sources_);

  if (config_["name"].as<std::string>() == "OPCUAXmlserver" &&
      !config_["server_nodes"]) {
    browseNodes(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), browse_nodes_);
    registerNodes2Yaml();
    std::cout
        << "Link Server Nodes with Queue items and execute again the Filter"
        << std::endl;
    return -1;
  }
  std::cout << "The number of brwsed nodes are : " << browse_nodes_.size()
            << std::endl;
  registerYaml2OpcUa();
  return 0;
}

// UA_StatusCode OPCUAXmlserver::commandMethod(
//     UA_Server *server, const UA_NodeId * /*sessionId*/,
//     void * /*sessionHandle*/, const UA_NodeId *method_id, void
//     *method_context, const UA_NodeId * /*objectId*/, void *
//     /*objectContext*/, size_t /*inputSize*/, const UA_Variant * /*input*/,
//     size_t /*outputSize*/, UA_Variant *output)
// {
//   // Obtain the node name from the NodeId
//   UA_LocalizedText node_name;
//   UA_Server_readDisplayName(server, *method_id, &node_name);
//   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Executing command:
//   %s",
//               (char *)node_name.text.data);

//   OPCUAXmlserver *server_obj = ((nodexml_context_t *)method_context)->srvPtr;
//   size_t filter_idx = ((nodexml_context_t *)method_context)->index;
//   ep::Filter *src_filter = server_obj->src_filters_[filter_idx];
//   ep::Settings *src_settings = src_filter->settings();
//   ep::Setting *node = src_settings->get((const char *)node_name.text.data);

//   int ret_code;
//   switch (node->type()) {
//     case EP_CMD:
//       ret_code = src_settings->set((const char *)node_name.text.data, NULL);
//       UA_Variant_setScalarCopy(output, (void *)&ret_code,
//                                &UA_TYPES[UA_TYPES_INT32]);

//       break;
//     default:
//       UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
//                      "Command %s fail: Command type not implemented: %d",
//                      node_name.text.data, node->type());
//       return UA_STATUSCODE_BADSERVICEUNSUPPORTED;
//   }
//   return UA_STATUSCODE_GOOD;
// }

int OPCUAXmlserver::browseNodes(UA_NodeId nodeId,
                                std::vector<OpcuaNodeToYaml> &nodes)
{
  int err = 0;
  UA_BrowseDescription browse_desc;
  UA_BrowseDescription_init(&browse_desc);
  browse_desc.nodeId = nodeId;
  browse_desc.resultMask = UA_BROWSERESULTMASK_ALL;

  UA_BrowseResult browse_res = UA_Server_browse(server_, 0, &browse_desc);

  for (size_t i = 0; i < browse_res.referencesSize; ++i) {
    UA_ReferenceDescription *ref_descr = &(browse_res.references[i]);

    // Verificar el referenceTypeId
    if (UA_NodeId_isNull(&ref_descr->referenceTypeId)) {
      std::cerr << "referenceTypeId is null for the node "
                << ref_descr->browseName.name.data << std::endl;
      continue;
    }

    const UA_NodeId *reftype = (const UA_NodeId *)&ref_descr->referenceTypeId;
    if (!reftype) {
      std::cerr << "Invalid reference type for the node "
                << ref_descr->browseName.name.data << std::endl;
      continue;
    }

    if (ref_descr->nodeId.nodeId.namespaceIndex != 0) {
      std::string name(
          reinterpret_cast<char *>(ref_descr->browseName.name.data),
          ref_descr->browseName.name.length);
      std::string node_id =
          "ns=" + std::to_string(ref_descr->nodeId.nodeId.namespaceIndex) +
          ";i=" + std::to_string(ref_descr->nodeId.nodeId.identifier.numeric);
      OpcuaNodeToYaml node{name, node_id, "", "", 0, {}};

      if (ref_descr->nodeClass == UA_NODECLASS_OBJECT ||
          ref_descr->nodeClass == UA_NODECLASS_VARIABLE) {
        UA_NodeId child_node_id =
            UA_NODEID_NUMERIC(ref_descr->nodeId.nodeId.namespaceIndex,
                              ref_descr->nodeId.nodeId.identifier.numeric);
        browseNodes(child_node_id, node.references);
      }
      nodes.push_back(node);
    }
  }
  if (err == 0) {
    UA_BrowseResult_clear(&browse_res);
    return 0;
  }
  else {
    std::cout << "Filter::set error -- unable to browse server " << std::endl;
    return -1;
  }
}

YAML::Node OPCUAXmlserver::convertNodesToYaml(
    const std::vector<OpcuaNodeToYaml> &nodes)
{
  YAML::Node yaml_nodes = YAML::Node(YAML::NodeType::Sequence);
  for (const auto &node : nodes) {
    YAML::Node yaml_node;
    yaml_node["name"] = node.name;
    yaml_node["node_id"] = node.node_id;
    yaml_node["src_port"] = node.src_port;
    yaml_node["msg_item"] = node.msg_item;
    yaml_node["edge_to_opcua"] = 0;
    yaml_node["references"] = convertNodesToYaml(node.references);
    yaml_nodes.push_back(yaml_node);
  }
  return yaml_nodes;
}

int OPCUAXmlserver::registerNodes2Yaml()
{
  if (!config_ || (config_)["name"].as<std::string>() != "OPCUAXmlserver") {
    std::cerr << "OPCUAXmlserver not found in configuration" << std::endl;

    return -1;
  }

  YAML::Node node_opc_settings = config_["server_nodes"];
  node_opc_settings = convertNodesToYaml(browse_nodes_);

  if (!config_output_.empty()) {
    std::ofstream fout(config_output_);
    fout << config_;
    fout.close();
  }

  return -1;
}

bool isNumber(const std::string &str)
{
  for (char const &c : str) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

int OPCUAXmlserver::processYamlNode(const YAML::Node &reference)
{
  for (const auto &node : reference) {
    int edge_to_opcua = node["edge_to_opcua"].as<int>();
    if (node["msg_item"].as<std::string>() != "" &&
        node["src_port"].as<std::string>() != "") {
      UA_ValueCallback callback;


      UA_NodeId ua_node;
      UA_String ua_node_name =
          UA_String_fromChars(node["node_id"].as<std::string>().c_str());
      UA_NodeId_parse(&ua_node, ua_node_name);
      UA_String_clear(&ua_node_name);

      if (edge_to_opcua == 2) {
        if (!isNumber(node["msg_item"].as<std::string>())) {
          std::cout
              << "Process Error: Queue Node must have a numeric in message item"
              << std::endl;
          return -1;
        }
        QueueNodeContext *context = new QueueNodeContext;
        context->srv_ptr = this;
        context->src_port = node["src_port"].as<int32_t>();
        context->item_index = node["msg_item"].as<int32_t>();;

        callback.onRead = beforeReadQueue;
        callback.onWrite = afterWriteQueue;

        queue_node_opcua_id_[context->src_port].push_back(ua_node);
        UA_StatusCode status =
            UA_Server_setNodeContext(server_, ua_node, context);

        if (status != UA_STATUSCODE_GOOD) {
          std::cout << "Error in seting context for node "
                    << node["name"].as<std::string>() << std::endl;
          return -1;
        }

        variable_node_contexts_.push_back(context);
      }
      else if (edge_to_opcua == 1) {
        if (isNumber(node["src_port"].as<std::string>())) {
          std::cout << "Process Error: Setting Node must have a string in "
                       "message item"
                    << std::endl;
          return -1;
        }
        SettingsNodeContext *context = new SettingsNodeContext;
        context->node_name = node["msg_item"].as<std::string>();
        int filter_index = node["src_port"].as<int32_t>();
	//        context->filter = src_filters_at_ports_[filter_index];
	context->filter = source_ports_[filter_index].src_filter_;

        callback.onRead = beforeReadSetting;
        callback.onWrite = writeSetting;

        // Ensure the node has write permissions
        UA_Byte access_level;
        UA_Byte_init(&access_level);
        UA_Byte new_access_level =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_StatusCode retval =
            UA_Server_readAccessLevel(server_, ua_node, &access_level);
        if (retval == UA_STATUSCODE_GOOD) {
          if (access_level != new_access_level) {
            UA_Server_writeAccessLevel(server_, ua_node, new_access_level);
          }
        }

        UA_StatusCode status =
            UA_Server_setNodeContext(server_, ua_node, context);

        if (status != UA_STATUSCODE_GOOD) {
          std::cout << "Error in seting context for node "
                    << node["name"].as<std::string>() << std::endl;
          return -1;
        }

        filter_setting_contexts_.push_back(context);
      }

      UA_Server_setVariableNode_valueCallback(server_, ua_node, callback);
    }
    processYamlNode(node["references"]);
  }
  return 0;
}

int OPCUAXmlserver::registerYaml2OpcUa()
{
  processYamlNode(config_["server_nodes"]);
  return 0;
}

void OPCUAXmlserver::beforeReadSetting(UA_Server * /*server*/,
                                       const UA_NodeId * /*sessionId*/,
                                       void * /*sessionContext*/,
                                       const UA_NodeId * /*node_id*/,
                                       void *node_context,
                                       const UA_NumericRange * /*range*/,
                                       const UA_DataValue *data_value)
{
  // Obtain the server object reference and the filter pointer
  // srcFilters
  ep::Filter *src_filter =
      (static_cast<SettingsNodeContext *>(node_context))->filter;
  std::string setting_name =
      (static_cast<SettingsNodeContext *>(node_context))->node_name;

  void *data;
  Node2 *setting_node = (Node2 *)src_filter->settingNode(setting_name);

  if (setting_node != NULL && setting_node->isDataNode()) {
    DataNode *setting_datanode = (DataNode *)setting_node;
    data = setting_datanode->value();
    // if (session_instance->dataTypePtr(setting_datanode->datatype()) ==
    //     data_value->value.type) {
    UA_StatusCode retval = UA_Variant_setScalarCopy(
        const_cast<UA_Variant *>(
            &data_value->value),  // Remove constness to match the function call
        data, basetype_to_opcuatype(setting_datanode->datatype()));

    if (retval != UA_STATUSCODE_GOOD) {
      UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Error reading setting node");
    }
  }
  // }

  const_cast<UA_DataValue *>(data_value)->hasValue = true;
}

void OPCUAXmlserver::writeSetting(UA_Server * /*server*/,
                                  const UA_NodeId * /*sessionId*/,
                                  void * /*sessionContext*/,
                                  const UA_NodeId * /*node_id*/,
                                  void *node_context,
                                  const UA_NumericRange * /*range*/,
                                  const UA_DataValue *data)
{
  // Obtain the server object reference and the filter pointer
  // srcFilters
  SettingsNodeContext *context =
      static_cast<SettingsNodeContext *>(node_context);

  // Check the edge type and write the new value to the node setting in the
  // filter
  BaseType setting_type = opcuatype_to_basetype(data->value.type);
  int ret = -1;
  switch (setting_type) {
    case EP_8U:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(uint8_t *)data->value.data);
      break;
    case EP_8S:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(int8_t *)data->value.data);
      break;
    case EP_16U:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(uint16_t *)data->value.data);

      break;
    case EP_16S:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(int16_t *)data->value.data);

      break;
    case EP_32U:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(uint32_t *)data->value.data);

      break;
    case EP_32S:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(int32_t *)data->value.data);

      break;
    case EP_64U:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(uint64_t *)data->value.data);

      break;
    case EP_64S:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(int64_t *)data->value.data);

      break;
    case EP_32F:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(float *)data->value.data);

      break;
    case EP_64F:
      ret = context->filter->setSettingValue(context->node_name,
                                             *(double *)data->value.data);

      break;
    case EP_8C:

      break;
    // case EP_CHAR256:
    //   // stringOut = std::string((char*)ptrToData);
    //   ss << std::string((char *)ptr_to_data);
    //   break;
    default:
      break;
  }

  if (ret == 0) {
  }
  else {
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "Error writing node %s", context->node_name.c_str());
  }
}

}  // namespace ep
