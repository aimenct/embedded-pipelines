// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "settings.h"

using namespace ep;

#define UNUSED(x) (void)(x)

template <typename T>
int32_t Settings2::addSetting(const std::string name, T &value,
                              const SettingType setting_type,
                              std::string tooltip, AccessType accessmode)
{
  // Check wether it should be added to the filter or device node tree.
  int32_t parent_node = 1;  //; = device_setting ? 2 : 1;

  // Device settings are memory managed by the NodeTree
  bool mem_managed = true;

  switch (setting_type) {
    case FILTER_SETTING:
      parent_node = 1;
      mem_managed = false;
      break;
    case FILTER_COMMAND:
      parent_node = 2;
      mem_managed = false;
      break;

    case DEVICE_SETTING:
      parent_node = 3;
      mem_managed = true;
      break;
    case QUEUE_SETTING:
      parent_node = 4;
      mem_managed = false;
      break;
  }

  // Variable declaration for the node addition.
  Node2 *new_node;
  bool unique_name;

  // Automatic internal type deduction
  if constexpr (std::is_same<T, bool>::value) {
    new_node = new DataNode(name, ep::EP_BOOL, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, char>::value) {
    new_node = new DataNode(name, ep::EP_8C, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, uint8_t>::value) {
    new_node = new DataNode(name, ep::EP_8U, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, int8_t>::value) {
    new_node = new DataNode(name, ep::EP_8S, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, uint16_t>::value) {
    new_node = new DataNode(name, ep::EP_16U, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, int16_t>::value) {
    new_node = new DataNode(name, ep::EP_16S, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, uint32_t>::value) {
    new_node = new DataNode(name, ep::EP_32U, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, int32_t>::value) {
    new_node = new DataNode(name, ep::EP_32S, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, int64_t>::value) {
    new_node = new DataNode(name, ep::EP_64S, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, uint64_t>::value) {
    new_node = new DataNode(name, ep::EP_64U, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, float>::value) {
    new_node = new DataNode(name, ep::EP_32F, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, double>::value) {
    new_node = new DataNode(name, ep::EP_64F, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  }
  else if constexpr (std::is_same<T, std::string>::value) {
    new_node = new DataNode(name, ep::EP_STRING, {1}, &value, mem_managed,
                            tooltip, false, accessmode);
    unique_name = setting_map_.try_emplace(name, new_node).second;
  } /*
   // else if constexpr (std::is_same<T, std::string>::value) {
   //   new_node = new StringNode(name, value, tootltip);
   //   unique_name = setting_map_.try_emplace(name, new_node).second;
   // }*/
  else {
    UNUSED(mem_managed);
    UNUSED(accessmode);
    std::cout << "addSetting: Type deduction failed for setting name: " << name
              << std::endl;
    return -1;
  }

  // Check if the node additon was completed.
  if (unique_name) {
    // When added, NodeTree acquires ownership of the dynamically allocated
    // node.
    add(new_node, ep::EP_HAS_CHILD, parent_node);
    return 0;
  }
  else {
    // Delete dynamically allocated node.
    delete new_node;
    std::cout << "addSetting: Setting name already in use." << std::endl;
    return -1;
  }
}

template <typename T>
int32_t Settings2::setValue(std::string name, T value)
{
  Node2 *node = (Node2 *)operator[](name);

  // If found...
  if (node) {
    if (node->nodetype() == ep::EP_DATANODE) {
      ep::DataNode *data_node = static_cast<ep::DataNode *>(node);
      BaseType datatype = data_node->datatype();

      // Automatic internal type deduction
      if (datatype == ep::EP_BOOL) {
        if constexpr (std::is_same<T, bool>::value) {
          *static_cast<bool *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<bool *>(data_node->value()) =
              YAML::Node(value).as<bool>();
          return 0;
        }
      }
      else if (datatype == ep::EP_8C) {
        if constexpr (std::is_same<T, char>::value) {
          *static_cast<char *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<char *>(data_node->value()) = *value.c_str();
          return 0;
        }
      }
      else if (datatype == ep::EP_8U) {
        if constexpr (std::is_same<T, uint8_t>::value) {
          *static_cast<uint8_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<uint8_t *>(data_node->value()) =
              static_cast<uint8_t>(std::stoul(value));
          return 0;
        }
      }
      else if (datatype == ep::EP_8S) {
        if constexpr (std::is_same<T, int8_t>::value) {
          *static_cast<int8_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<int8_t *>(data_node->value()) =
              static_cast<int8_t>(std::stoi(value));
          return 0;
        }
      }
      else if (datatype == ep::EP_16U) {
        if constexpr (std::is_same<T, uint16_t>::value) {
          *static_cast<uint16_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<uint16_t *>(data_node->value()) =
              static_cast<uint16_t>(std::stoul(value));
          return 0;
        }
      }
      else if (datatype == ep::EP_16S) {
        if constexpr (std::is_same<T, int16_t>::value) {
          *static_cast<int16_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<int16_t *>(data_node->value()) =
              static_cast<int16_t>(std::stoi(value));
          return 0;
        }
      }
      else if (datatype == ep::EP_32U) {
        if constexpr (std::is_same<T, uint32_t>::value) {
          *static_cast<uint32_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<uint32_t *>(data_node->value()) =
              static_cast<uint32_t>(std::stoul(value));
          return 0;
        }
      }
      else if (datatype == ep::EP_32S) {
        if constexpr (std::is_same<T, int32_t>::value) {
          *static_cast<int32_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<int32_t *>(data_node->value()) = std::stoi(value);
          return 0;
        }
      }
      else if (datatype == ep::EP_64U) {
        if constexpr (std::is_same<T, uint64_t>::value) {
          *static_cast<uint64_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<uint64_t *>(data_node->value()) = std::stoul(value);
          return 0;
        }
      }
      else if (datatype == ep::EP_64S) {
        if constexpr (std::is_same<T, int64_t>::value) {
          *static_cast<int64_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<int64_t *>(data_node->value()) = std::stol(value);
          return 0;
        }
      }
      else if (datatype == ep::EP_32F) {
        if constexpr (std::is_same<T, float>::value) {
          *static_cast<float *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<float *>(data_node->value()) = std::stof(value);
          return 0;
        }
      }
      else if (datatype == ep::EP_64F) {
        if constexpr (std::is_same<T, double>::value) {
          *static_cast<double *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<double *>(data_node->value()) = std::stod(value);
          return 0;
        }
      }
      else if (datatype == ep::EP_STRING) {
        if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<std::string *>(data_node->value()) = value;
          return 0;
        }
      }
      std::cout << "setSetting: Input type incompatible with datatype: "
                << basetype_to_string(datatype) << std::endl;
      return -1;
    }
    // else if (node->nodetype() == ep::EP_STRINGNODE) {
    //   edg e::StringNode *string_node = (ep::StringNode *)node;
    //   if constexpr (std::is_same<T, std::string>::value) {
    //     string_node->setValue(value);
    //     return 0;
    //   }
    // }
    else {
      std::cout << "setValue(): \"" << name << "\" is a command setting."
                << std::endl;
      return -1;
    }
  }
  else {
    std::cout << "setSetting: Setting key \"" << name << "\" not found. "
              << std::endl;
    return -1;
  }
  return -1;
}

template <typename T>
const T *Settings2::value(std::string name)
{
  const Node2 *node = operator[](name);

  // If found...
  if (node) {
    if (node->nodetype() == ep::EP_DATANODE) {
      const ep::DataNode *data_node =
          static_cast<const ep::DataNode *>(node);

      // Automatic internal type deduction
      if constexpr (std::is_same<T, bool>::value) {
        if (data_node->datatype() == ep::EP_BOOL) {
          return static_cast<bool *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, char>::value) {
        if (data_node->datatype() == ep::EP_8C) {
          return static_cast<char *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint8_t>::value) {
        if (data_node->datatype() == ep::EP_8U) {
          return static_cast<uint8_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int8_t>::value) {
        if (data_node->datatype() == ep::EP_8S) {
          return static_cast<int8_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint16_t>::value) {
        if (data_node->datatype() == ep::EP_16U) {
          return static_cast<uint16_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int16_t>::value) {
        if (data_node->datatype() == ep::EP_16S) {
          return static_cast<int16_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint32_t>::value) {
        if (data_node->datatype() == ep::EP_32U) {
          return static_cast<uint32_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int32_t>::value) {
        if (data_node->datatype() == ep::EP_32S) {
          return static_cast<int32_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint64_t>::value) {
        if (data_node->datatype() == ep::EP_64U) {
          return static_cast<uint64_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int64_t>::value) {
        if (data_node->datatype() == ep::EP_64S) {
          return static_cast<int64_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, float>::value) {
        if (data_node->datatype() == ep::EP_32F) {
          return static_cast<float *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, double>::value) {
        if (data_node->datatype() == ep::EP_64F) {
          return static_cast<double *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, std::string>::value) {
        if (data_node->datatype() == ep::EP_STRING) {
          return static_cast<std::string *>(data_node->value());
        }
      }
      T print_type_holder;
      std::cout << "setting: \"" << name << "\" is a "
                << basetype_to_string(data_node->datatype())
                << ". Trying to access it using \""
                << typeid(print_type_holder).name() << "\". Returning nullptr."
                << std::endl;
      return nullptr;
    }
    // else if (node->nodetype() == ep::EP_STRINGNODE) {
    //   ep::StringNode *string_node = (ep::StringNode *)node;
    //   if constexpr (std::is_same<T, std::string>::value) {
    //     return &string_node->value();
    //   }
    //   T print_type_holder;
    //   std::cout << "setting: \"" << name
    //             << "\" is a String. Trying to access it using \""
    //             << typeid(print_type_holder).name()
    //             << "\". Returning nullptr." << std::endl;
    //   return nullptr;
    // }
    else {
      std::cout << "value(): \"" << name << "\" is a command setting."
                << std::endl;
      return nullptr;
    }
  }
  std::cout << "value: Setting key \"" << name << "\" not found. " << std::endl;
  return nullptr;
}
