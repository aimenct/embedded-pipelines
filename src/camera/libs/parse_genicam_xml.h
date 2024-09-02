// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>

#include <yaml-cpp/yaml.h>

#include <pthread.h>

#include <time.h>

#include "core.h"
#include "filters/arv_cam.h"


class GenicamXMLParser{
    public:
        ep::ObjectNode* root = new ep::ObjectNode (std::string("Root"));
        ep::Nodeset2 nodeset;
        ep::ArvCam* parcam;
        std::string svalue;
        const xmlChar* namegroup = NULL;
        char* pvalue;
        std::vector<int> newdata;
        /** @brief Constructor of the class */
        GenicamXMLParser(const char* filename);
        GenicamXMLParser(ep::ArvCam* camera,ep::Nodeset2 camnodeset);

        /** @brief Function to parser Nodes containing pFeatures*/
        void parseObjNode(xmlNode *node);

        /** @brief Funtion to parser Strings or Strings Registers as String Nodes */
        void parseStringRegNode(xmlNode *node);

        /** @brief Funtion to parser Integers or Integers Registers as Data Nodes*/
        void parseIntNode(xmlNode*node);

        /** @brief Funtion to parser Enumerates as Data Nodes*/
        void parseEnumNode(xmlNode*node);

        /** @brief Funtion to parser Commands as Command Nodes*/
        void parseComNode(xmlNode*node);

        /** @brief Funtion to parser Floats to Data Nodes*/
        void parseFloatNode(xmlNode*node);

        /** @brief Funtion to parser Booleans as Data Nodes*/
        void parseBoolNode(xmlNode*node);

        /** @brief Funtion that parser pValue nodes
         *  @param node XML node element
         *  @param nnode ep::Node element where the reference between nodes will be stored
         *  @param nodename name of the node to be found
         */
        void parsePvalue(xmlNode *node,ep::Node2* nnode,std::string nodename,std::string pvaluetype);

        void parsePindex(xmlNode*node,ep::Node2*nnode);
    private:

        /** @brief Funtion that parser the main nodes in the XML and assigns the data and node type in the tree.
         *  This funtion also creates the references btween nodes
         *  @param a_node XML node
         *  @param node Node where the reference is stored.
         *  @param nodename Name of the node to be found in the XML tree.
         */
        void _processNodeXML(xmlNode*a_node,ep::Node2*node,std::string nodename);

        /** @brief This Function parser the Category Nodes and creates the corresponding node in nodeset with his nodetype*/
        void _processNodeTree(xmlNode *a_node);

        
        /** @brief This function process all nodes that contain data that are not the category nodes*/
        void _processNodeAttr(xmlNode *a_node);
};