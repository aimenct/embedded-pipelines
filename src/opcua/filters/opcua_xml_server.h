// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_OPCUA_XML_SERVER_H
#define _EP_OPCUA_XML_SERVER_H

#include <errno.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodesetloader.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <algorithm>
#include <fstream>
#include <unordered_map>

#include "core.h"
#include "opcua_server.h"

namespace ep {

class OPCUAXmlserver;
}

struct OpcuaNodeToYaml {
    std::string name;
    std::string node_id;
    std::string src_port;
    std::string msg_item;
    int edge_to_opcua;
    std::vector<OpcuaNodeToYaml> references;
};

namespace ep {
class OPCUAXmlserver : public OPCUAserver {
  public:
    OPCUAXmlserver(YAML::Node &config, std::string xmlfile);
    ~OPCUAXmlserver();

    static void beforeReadSetting(UA_Server *server, const UA_NodeId *sessionId,
                                  void *sessionContext, const UA_NodeId *nodeId,
                                  void *nodeContext,
                                  const UA_NumericRange *range,
                                  const UA_DataValue *data);
    static void writeSetting(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext, const UA_NodeId *nodeId,
                             void *nodeContext, const UA_NumericRange *range,
                             const UA_DataValue *data);

    int registerNodes2Yaml();
    int registerYaml2OpcUa();
    int browseNodes(UA_NodeId nodeid, std::vector<OpcuaNodeToYaml> &nodes);
    YAML::Node convertNodesToYaml(const std::vector<OpcuaNodeToYaml> &nodes);
    int processYamlNode(const YAML::Node &reference);

  protected:
    int32_t _open();  // override
    int32_t _set();   // override

    // Vector to store the changed descriptions, e.g. for expressing the range
    std::vector<OpcuaNodeToYaml> browse_nodes_;

    std::string xmlfile_;
    std::string config_output_;
};

}  // namespace ep

#endif  //_EP_OPCUA_XML_SERVER_H
