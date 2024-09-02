// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "serialization.h"

namespace ep {

void message_node_to_yaml(const Node2 &my_node, YAML::Emitter &out, bool flow)
{
  // if (flow) out << YAML::Flow;
  // out << YAML::BeginMap;  // node properties

  bool defined_object = 0;

  switch (my_node.nodetype()) {
    case EP_DATANODE: {
      DataNode &dn = *(DataNode *)&my_node;

      out << YAML::Key << "name" << YAML::Value << dn.name();

      out << YAML::Key << "node type" << YAML::Value
          << nodetype_to_string(my_node.nodetype());

      if (my_node.tooltip() != "")
        out << YAML::Key << "tooltip" << YAML::Value << my_node.tooltip();

      out << YAML::Key << "data type" << YAML::Value
          << basetype_to_string(dn.datatype());

      if ((dn.arraydimensions().size() != 1) ||
          (dn.arraydimensions()[0] != 1)) {
        out << YAML::Key << "dimensions" << YAML::Value << YAML::Flow
            << YAML::BeginSeq;
        for (size_t dim : dn.arraydimensions()) {
          out << dim;
        }
        out << YAML::EndSeq;
      }

      if (!dn.isStreamed()) {
        std::string value = dataNodeValueToString(dn);
        out << YAML::Key << "value" << YAML::Value << value;
      }
      else {
        out << YAML::Key << "streamed" << YAML::Value << dn.isStreamed();
        out << YAML::Key << "offset" << YAML::Value << dn.offset();
      }
      break;
    }
    case EP_OBJECTNODE: {
      ObjectNode &on = *(ObjectNode *)&my_node;
      out << YAML::Key << "name" << YAML::Value << on.name();
      out << YAML::Key << "node type" << YAML::Value
          << nodetype_to_string(on.nodetype());
      if (on.tooltip() != "")
        out << YAML::Key << "tooltip" << YAML::Value << on.tooltip();
      out << YAML::Key << "object type" << YAML::Value
          << objecttype_to_string(on.objecttype());

      switch (on.objecttype()) {
        case EP_IMAGE_OBJ: {
          defined_object = 1;

          ImageObject img_obj(&on);

          out << YAML::Key << "width" << YAML::Value << img_obj.width();
          out << YAML::Key << "height" << YAML::Value << img_obj.height();
          out << YAML::Key << "channels" << YAML::Value << img_obj.channels();
          out << YAML::Key << "pixel format" << YAML::Value
              << pixelformat_to_string(img_obj.pixelFormat());
          // check if streamed (data)
          if (on.references()[4].address()->isDataNode()) {
            DataNode *dn =
                static_cast<DataNode *>(on.references()[4].address());
            if (dn->isStreamed()) {
              out << YAML::Key << "streamed" << YAML::Value << dn->isStreamed();
              out << YAML::Key << "offset" << YAML::Value << dn->offset();
            }
          }
          break;
        }
        case EP_OBJ:
          defined_object = 0;
          break;
      }
      break;
    }
    default:
      std::cout << "Message to Node - Default case - nodetype not allowed"
                << std::endl;
      return;
  }

  if ((my_node.references().size() != 0) && (defined_object == 0)) {
    out << YAML::Key << "references" << YAML::Value << YAML::BeginSeq;
    for (const ep::Reference &ref : my_node.references()) {
      Node2 *n = ref.address();

      if (flow) out << YAML::Flow;
      out << YAML::BeginMap;  // node properties
      out << YAML::Key << "ref type" << YAML::Value
          << reftype_to_string(ref.type());
      message_node_to_yaml(*n, out);
      out << YAML::EndMap;  // End Node map
    }
    out << YAML::EndSeq;  // End references sequence
  }
}

int message_to_yaml(const Message &msg, const std::string &filename)
{
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Item count" << YAML::Value << msg.itemCount();
  out << YAML::Key << "Streamed data size" << YAML::Value << msg.size();
  out << YAML::Key << "Items";
  out << YAML::BeginSeq;

  for (size_t i = 0; i < msg.itemCount(); i++) {
    Node2 *my_node = msg.item(i);
    out << YAML::BeginMap;  // node properties
    message_node_to_yaml(*my_node, out, false);
    out << YAML::EndMap;  // End Node map
  }

  out << YAML::EndSeq;  // End NodeTree sequence
  out << YAML::EndMap;  // End root map

  // Output the YAML
  if (filename.empty()) {
    std::cout << out.c_str() << std::endl;
  }
  else {
    std::ofstream file(filename);
    if (file.is_open()) {
      file << out.c_str();
      file.close();
    }
    else {
      std::cerr << "Error: Unable to open file " << filename << std::endl;
      return 1;  // Indicate failure to open file
    }
  }

  return 0;
}

Node2 *message_node_from_yaml(const YAML::Node &node)
{
  // Node2
  int id = -1;
  std::string name = "";
  NodeType node_type = EP_DATANODE;
  std::string tooltip = "";
  //  VisibilityType visibility = EP_BEGINNER;
  AccessType access_type = R;

  // DataNode
  bool memory_managed = false;
  bool streamed = false;
  std::vector<size_t> array_dim = {};
  void *value = nullptr;
  BaseType data_type = EP_8C;

  // ObjectNode
  ObjectType object_type = EP_OBJ;

  // StringNode
  std::string string_value = "";

  if (node["node type"]) {
    node_type = static_cast<NodeType>(
        string_to_nodetype(node["node type"].as<std::string>()));
  }
  else {
    std::cout << "parse from yaml unknown nodetype " << std::endl;
    exit(0);
  }

  if (node["id"]) id = node["id"].as<int>();
  if (node["name"]) name = node["name"].as<std::string>();
  if (node["tooltip"]) tooltip = node["tooltip"].as<std::string>();
  // if (node["visibility"])
  //   visibility = static_cast<VisibilityType>(node["visibility"].as<int>());
  if (node["access mode"])
    access_type =
        static_cast<AccessType>(node["access mode"].as<unsigned char>());
  if (node["object type"])
    object_type = string_to_objecttype(node["object type"].as<std::string>());
  if (node["data type"])
    data_type = string_to_basetype(node["data type"].as<std::string>());
  //  if (node["dimensions"]) {
  if (node["dimensions"] && node["dimensions"].IsSequence())
    for (const auto &dim : node["dimensions"]) {
      array_dim.push_back(dim.as<int>());
    }
  else
    array_dim.push_back(1);

  if (node["streamed"]) streamed = node["streamed"].as<bool>();

  DataNode *dn = nullptr;

  // DATANODE
  if (node_type == EP_DATANODE) {
    if ((!node["streamed"]) ||  // if not streamed -> memory managed
        ((node["streamed"]) && (!node["streamed"].as<bool>()))) {
      memory_managed = true;
      switch (data_type) {
        case EP_8C:
          value = parseFlatBufferFromNestedArray<char>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<char *>(value);
          break;
        case EP_8U:
          value = parseFlatBufferFromNestedArray<unsigned char>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<unsigned char *>(value);
          break;
        case EP_8S:
          value = parseFlatBufferFromNestedArray<signed char>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<signed char *>(value);
          break;
        case EP_16U:
          value = parseFlatBufferFromNestedArray<unsigned short>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<unsigned short *>(value);
          break;
        case EP_16S:
          value = parseFlatBufferFromNestedArray<signed short>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<signed short *>(value);
          break;
        case EP_32U:
          value = parseFlatBufferFromNestedArray<unsigned int>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<unsigned int *>(value);
          break;
        case EP_32S:
          value = parseFlatBufferFromNestedArray<int>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<int *>(value);
          break;
        case EP_64S:
          value = parseFlatBufferFromNestedArray<long>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<long *>(value);
          break;
        case EP_64U:
          value = parseFlatBufferFromNestedArray<unsigned long>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<unsigned long *>(value);
          break;
        case EP_32F:
          value = parseFlatBufferFromNestedArray<float>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<float *>(value);
          break;
        case EP_64F:
          value = parseFlatBufferFromNestedArray<double>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<double *>(value);
          break;
        case EP_BOOL:
          value = parseFlatBufferFromNestedArray<bool>(
              node["value"].as<std::string>());
          dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                            tooltip, streamed, access_type);
          delete[] static_cast<bool *>(value);
          break;
        case EP_STRING:
          value = parseFlatBufferFromNestedArray<std::string>(
              node["value"].as<std::string>());

          dn = new DataNode(name, data_type, array_dim, tooltip, access_type);
          for (size_t i = 0, k = 0; i < array_dim.size(); i++)
            for (size_t j = 0; j < array_dim[i]; j++, k++) {
              static_cast<std::string *>(dn->value())[k] =
                  static_cast<std::string *>(value)[k];
            }

          delete[] static_cast<std::string *>(value);

          break;
      }
    }
    else {  // if streamed{
      dn = new DataNode(name, data_type, array_dim, value, memory_managed,
                        tooltip, streamed, access_type);
    }

    if ((node["id"]) && (dn != nullptr)) dn->setId(id);

    if (node["references"] && node["references"].IsSequence()) {
      for (const auto &ref : node["references"]) {
        Node2 *nr = message_node_from_yaml(ref);
        if (node["ref type"])
          dn->addReference(string_to_reftype(ref["ref type"].as<std::string>()),
                           nr);
        else
          dn->addReference(EP_HAS_CHILD, nr);
      }
    }
    return dn;
  }
  // OBJECTNODE
  else if (node_type == EP_OBJECTNODE) {
    // IMAGE OBJECT
    if (object_type == EP_IMAGE_OBJ) {
      int32_t width, height, channels = 0;
      PixelFormat pixel_format;
      if (node["width"])
        width = node["width"].as<int>();
      else {
        std::cerr << "ImageObj width not found" << std::endl;
        return nullptr;
      }
      if (node["height"])
        height = node["height"].as<int>();
      else {
        std::cerr << "ImageObj height not found" << std::endl;
        return nullptr;
      }
      if (node["channels"]) channels = node["channels"].as<int>();
      if (node["pixel format"])
        pixel_format =
            string_to_pixelformat(node["pixel format"].as<std::string>());
      else {
        std::cerr << "ImageObj PixelFormat not found" << std::endl;
        return nullptr;
      }
      ImageObject *img =
          new ImageObject(name, width, height, channels, pixel_format, nullptr);
      // if (node["id"]) dn->setId(id);
      Node2 *nn = img->copyNodeTree();
      delete img;
      //  add_references(on,ref_type);
      if (node["references"] && node["references"].IsSequence()) {
        for (const auto &ref : node["references"]) {
          Node2 *nr = message_node_from_yaml(ref);
          if (node["ref type"])
            nn->addReference(
                string_to_reftype(ref["ref type"].as<std::string>()), nr);
          else
            nn->addReference(EP_HAS_CHILD, nr);
        }
      }
      return nn;
    }
    // DEFAULT OBJECT
    else if (object_type == EP_OBJ) {
      //      std::cout << "Object" << std::endl;
      ObjectNode *on = new ObjectNode(name, object_type, tooltip);

      //  add_references(on,ref_type);
      if (node["references"] && node["references"].IsSequence()) {
        for (const auto &ref : node["references"]) {
          Node2 *nr = message_node_from_yaml(ref);
          if (node["ref type"])
            on->addReference(
                string_to_reftype(ref["ref type"].as<std::string>()), nr);
          else
            on->addReference(EP_HAS_CHILD, nr);
        }
      }
      return on;
    }
    // CUSTOM OBJECT
    else {
      //  add_references(on,ref_type);
      return nullptr;
    }
  }
  return nullptr;
}

Message *message_from_yaml(const YAML::Node &node)
{
  Message *msg = new Message();
  if (node["Items"] && node["Items"].IsSequence()) {
    for (const auto &item : node["Items"]) {
      Node2 *n = message_node_from_yaml(item);
      msg->addItem(n);
    }
  }
  return msg;
}

void nodeTree2Yaml(const NodeTree &my_tree)
{
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "NodeTree";
  out << YAML::BeginSeq;

  for (int32_t i = 0; i < my_tree.length(); i++) {
    out << YAML::BeginMap;  // node properties
    out << YAML::Key << "id" << YAML::Value << my_tree.nodeIndex(my_tree[i]);

    Node2 *my_node = my_tree[i];

    out << YAML::Key << "name" << YAML::Value << my_node->name();
    out << YAML::Key << "node type" << YAML::Value << my_node->nodetype();
    out << YAML::Key << "tooltip" << YAML::Value << my_node->tooltip();
    out << YAML::Key << "visibility" << YAML::Value << my_node->visibility();

    switch (my_node->nodetype()) {
      case EP_DATANODE: {
        DataNode &dn = *(DataNode *)my_node;

        out << YAML::Key << "data type" << YAML::Value << dn.datatype();
        out << YAML::Key << "size" << YAML::Value << dn.size();
        out << YAML::Key << "accessmode" << YAML::Value << dn.accessMode();
        out << YAML::Key << "dimensions" << YAML::Value << YAML::Flow
            << YAML::BeginSeq;
        for (size_t dim : dn.arraydimensions()) {
          out << dim;
        }
        out << YAML::EndSeq;
        out << YAML::Key << "streamed" << YAML::Value << dn.isStreamed();
        out << YAML::Key << "offset" << YAML::Value << dn.offset();
        out << YAML::Key << "memory managed" << YAML::Value << dn.memMgmt();

        std::string value = dataNodeValueToString(dn);
        out << YAML::Key << "value" << YAML::Value << value;

        break;
      }
      case EP_OBJECTNODE: {
        //        std::cout << "OBJECTNODE" << std::endl;

        ObjectNode &on = *(ObjectNode *)my_node;
        out << YAML::Key << "object type" << YAML::Value << on.objecttype();
        break;
      }
      default:
        break;
    }

    out << YAML::Key << "references" << YAML::Value << YAML::BeginSeq;
    for (const ep::Reference &ref : my_node->references()) {
      Node2 *n = ref.address();

      out << YAML::Flow << YAML::BeginMap << YAML::Key << "node_id"
          << YAML::Value << my_tree.nodeIndex(n) << YAML::Key << "ref_type"
          << YAML::Value << ref.type() << YAML::EndMap;
    }
    out << YAML::EndSeq;  // End references sequence

    out << YAML::EndMap;  // End Node map
  }

  out << YAML::EndSeq;  // End NodeTree sequence
  out << YAML::EndMap;  // End root map

  // Output the YAML
  std::cout << out.c_str() << std::endl;

  return;
}

std::vector<int> parseDimensions(const YAML::Node node)
{
  std::vector<int> dimensions;
  if (node["dimensions"] && node["dimensions"].IsSequence()) {
    for (const auto &dim : node["dimensions"]) {
      dimensions.push_back(dim.as<int>());
    }
  }
  return dimensions;
}

std::string dataNodeValueToString(const DataNode &node)
{
  std::ostringstream oss;

  if (node.value() != nullptr) switch (node.datatype()) {
      case EP_8C:
        streamValues(oss, static_cast<char *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_8U:
        streamValues(oss, static_cast<unsigned char *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_8S:
        streamValues(oss, static_cast<signed char *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_16U:
        streamValues(oss, static_cast<unsigned short *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_16S:
        streamValues(oss, static_cast<short *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_32U:
        streamValues(oss, static_cast<unsigned int *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_32S:
        streamValues(oss, static_cast<int *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_64S:
        streamValues(oss, static_cast<long *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_64U:
        streamValues(oss, static_cast<unsigned long *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_32F:
        streamValues(oss, static_cast<float *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_64F:
        streamValues(oss, static_cast<double *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_BOOL:
        streamValues(oss, static_cast<bool *>(node.value()),
                     node.arraydimensions());
        break;
      case EP_STRING: {
        streamValues(oss, static_cast<std::string *>(node.value()),
                     node.arraydimensions());
      } break;
      // Add other cases as needed
      default:
        oss << "Unsupported data type";
    }

  return oss.str();
}

std::string trim(const std::string &str)
{
  size_t start = str.find_first_not_of(" \t\n\r");
  size_t end = str.find_last_not_of(" \t\n\r");
  return (start == std::string::npos || end == std::string::npos)
             ? ""
             : str.substr(start, end - start + 1);
}
}  // namespace ep
