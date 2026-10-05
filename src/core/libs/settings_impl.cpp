// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "settings_templates.cpp"

namespace epf {

template int32_t Settings::addSettingUnder<bool>(const std::string name,
                                                 bool &value,
                                                 const int32_t parent_index,
                                                 std::string tootltip,
                                                 AccessType accessmode);
template int32_t Settings::addSettingUnder<const bool>(
    const std::string name, const bool &value, const int32_t parent_index,
    std::string tootltip, AccessType accessmode);
template int32_t Settings::addSettingUnder<char>(const std::string name,
                                                 char &value,
                                                 const int32_t parent_index,
                                                 std::string tootltip,
                                                 AccessType accessmode);
template int32_t Settings::addSettingUnder<uint8_t>(const std::string name,
                                                    uint8_t &value,
                                                    const int32_t parent_index,
                                                    std::string tootltip,
                                                    AccessType accessmode);
template int32_t Settings::addSettingUnder<int8_t>(const std::string name,
                                                   int8_t &value,
                                                   const int32_t parent_index,
                                                   std::string tootltip,
                                                   AccessType accessmode);
template int32_t Settings::addSettingUnder<uint16_t>(const std::string name,
                                                     uint16_t &value,
                                                     const int32_t parent_index,
                                                     std::string tootltip,
                                                     AccessType accessmode);
template int32_t Settings::addSettingUnder<int16_t>(const std::string name,
                                                    int16_t &value,
                                                    const int32_t parent_index,
                                                    std::string tootltip,
                                                    AccessType accessmode);
template int32_t Settings::addSettingUnder<uint32_t>(const std::string name,
                                                     uint32_t &value,
                                                     const int32_t parent_index,
                                                     std::string tootltip,
                                                     AccessType accessmode);
template int32_t Settings::addSettingUnder<int32_t>(const std::string name,
                                                    int32_t &value,
                                                    const int32_t parent_index,
                                                    std::string tootltip,
                                                    AccessType accessmode);
template int32_t Settings::addSettingUnder<const int32_t>(
    const std::string name, const int32_t &value, const int32_t parent_index,
    std::string tootltip, AccessType accessmode);
template int32_t Settings::addSettingUnder<uint64_t>(const std::string name,
                                                     uint64_t &value,
                                                     const int32_t parent_index,
                                                     std::string tootltip,
                                                     AccessType accessmode);
template int32_t Settings::addSettingUnder<int64_t>(const std::string name,
                                                    int64_t &value,
                                                    const int32_t parent_index,
                                                    std::string tootltip,
                                                    AccessType accessmode);
template int32_t Settings::addSettingUnder<float>(const std::string name,
                                                  float &value,
                                                  const int32_t parent_index,
                                                  std::string tootltip,
                                                  AccessType accessmode);
template int32_t Settings::addSettingUnder<double>(const std::string name,
                                                   double &value,
                                                   const int32_t parent_index,
                                                   std::string tootltip,
                                                   AccessType accessmode);
template int32_t Settings::addSettingUnder<std::string>(
    const std::string name, std::string &value, const int32_t parent_index,
    std::string tootltip, AccessType accessmode);
template int32_t Settings::addSettingUnder<QueueType>(
    const std::string name, QueueType &value, const int32_t parent_index,
    std::string tootltip, AccessType accessmode);
template int32_t Settings::addSettingUnder<const QueueType>(
    const std::string name, const QueueType &value, const int32_t parent_index,
    std::string tootltip, AccessType accessmode);

template int32_t Settings::addSetting<bool>(const std::string name, bool &value,
                                            const SettingType setting_type,
                                            std::string tootltip,
                                            AccessType accessmode);
template int32_t Settings::addSetting<char>(const std::string name, char &value,
                                            const SettingType setting_type,
                                            std::string tootltip,
                                            AccessType accessmode);
template int32_t Settings::addSetting<uint8_t>(const std::string name,
                                               uint8_t &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings::addSetting<int8_t>(const std::string name,
                                              int8_t &value,
                                              const SettingType setting_type,
                                              std::string tootltip,
                                              AccessType accessmode);
template int32_t Settings::addSetting<uint16_t>(const std::string name,
                                                uint16_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings::addSetting<int16_t>(const std::string name,
                                               int16_t &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings::addSetting<uint32_t>(const std::string name,
                                                uint32_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings::addSetting<int32_t>(const std::string name,
                                               int32_t &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings::addSetting<uint64_t>(const std::string name,
                                                uint64_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings::addSetting<int64_t>(const std::string name,
                                               int64_t &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings::addSetting<float>(const std::string name,
                                             float &value,
                                             const SettingType setting_type,
                                             std::string tootltip,
                                             AccessType accessmode);
template int32_t Settings::addSetting<double>(const std::string name,
                                              double &value,
                                              const SettingType setting_type,
                                              std::string tootltip,
                                              AccessType accessmode);
template int32_t Settings::addSetting<std::string>(
    const std::string name, std::string &value, const SettingType setting_type,
    std::string tootltip, AccessType accessmode);

template int32_t Settings::setValue<bool>(std::string name, bool value);
template int32_t Settings::setValue<char>(std::string name, char value);
template int32_t Settings::setValue<uint8_t>(std::string name, uint8_t value);
template int32_t Settings::setValue<int8_t>(std::string name, int8_t value);
template int32_t Settings::setValue<uint16_t>(std::string name, uint16_t value);
template int32_t Settings::setValue<int16_t>(std::string name, int16_t value);
template int32_t Settings::setValue<uint32_t>(std::string name, uint32_t value);
template int32_t Settings::setValue<int32_t>(std::string name, int32_t value);
template int32_t Settings::setValue<uint64_t>(std::string name, uint64_t value);
template int32_t Settings::setValue<int64_t>(std::string name, int64_t value);
template int32_t Settings::setValue<float>(std::string name, float value);
template int32_t Settings::setValue<double>(std::string name, double value);
template int32_t Settings::setValue<std::string>(std::string name,
                                                 std::string value);

template const bool *Settings::value<bool>(std::string name) const;
template const char *Settings::value<char>(std::string name) const;
template const uint8_t *Settings::value<uint8_t>(std::string name) const;
template const int8_t *Settings::value<int8_t>(std::string name) const;
template const uint16_t *Settings::value<uint16_t>(std::string name) const;
template const int16_t *Settings::value<int16_t>(std::string name) const;
template const uint32_t *Settings::value<uint32_t>(std::string name) const;
template const int32_t *Settings::value<int32_t>(std::string name) const;
template const uint64_t *Settings::value<uint64_t>(std::string name) const;
template const int64_t *Settings::value<int64_t>(std::string name) const;
template const float *Settings::value<float>(std::string name) const;
template const double *Settings::value<double>(std::string name) const;
template const std::string *Settings::value<std::string>(
    std::string name) const;

}  // namespace epf