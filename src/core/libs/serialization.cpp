// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "serialization.h"

namespace epf {

void free_flatbuffer(void *value, BaseType data_type);
void *parse_flatbuffer(const YAML::Node &node, BaseType data_type);
std::unique_ptr<ObjectNode> create_image_object(const YAML::Node &node,
                                                const std::string &name);
std::unique_ptr<StringNode> create_string_node(const YAML::Node &node,
                                               const std::string &name,
                                               AccessType access_type,
                                               const std::string &tooltip);
std::unique_ptr<ObjectNode> create_object_node(const YAML::Node &node,
                                               const std::string &name,
                                               ObjectType object_type,
                                               const std::string &tooltip);
std::unique_ptr<DataNode> create_data_node(
    const YAML::Node &node, const std::string &name, BaseType data_type,
    const std::vector<size_t> &array_dim, bool memory_managed,
    const std::string &tooltip, AccessType access_type, int id);

namespace {

void serialize_common_fields(const Node &node, YAML::Emitter &out_yaml)
{
  out_yaml << YAML::Key << "name" << YAML::Value << node.name();
  out_yaml << YAML::Key << "node_type" << YAML::Value
           << nodetype_to_string(node.nodetype());

  if (!node.tooltip().empty())
    out_yaml << YAML::Key << "tooltip" << YAML::Value << node.tooltip();

  if (node.accessMode() != R)
    out_yaml << YAML::Key << "access_mode" << YAML::Value
             << accesstype_to_string(node.accessMode());
}

void serialize_data_node(const DataNode &node, YAML::Emitter &out_yaml)
{
  out_yaml << YAML::Key << "data_type" << YAML::Value
           << basetype_to_string(node.datatype());

  if ((node.arraydimensions().size() != 1) ||
      (node.arraydimensions()[0] != 1)) {
    out_yaml << YAML::Key << "dimensions" << YAML::Value << YAML::Flow
             << YAML::BeginSeq;
    for (size_t dimension : node.arraydimensions()) {
      out_yaml << dimension;
    }
    out_yaml << YAML::EndSeq;
  }

  out_yaml << YAML::Key << "managed" << YAML::Value << node.memMgmt();
  if (node.memMgmt()) {
    out_yaml << YAML::Key << "value" << YAML::Value
             << data_node_value_to_string(node);
  }
}

void serialize_string_node(const StringNode &node, YAML::Emitter &out_yaml)
{
  out_yaml << YAML::Key << "value" << YAML::Value << *node.value();
}

bool serialize_object_node(const ObjectNode &node, YAML::Emitter &out_yaml)
{
  out_yaml << YAML::Key << "object_type" << YAML::Value
           << objecttype_to_string(node.objecttype());

  switch (node.objecttype()) {
    case EP_IMAGE_RAW: {
      // ImageObject currently has no const view, but construction and the
      // accessors used below do not modify the ObjectNode.
      ImageObject image(const_cast<ObjectNode *>(&node));

      out_yaml << YAML::Key << "width" << YAML::Value << image.width();
      out_yaml << YAML::Key << "height" << YAML::Value << image.height();
      out_yaml << YAML::Key << "channels" << YAML::Value << image.channels();
      out_yaml << YAML::Key << "pixel_format" << YAML::Value
               << to_string(image.pixelFormat());
      return false;
    }
    case EP_OBJ:
      return true;
  }

  return true;
}

void serialize_references(const Node &node, YAML::Emitter &out_yaml)
{
  if (node.references().empty()) return;

  out_yaml << YAML::Key << "references" << YAML::Value << YAML::BeginSeq;
  for (const Reference &reference : node.references()) {
    const Node *child = reference.address();

    out_yaml << YAML::BeginMap;
    out_yaml << YAML::Key << "ref_type" << YAML::Value
             << reftype_to_string(reference.type());
    serialize_node(*child, out_yaml);
    out_yaml << YAML::EndMap;
  }
  out_yaml << YAML::EndSeq;
}

}  // namespace

void serialize_node(const Node &node, YAML::Emitter &out_yaml)
{
  serialize_common_fields(node, out_yaml);
  bool include_references = true;

  switch (node.nodetype()) {
    case EP_DATANODE:
      serialize_data_node(static_cast<const DataNode &>(node), out_yaml);
      break;
    case EP_STRINGNODE:
      serialize_string_node(static_cast<const StringNode &>(node), out_yaml);
      break;
    case EP_OBJECTNODE:
      include_references = serialize_object_node(
          static_cast<const ObjectNode &>(node), out_yaml);
      break;
    default:
      std::cout << "[serialize_node] - Default case - nodetype not allowed"
                << std::endl;
      return;
  }

  if (include_references) serialize_references(node, out_yaml);
}

int serialize_message(const Message &msg, const std::string &filename)
{
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "item_count" << YAML::Value << msg.itemCount();
  out << YAML::Key << "message_size" << YAML::Value << msg.size();
  out << YAML::Key << "items";
  out << YAML::BeginSeq;

  for (size_t i = 0; i < msg.itemCount(); i++) {
    Node *node = msg.item(i);
    out << YAML::BeginMap;  // node properties
    serialize_node(*node, out);
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

// Helper function to create DataNode
std::unique_ptr<DataNode> create_data_node(
    const YAML::Node &node, const std::string &name, BaseType data_type,
    const std::vector<size_t> &array_dim, bool memory_managed,
    const std::string &tooltip, AccessType access_type, int /*id*/)
{
  std::unique_ptr<DataNode> dn;

  if (memory_managed) {
    void *value = parse_flatbuffer(node, data_type);
    if (!value) return nullptr;

    dn = std::make_unique<DataNode>(name, data_type, array_dim, value,
                                    memory_managed, tooltip, access_type);
    free_flatbuffer(value, data_type);
  }
  else {
    dn = std::make_unique<DataNode>(name, data_type, array_dim, nullptr,
                                    memory_managed, tooltip, access_type);
  }

  // if (id != -1) {
  //   dn->setId(id);
  // }

  if (node["references"] && node["references"].IsSequence()) {
    add_references(dn, node);
  }

  return dn;
}

// Helper function to create ObjectNode
std::unique_ptr<ObjectNode> create_object_node(const YAML::Node &node,
                                               const std::string &name,
                                               ObjectType object_type,
                                               const std::string &tooltip)
{
  if (object_type == EP_IMAGE_RAW) {
    return create_image_object(node, name);
  }
  else {
    auto obj = std::make_unique<ObjectNode>(name, object_type, tooltip);
    if (node["references"] && node["references"].IsSequence()) {
      add_references(obj, node);
    }
    return obj;
  }
}

// Helper function to create StringNode
std::unique_ptr<StringNode> create_string_node(const YAML::Node &node,
                                               const std::string &name,
                                               AccessType access_type,
                                               const std::string &tooltip)
{
  std::string value;
  if (node["value"]) value = node["value"].as<std::string>();

  auto sn = std::make_unique<StringNode>(name, value, access_type, tooltip);
  if (node["references"] && node["references"].IsSequence()) {
    add_references(sn, node);
  }
  return sn;
}

// Helper function to create ImageObject
std::unique_ptr<ObjectNode> create_image_object(const YAML::Node &node,
                                                const std::string &name)
{
  if (!node["width"] || !node["height"] || !node["pixel_format"]) {
    std::cerr << "Missing image properties in YAML" << std::endl;
    return nullptr;
  }

  int width = node["width"].as<int>();
  int height = node["height"].as<int>();
  int channels = node["channels"] ? node["channels"].as<int>() : 0;
  PixelFormat pixel_format =
      from_string_pixel_format(node["pixel_format"].as<std::string>());
  auto img = std::make_unique<ImageObject>(name, width, height, channels,
                                           pixel_format, nullptr);

  auto node_copy = img->copyNode();

  if (node["references"] && node["references"].IsSequence()) {
    add_references(node_copy, node);
  }

  return node_copy;
}

// Helper function to parse flat buffers safely
void *parse_flatbuffer(const YAML::Node &node, BaseType data_type)
{
  if (!node["value"]) return nullptr;

  switch (data_type) {
    case EP_8C:
      return parse_flat_buffer_from_nested_array<char>(
          node["value"].as<std::string>());
    case EP_8U:
      return parse_flat_buffer_from_nested_array<unsigned char>(
          node["value"].as<std::string>());
    case EP_8S:
      return parse_flat_buffer_from_nested_array<signed char>(
          node["value"].as<std::string>());
    case EP_16U:
      return parse_flat_buffer_from_nested_array<unsigned short>(
          node["value"].as<std::string>());
    case EP_16S:
      return parse_flat_buffer_from_nested_array<short>(
          node["value"].as<std::string>());
    case EP_32U:
      return parse_flat_buffer_from_nested_array<unsigned int>(
          node["value"].as<std::string>());
    case EP_32S:
      return parse_flat_buffer_from_nested_array<int>(
          node["value"].as<std::string>());
    case EP_64S:
      return parse_flat_buffer_from_nested_array<long>(
          node["value"].as<std::string>());
    case EP_64U:
      return parse_flat_buffer_from_nested_array<unsigned long>(
          node["value"].as<std::string>());
    case EP_32F:
      return parse_flat_buffer_from_nested_array<float>(
          node["value"].as<std::string>());
    case EP_64F:
      return parse_flat_buffer_from_nested_array<double>(
          node["value"].as<std::string>());
    case EP_BOOL:
      return parse_flat_buffer_from_nested_array<bool>(
          node["value"].as<std::string>());
    default:
      return nullptr;
  }
}

// Helper function to free parsed buffer
void free_flatbuffer(void *value, BaseType data_type)
{
  switch (data_type) {
    case EP_8C:
      delete[] static_cast<char *>(value);
      break;
    case EP_8U:
      delete[] static_cast<unsigned char *>(value);
      break;
    case EP_8S:
      delete[] static_cast<signed char *>(value);
      break;
    case EP_16U:
      delete[] static_cast<unsigned short *>(value);
      break;
    case EP_16S:
      delete[] static_cast<short *>(value);
      break;
    case EP_32U:
      delete[] static_cast<unsigned int *>(value);
      break;
    case EP_32S:
      delete[] static_cast<int *>(value);
      break;
    case EP_64S:
      delete[] static_cast<long *>(value);
      break;
    case EP_64U:
      delete[] static_cast<unsigned long *>(value);
      break;
    case EP_32F:
      delete[] static_cast<float *>(value);
      break;
    case EP_64F:
      delete[] static_cast<double *>(value);
      break;
    case EP_BOOL:
      delete[] static_cast<bool *>(value);
      break;
    default:
      break;
  }
}

std::unique_ptr<Node> deserialize_node(const YAML::Node &node)
{
  int id = -1;
  std::string name;
  NodeType node_type = EP_DATANODE;
  std::string tooltip;
  AccessType access_type = R;
  bool memory_managed = false;
  std::vector<size_t> array_dim = {1};  // Default dimension is 1
  BaseType data_type = EP_8C;
  ObjectType object_type = EP_OBJ;

  if (node["node_type"]) {
    node_type = static_cast<NodeType>(
        string_to_nodetype(node["node_type"].as<std::string>()));
  }
  else {
    std::cerr << "Unknown node type in YAML" << std::endl;
    return nullptr;
  }

  // if (node["id"]) id = node["id"].as<int>();
  if (node["name"]) name = node["name"].as<std::string>();
  if (node["tooltip"]) tooltip = node["tooltip"].as<std::string>();
  if (node["access_mode"])
    access_type = string_to_accesstype(node["access_mode"].as<std::string>());
  if (node["object_type"])
    object_type = string_to_objecttype(node["object_type"].as<std::string>());
  if (node["data_type"])
    data_type = string_to_basetype(node["data_type"].as<std::string>());

  if (node["dimensions"] && node["dimensions"].IsSequence()) {
    array_dim = parse_dimensions(node["dimensions"]);
  }

  if (node["managed"]) memory_managed = node["managed"].as<bool>();

  // Handle different node types
  if (node_type == EP_DATANODE) {
    return create_data_node(node, name, data_type, array_dim, memory_managed,
                            tooltip, access_type, id);
  }
  else if (node_type == EP_OBJECTNODE) {
    return create_object_node(node, name, object_type, tooltip);
  }
  else if (node_type == EP_STRINGNODE) {
    return create_string_node(node, name, access_type, tooltip);
  }

  std::cerr << "Error parsing YAML" << std::endl;
  return nullptr;
}

std::unique_ptr<Message> deserialize_message(const YAML::Node &node)
{
  auto msg = std::make_unique<Message>();  // Properly initialize msg
  if (node["items"] && node["items"].IsSequence()) {
    for (const auto &item : node["items"]) {
      auto n = deserialize_node(item);
      if (n) {
        msg->addItem(std::move(n));
      }
    }
  }
  return msg;
}

std::vector<size_t> parse_dimensions(const YAML::Node &dimensions_node)
{
  std::vector<size_t> dimensions;
  if (!dimensions_node.IsSequence()) return dimensions;

  for (const auto &dimension : dimensions_node) {
    dimensions.push_back(dimension.as<size_t>());
  }
  return dimensions;
}

std::string data_node_value_to_string(const DataNode &node)
{
  std::ostringstream oss;

  if (node.value() != nullptr) switch (node.datatype()) {
      case EP_8C:
        stream_values(oss, static_cast<char *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_8U:
        stream_values(oss, static_cast<unsigned char *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_8S:
        stream_values(oss, static_cast<signed char *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_16U:
        stream_values(oss, static_cast<unsigned short *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_16S:
        stream_values(oss, static_cast<short *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_32U:
        stream_values(oss, static_cast<unsigned int *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_32S:
        stream_values(oss, static_cast<int *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_64S:
        stream_values(oss, static_cast<long *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_64U:
        stream_values(oss, static_cast<unsigned long *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_32F:
        stream_values(oss, static_cast<float *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_64F:
        stream_values(oss, static_cast<double *>(node.value()),
                      node.arraydimensions());
        break;
      case EP_BOOL:
        stream_values(oss, static_cast<bool *>(node.value()),
                      node.arraydimensions());
        break;
      // case EP_STRING: {
      //   streamValues(oss, static_cast<std::string *>(node.value()),
      //                node.arraydimensions());
      // } break;
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
}  // namespace epf
