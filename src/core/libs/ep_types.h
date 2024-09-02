// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_TYPES_H
#define _EP_TYPES_H

/** @file ep_types.h
 * @brief Embedded Pipelines Types
 */

#include <float.h>
#include <limits.h>

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ep {

enum BaseType {
  /* fundamental types native to the computer */
  EP_8C = 7,     // char_t           - 1
  EP_8U = 0,     // uint8_t          - 1
  EP_8S = 1,     // int8_t           - 1
  EP_16U = 2,    // uint16_t         - 2
  EP_16S = 3,    // int16_t          - 2
  EP_32U = 8,    // uint32_t         - 4
  EP_32S = 4,    // int32_t          - 4
  EP_64S = 9,    // int64_t          - 8
  EP_64U = 10,   // uint64_t         - 8
  EP_32F = 5,    // float            - 4
  EP_64F = 6,    // double           - 8
  EP_BOOL = 11,  // bool
  /* object data types */
  EP_STRING = 12
};

/*Refernce Types to indicate the relation of a parent node with its childs*/
enum RefType {
  EP_HAS_CHILD = 0,    /*!< Simple child relation */
  EP_HAS_PROPERTY = 1, /*!< Property relation */
  EP_HAS_DATA = 2,     /*!< Relation to indicate where to find its data */
  EP_HAS_COMMANDPARAMETER = 3, /*!< Relation to indicate which command. */
  EP_HAS_MAX = 4,              /*!< Relation to indicate the maximum value */
  EP_HAS_MIN = 5,              /*!< Relation to indicate the minimum value*/
  EP_HAS_INCREMENT = 6,        /*!< Relation to indicate the increment value*/
  EP_HAS_ENUMVALUE =
      7, /*!< Relation to indicate that is one of its enum values*/
};

/*Node Types*/
enum NodeType {
  EP_DATANODE = 0,
  EP_STRINGNODE = 1,
  EP_COMMANDNODE = 2,
  EP_OBJECTNODE = 3
};

/*Object Types*/
typedef int ObjectType;
constexpr int EP_OBJ = 0;
constexpr int EP_IMAGE_OBJ = 1;

/*AccesType*/
enum AccessType {
  R = 0b0000000,    // READ
  W = 0b0011111,    // WRITE
  W_D = 0b0000001,  // WRITE IF DISCONNECTED
  W_C = 0b0000010,  // WRITE IF CONNECTED
  W_S = 0b0000100,  // WRITE IF SETTED
  W_R = 0b0001000,  // WRITE IF RUNNING
};

/*Visibility*/
enum VisibilityType {
  EP_BEGINNER = 1,
  EP_EXPERT = 2,
  EP_GURU = 3,
  EP_ENUMENTRY = 4
};

int type_size(BaseType type);

std::string basetype_to_string(BaseType type);
std::string nodetype_to_string(NodeType type);
std::string accesstype_to_string(AccessType type);
std::string objecttype_to_string(ObjectType type);
std::string reftype_to_string(RefType type);

BaseType string_to_basetype(const std::string &name);
NodeType string_to_nodetype(const std::string &name);
AccessType string_to_accesstype(const std::string &str);
ObjectType string_to_objecttype(const std::string &type);
RefType string_to_reftype(const std::string &name);

}  // namespace ep

#endif  // _EP_TYPES_H
