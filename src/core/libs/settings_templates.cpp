// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "settings.h"

using namespace epf;

#define UNUSED(x) (void)(x)

template <typename T>
int32_t Settings::addSettingUnder(const std::string name, T &value,
                                  const int32_t parent_index,
                                  std::string tooltip, AccessType accessmode)
{
  // Device settings are memory managed by the NodeTree
  bool mem_managed = parent_index == DEVICE_SETTING ? true : false;

  // Set accessmode based on const/non-const
  AccessType amode_internal;
  if constexpr (std::is_const<T>::value) {
    amode_internal = epf::R;
  }
  else {
    amode_internal = accessmode;
  }
  using BaseT = typename std::remove_const<T>::type;

  // Variable declaration for the node addition.
  std::unique_ptr<Node> node;
  std::vector<size_t> dims = {1};

  // Automatic internal type deduction
  if constexpr (std::is_same<BaseT, bool>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_BOOL, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, char>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_8C, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, uint8_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_8U, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, int8_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_8S, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, uint16_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_16U, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, int16_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_16S, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, uint32_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_32U, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, int32_t>::value ||
                     std::is_same<BaseT, epf::QueueType>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_32S, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, int64_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_64S, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, uint64_t>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_64U, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, float>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_32F, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, double>::value) {
    node = std::make_unique<DataNode>(name, epf::EP_64F, dims,
                                      const_cast<BaseT *>(&value), mem_managed,
                                      tooltip, amode_internal);
  }
  else if constexpr (std::is_same<BaseT, std::string>::value) {
    node = mem_managed
               ? std::make_unique<StringNode>(name, value, accessmode, tooltip)
               : std::make_unique<StringNode>(name, const_cast<BaseT *>(&value),
                                              accessmode, tooltip);

  } /*
   // else if constexpr (std::is_same<T, std::string>::value) {
   //   new_node = new StringNode(name, value, tootltip);
   //   unique_name = setting_map_.try_emplace(name, new_node).second;
   // }*/
  else {
    UNUSED(mem_managed);
    // UNUSED(accessmode);
    std::clog
        << "WARNING: settings::addSetting failed. \"" << name
        << "\" setting in \"" << this->name()
        << "\" filter. Type deduction failed, setting datatype not supported. "
        << std::endl;

    return -1;
  }

  return addSettingNode(std::move(node), parent_index);
}

template <typename T>
int32_t Settings::addSetting(const std::string name, T &value,
                             const SettingType type, std::string tooltip,
                             AccessType accessmode)
{
  auto index = settingIndex(settingtype_to_string(type));
  return addSettingUnder(name, value, index, tooltip, accessmode);
}

template <typename T>
int32_t Settings::setValue(std::string key_name, T value)
{
  int32_t index = settingIndex(key_name);
  Node *node = (Node *)operator[](index);

  // If found...
  if (node) {
    if (node->nodetype() == epf::EP_DATANODE) {
      epf::DataNode *data_node = static_cast<epf::DataNode *>(node);
      BaseType datatype = data_node->datatype();

      // Automatic internal type deduction
      if (datatype == epf::EP_BOOL) {
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
      else if (datatype == epf::EP_8C) {
        if constexpr (std::is_same<T, char>::value) {
          *static_cast<char *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<char *>(data_node->value()) = *value.c_str();
          return 0;
        }
      }
      else if (datatype == epf::EP_8U) {
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
      else if (datatype == epf::EP_8S) {
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
      else if (datatype == epf::EP_16U) {
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
      else if (datatype == epf::EP_16S) {
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
      else if (datatype == epf::EP_32U) {
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
      else if (datatype == epf::EP_32S) {
        if constexpr (std::is_same<T, int32_t>::value) {
          if (this->hasEnumOptions(key_name)) {
            const auto options = this->enumOptions(key_name);
            auto it = std::find_if(
                options.begin(), options.end(),
                [value](const auto &option) { return option.value == value; });
            if (it == options.end()) return -1;
          }
          *static_cast<int32_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          if (this->hasEnumOptions(key_name)) {
            if (auto maybe_enum_value =
                    this->enumValueForLabel(key_name, value);
                maybe_enum_value.has_value()) {
              *static_cast<int32_t *>(data_node->value()) = *maybe_enum_value;
              return 0;
            }

            int32_t parsed_value = 0;
            try {
              size_t idx = 0;
              parsed_value = std::stoi(value, &idx);
              if (idx != value.size()) return -1;
            }
            catch (const std::exception &) {
              return -1;
            }

            const auto options = this->enumOptions(key_name);
            auto it = std::find_if(options.begin(), options.end(),
                                   [parsed_value](const auto &option) {
                                     return option.value == parsed_value;
                                   });
            if (it == options.end()) return -1;

            *static_cast<int32_t *>(data_node->value()) = parsed_value;
            return 0;
          }

          *static_cast<int32_t *>(data_node->value()) = std::stoi(value);
          return 0;
        }
      }
      else if (datatype == epf::EP_64U) {
        if constexpr (std::is_same<T, uint64_t>::value) {
          *static_cast<uint64_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<uint64_t *>(data_node->value()) = std::stoul(value);
          return 0;
        }
      }
      else if (datatype == epf::EP_64S) {
        if constexpr (std::is_same<T, int64_t>::value) {
          *static_cast<int64_t *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<int64_t *>(data_node->value()) = std::stol(value);
          return 0;
        }
      }
      else if (datatype == epf::EP_32F) {
        if constexpr (std::is_same<T, float>::value) {
          *static_cast<float *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<float *>(data_node->value()) = std::stof(value);
          return 0;
        }
      }
      else if (datatype == epf::EP_64F) {
        if constexpr (std::is_same<T, double>::value) {
          *static_cast<double *>(data_node->value()) = value;
          return 0;
        }
        else if constexpr (std::is_same<T, std::string>::value) {
          *static_cast<double *>(data_node->value()) = std::stod(value);
          return 0;
        }
      }
      // else if (datatype == epf::EP_STRING) {
      //   if constexpr (std::is_same<T, std::string>::value) {
      //     *static_cast<std::string *>(data_node->value()) = value;
      //     return 0;
      //   }
      // }
      std::clog << "WARNING: settings::setSetting failed. \"" << key_name
                << "\" setting in \"" << this->name()
                << "\" filter. Input type incompatible with datatype: "
                << basetype_to_string(datatype) << std::endl;
      return -1;
    }
    else if (node->nodetype() == epf::EP_STRINGNODE) {
      StringNode *string_node = dynamic_cast<StringNode *>(node);
      if constexpr (std::is_same<T, std::string>::value) {
        *string_node->value() = value;
        return 0;
      }
    }
    else {
      std::clog << "WARNING: settings::setSetting failed. \"" << key_name
                << "\" setting in \"" << this->name()
                << "\" filter. Command setting." << std::endl;
      return -1;
    }
  }
  else {
    std::clog << "WARNING: settings::setSetting failed. \"" << key_name
              << "\" setting in \"" << this->name() << "\" filter. Not found."
              << std::endl;
    return -1;
  }
  return -1;
}

template <typename T>
const T *Settings::value(std::string key_name) const
{
  int32_t index = settingIndex(key_name);
  Node *node = (Node *)operator[](index);

  // If found...
  if (node) {
    if (node->nodetype() == epf::EP_DATANODE) {
      const epf::DataNode *data_node = static_cast<const epf::DataNode *>(node);

      // Automatic internal type deduction
      if constexpr (std::is_same<T, bool>::value) {
        if (data_node->datatype() == epf::EP_BOOL) {
          return static_cast<bool *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, char>::value) {
        if (data_node->datatype() == epf::EP_8C) {
          return static_cast<char *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint8_t>::value) {
        if (data_node->datatype() == epf::EP_8U) {
          return static_cast<uint8_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int8_t>::value) {
        if (data_node->datatype() == epf::EP_8S) {
          return static_cast<int8_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint16_t>::value) {
        if (data_node->datatype() == epf::EP_16U) {
          return static_cast<uint16_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int16_t>::value) {
        if (data_node->datatype() == epf::EP_16S) {
          return static_cast<int16_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint32_t>::value) {
        if (data_node->datatype() == epf::EP_32U) {
          return static_cast<uint32_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int32_t>::value) {
        if (data_node->datatype() == epf::EP_32S) {
          return static_cast<int32_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, uint64_t>::value) {
        if (data_node->datatype() == epf::EP_64U) {
          return static_cast<uint64_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, int64_t>::value) {
        if (data_node->datatype() == epf::EP_64S) {
          return static_cast<int64_t *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, float>::value) {
        if (data_node->datatype() == epf::EP_32F) {
          return static_cast<float *>(data_node->value());
        }
      }
      else if constexpr (std::is_same<T, double>::value) {
        if (data_node->datatype() == epf::EP_64F) {
          return static_cast<double *>(data_node->value());
        }
      }
      // else if constexpr (std::is_same<T, std::string>::value) {
      //   if (data_node->datatype() == epf::EP_STRING) {
      //     return static_cast<std::string *>(data_node->value());
      //   }
      // }
      T print_type_holder;
      std::clog << "WARNING: settings::value failed. \"" << key_name
                << "\" setting in \"" << this->name() << "\" filter is a "
                << basetype_to_string(data_node->datatype())
                << ". Trying to access it using \""
                << typeid(print_type_holder).name() << "\". Returning nullptr."
                << std::endl;
      return nullptr;
    }
    else if (node->nodetype() == epf::EP_STRINGNODE) {
      epf::StringNode *string_node = (epf::StringNode *)node;
      if constexpr (std::is_same<T, std::string>::value) {
        return string_node->value();
      }
      T print_type_holder;
      std::cout << "setting: \"" << key_name
                << "\" is a String. Trying to access it using \""
                << typeid(print_type_holder).name() << "\". Returning nullptr."
                << std::endl;
      return nullptr;
    }
    else {
      std::clog << "WARNING: settings::value failed. \"" << key_name
                << "\" setting in \"" << this->name()
                << "\" filter. Command setting." << std::endl;
      return nullptr;
    }
  }
  std::clog << "WARNING: settings::value failed. \"" << key_name
            << "\" setting in \"" << this->name() << "\" filter. Not found."
            << std::endl;
  return nullptr;
}
