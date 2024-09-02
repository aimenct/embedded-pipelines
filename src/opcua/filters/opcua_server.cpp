// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "opcua_server.h"

namespace ep {
OPCUAserver::OPCUAserver(YAML::Node &config)
    : Filter(),
      config_(config)

{
  std::cout << "OPCUAserver constructor" << std::endl;

  if ((config)["name"]) {
    name_ = (config)["name"].as<std::string>();
  }

  enable_images_ = true;
  server_port_ = 4840;
  max_sources_ = 2;

  addSetting("enable images", enable_images_);
  addSetting("server_port", server_port_);

  readSettings(config_);
}

OPCUAserver::~OPCUAserver()
{
  std::cout << "OPCUAserver destructor " << std::endl;
  // delete cfgFile;
}

int32_t OPCUAserver::_open()
{
  server_ = UA_Server_new();
  server_config_ = UA_Server_getConfig(server_);

  // UA_ServerConfig_setDefault(serverConfig);
  UA_ServerConfig_setMinimalCustomBuffer(server_config_, server_port_, NULL,
                                         524288000, 524288000);

  UA_StatusCode retval = UA_Server_run_startup(server_);
  if (retval != UA_STATUSCODE_GOOD) {
    UA_Server_delete(server_);
    return -1;
  }
  return 0;
}

int32_t OPCUAserver::_set()
{
  // Resize the queue vectors to match the OPCUA filter ports in use
  items_to_update_.resize(max_sources_);
  queue_node_opcua_id_.resize(max_sources_);

  registerPipeline();

  return 0;
}

int32_t OPCUAserver::_reset()
{
  return 0;
}

int32_t OPCUAserver::_close()
{
  // The epilogue part of UA_Server_run
  UA_StatusCode retval = UA_Server_run_shutdown(server_);

  // UA_ServerConfig_clean(server_config);
  UA_Server_delete(server_);

  if (retval != UA_STATUSCODE_GOOD) {
    std::cout
        << "Something went wrong with UA_Server_run_shutdown(), exiting..."
        << std::endl;
  }

  unregisterQueues();
  unregisterFilters();

  return 0;
}

int32_t OPCUAserver::_start()
{
  return 0;
}

int32_t OPCUAserver::_stop()
{
  return 0;
}

int32_t OPCUAserver::_job()
{
  // The prologue part of UA_Server_run

  // Executes a single iteration of the server’s main loop
  UA_UInt16 max_timeout = UA_Server_run_iterate(server_, UA_FALSE);

  // Update the server nodes with the data from the queues
  //  for (size_t port = 0; port < readers_.size(); port++) {
  for (size_t port = 0; port < source_ports_.size(); port++) {
    if (!items_to_update_[port].empty()) {
      //      int32_t err = readers_[port]->startRead();
      int32_t err = reader(static_cast<int32_t>(port))->startRead();
      if (err == 0) {
        updateServerFromQueue(static_cast<int32_t>(port),
                              items_to_update_[port]);
        //        readers_[port]->endRead();
        reader(static_cast<int32_t>(port))->endRead();
      }
    }
  }
  // sleep the maximum time possible for the server to work properly
  usleep(max_timeout * 10);
  return 0;
}

int OPCUAserver::registerPipeline()
{
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
              "Registering Filters to OPCUA");

  UA_NodeId pipeline_id;
  std::string folder_name = "Embedded Pipelines Pipeline";
  createServerFolder(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), folder_name,
                     &pipeline_id);

  // for (size_t i = 0; i < src_filters_at_ports_.size(); i++) {
  //   if (src_filters_at_ports_[i]) {
  for (size_t i = 0; i < source_ports_.size(); i++) {
    if (source_ports_[i].src_filter_) {
      UA_NodeId filters_id;
      // createServerFolder(pipeline_id, src_filters_at_ports_.at(i)->name(),
      //                    &filters_id);
      createServerFolder(pipeline_id, source_ports_[i].src_filter_->name(),
                         &filters_id);

      UA_NodeId folder_settings_id;
      folder_name = "Settings";
      createServerFolder(filters_id, folder_name, &folder_settings_id);

      UA_NodeId folder_queues_id;
      folder_name = "Queues";
      createServerFolder(filters_id, folder_name, &folder_queues_id);

      registerFilterSettings(folder_settings_id, i);
      registerQueues(folder_queues_id, i);

      UA_NodeId_clear(&filters_id);
      UA_NodeId_clear(&folder_settings_id);
      UA_NodeId_clear(&folder_queues_id);
    }
  }
  return 0;
}

int32_t OPCUAserver::createServerFolder(const UA_NodeId &parent,
                                        const std::string &name,
                                        UA_NodeId *out_node)
{
  UA_ObjectAttributes attr = UA_ObjectAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(), name.c_str());

  UA_StatusCode ret = UA_Server_addObjectNode(
      server_, UA_NODEID_NUMERIC(1, 0), parent,
      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
      UA_QUALIFIEDNAME(1, (char *)name.c_str()),
      UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, NULL, out_node);

  UA_ObjectAttributes_clear(&attr);

  if (ret == UA_STATUSCODE_GOOD) {
    return 0;
  }
  else {
    std::cout << "Failed to add folder: \"" << name
              << "\" with code: " << UA_StatusCode_name(ret) << std::endl;
    return -1;
  }
}

int32_t OPCUAserver::registerFilterSettings(const UA_NodeId &filter_node_id,
                                            size_t filter_idx)
{
  // It is necessary to copy the filter settings to include them in the OPCUA
  // server
  // const Settings2 *filter_settings =
  //     src_filters_at_ports_[filter_idx]->settings();
  const Settings2 *filter_settings =
      source_ports_[filter_idx].src_filter_->settings();

  UA_NodeId filter_settings_node_id;
  std::string folder_name = "Filter Settings";
  createServerFolder(filter_node_id, folder_name, &filter_settings_node_id);

  UA_NodeId device_settings_node_id;
  folder_name = "Device Settings";
  createServerFolder(filter_node_id, folder_name, &device_settings_node_id);

  UA_NodeId commands_settings_node_id;
  folder_name = "Commands";
  createServerFolder(filter_node_id, folder_name, &commands_settings_node_id);

  UA_NodeId queue_settings_node_id;
  folder_name = "Queue Settings";
  createServerFolder(filter_node_id, folder_name, &queue_settings_node_id);

  for (auto &ref : (*filter_settings)["filter_settings"]->references()) {
    ep::Node2 *node = ref.address();
    int err = addSettingNode(node, filter_settings_node_id, filter_idx);
    if (err < 0) {
      std::cout << "Error adding setting node : " << node->name() << std::endl;
    }
  }

  for (auto &ref : (*filter_settings)["device_settings"]->references()) {
    ep::Node2 *node = ref.address();
    int err = addSettingNode(node, device_settings_node_id, filter_idx);
    if (err < 0) {
      std::cout << "Error adding setting node : " << node->name() << std::endl;
    }
  }

  for (auto &ref : (*filter_settings)["filter_commands"]->references()) {
    ep::Node2 *node = ref.address();
    int err = addCommandNode(node, commands_settings_node_id, filter_idx);
    if (err < 0) {
      std::cout << "Error adding setting node : " << node->name() << std::endl;
    }
  }

  for (auto &ref : (*filter_settings)["queue_settings"]->references()) {
    ep::Node2 *node = ref.address();
    int err = addSettingNode(node, queue_settings_node_id, filter_idx);
    if (err < 0) {
      std::cout << "Error adding setting node : " << node->name() << std::endl;
    }
  }

  return 0;
}

int32_t OPCUAserver::addSettingNode(ep::Node2 *node, UA_NodeId parent_id,
                                    size_t filter_idx)
{
  if (node->isDataNode()) {
    UA_NodeId var_id;

    UA_VariableAttributes var_attr = UA_VariableAttributes_default;

    var_attr.displayName =
        UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(), node->name().c_str());
    var_attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    var_attr.description =
        UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(), node->tooltip().c_str());

    // Add data sources callbacks to read/write edge settings nodes via OPCUA
    // nodes
    UA_DataSource data_source;
    data_source.read = beforeReadSetting;
    data_source.write = afterWriteSetting;

    SettingsNodeContext *context = new SettingsNodeContext;
    //    context->filter = src_filters_at_ports_[filter_idx];
    context->filter = source_ports_[filter_idx].src_filter_;
    context->node_name = node->name();
    filter_setting_contexts_.push_back(context);

    // Add edge setting nodes as datas sources for OPCUA nodes
    UA_StatusCode retval = UA_Server_addDataSourceVariableNode(
        server_, UA_NODEID_NUMERIC(1, 0), parent_id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, (char *)node->name().c_str()),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), var_attr,
        data_source, context, &var_id);

    UA_VariableAttributes_clear(&var_attr);

    if (retval != UA_STATUSCODE_GOOD) {
      return -1;
    }
    else {
      return 0;
    }
  }
  else {
    return -1;
  }
}

int32_t OPCUAserver::addCommandNode(ep::Node2 *node, UA_NodeId parent_id,
                                    size_t filter_idx)
{
  if (node->nodetype() == ep::EP_COMMANDNODE) {
    UA_NodeId command_id;

    UA_MethodAttributes method_attr = UA_MethodAttributes_default;
    method_attr.displayName =
        UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(), node->name().c_str());
    method_attr.description =
        UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(), node->tooltip().c_str());

    SettingsNodeContext *context = new SettingsNodeContext;
    //    context->filter = src_filters_at_ports_[filter_idx];
    context->filter = source_ports_[filter_idx].src_filter_;
    context->node_name = node->name();
    filter_setting_contexts_.push_back(context);

    // Add edge setting nodes as datas sources for OPCUA nodes
    UA_StatusCode retval = UA_Server_addMethodNode(
        server_, UA_NODEID_NUMERIC(1, 0), parent_id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, (char *)node->name().c_str()), method_attr,
        commandMethod, 0, NULL, 0, NULL, context, &command_id);

    UA_MethodAttributes_clear(&method_attr);

    std::cout << UA_StatusCode_name(retval) << std::endl;

    if (retval != UA_STATUSCODE_GOOD) {
      return -1;
    }
    else {
      return 0;
    }
  }
  else {
    return -1;
  }
}

int32_t OPCUAserver::registerQueues(const UA_NodeId &filter_queues, size_t port)
{
  //  if (port < src_filters_at_ports_.size()) {
  if ((port < source_ports_.size()) && (source_ports_[port].src_filter_)) {
    // Filter *port_filter = src_filters_at_ports_[port];
    Filter *port_filter = source_ports_[port].src_filter_;
    std::cout << "Port filter is " << port_filter->name() << std::endl;
    std::string queue_name = "Queue at port " + std::to_string(port);

    UA_NodeId queue_node_id;
    createServerFolder(filter_queues, queue_name, &queue_node_id);
    addQueueItems(port, queue_node_id);
    //    // readers_[port]->setBlockingCalls(false);
    // reader(port)->setBlockingCalls(false);
  }

  return 0;
}

int32_t OPCUAserver::unregisterQueues()
{
  for (auto &src_queue : queue_node_opcua_id_) {
    for (UA_NodeId &node_id : src_queue) {
      UA_NodeId_clear(&node_id);
    }
    src_queue.clear();
  }

  return 0;
}

void OPCUAserver::addQueueItems(size_t port, UA_NodeId &queue_node_id)
{
  Message *msg = this->reader(static_cast<int>(port))->dataSchema();

  for (size_t item_index = 0; item_index != msg->itemCount(); item_index++) {
    Node2 *node = msg->item(item_index);
    if (node->isObjectNode()) {
      ObjectNode *object_node = (ObjectNode *)node;
      if (object_node->objecttype() == EP_IMAGE_OBJ) {
        // ImageObject img_node(object_node);
      }
    }
    else if (node->isDataNode()) {
      DataNode *data_node = static_cast<DataNode *>(node);

      const UA_DataType *ua_datatype =
          basetype_to_opcuatype(data_node->datatype());

      // Verify all the parameters of the edge node and convert them into an
      // OPCUA variant
      if (ua_datatype) {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.dataType = ua_datatype->typeId;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ;
        attr.displayName = UA_LOCALIZEDTEXT_ALLOC(lang_code_.c_str(),
                                                  data_node->name().c_str());

        if (data_node->arraydimensions()[0] == 1) {
          attr.valueRank = UA_VALUERANK_SCALAR;
        }
        else if (data_node->rank() <= 3) {
          attr.valueRank = data_node->rank();
        }
        else {
          attr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
        }

        if (attr.valueRank != UA_VALUERANK_SCALAR) {
          attr.arrayDimensionsSize = data_node->arraydimensions().size();
          if (attr.arrayDimensionsSize > 0) {
            // UA_VariableAttributes acquires ownership of the dynamic
            // allocated pointer
            attr.arrayDimensions = (UA_UInt32 *)UA_Array_new(
                attr.arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
            for (size_t i = 0; i < data_node->arraydimensions().size(); i++) {
              attr.arrayDimensions[i] =
                  static_cast<UA_UInt32>(data_node->arraydimensions()[i]);
            }
          }
        }

        QueueNodeContext *context = new QueueNodeContext;
        context->item_index = static_cast<int32_t>(item_index);
        context->src_port = static_cast<int32_t>(port);
        context->srv_ptr = this;
        context->node_name = data_node->name();
        variable_node_contexts_.push_back(context);

        UA_NodeId item_node_id;

        UA_Server_addVariableNode(
            server_, UA_NODEID_NUMERIC(1, 0), queue_node_id,
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)data_node->name().c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, context,
            &item_node_id);

        // Add UA_NodeId to the vector containing each item correspondence
        queue_node_opcua_id_[port].push_back(item_node_id);

        // Callback definition to update the variable values before reading
        UA_ValueCallback callback;
        callback.onRead = beforeReadQueue;
        callback.onWrite = afterWriteQueue;
        UA_Server_setVariableNode_valueCallback(server_, item_node_id,
                                                callback);

        // Release dynamically allocated memory in UA_VariableAtributes
        UA_VariableAttributes_clear(&attr);
      }
      else {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Registering variable: %s \n Type not implemented: %s",
                       data_node->name().c_str(),
                       basetype_to_string(data_node->datatype()).c_str());
      }
    }
  }
}

int32_t OPCUAserver::unregisterFilters()
{
  for (auto settings_context : filter_setting_contexts_) {
    delete settings_context;
  }
  for (auto context : variable_node_contexts_) {
    delete context;
  }
  variable_node_contexts_.clear();
  filter_setting_contexts_.clear();

  return 0;
}

UA_StatusCode OPCUAserver::commandMethod(
    UA_Server * /*server*/, const UA_NodeId * /*sessionId*/,
    void * /*sessionHandle*/, const UA_NodeId * /*method_id*/,
    void *method_context, const UA_NodeId * /*objectId*/,
    void * /*objectContext*/, size_t /*inputSize*/,
    const UA_Variant * /*input*/, size_t /*outputSize*/,
    UA_Variant * /*output*/)
{
  // Obtain the node name from the NodeId
  SettingsNodeContext *context =
      static_cast<SettingsNodeContext *>(method_context);
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Executing command: %s",
              context->node_name.c_str());

  int32_t ret = context->filter->runCommand(context->node_name);

  if (ret < 0) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Command: %s failed.",
                context->node_name.c_str());
    return UA_STATUSCODE_BADUNEXPECTEDERROR;
  }
  return UA_STATUSCODE_GOOD;
}

UA_StatusCode OPCUAserver::beforeReadSetting(
    UA_Server * /*server*/, const UA_NodeId * /*sessionId*/,
    void * /*sessionContext*/, const UA_NodeId * /*node_id*/,
    void *node_context, UA_Boolean /*sourceTimeStamp*/,
    const UA_NumericRange * /*range*/, UA_DataValue *data_value)
{
  SettingsNodeContext *context =
      static_cast<SettingsNodeContext *>(node_context);

  // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
  //             "Received READ request for Node: %s",
  //             context->node_name.c_str());

  const Node2 *setting_node = context->filter->settingNode(context->node_name);

  if (setting_node != NULL && setting_node->isDataNode()) {
    const DataNode *setting_datanode =
        static_cast<const DataNode *>(setting_node);

    UA_StatusCode retval;

    if (setting_datanode->datatype() != EP_STRING) {
      retval = UA_Variant_setScalarCopy(
          &data_value->value, setting_datanode->value(),
          basetype_to_opcuatype(setting_datanode->datatype()));
    }
    else {
      UA_String ua_string = UA_String_fromChars(
          static_cast<std::string *>(setting_datanode->value())->c_str());
      retval = UA_Variant_setScalarCopy(
          &data_value->value, &ua_string,
          basetype_to_opcuatype(setting_datanode->datatype()));
      UA_String_clear(&ua_string);
    }

    if (retval != UA_STATUSCODE_GOOD) {
      UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Error reading setting node %s",
                     context->node_name.c_str());
      return retval;
    }
  }
  data_value->hasValue = true;

  return UA_STATUSCODE_GOOD;
}

UA_StatusCode OPCUAserver::afterWriteSetting(UA_Server * /*server*/,
                                             const UA_NodeId * /*sessionId*/,
                                             void * /*sessionContext*/,
                                             const UA_NodeId * /*node_id*/,
                                             void *node_context,
                                             const UA_NumericRange * /*range*/,
                                             const UA_DataValue *data)
{
  SettingsNodeContext *context =
      static_cast<SettingsNodeContext *>(node_context);

  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
              "Received WRITE request for Node: %s",
              context->node_name.c_str());

  // Check the edge type and write the new value to the node setting in the
  // filter
  BaseType setting_type = opcuatype_to_basetype(data->value.type);
  int ret = -1;
  if (setting_type == EP_8U) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(uint8_t *)data->value.data);
  }
  else if (setting_type == EP_8S) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(int8_t *)data->value.data);
  }
  else if (setting_type == EP_16U) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(uint16_t *)data->value.data);
  }
  else if (setting_type == EP_16S) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(int16_t *)data->value.data);
  }
  else if (setting_type == EP_32U) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(uint32_t *)data->value.data);
  }
  else if (setting_type == EP_32S) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(int32_t *)data->value.data);
  }
  else if (setting_type == EP_64U) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(uint64_t *)data->value.data);
  }
  else if (setting_type == EP_64S) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(int64_t *)data->value.data);
  }
  else if (setting_type == EP_32F) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(float *)data->value.data);
  }
  else if (setting_type == EP_64F) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(double *)data->value.data);
  }
  else if (setting_type == EP_8C) {
    ret = context->filter->setSettingValue(context->node_name,
                                           *(char *)data->value.data);
  }
  else if (setting_type == EP_STRING) {
    UA_String *ua_string = (UA_String *)data->value.data;
    std::string std_string((char *)ua_string->data, ua_string->length);
    ret = context->filter->setSettingValue(context->node_name, std_string);
  }

  if (ret == 0) {
    return UA_STATUSCODE_GOOD;
  }
  else {
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "Error writing node %s", context->node_name.c_str());
    return UA_STATUSCODE_BADINTERNALERROR;
  }
}

void OPCUAserver::beforeReadQueue(UA_Server * /*server*/,
                                  const UA_NodeId * /*sessionId*/,
                                  void * /*sessionContext*/,
                                  const UA_NodeId * /*node_id*/,
                                  void *node_context,
                                  const UA_NumericRange *
                                  /*range*/,
                                  const UA_DataValue * /*data*/)
{
  QueueNodeContext *context = (QueueNodeContext *)node_context;

  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
              "Received READ request for Node: %s", context->node_name.c_str());

  // Get the server reference and the queueInfo position in the array
  // from the nodeContext
  if (node_context != nullptr) {
    int32_t queue_port = context->src_port;
    int32_t item_index = context->item_index;

    // Add node to the list of nodes to be updated
    context->srv_ptr->items_to_update_[queue_port].push_back(item_index);
    std::cout << context->srv_ptr->items_to_update_[queue_port].size()
              << std::endl;
  }
}

void OPCUAserver::afterWriteQueue(UA_Server * /*server*/,
                                  const UA_NodeId * /*sessionId*/,
                                  void * /*sessionContext*/,
                                  const UA_NodeId * /*nodeId*/,
                                  void * /*nodeContext*/,
                                  const UA_NumericRange * /*range*/,
                                  const UA_DataValue * /*data*/)
{
  // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
  //             "The variable was updated");
}

void OPCUAserver::updateServerFromQueue(int32_t filter_port,
                                        std::vector<int32_t> &updating_nodes)
{
  for (int32_t item_index : updating_nodes) {
    UA_Variant variant_value;
    UA_Variant_init(&variant_value);
    //    Node2 *node = this->reader(filter_port)->dataMsg()->item(item_index);
    Message msg = this->reader(filter_port)->dataMsg();
    Node2 *node = msg.item(item_index);

    if (node->isDataNode()) {
      DataNode *data_node = (DataNode *)node;
      const UA_DataType *opcua_type_ptr =
          basetype_to_opcuatype(data_node->datatype());
      if (data_node->arraydimensions()[0] > 1) {
        size_t value_count = 1;
        for (size_t dim : data_node->arraydimensions()) {
          value_count *= dim;
        }
        UA_Variant_setArray(&variant_value, data_node->value(), value_count,
                            opcua_type_ptr);
        variant_value.arrayDimensionsSize = data_node->arraydimensions().size();

        variant_value.arrayDimensions =
            new UA_UInt32[variant_value.arrayDimensionsSize];
        for (size_t i = 0; i < data_node->arraydimensions().size(); i++) {
          variant_value.arrayDimensions[i] =
              static_cast<UA_UInt32>(data_node->arraydimensions()[i]);
        }
      }
      else {
        UA_Variant_setScalar(&variant_value, data_node->value(),
                             opcua_type_ptr);
      }
      UA_StatusCode retval = UA_Server_writeValue(
          server_, queue_node_opcua_id_[filter_port][item_index],
          variant_value);

      if (variant_value.arrayDimensionsSize > 0) {
        delete[] variant_value.arrayDimensions;
      }

      if (retval != UA_STATUSCODE_GOOD) {
        std::cout << "Error updating " << data_node->name() << "Node"
                  << ". ErrorCode: " << UA_StatusCode_name(retval) << std::endl;
      }
    }
    else if (node->isObjectNode()) {
      // ObjectNode *object_node = (ObjectNode *)update_node.node;
      // if (object_node->objecttype() == EP_IMAGE_OBJ) {
      //   UA_ByteString jpg_image;
      //   jpg_image.length = update_node.image_data->size();
      //   jpg_image.data = (UA_Byte *)update_node.image_data->data();
      //   UA_Variant_setScalarCopy(&variant_value, &jpg_image,
      //                            &UA_TYPES[UA_TYPES_BYTESTRING]);
      //   UA_StatusCode retval =
      //       UA_Server_writeValue(server_, update_node.node_id,
      //       variant_value);
      //   if (retval != UA_STATUSCODE_GOOD) {
      //     std::cout << "Error updating " << object_node->name() << "Node"
      //               << std::endl;
      //   }
      // }
    }
    // UA_Variant_clear(&variant_value);
  }
  updating_nodes.clear();
  std::cout << "updating_nodes.clear()" << std::endl;
}

}  // namespace ep
