// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "settings_templates.cpp"

template int32_t Settings2::addSetting<bool>(const std::string name,
                                             bool &value,
                                             const SettingType setting_type,
                                             std::string tootltip,
                                             AccessType accessmode);
template int32_t Settings2::addSetting<char>(const std::string name,
                                             char &value,
                                             const SettingType setting_type,
                                             std::string tootltip,
                                             AccessType accessmode);
template int32_t Settings2::addSetting<uint8_t>(const std::string name,
                                                uint8_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings2::addSetting<int8_t>(const std::string name,
                                               int8_t &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings2::addSetting<uint16_t>(const std::string name,
                                                 uint16_t &value,
                                                 const SettingType setting_type,
                                                 std::string tootltip,
                                                 AccessType accessmode);
template int32_t Settings2::addSetting<int16_t>(const std::string name,
                                                int16_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings2::addSetting<uint32_t>(const std::string name,
                                                 uint32_t &value,
                                                 const SettingType setting_type,
                                                 std::string tootltip,
                                                 AccessType accessmode);
template int32_t Settings2::addSetting<int32_t>(const std::string name,
                                                int32_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings2::addSetting<uint64_t>(const std::string name,
                                                 uint64_t &value,
                                                 const SettingType setting_type,
                                                 std::string tootltip,
                                                 AccessType accessmode);
template int32_t Settings2::addSetting<int64_t>(const std::string name,
                                                int64_t &value,
                                                const SettingType setting_type,
                                                std::string tootltip,
                                                AccessType accessmode);
template int32_t Settings2::addSetting<float>(const std::string name,
                                              float &value,
                                              const SettingType setting_type,
                                              std::string tootltip,
                                              AccessType accessmode);
template int32_t Settings2::addSetting<double>(const std::string name,
                                               double &value,
                                               const SettingType setting_type,
                                               std::string tootltip,
                                               AccessType accessmode);
template int32_t Settings2::addSetting<std::string>(const std::string name,
                                                    std::string &value,
                                                    const SettingType setting_type,
                                                    std::string tootltip,
                                                    AccessType accessmode);

template int32_t Settings2::setValue<bool>(std::string name, bool value);
template int32_t Settings2::setValue<char>(std::string name, char value);
template int32_t Settings2::setValue<uint8_t>(std::string name, uint8_t value);
template int32_t Settings2::setValue<int8_t>(std::string name, int8_t value);
template int32_t Settings2::setValue<uint16_t>(std::string name,
                                               uint16_t value);
template int32_t Settings2::setValue<int16_t>(std::string name, int16_t value);
template int32_t Settings2::setValue<uint32_t>(std::string name,
                                               uint32_t value);
template int32_t Settings2::setValue<int32_t>(std::string name, int32_t value);
template int32_t Settings2::setValue<uint64_t>(std::string name,
                                               uint64_t value);
template int32_t Settings2::setValue<int64_t>(std::string name, int64_t value);
template int32_t Settings2::setValue<float>(std::string name, float value);
template int32_t Settings2::setValue<double>(std::string name, double value);
template int32_t Settings2::setValue<std::string>(std::string name,
                                                  std::string value);

template const bool *Settings2::value<bool>(std::string name);
template const char *Settings2::value<char>(std::string name);
template const uint8_t *Settings2::value<uint8_t>(std::string name);
template const int8_t *Settings2::value<int8_t>(std::string name);
template const uint16_t *Settings2::value<uint16_t>(std::string name);
template const int16_t *Settings2::value<int16_t>(std::string name);
template const uint32_t *Settings2::value<uint32_t>(std::string name);
template const int32_t *Settings2::value<int32_t>(std::string name);
template const uint64_t *Settings2::value<uint64_t>(std::string name);
template const int64_t *Settings2::value<int64_t>(std::string name);
template const float *Settings2::value<float>(std::string name);
template const double *Settings2::value<double>(std::string name);
template const std::string *Settings2::value<std::string>(std::string name);
