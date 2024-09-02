// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "settings.h"

using namespace ep;

Settings2::Settings2(std::string filter_name)
    : NodeTree(filter_name)
{
  for (std::string name : folder_name_) {
    ObjectNode *object_node = new ObjectNode(name, 0);

    // Add folders for filter and device settings to the node tree and
    // node map.
    add(object_node, ep::EP_HAS_CHILD);
    setting_map_.emplace(object_node->name(), object_node);
  }
}

Settings2::Settings2(const Settings2 &obj)
    : NodeTree(obj)
{
  *this = obj;
}

const Settings2 &Settings2::operator=(const Settings2 &obj)
{
  NodeTree::operator=(obj);

  for (ep::Node2 *node : node_list_) {
    setting_map_.emplace(node->name(), node);
  }

  return *this;
}

const Node2 *Settings2::operator[](std::string name) const
{
  // Look for the setting name in the setting_map_
  auto iterator = setting_map_.find(name);

  // If found...
  if (iterator != setting_map_.end()) {
    return iterator->second;
  }

  return nullptr;
}

void Settings2::setFilterName(std::string name)
{
  // Look for the setting name in the setting_map_
  Node2 *root = NodeTree::operator[](0);

  root->setName(name);

  return;
}

int32_t Settings2::addSetting(std::string name, void *value,
                              ep::BaseType data_type, SettingType setting_type,
                              std::string tooltip, AccessType accessmode)
{
  // Check wether it should be added to the filter or device node tree.
  int32_t parent_node = 1;

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

  new_node = new DataNode(name, data_type, {1}, &value, mem_managed, tooltip,
                          false, accessmode);
  unique_name = setting_map_.try_emplace(name, new_node).second;

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

int32_t Settings2::addCommand(std::string name,
                              std::function<int32_t()> command,
                              std::string tooltip, AccessType accessmode)
{
  // index of filter_commands folder
  int32_t parent_node = 2;

  CommandNode *new_node = new CommandNode(name, command, tooltip, accessmode);
  bool unique_name = setting_map_.try_emplace(name, new_node).second;

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

int32_t Settings2::runCommand(std::string name)
{
  const Node2 *node = operator[](name);
  if (node) {
    if (node->nodetype() == ep::EP_COMMANDNODE) {
      const CommandNode *command_node = static_cast<const CommandNode *>(node);
      return command_node->run();
    }
    else {
      std::cout << "runCommand(): \"" << name << "\" is not a command setting."
                << std::endl;
      return -1;
    }
  }
  std::cout << "runCommand(): \"" << name << "\" not found." << std::endl;
  return -1;
}

bool Settings2::isDeviceSetting(std::string name) const
{
  const Node2 *node = operator[](name);
  // If found...
  if (node) {
    size_t idx = parentIndex(node);
    if (node_list_[idx]->name() == folder_name_[2]) {
      return true;
    }
    else {
      return false;
    }
  }

  std::cout << "value: Setting key \"" << name << "\" not found. " << std::endl;
  return false;
}

AccessType Settings2::settingAccessMode(std::string name)
{
  // If found...
  const Node2 *node = operator[](name);
  if (node) {
    if (node->nodetype() == EP_DATANODE) {
      return static_cast<const DataNode *>(node)->accessMode();
    }
    if (node->nodetype() == EP_COMMANDNODE) {
      return static_cast<const CommandNode *>(node)->accessMode();
    }
  }
  std::cout << "settingAccessMode: key \"" << name << "\" not found"
            << std::endl;
  
  return ep::R;
}

int32_t Settings2::fromYAML(const YAML::Node &config)
{
  std::cout << "Updating config from yaml " << std::endl;

  for (std::string name : folder_name_) {
    if (name != folder_name_[2])
      parseYAMLnode(config[name]);
  }
  return 0;
}

int32_t Settings2::toYAML(YAML::Node &filter_node)
{
  // Filter Settings Node
  YAML::Node filter_settings_node;

  Node2 *root = this->root().references()[0].address();
  for (const auto &node_ref : root->references()) {
    Node2 *n = node_ref.address();
    filter_settings_node[n->name()] = this->valueString(n->name());
  }

  filter_node["filter_settings"] = filter_settings_node;

  // Command Settings Node
  YAML::Node filter_command_node;
  root = this->root().references()[1].address();
  for (const auto &command : root->references()) {
    filter_command_node[command.address()->name()] = "";
  }
    
  if (filter_command_node.size() != 0)
    filter_node["filter_commands"] = filter_command_node;

  // Device Settings Node
  YAML::Node device_settings_node;
  root = this->root().references()[2].address();
  for (const auto &setting : root->references()) {
    device_settings_node[setting.address()->name()] =
        this->valueString(setting.address()->name());
  }

  if (device_settings_node.size() != 0)
    filter_node["device_settings"] = device_settings_node;

  return 0;
}

int32_t Settings2::parseYAMLnode(YAML::Node yaml_node)
{
  for (YAML::const_iterator it = yaml_node.begin(); it != yaml_node.end();
       ++it) {
    std::string key = it->first.as<std::string>();
    std::string value;

    if (yaml_node[key].Type() == YAML::NodeType::Scalar) {
      value = it->second.as<std::string>();
      // std::cout << key << " : " << value << std::endl;

      setValue(key, value);
    }
    else if (yaml_node[key].Type() == YAML::NodeType::Map) {
      // std::cout << key << " : Map" << std::endl;

      parseYAMLnode(yaml_node[key]);
    }
    else {
      std::cout << "YAML NodeType not supported." << std::endl;
    }
  }
  return 0;
}

std::vector<std::string> Settings2::listNames(int32_t device_setting) const
{
  std::vector<std::string> names_list;
  if (device_setting == 0) {
    for (auto it : setting_map_) {
      if (!isDeviceSetting(it.first)) {
        names_list.push_back(it.first);
      }
    }
  }  //
  else if (device_setting == 1) {
    for (auto it : setting_map_) {
      if (isDeviceSetting(it.first)) {
        names_list.push_back(it.first);
      }
    }
  }
  else {
    for (auto it : setting_map_) {
      names_list.push_back(it.first);
    }
  }
  return names_list;
}

std::string Settings2::valueString(const std::string name) const
{
  // Look for the setting name in the setting_map_
  auto iterator = setting_map_.find(name);

  // If found...
  if (iterator != setting_map_.end()) {
    if (iterator->second->nodetype() == ep::EP_DATANODE) {
      ep::DataNode *node = (ep::DataNode *)iterator->second;

      // Automatic internal type deduction
      if (node->datatype() == ep::EP_BOOL) {
        return std::to_string(*static_cast<bool *>(node->value()));
      }
      else if (node->datatype() == ep::EP_8C) {
        return std::to_string(*static_cast<char *>(node->value()));
      }
      else if (node->datatype() == ep::EP_8U) {
        return std::to_string(*static_cast<uint8_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_8S) {
        return std::to_string(*static_cast<int8_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_16U) {
        return std::to_string(*static_cast<uint16_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_16S) {
        return std::to_string(*static_cast<int16_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_32U) {
        return std::to_string(*static_cast<uint32_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_32S) {
        return std::to_string(*static_cast<int32_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_64U) {
        return std::to_string(*static_cast<uint64_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_64S) {
        return std::to_string(*static_cast<int64_t *>(node->value()));
      }
      else if (node->datatype() == ep::EP_32F) {
        return std::to_string(*static_cast<float *>(node->value()));
      }
      else if (node->datatype() == ep::EP_64F) {
        return std::to_string(*static_cast<double *>(node->value()));
      }
      else if (node->datatype() == ep::EP_STRING) {
        return *static_cast<std::string *>(node->value());
      }
      else {
        return std::string();
      }
    }
    else {
      std::cout << "valueStr: Invalid nodetype. " << std::endl;
      std::string not_found;
      return not_found;
    }
  }
  else {
    std::cout << "value: Setting key \"" << name << "\" not found. "
              << std::endl;
    std::string not_found;
    return not_found;
  }
}
