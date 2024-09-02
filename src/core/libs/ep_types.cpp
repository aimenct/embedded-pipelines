// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ep_types.h"

namespace ep {

int type_size(BaseType type)
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
  else if (type == EP_STRING)
    return sizeof(std::string);
  return -1;
}

std::string basetype_to_string(BaseType type)
{
  switch (type) {
    case EP_8C:
      return "char";

    case EP_8U:
      return "unsinged char";

    case EP_8S:
      return "signed char";

    case EP_16U:
      return "unsigned short";

    case EP_16S:
      return "short int";

    case EP_32U:
      return "unsigned int";

    case EP_32S:
      return "int";

    case EP_64U:
      return "unsigned long";

    case EP_64S:
      return "long";

    case EP_32F:
      return "float";

    case EP_64F:
      return "double";

    case EP_STRING:
      return "string";

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
  bool isFirst = true;  // To handle the separator correctly

  // Check each flag and append its string representation to the stream
  if (access == R) {
    return "R";  // Special case for READ, which is 0 and would otherwise be
                 // skipped
  }
  if (access & W) {
    oss << (isFirst ? "" : " | ") << "W";

    access = static_cast<AccessType>(
        access & ~W);  // Remove W to avoid repeating it in the combination

    isFirst = false;
  }
  if (access & W_D) {
    oss << (isFirst ? "" : " | ") << "W_D";
    isFirst = false;
  }
  if (access & W_C) {
    oss << (isFirst ? "" : " | ") << "W_C";
    isFirst = false;
  }
  if (access & W_S) {
    oss << (isFirst ? "" : " | ") << "W_S";
    isFirst = false;
  }
  if (access & W_R) {
    oss << (isFirst ? "" : " | ") << "W_R";
  }

  return oss.str();
}

// Function to convert a string representation to AccessType bitmask
AccessType string_to_accesstype(const std::string& str)
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
    case EP_IMAGE_OBJ:
      return "image";
    default:
      return "Unknown Type";
  }
}

BaseType string_to_basetype(const std::string& name)
{
  if (name == "char") {
    return EP_8C;
  }
  else if (name == "unsigned char") {
    return EP_8U;
  }
  else if (name == "signed char") {
    return EP_8S;
  }
  else if (name == "short") {
    return EP_16S;
  }
  else if (name == "unsigned short") {
    return EP_16U;
  }
  else if (name == "int") {
    return EP_32S;
  }
  else if (name == "unsigned int") {
    return EP_32U;
  }
  else if (name == "unsigned long") {
    return EP_64U;
  }
  else if (name == "long") {
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
  else if (name == "string") {
    return EP_STRING;
  }
  else {
    std::cout << "unrecognised base type" << std::endl;
    exit(0);
  }
}

NodeType string_to_nodetype(const std::string& name)
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

ObjectType string_to_objecttype(const std::string& name)
{
  if (name == "default") {
    return EP_OBJ;
  }
  else if (name == "image") {
    return EP_IMAGE_OBJ;
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

RefType string_to_reftype(const std::string& type)
{
  static const std::unordered_map<std::string, RefType> refTypeMap = {
      {"child", EP_HAS_CHILD},
      {"property", EP_HAS_PROPERTY},
      {"data", EP_HAS_DATA},
      {"commandparameter", EP_HAS_COMMANDPARAMETER},
      {"max", EP_HAS_MAX},
      {"min", EP_HAS_MIN},
      {"increment", EP_HAS_INCREMENT},
      {"enumvalue", EP_HAS_ENUMVALUE}};

  auto it = refTypeMap.find(type);
  if (it != refTypeMap.end()) {
    return it->second;
  }
  else {
    throw std::invalid_argument("Invalid RefType name: " + type);
  }
}

}  // namespace ep
