// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_OPCUA_DATA_H
#define _EP_OPCUA_DATA_H

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "core.h"

namespace ep {

BaseType opcuatype_to_basetype(const UA_DataType* datatype);

const UA_DataType* basetype_to_opcuatype(BaseType datatype);

}

#endif //_EP_OPCUA_DATA_H
