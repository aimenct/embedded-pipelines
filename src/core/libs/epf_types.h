// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EPF_TYPES_H
#define _EPF_TYPES_H

/** @file epf_types.h
 * @brief Embedded Pipelines Types
 */

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace epf {

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
  // EP_STRING = 12
};

// Enum to represent the filter states.
enum FilterState {
  DISCONNECTED = 0b0000001,
  CONNECTED = 0b0000010,
  SET = 0b0000100,
  RUNNING = 0b0001000,
  STOP_REQUEST = 0b0010000,
};

/**
 * @brief Enum to represent different job execution models for the filter.
 */
enum JobExecutionModel { EXTERNAL_THREAD, OWN_THREAD, MAIN_LOOP };

enum QueueType { fifo, lifo };
enum ScheduleMode { push, pull };

/*
 * Proposed strongly typed replacements for the Queue's raw character states.
 *
 * These values intentionally retain the current character representation to
 * make a future migration easier. They are placeholders and are not yet used
 * by Queue, Qproducer, or Qconsumer.
 */
enum class QueueState : char {
  Connected = 'c',
  Disconnected = 'd',
};

enum class QueueEndpointState : char {
  Subscribed = 's',
  Unsubscribed = 'u',
  Working = 'w',
};

/* Queue - return error definition */
constexpr int QE_OK = 0;              // success
constexpr int QE_NOT_PERMITTED = -1;  // operation not permitted
constexpr int QE_DISABLED = -2;       // queue disabled

/*Refernce Types to indicate the relation of a parent node with its childs*/
enum RefType {
  EP_HAS_CHILD = 0,    /*!< Simple child relation */
  EP_HAS_PROPERTY = 1, /*!< Property relation */
  EP_HAS_DATA = 2,     /*!< Relation to indicate where to find its data */
  EP_HAS_COMMANDPARAMETER = 3, /*!< Relation to indicate which command. */
  EP_HAS_MAX = 4,              /*!< Relation to indicate the maximum value */
  EP_HAS_MIN = 5,              /*!< Relation to indicate the minimum value*/
  EP_HAS_INCREMENT = 6,        /*!< Relation to indicate the increment value*/
  EP_HAS_ENUMVALUE = 7,        /*!< Indicates that is one of its enum values*/
};

// Node Types
enum NodeType {
  EP_DATANODE = 0,
  EP_STRINGNODE = 1,
  EP_COMMANDNODE = 2,
  EP_OBJECTNODE = 3
};

// Object Types
typedef int ObjectType;
constexpr ObjectType EP_OBJ = 0;
constexpr ObjectType EP_IMAGE_RAW = 1;
constexpr ObjectType EP_IMAGE_COMPRESSED = 2;

// AccesType
// EP_IMAGE_RAW
// Name: Object Node
// - Width: DataNode ( int32_t )
// - Height: DataNode ( int32_t )
// - Channels: DataNode ( int32_t )
// - PixelFormat: DataNode ( int32_t / enum  )
// - data: DataNode ( unsigned char array )

// WRITE is always forbidden in STOP_REQUEST
enum AccessType : uint8_t {
  R = 0b0000000,              // READ
  W_D = 0b0000001,            // WRITE IF DISCONNECTED
  W_C = 0b0000010,            // WRITE IF CONNECTED
  W_S = 0b0000100,            // WRITE IF SETTED
  W_R = 0b0001000,            // WRITE IF RUNNING
  W = W_D | W_C | W_S | W_R,  // WRITE
};

constexpr AccessType operator|(AccessType acc_a, AccessType acc_b) noexcept
{
  return static_cast<AccessType>(static_cast<uint8_t>(acc_a) |
                                 static_cast<uint8_t>(acc_b));
}

inline bool access_allowed_in_state(AccessType access, FilterState state)
{
  return (static_cast<int>(access) & static_cast<int>(state)) != 0;
}

// Visibility
enum VisibilityType {
  EP_BEGINNER = 1,
  EP_EXPERT = 2,
  EP_GURU = 3,
  EP_ENUMENTRY = 4
};

// Types of settings available.
enum SettingType {
  BASE_SETTING = 1,    /**< Base setting type. */
  DEVICE_SETTING = 2,  /**< Device-specific setting type. */
  CONTROL_SETTING = 3, /**< Filter control commands. */
  SOURCE_PORT_SETTING = 4,
  SINK_PORT_SETTING = 5,
  SETTING_TYPE_COUNT  // sentinel value
};

using FilterId = uint64_t;

enum class SettingsChangeKind {
  Unknown = 0,
  Added,
  Updated,
  CommandAdded,
  Loaded,
  StructureChanged
};

// Returns the size in bytes of a given BaseType.
size_t type_size(BaseType type);

// Fills min/max for the given BaseType (returns 0 on success)
int basetype_range(BaseType type, double &min, double &max);

std::string state2string(const FilterState &state);

std::string basetype_to_string(BaseType type);
std::string nodetype_to_string(NodeType type);
std::string accesstype_to_string(AccessType type);
std::string settingtype_to_string(SettingType type);
std::string objecttype_to_string(ObjectType type);
std::string reftype_to_string(RefType type);

BaseType string_to_basetype(const std::string &name);
NodeType string_to_nodetype(const std::string &name);
AccessType string_to_accesstype(const std::string &str);
SettingType string_to_settingtype(const std::string &str);
ObjectType string_to_objecttype(const std::string &type);
RefType string_to_reftype(const std::string &name);

}  // namespace epf

#endif  // _EPF_TYPES_H
