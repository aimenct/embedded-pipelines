// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "settings.h"

#include <algorithm>

using namespace epf;

std::string setting_simple_name(std::string_view complete_path)
{
  auto pos = complete_path.rfind('.');
  if (pos == std::string_view::npos) {
    return std::string(complete_path);
  }
  return std::string(complete_path.substr(pos + 1));
}

Settings::Settings(std::string filter_name)
    : NodeTree(filter_name)
{
  for (int i = BASE_SETTING; i < SETTING_TYPE_COUNT; ++i) {
    SettingType type = static_cast<SettingType>(i);
    auto object_node =
        std::make_unique<ObjectNode>(settingtype_to_string(type), 0);

    // Save raw pointer before moving ownership
    Node *node_ptr = object_node.get();
    std::string node_name = object_node->name();

    // Transfer ownership to add()
    add(std::move(object_node), epf::EP_HAS_CHILD);

    // Store raw pointer in the map
    setting_map_.emplace(node_name, node_ptr);
  }
}

Settings::Settings(const Settings &obj)
    : NodeTree(obj)
{
  *this = obj;
}

const Settings &Settings::operator=(const Settings &obj)
{
  NodeTree::operator=(obj);

  for (epf::Node *node : node_list_) {
    setting_map_.emplace(node->name(), node);
  }

  return *this;
}

const Node *Settings::operator[](std::string name) const
{
  // Look for the setting name in the setting_map_
  auto iterator = setting_map_.find(name);

  // If found...
  if (iterator != setting_map_.end()) {
    return iterator->second;
  }

  return nullptr;
}

int32_t Settings::settingIndex(std::string key_name) const
{
  auto it = setting_map_.find(key_name);
  if (it == setting_map_.end()) {
    std::cout << "keyname: " << key_name << " not found." << std::endl;
    return -1;
  }
  return nodeIndex(it->second);
}

bool Settings::settingExists(std::string key_name) const
{
  try {
    setting_map_.at(key_name);
    return true;
  }
  catch (const std::out_of_range &e) {
    return false;
  }
}

std::string Settings::retrieveKeyName(Node *node) const
{
  int32_t index = nodeIndex(node);
  return retrieveKeyName(index);
}

std::string Settings::retrieveKeyName(int32_t index) const
{
  if (index < 0 || index >= static_cast<int32_t>(node_list_.size())) {
    std::cout << "retrieveKeyName: value index " << index
              << " is invalid. Should be in [0, " << node_list_.size() << ")."
              << std::endl;
    return "";
  }
  // Get the node at the specified index
  const Node *node = this->operator[](index);
  std::string key_name;

  while (node != dynamic_cast<const Node *>(&root())) {
    key_name = node->name() + "." + key_name;
    // Retrieve the parent node
    size_t parent_index = parentIndex(node);
    node = this->operator[](parent_index);
  }
  // Remove the last dot
  key_name = key_name[key_name.size() - 1] == '.'
                 ? key_name.substr(0, key_name.size() - 1)
                 : key_name;
  return key_name;
}

int32_t Settings::addSettingNode(std::unique_ptr<Node> node,
                                 int32_t parent_index)
{
  if (parent_index < 1) {
    std::cerr << "[Settings] " << node->name()
              << " could not be added, parent index must be >=1." << std::endl;
    return -1;
  }

  std::string name = node->name();
  std::string key_name = retrieveKeyName(parent_index) + "." + name;
  bool unique_name = setting_map_.find(key_name) == setting_map_.end();

  // Check if the node additon was completed.
  if (unique_name) {
    // NodeTree acquires ownership of the dynamically allocated node.
    Node *node_ptr = node.get();
    int32_t ret = add(std::move(node), epf::EP_HAS_CHILD, parent_index);
    if (ret < 0) {
      return ret;
    }
    const bool emplaced = setting_map_.try_emplace(key_name, node_ptr).second;
    if (!emplaced) {
      std::cerr << "[Settings] Node was added, but key \"" << key_name
                << "\" could not be inserted into setting_map_." << std::endl;
    }
    return ret;
  }
  else {
    std::cerr << "[Settings] ::addSetting failed. \"" << name
              << "\" setting in \"" << key_name
              << "\" filter. Setting name already in use. " << std::endl;
    return -1;
  }
}

int32_t Settings::addCommandUnder(std::string name,
                                  std::function<int32_t()> command,
                                  const int32_t parent_index,
                                  std::string tooltip, AccessType accessmode)
{
  std::unique_ptr<Node> node =
      std::make_unique<CommandNode>(name, command, tooltip, accessmode);

  return addSettingNode(std::move(node), parent_index);
}

int32_t Settings::addCommand(std::string name, std::function<int32_t()> command,
                             const SettingType type, std::string tooltip,
                             AccessType accessmode)
{
  auto index = settingIndex(settingtype_to_string(type));
  return addCommandUnder(name, command, index, tooltip, accessmode);
}

int32_t Settings::runCommand(std::string key_name)
{
  try {
    const Node *node = setting_map_.at(key_name);
    if (node) {
      if (node->nodetype() == epf::EP_COMMANDNODE) {
        const CommandNode *command_node =
            static_cast<const CommandNode *>(node);
        return command_node->run();
      }
      else {
        std::cout << "runCommand(): \"" << key_name
                  << "\" is not a command setting." << std::endl;
        return -1;
      }
    }
  }
  catch (const std::out_of_range &e) {
    std::cout << "runCommand(): \"" << key_name << "\" not found." << std::endl;
    return -1;
  }
  throw;
}

bool Settings::isDeviceSetting(std::string name) const
{
  const Node *node = operator[](name);
  // If found...
  if (node) {
    int32_t idx = parentIndex(node);
    if (idx >= 0) {
      if (node_list_[idx]->name() == settingtype_to_string(DEVICE_SETTING)) {
        return true;
      }
    }
    return false;
  }

  std::cout << "isDeviceSetting: Setting key \"" << name << "\" not found. "
            << std::endl;
  return false;
}

std::optional<AccessType> Settings::settingAccessMode(std::string name)
{
  // If found...
  const Node *node = operator[](name);
  if (node) {
    return node->accessMode();
  }
  std::cout << "settingAccessMode: key \"" << name << "\" not found"
            << std::endl;

  return std::nullopt;
}

int32_t Settings::fromYAML(const YAML::Node &config)
{
  // std::cout << "Updating config from yaml " << std::endl;
  for (int i = BASE_SETTING; i < SETTING_TYPE_COUNT; ++i) {
    SettingType type = static_cast<SettingType>(i);
    auto name = settingtype_to_string(type);
    if ((config["settings"]) && (config["settings"][name])) {
      parseYAMLnode(config["settings"][name], name);
    }
    // else
    //   printf("not found %s\n", name.c_str());
  }
  return 0;
}

int32_t Settings::toYAML(YAML::Node &filter_node)
{
  // Filter Settings Node
  YAML::Node filter_settings_node;

  for (int i = BASE_SETTING; i < SETTING_TYPE_COUNT; ++i) {
    SettingType type = static_cast<SettingType>(i);
    auto folder = settingtype_to_string(type);

    Node *root = setting_map_.at(folder);
    for (const auto &node_ref : root->references()) {
      Node *node = node_ref.address();
      if (node->isDataNode() || node->isStringNode()) {
        std::string value = this->valueString(folder + "." + node->name());
        if (!value.empty()) {
          filter_settings_node[folder][node->name()] = value;
        }
      }
      else if (node->isObjectNode()) {
        YAML::Node child_yaml = YAML::Node();
        child_yaml["id"] = node->name();
        for (const auto &child_ref : node->references()) {
          Node *child = child_ref.address();
          if (child->isDataNode() || child->isStringNode()) {
            std::string value = this->valueString(folder + "." + node->name() +
                                                  "." + child->name());
            if (!value.empty()) {
              child_yaml[child->name()] = value;
            }
          }
        }
        filter_settings_node[folder].push_back(child_yaml);
      }
    }
  }

  filter_node["settings"] = filter_settings_node;

  return 0;
}

int32_t Settings::parseYAMLnode(YAML::Node yaml_node, std::string root_key_name)
{
  if (settingExists(root_key_name)) {
    if (yaml_node.IsMap()) {
      for (YAML::const_iterator it = yaml_node.begin(); it != yaml_node.end();
           ++it) {
        std::string key = it->first.as<std::string>();
        std::string key_name = root_key_name + "." + key;
        parseYAMLnode(yaml_node[key], key_name);
      }
    }
    else if (yaml_node.IsScalar()) {
      std::string key = root_key_name;
      std::string value = yaml_node.as<std::string>();
      // std::cout << root_key_name << " : " << value << std::endl;
      setValue(root_key_name, value);
    }
    else if (yaml_node.IsSequence()) {
      for (const auto &element : yaml_node) {
        std::string key_name =
            root_key_name + "." + element["id"].as<std::string>();
        YAML::Node element_copy = YAML::Clone(element);

        // Remove the "id" key from the copy
        element_copy.remove("id");
        parseYAMLnode(element_copy, key_name);
      }
    }
    else {
      std::cout << "YAML NodeType not supported." << std::endl;
    }
  }
  return 0;
}

std::vector<std::string> Settings::listNames(int32_t type) const
{
  std::vector<std::string> names_list;
  if (type == BASE_SETTING) {
    for (auto it : setting_map_) {
      if (!isDeviceSetting(it.first)) {
        names_list.push_back(it.first);
      }
    }
  }  //
  else if (type == DEVICE_SETTING) {
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

std::string Settings::valueString(const std::string name) const
{
  auto raw = valueStringRaw(name);
  auto maybe_numeric = [&raw]() -> std::optional<int32_t> {
    if (raw.empty()) return std::nullopt;
    try {
      size_t idx = 0;
      int32_t value = std::stoi(raw, &idx);
      if (idx != raw.size()) return std::nullopt;
      return value;
    }
    catch (const std::exception &) {
      return std::nullopt;
    }
  }();

  if (!maybe_numeric.has_value()) return raw;
  auto label = enumLabelForValue(name, *maybe_numeric);
  if (!label.has_value()) return raw;
  return *label;
}

std::string Settings::valueStringRaw(const std::string name) const
{
  // Look for the setting name in the setting_map_
  auto iterator = setting_map_.find(name);

  // If found...
  if (iterator != setting_map_.end()) {
    if (iterator->second->nodetype() == epf::EP_DATANODE) {
      epf::DataNode *node = (epf::DataNode *)iterator->second;

      // Automatic internal type deduction
      if (node->datatype() == epf::EP_BOOL) {
        if (*static_cast<bool *>(node->value())) {
          return "true";
        }
        else {
          return "false";
        }
      }
      else if (node->datatype() == epf::EP_8C) {
        return std::to_string(*static_cast<char *>(node->value()));
      }
      else if (node->datatype() == epf::EP_8U) {
        return std::to_string(*static_cast<uint8_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_8S) {
        return std::to_string(*static_cast<int8_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_16U) {
        return std::to_string(*static_cast<uint16_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_16S) {
        return std::to_string(*static_cast<int16_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_32U) {
        return std::to_string(*static_cast<uint32_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_32S) {
        return std::to_string(*static_cast<int32_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_64U) {
        return std::to_string(*static_cast<uint64_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_64S) {
        return std::to_string(*static_cast<int64_t *>(node->value()));
      }
      else if (node->datatype() == epf::EP_32F) {
        return std::to_string(*static_cast<float *>(node->value()));
      }
      else if (node->datatype() == epf::EP_64F) {
        return std::to_string(*static_cast<double *>(node->value()));
      }
      else {
        return std::string();
      }
    }
    else if (iterator->second->nodetype() == epf::EP_STRINGNODE) {
      epf::StringNode *node = dynamic_cast<StringNode *>(iterator->second);
      return *node->value();
    }
    else {
      std::cout << "valueString: Invalid nodetype -> "
                << nodetype_to_string(iterator->second->nodetype()) << "."
                << std::endl;
      std::string not_found;
      return not_found;
    }
  }
  else {
    std::cout << "valueString: Setting key \"" << name << "\" not found. "
              << std::endl;
    std::string not_found;
    return not_found;
  }
}

bool Settings::hasEnumOptions(const std::string &name) const
{
  return !enumOptions(name).empty();
}

std::vector<Settings::EnumOption> Settings::enumOptions(
    const std::string &name) const
{
  std::vector<EnumOption> options;
  auto iterator = setting_map_.find(name);
  if (iterator == setting_map_.end()) return options;

  const auto *node = iterator->second;
  for (const auto &ref : node->references()) {
    if (ref.type() != epf::EP_HAS_ENUMVALUE) continue;
    const auto *enum_node = ref.address();
    if (!enum_node || !enum_node->isObjectNode()) continue;

    std::optional<int32_t> value;
    std::string display_name;
    for (const auto &option_ref : enum_node->references()) {
      const auto *child = option_ref.address();
      if (!child) continue;

      if (child->isDataNode() && child->name() == "value") {
        const auto *data_node = static_cast<const DataNode *>(child);
        if (data_node->datatype() == epf::EP_32S) {
          value = *static_cast<int32_t *>(data_node->value());
        }
      }
      else if (child->isStringNode() && child->name() == "display_name") {
        const auto *str_node = static_cast<const StringNode *>(child);
        display_name = *str_node->value();
      }
    }

    if (!value.has_value()) continue;
    EnumOption option{*value, enum_node->name(),
                      display_name.empty() ? enum_node->name() : display_name};
    options.push_back(option);
  }

  std::sort(options.begin(), options.end(),
            [](const EnumOption &a, const EnumOption &b) {
              return a.value < b.value;
            });
  return options;
}

std::optional<std::string> Settings::enumLabelForValue(const std::string &name,
                                                       int32_t value) const
{
  for (const auto &option : enumOptions(name)) {
    if (option.value == value) return option.display_name;
  }
  return std::nullopt;
}

std::optional<int32_t> Settings::enumValueForLabel(
    const std::string &name, const std::string &label) const
{
  for (const auto &option : enumOptions(name)) {
    if (option.label == label || option.display_name == label) {
      return option.value;
    }
  }
  return std::nullopt;
}

int32_t Settings::addEnumOptions(const std::string &name,
                                 const std::vector<EnumOption> &options)
{
  auto iterator = setting_map_.find(name);
  if (iterator == setting_map_.end()) return -1;
  auto *node = iterator->second;
  if (!node) return -1;
  int32_t parent_index = nodeIndex(node);
  if (parent_index < 0) return -1;

  for (const auto &existing_ref : node->references()) {
    if (existing_ref.type() == epf::EP_HAS_ENUMVALUE) {
      return 0;
    }
  }

  for (const auto &option : options) {
    auto option_node = std::make_unique<ObjectNode>(option.label);

    auto value_node = std::make_unique<DataNode>(
        "value", epf::EP_32S, std::vector<size_t>{1}, "", epf::R);
    *static_cast<int32_t *>(value_node->value()) = option.value;
    option_node->addReference(epf::EP_HAS_CHILD, std::move(value_node));

    if (!option.display_name.empty()) {
      auto display_node = std::make_unique<StringNode>(
          "display_name", option.display_name, epf::R);
      option_node->addReference(epf::EP_HAS_CHILD, std::move(display_node));
    }

    add(std::move(option_node), epf::EP_HAS_ENUMVALUE, parent_index);
  }

  return 0;
}
