// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_OPCUA_SERVER_H
#define _EP_OPCUA_SERVER_H

#include <errno.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <unordered_map>

#include "../libs/opcua_data.h"
#include "core.h"

namespace ep {
class OPCUAserver;
}

/*Necessary struct to add methadata to the OPCUA server nodes and link them with
 * the item queues*/
struct QueueNodeContext {
    // Pointer to the OPCUA server filter.
    ep::OPCUAserver *srv_ptr;
    // name of the node
    std::string node_name;
    // Port of the OPCUA filter whose queue is currently being read
    int32_t src_port;
    // Item queue index
    int32_t item_index;
};

/*Necessary struct to add methadata to the OPCUA server settings nodes and link
 * them with the filter settings*/
struct SettingsNodeContext {
    // Pointer to the filter whose settings will be read or written
    ep::Filter *filter;
    // name of the node
    std::string node_name;
};

namespace ep {
class OPCUAserver : public Filter {
  public:
    OPCUAserver(YAML::Node &config);
    ~OPCUAserver();

    /*Static functions to enable the OPCUA server to perform callbacks. This
     * functions allow the server to execute the updating loop and update the
     * Nodes through the filter settings and queues */
    static UA_StatusCode commandMethod(
        UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
        const UA_NodeId *methodId, void *methodContext,
        const UA_NodeId *objectId, void *objectContext, size_t inputSize,
        const UA_Variant *input, size_t outputSize, UA_Variant *output);

    static UA_StatusCode beforeReadSetting(
        UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
        const UA_NumericRange *range, UA_DataValue *dataValue);

    static UA_StatusCode afterWriteSetting(
        UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data);

    static void beforeReadQueue(UA_Server *server, const UA_NodeId *sessionId,
                                void *sessionContext, const UA_NodeId *nodeId,
                                void *nodeContext, const UA_NumericRange *range,
                                const UA_DataValue *data);
    static void afterWriteQueue(UA_Server *server, const UA_NodeId *sessionId,
                                void *sessionContext, const UA_NodeId *nodeId,
                                void *nodeContext, const UA_NumericRange *range,
                                const UA_DataValue *data);

    /*Functions to register/unregister embedded pipelines pipeline*/
    int32_t registerPipeline();
    int32_t unregisterFilters();
    int32_t unregisterQueues();
    int32_t registerFilterSettings(const UA_NodeId &filter_settings,
                                   size_t filter_idx);
    int32_t registerQueues(const UA_NodeId &filter_queues, size_t filter_idx);

  protected:
    int32_t _job();
    int32_t _open();  
    int32_t _close(); 
    int32_t _set();   
    int32_t _reset(); 
    int32_t _start();
    int32_t _stop();

    bool enable_images_;
    int16_t server_port_;
    std::string lang_code_ = "en-US";

    YAML::Node config_;

    // Data to handle registered queues
    // Queues' OPCUA data
    std::vector<std::vector<int32_t>> items_to_update_;

    // vector containing each item UA_NodeId correspondence
    std::vector<std::vector<UA_NodeId>> queue_node_opcua_id_;

    std::vector<QueueNodeContext *> variable_node_contexts_;

    // Filters' OPCUA data
    std::vector<SettingsNodeContext *> filter_setting_contexts_;
    std::vector<char *> settings_description_reg_;

    UA_Server *server_;
    UA_StatusCode retval_;

    UA_ServerConfig *server_config_;

    int32_t createServerFolder(const UA_NodeId &parent, const std::string &name,
                               UA_NodeId *out_node);

    void addQueueItems(size_t queue_port, UA_NodeId &parent_id);

    int32_t addSettingNode(ep::Node2 *node, UA_NodeId parent_id,
                           size_t filter_idx);

    int32_t addCommandNode(ep::Node2 *node, UA_NodeId parent_id,
                           size_t filter_idx);

    void updateServerFromQueue(int32_t filter_port,
                               std::vector<int32_t> &updating_nodes);
};

}  // namespace ep

#endif  //_EP_OPCUA_SERVER_H
