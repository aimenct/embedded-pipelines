// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "opcua_data.h"


ep::BaseType ep::opcuatype_to_basetype(const UA_DataType* datatype)
{
  using namespace ep;
  if (datatype == &UA_TYPES[UA_TYPES_BOOLEAN]) {
    return EP_BOOL;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_SBYTE]) {
    return EP_8S;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_BYTE]) {
    return EP_8U;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_INT16]) {
    return EP_16S;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_UINT16]) {
    return EP_16U;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_INT32]) {
    return EP_32S;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_UINT32]) {
    return EP_32U;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_INT64]) {
    return EP_64S;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_UINT64]) {
    return EP_64U;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_FLOAT]) {
    return EP_32F;
  }
  else if (datatype == &UA_TYPES[UA_TYPES_DOUBLE]) {
    return EP_64F;
  }
  else {
    return EP_32S;
  }
}

const UA_DataType* ep::basetype_to_opcuatype(ep::BaseType datatype)
{
  using namespace ep;
  switch (datatype) {
    case EP_BOOL:
      return &UA_TYPES[UA_TYPES_BOOLEAN];
      break;
    case EP_8C:
      return &UA_TYPES[UA_TYPES_BYTE];
      break;
    case EP_8U:
      return &UA_TYPES[UA_TYPES_BYTE];
      break;
    case EP_8S:
      return &UA_TYPES[UA_TYPES_SBYTE];
      break;
    case EP_16U:
      return &UA_TYPES[UA_TYPES_UINT16];
      break;
    case EP_16S:
      return &UA_TYPES[UA_TYPES_INT16];
      break;
    case EP_32U:
      return &UA_TYPES[UA_TYPES_UINT32];
      break;
    case EP_32S:
      return &UA_TYPES[UA_TYPES_INT32];
      break;
    case EP_64U:
      return &UA_TYPES[UA_TYPES_UINT64];
      break;
    case EP_64S:
      return &UA_TYPES[UA_TYPES_INT64];
      break;
    case EP_32F:
      return &UA_TYPES[UA_TYPES_FLOAT];
      break;
    case EP_64F:
      return &UA_TYPES[UA_TYPES_DOUBLE];
      break;
    case EP_STRING:
      return &UA_TYPES[UA_TYPES_STRING];
      break;
    default:
      return nullptr;
  }
}