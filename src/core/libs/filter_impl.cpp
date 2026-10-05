// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filter_templates.cpp"

namespace epf {

template int32_t Filter::addSetting<bool>(std::string name, bool &value,
                                          epf::SettingType setting_type,
                                          std::string tooltip,
                                          AccessType accessmode);
template int32_t Filter::addSetting<char>(std::string name, char &value,
                                          epf::SettingType setting_type,
                                          std::string tooltip,
                                          AccessType accessmode);
template int32_t Filter::addSetting<uint8_t>(std::string name, uint8_t &value,
                                             epf::SettingType setting_type,
                                             std::string tooltip,
                                             AccessType accessmode);
template int32_t Filter::addSetting<int8_t>(std::string name, int8_t &value,
                                            epf::SettingType setting_type,
                                            std::string tooltip,
                                            AccessType accessmode);
template int32_t Filter::addSetting<uint16_t>(std::string name, uint16_t &value,
                                              epf::SettingType setting_type,
                                              std::string tooltip,
                                              AccessType accessmode);
template int32_t Filter::addSetting<int16_t>(std::string name, int16_t &value,
                                             epf::SettingType setting_type,
                                             std::string tooltip,
                                             AccessType accessmode);
template int32_t Filter::addSetting<uint32_t>(std::string name, uint32_t &value,
                                              epf::SettingType setting_type,
                                              std::string tooltip,
                                              AccessType accessmode);
template int32_t Filter::addSetting<int32_t>(std::string name, int32_t &value,
                                             epf::SettingType setting_type,
                                             std::string tooltip,
                                             AccessType accessmode);
template int32_t Filter::addSetting<uint64_t>(std::string name, uint64_t &value,
                                              epf::SettingType setting_type,
                                              std::string tooltip,
                                              AccessType accessmode);
template int32_t Filter::addSetting<int64_t>(std::string name, int64_t &value,
                                             epf::SettingType setting_type,
                                             std::string tooltip,
                                             AccessType accessmode);
template int32_t Filter::addSetting<float>(std::string name, float &value,
                                           epf::SettingType setting_type,
                                           std::string tooltip,
                                           AccessType accessmode);
template int32_t Filter::addSetting<double>(std::string name, double &value,
                                            epf::SettingType setting_type,
                                            std::string tooltip,
                                            AccessType accessmode);
template int32_t Filter::addSetting<std::string>(std::string name,
                                                 std::string &value,
                                                 epf::SettingType setting_type,
                                                 std::string tooltip,
                                                 AccessType accessmode);

template int32_t Filter::setSettingValue<bool>(std::string name, bool value);
template int32_t Filter::setSettingValue<char>(std::string name, char value);
template int32_t Filter::setSettingValue<uint8_t>(std::string name,
                                                  uint8_t value);
template int32_t Filter::setSettingValue<int8_t>(std::string name,
                                                 int8_t value);
template int32_t Filter::setSettingValue<uint16_t>(std::string name,
                                                   uint16_t value);
template int32_t Filter::setSettingValue<int16_t>(std::string name,
                                                  int16_t value);
template int32_t Filter::setSettingValue<uint32_t>(std::string name,
                                                   uint32_t value);
template int32_t Filter::setSettingValue<int32_t>(std::string name,
                                                  int32_t value);
template int32_t Filter::setSettingValue<uint64_t>(std::string name,
                                                   uint64_t value);
template int32_t Filter::setSettingValue<int64_t>(std::string name,
                                                  int64_t value);
template int32_t Filter::setSettingValue<float>(std::string name, float value);
template int32_t Filter::setSettingValue<double>(std::string name,
                                                 double value);
template int32_t Filter::setSettingValue<std::string>(std::string name,
                                                      std::string value);

template const bool *Filter::settingValue<bool>(std::string name);
template const char *Filter::settingValue<char>(std::string name);
template const uint8_t *Filter::settingValue<uint8_t>(std::string name);
template const int8_t *Filter::settingValue<int8_t>(std::string name);
template const uint16_t *Filter::settingValue<uint16_t>(std::string name);
template const int16_t *Filter::settingValue<int16_t>(std::string name);
template const uint32_t *Filter::settingValue<uint32_t>(std::string name);
template const int32_t *Filter::settingValue<int32_t>(std::string name);
template const uint64_t *Filter::settingValue<uint64_t>(std::string name);
template const int64_t *Filter::settingValue<int64_t>(std::string name);
template const float *Filter::settingValue<float>(std::string name);
template const double *Filter::settingValue<double>(std::string name);
template const std::string *Filter::settingValue<std::string>(std::string name);

}  // namespace epf