// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_OPCUA_CLIENT_H
#define _EP_OPCUA_CLIENT_H

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodestore.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

#include "../libs/opcua_data.h"
#include "core.h"

namespace ep {
class ItemOpcua {
  public:
    UA_NodeId ua_node_id_;
    UA_Variant variant_;
    int32_t src_queue_;
    int32_t item_index_;

    /**
     * @brief Constructor. The class copies UA_NodeId and UA_Variant frees it
     * with its destruction.*/
    ItemOpcua(UA_NodeId ua_node_id, const UA_Variant* variant,
              int32_t src_queue, int32_t item_index)
        : ua_node_id_(ua_node_id),
          src_queue_(src_queue),
          item_index_(item_index)
    {
      UA_NodeId_copy(&ua_node_id, &ua_node_id_);
      UA_Variant_init(&variant_);
      UA_Variant_copy(variant, &variant_);
    };

    ItemOpcua(const ItemOpcua& obj)
    {
      *this = obj;
    }

    const ItemOpcua& operator=(const ItemOpcua& obj)
    {
      UA_NodeId_copy(&obj.ua_node_id_, &ua_node_id_);
      UA_Variant_copy(&obj.variant_, &variant_);
      src_queue_ = obj.src_queue_;
      item_index_ = obj.item_index_;

      return *this;
    }

    ~ItemOpcua()
    {
      UA_NodeId_clear(&ua_node_id_);
      UA_Variant_clear(&variant_);
    }
};

class OPCUAClient : public ep::Filter {
  public:
    OPCUAClient(const YAML::Node& config);  
    ~OPCUAClient();                         

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();

  private:
    /* Add Filter settings */
    std::string config_path_;
    std::string endpointurl_;
    float sampling_interval_;

    UA_Client* client_;
    std::vector<int32_t> src_queues_in_use_;
    UA_WriteRequest write_request_;

    std::vector<ItemOpcua> compatible_items_;
    YAML::Node yaml_config_;

    bool checkCompatibility(const Node2* node, const UA_Variant* variant);

};
}  // namespace ep

#endif  //_EP_OPCUA_CLIENT_H
