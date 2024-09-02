// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef SETTINGS_H
#define SETTINGS_H

#include <yaml-cpp/yaml.h>

#include <string>

#include "node_tree.h"

namespace ep {

// Filter state
enum SettingType {
  FILTER_SETTING = 1,
  FILTER_COMMAND = 2,
  DEVICE_SETTING = 3,
  QUEUE_SETTING = 4
};

/**
 * @brief Class that maps the config settings of a filter. The object does not
 * manage the memory of the config, it just organises it and makes it easy
 * accessible by the filter methods.
 */
class Settings2 : private NodeTree {
  private:
    std::vector<std::string> folder_name_ = {
        "filter_settings", "filter_commands", "device_settings",
        "queue_settings"};

  public:
    /**
     * @brief Default constructor.
     **/
    Settings2()
    {
      Settings2("unamed_filter");
    };

    /**
     * @brief Default constructor.
     **/
    Settings2(std::string filter_name);

    /* Copy constructor (with its references)*/
    Settings2(const Settings2 &obj);

    /* Assignment operator */
    const Settings2 &operator=(const Settings2 &obj);

    /* Accessing operator */
    const Node2 *operator[](const std::string name) const;

    /* Set name */
    void setFilterName(std::string name);

    /**
     * @brief Deserialize YAML node to setting values.
     **/
    int32_t fromYAML(const YAML::Node &config);
    /**
     * @brief Serialize settings to YAML node.
     **/
    int32_t toYAML(YAML::Node &config);

    /**
     * @brief Method to add setting with automatic type deduction.
     */
    template <typename T>
    int32_t addSetting(const std::string name, T &value,
                       const SettingType setting_type = ep::FILTER_SETTING,
                       std::string tootltip = "",
                       AccessType accessmode = ep::W);

    /**
     * @brief Method to add setting with manual type specification.
     */
    int32_t addSetting(std::string name, void *value, ep::BaseType data_type,
                       SettingType setting_type = ep::FILTER_SETTING,
                       std::string tootltip = "",
                       AccessType accessmode = ep::W);

    /**
     * @brief Method to add a filter command.
     */
    int32_t addCommand(std::string name, std::function<int32_t()> command,
                       std::string tooltip = "", AccessType accessmode = W);

    /**
     * @brief Set the setting value. This function searches for the setting name
     * within the node_map_ and sets its value. The value can be passed as a
     * numeric type (such as bool, uint8_t, float, etc.) or as a string. In
     * the latter case, the method performs the conversion to the type specified
     * in the setting node.
     * @return Success or error code.
     */
    template <typename T>
    int32_t setValue(std::string name, T value);

    template <typename T>
    const T *value(std::string name);

    int32_t runCommand(std::string name);

    bool isDeviceSetting(std::string name) const;

    AccessType settingAccessMode(std::string name);

    AccessType settingAccessMode(std::size_t index);

    using NodeTree::print;

    using NodeTree::operator[];

    std::string valueString(const std::string name) const;

    /**
     * @brief List names of the settings. 0-> filter settings;
     * 1->device settings; other -> all
     */
    std::vector<std::string> listNames(int32_t device_setting = -1) const;

  private:
    std::unordered_map<std::string, ep::Node2 *> setting_map_;

    int32_t parseYAMLnode(YAML::Node yaml_node);
};

}  // namespace ep

#endif  // SETTINGS_H
