// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EPF_OPCUA_CLIENT_H
#define _EPF_OPCUA_CLIENT_H

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
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

#include "../../core/core.h"
#include "../libs/opcua_data.h"

namespace epf {
class ItemOpcua {
  public:
    UA_NodeId ua_node_id_;
    UA_Variant variant_;
    int32_t src_queue_;
    int32_t item_index_;

    /**
     * @brief Constructor. The class copies UA_NodeId and UA_Variant frees it
     * with its destruction.*/
    ItemOpcua(UA_NodeId *ua_node_id, const UA_Variant *variant,
              int32_t src_queue, int32_t item_index)
        : src_queue_(src_queue),
          item_index_(item_index)
    {
      UA_NodeId_init(&ua_node_id_);
      UA_NodeId_copy(ua_node_id, &ua_node_id_);
      UA_Variant_init(&variant_);
      if (variant) {
        UA_Variant_copy(variant, &variant_);
      }
    };

    ItemOpcua(const ItemOpcua &obj)
    {
      *this = obj;
    }

    const ItemOpcua &operator=(const ItemOpcua &obj)
    {
      UA_Variant_init(&variant_);
      UA_NodeId_init(&ua_node_id_);
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

// Forward declaration
class OPCUAClient;

struct ClientContext {
    OPCUAClient *client_filter_;
};

class OPCUAClient : public epf::Filter {
  public:
    OPCUAClient(const YAML::Node &config);
    ~OPCUAClient();

    int32_t connect();

    int32_t reverseConnect();
    int32_t disconnect();

    bool isConnected();

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
    std::string server_url_{"opc.tcp://localhost:4840"};
    float sampling_interval_{0.0};

    UA_Client *client_{nullptr};
    std::vector<int32_t> src_queues_in_use_;
    UA_ReadRequest read_request_;
    UA_WriteRequest write_request_;

    std::vector<epf::ItemOpcua> compatible_push_items_;
    std::vector<epf::ItemOpcua> compatible_pull_items_;

    bool checkCompatibility(epf::Node *node, const UA_Variant *variant);

    // TODO: Solve problem of constness with ImageObject
    std::unique_ptr<epf::DataNode> convertUAVariable2Node(
        UA_LocalizedText *display_name, const UA_Variant *variant);

    int32_t prepareReadRequest();

    int32_t prepareWriteRequest();

    int32_t clientRead();

    int32_t clientWrite();

    void connectionManagment();

    // Client Reverse Connection
    bool reverse_connection_{false};
    UA_UInt16 reverse_connect_port_{9966};

    std::unique_ptr<ClientContext> context_;
    std::thread conn_mngmnt_thread_{};  // Thread doing actual comm with server
    std::atomic<bool> conn_mngmnt_running_{false};  // To stop 2nd thread
    std::atomic<bool> connected_{false};
    std::atomic<bool> filter_started_{false};
    std::atomic<bool> connection_initiatied_{false};

    static void onConnect(UA_Client *client,
                          UA_SecureChannelState channel_state,
                          UA_SessionState session_state,
                          UA_StatusCode connect_status);
};
}  // namespace epf

#endif  //_EPF_OPCUA_CLIENT_H
