// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/* Filter constructor - create settings */
#include "opcua_client.h"

namespace ep {

OPCUAClient::OPCUAClient(const YAML::Node &config)
    : Filter()
{
  using namespace ep;

  std::cout << "OPCUAclient constructor " << std::endl;

  yaml_config_ = config;

  if ((config)["name"]) {
    name_ = (config)["name"].as<std::string>();
  }
  max_sources_ = 2;
  addSetting("max_src_queues", max_sources_);

  /******************* EDIT HERE **********************************/
  endpointurl_ = "opc.tcp://localhost:4850";
  addSetting("endPointUrl", endpointurl_);

  sampling_interval_ = 0;
  addSetting("sampling_interval", sampling_interval_);

  readSettings(config);
}

/* Filter destructor*/
OPCUAClient::~OPCUAClient()
{
  std::cout << "OPCUAclient destructor " << std::endl;
}

int32_t OPCUAClient::_open()
{
  /******************* EDIT HERE **********************************/
  // e.g. open connection with device
  client_ = UA_Client_new();
  UA_ClientConfig_setDefault(UA_Client_getConfig(client_));

  UA_StatusCode retval = UA_Client_connect(client_, endpointurl_.c_str());
  if (retval != UA_STATUSCODE_GOOD) {
    printf("Could not connect\n");
    UA_Client_delete(client_);
    return -1;
  }

  /******************* EDIT END  ***********************************/
  return 0;
}

int32_t OPCUAClient::_close()
{
  /******************* EDIT HERE **********************************/
  // e.g. close connection with device
  UA_Client_disconnect(client_);
  UA_Client_delete(client_);
  /******************* EDIT END  ***********************************/
  return 0;
}

int32_t OPCUAClient::_set()
{
  using namespace ep;

  YAML::Node yaml_sink_items = yaml_config_["sink_items"];
  if (yaml_sink_items.Type() != YAML::NodeType::Sequence) {
    std::cerr << "OPCUAClient error in YAML: read register definition"
              << std::endl;
    return -1;
  }
  for (size_t i = 0; i < yaml_sink_items.size(); i++) {
    // Parse node id from code "ns=#;i=#" or "ns=#;s=aaa"
    std::string ua_node_std_string =
        yaml_sink_items[i]["ua_node_id"].as<std::string>();
    UA_NodeId ua_node_id;
    UA_String ua_node_string = UA_String_fromChars(ua_node_std_string.c_str());
    UA_NodeId_parse(&ua_node_id, ua_node_string);
    UA_String_clear(&ua_node_string);

    // Retrieve source queue and item indexes from YAML file
    int32_t src_queue_index =
        yaml_sink_items[i]["src_queue_index"].as<int32_t>();
    int32_t item_index = yaml_sink_items[i]["item_index"].as<int32_t>();

    //    QueueReader *reader = readers_[src_queue_index];
    QueueReader *reader = this->reader(src_queue_index);

    if (reader != nullptr) {
      Node2 *item_node = reader->dataSchema()->item(item_index);
      UA_Variant variant;
      UA_StatusCode ret =
          UA_Client_readValueAttribute(client_, ua_node_id, &variant);

      if (ret == UA_STATUSCODE_GOOD) {
        if (checkCompatibility(item_node, &variant)) {
          compatible_items_.push_back(
              ItemOpcua(ua_node_id, &variant, src_queue_index, item_index));
          auto it = std::find(src_queues_in_use_.begin(),
                              src_queues_in_use_.end(), src_queue_index);

          // Add to used src queue in use list for efficient reading in job
          if (it == src_queues_in_use_.end()) {
            src_queues_in_use_.push_back(src_queue_index);
          }
        }
      }
      else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Read DataType faild with code: %s",
                    UA_StatusCode_name(ret));
      }
      UA_Variant_clear(&variant);
    }
    UA_NodeId_clear(&ua_node_id);
  }

  UA_WriteRequest_init(&write_request_);
  size_t n_items_to_write = compatible_items_.size();
  write_request_.nodesToWrite = (UA_WriteValue *)UA_Array_new(
      n_items_to_write, &UA_TYPES[UA_TYPES_WRITEVALUE]);
  write_request_.nodesToWriteSize = n_items_to_write;

  for (size_t i = 0; i < n_items_to_write; i++) {
    UA_NodeId_copy(&compatible_items_[i].ua_node_id_,
                   &write_request_.nodesToWrite[i].nodeId);
    write_request_.nodesToWrite[i].attributeId = UA_ATTRIBUTEID_VALUE;
    write_request_.nodesToWrite[i].value.hasValue = true;
    write_request_.nodesToWrite[i].value.value = compatible_items_[i].variant_;
    write_request_.nodesToWrite[i].value.value.storageType =
        UA_VARIANT_DATA_NODELETE;
  }
  return 0;
}

int32_t OPCUAClient::_reset()
{
  /* free sink queues */
  UA_WriteRequest_clear(&write_request_);

  printf("OPCUAclient reset\n");
  return 0;
}

int32_t OPCUAClient::_start()
{
  return 0;
}

int32_t OPCUAClient::_stop()
{
  return 0;
}

/* Thread function implementation */
int32_t OPCUAClient::_job()
{
  using namespace ep;

  int err = 0;

  for (auto queue_index : src_queues_in_use_) {
    QueueReader *reader = this->reader(queue_index);
    err = reader->startRead();
    if (err >= 0) {
      /* Write node attribute */
      for (size_t i = 0; i < compatible_items_.size(); i++) {
        ItemOpcua opcua_item = compatible_items_[i];

        // Check if the item corresponds to the opened queue
        if (opcua_item.src_queue_ == queue_index) {
          //          Node2 *item_node =
          //          reader->dataSchema()->item(opcua_item.item_index_);
          Message msg = reader->dataMsg();
          Node2 *item_node = msg.item(opcua_item.item_index_);

          if (item_node->isDataNode()) {
            ep::DataNode *data_node = static_cast<DataNode *>(item_node);

            // std::cout << data_node->name() << " -> "
            //           << *(double *)data_node->value() << std::endl;

            int32_t data_size = 1;
            UA_UInt32 *dimensions = opcua_item.variant_.arrayDimensions;
            for (size_t dim = 0; dim < opcua_item.variant_.arrayDimensionsSize;
                 dim++) {
              data_size *= dimensions[dim];
              dimensions++;
            }

            memcpy(write_request_.nodesToWrite[i].value.value.data,
                   data_node->value(), data_node->size());
          }
          // else if (row.node->isObjectNode()) {
          //   ep::ObjectNode *objnode = (ep::ObjectNode *)row.node;
          //   if (objnode->objecttype() == ep::EP_IMAGE_OBJ &&
          //       row.imageobject.has_value()) {
          //     UA_ByteString jpgimage;
          //     ep::DataNode *img_size = (ep::DataNode *)row.imagesize;
          //     int size = *(int *)img_size->value();
          //     jpgimage.length = size;
          //     jpgimage.data = (UA_Byte *)row.imageobject->data();
          //     UA_Variant_setScalarCopy(&wReq_->nodesToWrite[i].value.value,
          //                              &jpgimage,
          //                              &UA_TYPES[UA_TYPES_BYTESTRING]);
          //   }
          // }
        }
      }
      reader->endRead();
    }
  }

  UA_WriteResponse write_response =
      UA_Client_Service_write(client_, write_request_);
  if (write_response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
    UA_WriteResponse_clear(&write_response);
  }

  std::this_thread::sleep_for(std::chrono::duration<float>(sampling_interval_));

  return 0;
}

bool OPCUAClient::checkCompatibility(const Node2 *node,
                                     const UA_Variant *variant)
{
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

  return compatible;
}
}  // namespace ep
