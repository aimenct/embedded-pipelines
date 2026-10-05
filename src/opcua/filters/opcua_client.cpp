// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/* Filter constructor - create settings */
#include "opcua_client.h"

using namespace epf;

OPCUAClient::OPCUAClient(const YAML::Node &config)
    : Filter(config, 10, 10)
{
  addSetting("server_url", server_url_);
  addSetting("sampling_interval", sampling_interval_);
  addSetting("reverse_connection", reverse_connection_);
  addSetting("reverse_connect_port", reverse_connect_port_);
}

/* Filter destructor*/
OPCUAClient::~OPCUAClient()
{
  std::cout << "OPCUAClientPlus destructor " << std::endl;
}

int32_t OPCUAClient::connect()
{
  using namespace epf;

  if (!connection_initiatied_) {
    UA_StatusCode retval = UA_Client_connectAsync(client_, server_url_.c_str());
    if (retval != UA_STATUSCODE_GOOD) {
      printf("ERROR: OPC-UA client could not connect\n");
      UA_Client_delete(client_);
      return -1;
    }
    return 0;
  }
  std::cerr << name() << "::connect() connection already initiated."
            << std::endl;
  return -1;
}

bool OPCUAClient::isConnected()
{
  return connected_;
}

int32_t OPCUAClient::_open()
{
  client_ = UA_Client_new();
  UA_ClientConfig *config = UA_Client_getConfig(client_);
  UA_ClientConfig_setDefault(config);
  config->stateCallback = onConnect;

  context_ = std::make_unique<ClientContext>();
  context_->client_filter_ = this;
  config->clientContext = context_.get();

  int32_t ret;
  if (reverse_connection_) {
    ret = reverseConnect();
  }
  else {
    ret = connect();
  }

  if (ret == 0) {
    // Launch secondary thread to manage server communications
    conn_mngmnt_running_ = true;
    // this uses the move assignement, it does not create a new thread.
    conn_mngmnt_thread_ = std::thread(&OPCUAClient::connectionManagment, this);
    return 0;
  }
  else {
    return -1;
  }

  /******************* EDIT END  ***********************************/
}

int32_t OPCUAClient::_close()
{
  conn_mngmnt_running_ = false;
  conn_mngmnt_thread_.join();
  filter_started_ = false;

  UA_Client_run_iterate(client_, 250);

  UA_Client_disconnect(client_);
  UA_Client_delete(client_);

  return 0;
}

int32_t OPCUAClient::_set()
{
  if (connected_) {
    prepareReadRequest();
    prepareWriteRequest();
    return 0;
  }
  else {
    return -1;
  }
}

int32_t OPCUAClient::_reset()
{
  /* free sink queues */
  UA_ReadRequest_clear(&read_request_);
  UA_WriteRequest_clear(&write_request_);

  printf("OPCUAclient reset\n");
  return 0;
}

int32_t OPCUAClient::_start()
{
  for (auto &src_port : source_ports_) {
    if (src_port.isConnected()) {
      src_port.reader()->setBlockingCalls(false);
    }
  }
  for (auto &sink_port : sink_ports_) {
    if (sink_port.isActivated()) {
      sink_port.writer()->setBlockingCalls(false);
    }
  }

  std::cout << "_start()" << std::endl;
  return 0;
}

int32_t OPCUAClient::_stop()
{
  return 0;
}

/* Thread function implementation */
int32_t OPCUAClient::_job()
{
  using namespace epf;

  if (connected_) {
    UA_Client_run_iterate(client_, 250);
    if (connectedSources().size() > 0) {
      clientWrite();
    }
    if (activatedSinks().size() > 0) {
      clientRead();
    }
  }

  std::this_thread::sleep_for(std::chrono::duration<float>(sampling_interval_));

  return 0;
}

int32_t OPCUAClient::prepareReadRequest()
{
  YAML::Node yaml_sink_items = yaml_config_["pull_items"];
  if (yaml_sink_items.Type() != YAML::NodeType::Sequence) {
    std::cerr
        << name()
        << "::prepareReadRequest() error in YAML: wrong type on YAML::Node ->"
        << yaml_sink_items.Type() << std::endl;
    return -1;
  }

  std::unique_ptr<Message> message = std::make_unique<Message>();
  for (size_t i = 0; i < yaml_sink_items.size(); i++) {
    // Parse node id from code "ns=#;i=#" or "ns=#;s=aaa"
    std::string ua_node_std_string =
        yaml_sink_items[i]["ua_node_id"].as<std::string>();

    std::cout << ua_node_std_string << std::endl;

    UA_String ua_node_string = UA_String_fromChars(ua_node_std_string.c_str());
    UA_NodeId ua_node_id;
    UA_NodeId_init(&ua_node_id);
    UA_NodeId_parse(&ua_node_id, ua_node_string);
    UA_String_clear(&ua_node_string);

    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_StatusCode ret_value =
        UA_Client_readValueAttribute(client_, ua_node_id, &variant);
    // std::cout << "RET: " << UA_StatusCode_name(ret_value) << std::endl;
    UA_LocalizedText display_name;
    UA_StatusCode ret_dname =
        UA_Client_readDisplayNameAttribute(client_, ua_node_id, &display_name);

    if (ret_value == UA_STATUSCODE_GOOD && ret_dname == UA_STATUSCODE_GOOD) {
      std::unique_ptr<DataNode> data_node =
          convertUAVariable2Node(&display_name, &variant);

      if (data_node) {
        message->addItem(std::move(data_node));
        compatible_pull_items_.push_back(
            ItemOpcua(&ua_node_id, nullptr, 0,
                      static_cast<int32_t>(message->itemCount()) - 1));
      }
    }
    else {
      std::cout << "ERROR: " << name() << " _set() failed in node ->"
                << ua_node_std_string << std::endl;
      std::cout << "UA_Client_readValueAttribute -> "
                << UA_StatusCode_name(ret_value)
                << " UA_Client_readDisplayNameAttribute -> "
                << UA_StatusCode_name(ret_value) << std::endl;
      return -1;
    }

    UA_Variant_clear(&variant);
    UA_NodeId_clear(&ua_node_id);
  }

  UA_ReadRequest_init(&read_request_);
  if (message->itemCount() > 0) {
    size_t n_items_to_read = compatible_pull_items_.size();
    read_request_.nodesToRead = (UA_ReadValueId *)UA_Array_new(
        n_items_to_read, &UA_TYPES[UA_TYPES_READVALUEID]);
    read_request_.nodesToReadSize = n_items_to_read;

    for (size_t i = 0; i < n_items_to_read; i++) {
      UA_NodeId_init(&read_request_.nodesToRead[i].nodeId);
      UA_NodeId_copy(&compatible_pull_items_[i].ua_node_id_,
                     &read_request_.nodesToRead[i].nodeId);
      read_request_.nodesToRead[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }
    //    addSinkQueue(0, std::move(message));
    sinkPort(0)->activate(std::move(message), nullptr);
  }

  return 0;
}

int32_t OPCUAClient::prepareWriteRequest()
{
  YAML::Node yaml_push_items = yaml_config_["push_items"];
  if (yaml_push_items.Type() != YAML::NodeType::Sequence) {
    std::cerr
        << name()
        << "::prepareWriteRequest() error in YAML: wrong type on YAML::Node ->"
        << yaml_push_items.Type() << std::endl;
    return -1;
  }

  for (size_t i = 0; i < yaml_push_items.size(); i++) {
    // Parse node id from code "ns=#;i=#" or "ns=#;s=aaa"
    std::string ua_node_std_string =
        yaml_push_items[i]["ua_node_id"].as<std::string>();
    UA_NodeId ua_node_id;
    UA_String ua_node_string = UA_String_fromChars(ua_node_std_string.c_str());
    UA_NodeId_parse(&ua_node_id, ua_node_string);
    UA_String_clear(&ua_node_string);

    // Retrieve source queue and item indexes from YAML file
    int32_t src_queue_index =
        yaml_push_items[i]["src_queue_index"].as<int32_t>();
    int32_t item_index = yaml_push_items[i]["item_index"].as<int32_t>();

    //    QueueReader *reader = readers_[src_queue_index];
    QueueReader *reader = sourcePort(src_queue_index)->reader();

    if (reader != nullptr) {
      Node *item_node = reader->dataSchema()->item(item_index);
      UA_Variant variant;
      UA_StatusCode ret =
          UA_Client_readValueAttribute(client_, ua_node_id, &variant);

      if (ret == UA_STATUSCODE_GOOD) {
        if (checkCompatibility(item_node, &variant)) {
          compatible_push_items_.push_back(
              ItemOpcua(&ua_node_id, &variant, src_queue_index, item_index));
          auto it = std::find(src_queues_in_use_.begin(),
                              src_queues_in_use_.end(), src_queue_index);

          // Add to used src queue in use list for efficient reading in job
          if (it == src_queues_in_use_.end()) {
            src_queues_in_use_.push_back(src_queue_index);
          }
        }
        else {
          std::cout << "Item not compatible!!" << std::endl;
          item_node->printTree();
        }
        UA_Variant_clear(&variant);
      }
      else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Read DataType faild with code: %s",
                    UA_StatusCode_name(ret));
      }
    }
    UA_NodeId_clear(&ua_node_id);
  }

  UA_WriteRequest_init(&write_request_);
  size_t n_items_to_write = compatible_push_items_.size();
  write_request_.nodesToWrite = (UA_WriteValue *)UA_Array_new(
      n_items_to_write, &UA_TYPES[UA_TYPES_WRITEVALUE]);
  write_request_.nodesToWriteSize = n_items_to_write;

  for (size_t i = 0; i < n_items_to_write; i++) {
    UA_NodeId_copy(&compatible_push_items_[i].ua_node_id_,
                   &write_request_.nodesToWrite[i].nodeId);
    write_request_.nodesToWrite[i].attributeId = UA_ATTRIBUTEID_VALUE;
    write_request_.nodesToWrite[i].value.hasValue = true;
    write_request_.nodesToWrite[i].value.value =
        compatible_push_items_[i].variant_;
    write_request_.nodesToWrite[i].value.value.storageType =
        UA_VARIANT_DATA_NODELETE;
  }

  return 0;
}

int32_t OPCUAClient::clientRead()
{
  UA_ReadResponse read_response =
      UA_Client_Service_read(client_, read_request_);

  // std::cout << name() << " -> Return Service read: "
  //           << UA_StatusCode_name(read_response.responseHeader.serviceResult)
  //           << std::endl;

  if (read_response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
    // std::cout << "reader---> " << reader << std::endl;
    int32_t err = sinkPort(0)->writer()->startWrite();
    // std::cout << "err---> " << err << std::endl;

    if (err >= 0) {
      /* Write node attribute */
      for (size_t i = 0; i < compatible_pull_items_.size(); i++) {
        ItemOpcua &opcua_item = compatible_pull_items_[i];

        Message *msg = sinkPort(0)->writer()->dataMsg();
        Node *item_node = msg->item(opcua_item.item_index_);
        // std::cout << "Pulling ItemOPCUA!" << std::endl;
        // std::cout << item_node->name() << std::endl;

        if (item_node->isDataNode()) {
          // std::cout << "item_node->isDataNode()!" << std::endl;
          epf::DataNode *data_node = static_cast<DataNode *>(item_node);

          if (read_response.results->hasValue) {
            // if (data_node->datatype() == BaseType::EP_16S) {
            //   std::cout << data_node->name() << " read: "
            //             << *(int16_t *)read_response.results->value.data <<
            //             " "
            //             << data_node->size() << std::endl;
            // }
            memcpy(data_node->value(), read_response.results[i].value.data,
                   data_node->size());
            // std::cout << *(double*)data_node->value() << std::endl;
            // if (data_node->datatype() == BaseType::EP_BOOL) {
            //   std::cout << data_node->name() <<" read: " << *(bool
            //   *)read_response.results->value.data << " "
            //             << data_node->size() << std::endl;
            // }
            // std::cout << "Read: " << *(double *)data_node->value()
            //           << std::endl;
          }
        }
      }
      sinkPort(0)->writer()->endWrite();
    }
  }

  UA_ReadResponse_clear(&read_response);

  return 0;
}

int32_t OPCUAClient::clientWrite()
{
  int err = 0;
  for (auto queue_index : src_queues_in_use_) {
    QueueReader *reader = sourcePort(queue_index)->reader();
    // std::cout << "reader---> " << reader << std::endl;
    err = reader->startRead();
    // std::cout << "err---> " << err << std::endl;

    if (err >= 0) {
      /* Write node attribute */
      for (size_t i = 0; i < compatible_push_items_.size(); i++) {
        ItemOpcua opcua_item = compatible_push_items_[i];

        // std::cout << "Pushing ItemOpcua!" << std::endl;

        // Check if the item corresponds to the opened queue
        if (opcua_item.src_queue_ == queue_index) {
          //         Node *item_node =
          //          reader->dataSchema()->item(opcua_item.item_index_);
          Message *msg = reader->dataMsg();
          Node *item_node = msg->item(opcua_item.item_index_);

          if (item_node->isDataNode()) {
            // std::cout << "item_node->isDataNode()!" << std::endl;
            epf::DataNode *data_node = static_cast<DataNode *>(item_node);

            // std::cout << data_node->name() << " -> "
            //           << *(double *)data_node->value() << std::endl;

            int32_t data_size = 1;
            UA_UInt32 *dimensions = opcua_item.variant_.arrayDimensions;
            for (size_t dim = 0; dim < opcua_item.variant_.arrayDimensionsSize;
                 dim++) {
              data_size *= dimensions[dim];
              dimensions++;
            }
            if (data_size != static_cast<int32_t>(data_node->size())) {
              std::cout << "OPCUAClient::clientWrite8): OPCUA size not "
                           "coherent with queue size"
                        << std::endl;
            }

            memcpy(write_request_.nodesToWrite[i].value.value.data,
                   data_node->value(), data_node->size());
          }
          else if (item_node->isObjectNode()) {
            // std::cout << "preparing image to send..." << std::endl;
            ObjectNode *object_node = static_cast<ObjectNode *>(item_node);
            if (object_node->objecttype() == EP_IMAGE_RAW) {
              ImageObject image(object_node);

              int32_t data_size = 1;
              UA_UInt32 *dimensions = opcua_item.variant_.arrayDimensions;
              for (size_t dim = 0;
                   dim < opcua_item.variant_.arrayDimensionsSize; dim++) {
                data_size *= dimensions[dim];
                dimensions++;
              }
              if (data_size != static_cast<int32_t>(image.bufferSize())) {
                std::cout << "OPCUAClient::clientWrite8): OPCUA size not "
                             "coherent with queue size"
                          << std::endl;
              }

              memcpy(write_request_.nodesToWrite[i].value.value.data,
                     image.data(), image.bufferSize());
            }
            else if (object_node->objecttype() == EP_IMAGE_COMPRESSED) {
              // std::cout << "preparing compressed image to send..." <<
              // std::endl;
              StringNode *format = static_cast<StringNode *>(
                  object_node->references()[2].address());
              if (*format->value() == "jpg") {
                DataNode *buffer = static_cast<DataNode *>(
                    object_node->references()[0].address());
                DataNode *size = static_cast<DataNode *>(
                    object_node->references()[1].address());

                UA_ImageJPG jpg_image;
                jpg_image.length = *static_cast<uint64_t *>(size->value());
                jpg_image.data = static_cast<uint8_t *>(buffer->value());

                // std::cout << "JPG ---> addr: " << jpg_image.data
                //           << " size: " << jpg_image.length << std::endl;

                UA_Variant_setScalarCopy(
                    &write_request_.nodesToWrite[i].value.value, &jpg_image,
                    &UA_TYPES[UA_TYPES_IMAGEJPG]);
              }
            }
          }
        }
      }
      reader->endRead();
    }
  }

  UA_WriteResponse write_response =
      UA_Client_Service_write(client_, write_request_);
  // if (write_response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {

  // }
  // std::cout << name() << ": Return Service write: "
  //           <<
  //           UA_StatusCode_name(write_response.responseHeader.serviceResult)
  //           << std::endl;
  UA_WriteResponse_clear(&write_response);
  return 0;
}

std::unique_ptr<epf::DataNode> OPCUAClient::convertUAVariable2Node(
    UA_LocalizedText *display_name, const UA_Variant *variant)
{
  std::string name((char *)display_name->text.data, display_name->text.length);

  std::optional<BaseType> data_type = opcuatype_to_basetype(variant->type);
  size_t dimensions_size = variant->arrayDimensionsSize;
  std::vector<size_t> dimensions;
  if (dimensions_size == 0) {
    dimensions.push_back(1);
  }
  else {
    for (size_t i = 0; i < variant->arrayDimensionsSize; i++) {
      dimensions.push_back(variant->arrayDimensions[i]);
    }
  }
  if (data_type) {
    std::unique_ptr<epf::DataNode> data_node = std::make_unique<DataNode>(
        name, *data_type, dimensions, nullptr, false);

    return data_node;
  }
  else {
    return nullptr;
  }
}

int32_t OPCUAClient::reverseConnect()
{
  using namespace epf;

  std::cout << "Listening to stablish connection..." << std::endl;

  UA_String listen_hostnames[1];
  listen_hostnames[0] = UA_String_fromChars("localhost");

  UA_StatusCode retval = UA_Client_startListeningForReverseConnect(
      client_, listen_hostnames, 1, reverse_connect_port_);

  if (retval != UA_STATUSCODE_GOOD) {
    std::cerr << "ERROR: " << name()
              << " could not add reverse connect. Return -> "
              << UA_StatusCode_name(retval) << std::endl;
    return -1;
  }
  else {
    std::clog << "Started Reverse connect listening. "
              << UA_StatusCode_name(retval) << std::endl;
  }
  return 0;
}

bool OPCUAClient::checkCompatibility(Node *node, const UA_Variant *variant)
{
  std::cout << "Checking compatibility..." << std::endl;
  bool compatible = false;
  if (node->isDataNode()) {
    // Datatype Compatibility
    const DataNode *data_node = static_cast<const DataNode *>(node);
    bool type_compatibility =
        (data_node->datatype() == opcuatype_to_basetype(variant->type));

    bool arraydimensions_compatibility;
    // Array Dimensions Compatibility
    if (variant->arrayDimensionsSize == 0) {
      // Scalar
      arraydimensions_compatibility =
          (data_node->arraydimensions().size() == 1);
      arraydimensions_compatibility &= (data_node->arraydimensions()[0] == 1);
    }
    else {
      // Array
      arraydimensions_compatibility =
          (data_node->arraydimensions().size() == variant->arrayDimensionsSize);
      for (size_t i = 0; i < variant->arrayDimensionsSize; i++) {
        arraydimensions_compatibility &=
            (data_node->arraydimensions()[i] == variant->arrayDimensions[i]);
      }
    }
    compatible = type_compatibility & arraydimensions_compatibility;
  }
  else if (node->isObjectNode()) {
    ObjectNode *object_node = static_cast<ObjectNode *>(node);
    if (object_node->objecttype() == EP_IMAGE_RAW) {
      const ImageObject image(object_node);

      bool type_compatibility =
          (image.baseType() == opcuatype_to_basetype(variant->type));
      bool arraydimensions_compatibility;

      arraydimensions_compatibility = (3 == variant->arrayDimensionsSize);
      if (arraydimensions_compatibility) {
        arraydimensions_compatibility &=
            (image.height() ==
             static_cast<int32_t>(variant->arrayDimensions[0]));
        std::cout << "height_compatibility: " << arraydimensions_compatibility
                  << std::endl;
        arraydimensions_compatibility &=
            (image.width() ==
             static_cast<int32_t>(variant->arrayDimensions[1]));
        std::cout << "width_compatibility: " << arraydimensions_compatibility
                  << std::endl;
        arraydimensions_compatibility &=
            (image.channels() ==
             static_cast<int32_t>(variant->arrayDimensions[2]));
        std::cout << "channels_compatibility: " << arraydimensions_compatibility
                  << std::endl;
      }
      compatible = arraydimensions_compatibility & type_compatibility;
    }
    else if (object_node->objecttype() == EP_IMAGE_COMPRESSED) {
      const StringNode *format = static_cast<const StringNode *>(
          object_node->references()[2].address());
      if (*format->value() == "jpg") {
        compatible =
            UA_Variant_hasScalarType(variant, &UA_TYPES[UA_TYPES_BYTESTRING]);
        std::cout << "JPG_compatibility: " << compatible << std::endl;
        std::cout << "Variant Scalar: " << UA_Variant_isScalar(variant)
                  << std::endl;
        std::cout << "Variant Type: " << variant->type->typeName << std::endl;
        std::cout << "ORIG Type: " << (&UA_TYPES[UA_TYPES_IMAGEJPG])->typeName
                  << std::endl;
      }
    }
  }

  return compatible;
}

// State callback function
void OPCUAClient::onConnect(UA_Client *client,
                            UA_SecureChannelState channel_state,
                            UA_SessionState session_state,
                            UA_StatusCode connect_status)
{
  ClientContext *context = (ClientContext *)UA_Client_getContext(client);

  // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
  //             "Reverse connect state callback: channel_state %d
  //             session_state"
  //             "%d: State %s",
  //             channel_state, session_state,
  //             UA_StatusCode_name(connect_status));
  context->client_filter_->connection_initiatied_ =
      channel_state >= UA_SECURECHANNELSTATE_CONNECTING ? true : false;
  if (connect_status == UA_STATUSCODE_GOOD) {
    if (channel_state == UA_SECURECHANNELSTATE_OPEN &&
        session_state == UA_SESSIONSTATE_ACTIVATED) {
      // Log an info message for successful connection
      // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
      //             "Secure channel and session established.");
      context->client_filter_->connected_ = true;
      return;
    }
  }
  else {
    // Log an error message for connection issues
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Connection error: %s",
                UA_StatusCode_name(connect_status));
  }
  context->client_filter_->connected_ = false;
}

void OPCUAClient::connectionManagment()
{
  while (conn_mngmnt_running_) {
    // std::cout << name() << " -> Connected: " << connected_
    //           << " Filter Started: " << filter_started_ << " State: " <<
    //           state()
    //           << std::endl;
    if (!connected_) {
      UA_Client_run_iterate(client_, 250);
      if (filter_started_) {
        stop();
        if (reverse_connection_) {
          reverseConnect();
        }
        filter_started_ = false;
      }
      if (!reverse_connection_) {
        if (!connection_initiatied_) {
          connect();
        }
      }
    }
    else {
      if (!filter_started_) {
        UA_Client_run_iterate(client_, 250);
        // reset();
        if (state() == CONNECTED) {
          std::cout << name() << " -> Running set(). Ret-> " << set()
                    << std::endl;
        }
        if (state() == SET) {
          int32_t ret = start();
          std::cout << "START return: " << ret << std::endl;
        }
        if (state() == RUNNING) {
          filter_started_ = true;
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::duration<float>(1));
  }
}
