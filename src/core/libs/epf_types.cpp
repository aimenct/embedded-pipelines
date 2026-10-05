// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "epf_types.h"

namespace epf {

size_t type_size(BaseType type)
{
  if ((type == EP_8C) || (type == EP_8U) || (type == EP_8S))
    return sizeof(char);
  else if ((type == EP_16U) || (type == EP_16S))
    return sizeof(short int);
  else if ((type == EP_32U) || (type == EP_32S) || (type == EP_32F))
    return sizeof(int);
  else if ((type == EP_64U) || (type == EP_64S) || (type == EP_64F))
    return sizeof(double);
  else if (type == EP_BOOL)
    return sizeof(bool);
  // else if (type == EP_STRING)
  //   return sizeof(std::string);
  return 0;
}

int basetype_range(BaseType type, double &min, double &max)
{
  switch (type) {
    case EP_8C:  // char_t (signed char)
      min = std::numeric_limits<int8_t>::min();
      max = std::numeric_limits<int8_t>::max();
      return 0;
    case EP_8U:  // uint8_t
      min = 0;
      max = std::numeric_limits<uint8_t>::max();
      return 0;
    case EP_8S:  // int8_t
      min = std::numeric_limits<int8_t>::min();
      max = std::numeric_limits<int8_t>::max();
      return 0;
    case EP_16U:  // uint16_t
      min = 0;
      max = std::numeric_limits<uint16_t>::max();
      return 0;
    case EP_16S:  // int16_t
      min = std::numeric_limits<int16_t>::min();
      max = std::numeric_limits<int16_t>::max();
      return 0;
    case EP_32U:  // uint32_t
      min = 0;
      max = std::numeric_limits<uint32_t>::max();
      return 0;
    case EP_32S:  // int32_t
      min = std::numeric_limits<int32_t>::min();
      max = std::numeric_limits<int32_t>::max();
      return 0;
    case EP_64S:  // int64_t
      min = static_cast<double>(std::numeric_limits<int64_t>::min());
      max = static_cast<double>(std::numeric_limits<int64_t>::max());
      return 0;
    case EP_64U:  // uint64_t
      min = 0.0;
      max = static_cast<double>(std::numeric_limits<uint64_t>::max());
      return 0;
    case EP_32F:  // float
      min = std::numeric_limits<float>::lowest();
      max = std::numeric_limits<float>::max();
      return 0;
    case EP_64F:  // double
      min = std::numeric_limits<double>::lowest();
      max = std::numeric_limits<double>::max();
      return 0;
    case EP_BOOL:  // bool
      min = 0.0;
      max = 1.0;
      return 0;
    // case EP_STRING:
    //   std::cerr << " string (no numeric range) " << std::endl;
    //   return -1;     // Indicate error for non-numeric type
    default:
      std::cerr << "Unsupported type" << std::endl;
      return -1;
  }
}

std::string state2string(const epf::FilterState &state)
{
  switch (state) {
    case FilterState::CONNECTED:
      return "connected";
      break;
    case FilterState::DISCONNECTED:
      return "disconnected";
      break;
    case FilterState::SET:
      return "set";
      break;
    case FilterState::RUNNING:
      return "running";
      break;
    case FilterState::STOP_REQUEST:
      return "stop_request";
      break;
  }
  return "";
}

std::string basetype_to_string(BaseType type)
{
  switch (type) {
    case EP_8C:
      return "char";

    case EP_8U:
      return "uint8_t";

    case EP_8S:
      return "int8_t";

    case EP_16U:
      return "uint16_t";

    case EP_16S:
      return "int16_t";

    case EP_32U:
      return "uint32_t";

    case EP_32S:
      return "int32_t";

    case EP_64U:
      return "uint64_t";

    case EP_64S:
      return "int64_t";

    case EP_32F:
      return "float";

    case EP_64F:
      return "double";

      // case EP_STRING:
      //   return "string";

    case EP_BOOL:
      return "bool";
    default:
      return "Unknown type";
  }
}

// Function to convert AccessType to a string representation using streams
std::string accesstype_to_string(AccessType access)
{
  std::ostringstream oss;
  bool is_first{true};  // To handle the separator correctly

  // Check each flag and append its string representation to the stream
  if (access == R) {
    return "R";  // Special case for READ, which is 0 and would otherwise be
                 // skipped
  }
  if (access & W) {
    oss << (is_first ? "" : " | ") << "W";

    access = static_cast<AccessType>(
        access & ~W);  // Remove W to avoid repeating it in the combination

    is_first = false;
  }
  if (access & W_D) {
    oss << (is_first ? "" : " | ") << "W_D";
    is_first = false;
  }
  if (access & W_C) {
    oss << (is_first ? "" : " | ") << "W_C";
    is_first = false;
  }
  if (access & W_S) {
    oss << (is_first ? "" : " | ") << "W_S";
    is_first = false;
  }
  if (access & W_R) {
    oss << (is_first ? "" : " | ") << "W_R";
  }

  return oss.str();
}

// Function to convert a string representation to AccessType bitmask
AccessType string_to_accesstype(const std::string &str)
{
  // Mapping from string representation to AccessType values
  std::unordered_map<std::string, AccessType> access_map = {
      {"R", R},     {"W", W},     {"W_D", W_D},
      {"W_C", W_C}, {"W_S", W_S}, {"W_R", W_R}};

  AccessType access = R;  // Default to READ if no other flags are set
  std::istringstream iss(str);
  std::string token;

  while (std::getline(iss, token, '|')) {
    // Trim whitespace from token
    token.erase(0, token.find_first_not_of(" \t"));
    token.erase(token.find_last_not_of(" \t") + 1);

    // Check if the token is in the access_map and update access
    auto it = access_map.find(token);
    if (it != access_map.end()) {
      access = static_cast<AccessType>(
          access | it->second);  // Use bitwise OR to combine AccessType values
    }
  }

  return access;
}

std::string nodetype_to_string(NodeType type)
{
  std::string name;
  switch (type) {
    case EP_DATANODE:
      return "data";

    case EP_STRINGNODE:
      return "string";

    case EP_COMMANDNODE:
      return "command";

    case EP_OBJECTNODE:
      return "object";
    default:
      return "Unknown Type";
  }
}

std::string objecttype_to_string(int type)
{
  std::string name;
  switch (type) {
    case EP_OBJ:
      return "default";
    case EP_IMAGE_RAW:
      return "image";
    case EP_IMAGE_COMPRESSED:
      return "compressed_image";
    default:
      return "Unknown Type";
  }
}

BaseType string_to_basetype(const std::string &name)
{
  if (name == "char") {
    return EP_8C;
  }
  else if (name == "uint8_t") {
    return EP_8U;
  }
  else if (name == "int8_t") {
    return EP_8S;
  }
  else if (name == "uint16_t") {
    return EP_16S;
  }
  else if (name == "int16_t") {
    return EP_16U;
  }
  else if (name == "uint32_t") {
    return EP_32U;
  }
  else if (name == "int32_t") {
    return EP_32S;
  }
  else if (name == "uint64_t") {
    return EP_64U;
  }
  else if (name == "int64_t") {
    return EP_64S;
  }
  else if (name == "float") {
    return EP_32F;
  }
  else if (name == "double") {
    return EP_64F;
  }
  else if (name == "bool") {
    return EP_BOOL;
  }
  else {
    std::cout << "unrecognised base type" << std::endl;
    exit(0);
  }
}

NodeType string_to_nodetype(const std::string &name)
{
  if (name == "data") {
    return EP_DATANODE;
  }
  else if (name == "string") {
    return EP_STRINGNODE;
  }
  else if (name == "command") {
    return EP_COMMANDNODE;
  }
  else if (name == "object") {
    return EP_OBJECTNODE;
  }
  else {
    std::cout << "unrecognised nodetype" << std::endl;
    exit(0);
  }
}

ObjectType string_to_objecttype(const std::string &name)
{
  if (name == "default") {
    return EP_OBJ;
  }
  else if (name == "image") {
    return EP_IMAGE_RAW;
  }
  else if (name == "compressed_image") {
    return EP_IMAGE_COMPRESSED;
  }
  else
    return std::stoi(name);
}

// Function to convert RefType to string
std::string reftype_to_string(RefType refType)
{
  switch (refType) {
    case EP_HAS_CHILD:
      return "child";
    case EP_HAS_PROPERTY:
      return "property";
    case EP_HAS_DATA:
      return "data";
    case EP_HAS_COMMANDPARAMETER:
      return "commandparameter";
    case EP_HAS_MAX:
      return "max";
    case EP_HAS_MIN:
      return "min";
    case EP_HAS_INCREMENT:
      return "increment";
    case EP_HAS_ENUMVALUE:
      return "enumvalue";
    default:
      return "Unknown RefType";
  }
}

RefType string_to_reftype(const std::string &type)
{
  static const std::unordered_map<std::string, RefType> ref_type_map = {
      {"child", EP_HAS_CHILD},
      {"property", EP_HAS_PROPERTY},
      {"data", EP_HAS_DATA},
      {"commandparameter", EP_HAS_COMMANDPARAMETER},
      {"max", EP_HAS_MAX},
      {"min", EP_HAS_MIN},
      {"increment", EP_HAS_INCREMENT},
      {"enumvalue", EP_HAS_ENUMVALUE}};

  auto it = ref_type_map.find(type);
  if (it != ref_type_map.end()) {
    return it->second;
  }
  else {
    throw std::invalid_argument("Invalid RefType name: " + type);
  }
}

std::string settingtype_to_string(SettingType type)
{
  switch (type) {
    case BASE_SETTING:
      return "base";
    case DEVICE_SETTING:
      return "device";
    case CONTROL_SETTING:
      return "control";
    case SOURCE_PORT_SETTING:
      return "source_ports";
    case SINK_PORT_SETTING:
      return "sink_ports";
    default:
      std::cout << "setting type uknown" << std::endl;
      return "unknown";
  }
}

SettingType string_to_settingtype(const std::string &str)
{
  static const std::unordered_map<std::string, SettingType> str_to_enum = {
      {"base", BASE_SETTING},
      {"device", DEVICE_SETTING},
      {"control", CONTROL_SETTING},
      {"source_ports", SOURCE_PORT_SETTING},
      {"sink_ports", SINK_PORT_SETTING}};

  auto it = str_to_enum.find(str);
  if (it != str_to_enum.end()) {
    return it->second;
  }
  throw std::invalid_argument("Invalid SettingType string: " + str);
}

}  // namespace epf
