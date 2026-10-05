// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef SETTINGS_H
#define SETTINGS_H

#include <yaml-cpp/yaml.h>

#include <optional>
#include <string>

#include "node_tree.h"

std::string setting_simple_name(std::string_view complete_path);

namespace epf {
/**
 * @brief Class that maps the config settings of a filter. The object does not
 * manage the memory of the config, it just organises it and makes it easy
 * accessible by the filter methods.
 */
class Settings : private NodeTree {
    //  private:
    //  std::vector<std::string> folder_name_{"base", "device", "control",
    //                                          "source_ports", "sink_ports"};

  public:
    struct EnumOption {
        int32_t value;
        std::string label;
        std::string display_name;
    };
    /**
     * @brief Default constructor.
     **/
    Settings()
    {
      Settings("unamed_filter");
    };

    /**
     * @brief Constructor with the name of the filter.
     * @param filter_name name of the filter associated to the settings
     * instance.
     */
    Settings(std::string filter_name);

    /**
     * @brief Copy constructor.
     */
    Settings(const Settings &obj);

    /**
     * @brief Assignment operator.
     */
    const Settings &operator=(const Settings &obj);

    /**
     * @brief Accessing by setting name.
     * @param filter_name
     */
    const Node *operator[](const std::string keyname) const;

    /**
     * @brief Accessing by index.
     * @param index
     */
    using NodeTree::operator[];

    using NodeTree::setName;

    /**
     * @brief Get the index of the setting.
     * @param keyname Name of the setting.
     * @return index of the setting.
     */
    int32_t settingIndex(std::string key_name) const;

    bool settingExists(std::string key_name) const;

    /**
     * @brief Deserialize YAML node to setting values.
     * @param config YAML::Node object
     * @return errorcode
     */
    int32_t fromYAML(const YAML::Node &config);

    /**
     * @brief Serialize YAML node to setting values.
     * @param config YAML::Node object
     */
    int32_t toYAML(YAML::Node &config);

    /**
     * @brief
     */
    int32_t addSettingNode(std::unique_ptr<Node> node, int32_t parent_index);

    template <typename T>
    int32_t addSettingUnder(const std::string name, T &value,
                            const int32_t parent_index,
                            std::string tooltip = "",
                            AccessType accessmode = epf::W);

    /**
     * @brief Method to add setting with automatic type deduction.
     * @tparam T
     * @param name Name of the setting
     * @param value Variable containing the value (by-reference)
     * @param type Type of setting (i.e. filter, device, source_port or
     * sink_port)
     * @param tootltip Description fo the setting.
     * @param accessmode
     * @return the index of the setting added
     */
    template <typename T>
    int32_t addSetting(const std::string name, T &value,
                       const SettingType type = BASE_SETTING,
                       std::string tooltip = "",
                       AccessType accessmode = epf::W);

    /**
     * @brief Method to add a filter command.
     * @param name Name of the command.
     * @param command Function that outputs and error code associated to this
     * command.
     * @param parent_index.
     * @param setting_type Type of the setting.
     * @param tooltip Description fo the setting.
     * @param accessmode
     * @return error code
     */
    int32_t addCommandUnder(std::string name, std::function<int32_t()> command,
                            const int32_t parent_index = 1,
                            std::string tooltip = "",
                            AccessType accessmode = R);

    /**
     * @brief Method to add a filter command.
     * @param name Name of the command.
     * @param command Function that outputs and error code associated to this
     * command.
     * @param type Type of setting (i.e. filter, device, control, source_port or
     * sink_port)
     * @param setting_type Type of the setting.
     * @param tooltip Description fo the setting.
     * @param accessmode
     * @return error code
     */
    int32_t addCommand(std::string name, std::function<int32_t()> command,
                       const SettingType type = CONTROL_SETTING,
                       std::string tooltip = "", AccessType accessmode = R);

    /**
     * @brief Set the setting value. This function searches for the setting name
     * within the node_map_ and sets its value. The value can be passed as a
     * numeric type (such as bool, uint8_t, float, etc.) or as a string. In
     * the latter case, the method performs the conversion to the type specified
     * in the setting node.
     * @tparam T
     * @param name Name of the setting
     * @param value Variable containing the value (by-reference)
     * @return errorcode.
     */
    template <typename T>
    int32_t setValue(std::string key_name, T value);

    /**
     * @brief Method to access the value of the setting searching by name.
     * @tparam T
     * @param name Name of the setting
     * @return value of the setting.
     */
    template <typename T>
    const T *value(std::string key_name) const;

    /**
     * @brief Run command name.
     * @param name Name of the command.
     * @return errorcode.
     */
    int32_t runCommand(std::string key_name);

    /**
     * @brief Check if some setting is has DEVICE_SETTING type.
     * @param name Name of the command.
     * @return True if so.
     */
    bool isDeviceSetting(std::string name) const;

    /**
     * @brief Retrieve AccessType of the named setting..
     * @param name Setting name.
     * @return AccessType.
     */
    std::optional<AccessType> settingAccessMode(std::string name);

    // /**
    //  * @brief [NOT IMPLEMENTED].
    //  * @param index Setting index.
    //  * @return AccessType.
    //  */
    // AccessType settingAccessMode(std::size_t index);

    using NodeTree::print;
    using NodeTree::root;

    /**
     * @brief Retrieves string-seriliazed value of one setting serialized.
     * @param name Setting name.
     * @return Stirng value of the setting..
     */
    std::string valueString(const std::string name) const;
    std::string valueStringRaw(const std::string name) const;

    bool hasEnumOptions(const std::string &name) const;
    std::vector<EnumOption> enumOptions(const std::string &name) const;
    std::optional<std::string> enumLabelForValue(const std::string &name,
                                                 int32_t value) const;
    std::optional<int32_t> enumValueForLabel(const std::string &name,
                                             const std::string &label) const;
    int32_t addEnumOptions(const std::string &name,
                           const std::vector<EnumOption> &options);

    /**
     * @brief List names of the settings.
     * @param setting_type Code to select which type of settings to list.
     * 1-> filter settings;
     * 2-> device settings;
     * 3-> queue settings;
     * 4-> command_settings;
     * other -> all
     * @return Stirng value of the setting..
     */
    std::vector<std::string> listNames(int32_t type = 0) const;

    std::string retrieveKeyName(Node *node) const;

  private:
    std::unordered_map<std::string, epf::Node *> setting_map_;

    int32_t parseYAMLnode(YAML::Node yaml_node, std::string root_key_name = "");

    std::string retrieveKeyName(int32_t index) const;
};

}  // namespace epf

#endif  // SETTINGS_H
