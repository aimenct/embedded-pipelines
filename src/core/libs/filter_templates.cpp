// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filter.h"

using namespace epf;

template <typename T>
int32_t Filter::addSetting(std::string name, T &value,
                           epf::SettingType setting_type, std::string tooltip,
                           AccessType accessmode)
{
  // check if setting exists in device getting value
  if (setting_type == DEVICE_SETTING) {
    int err = deviceSettingValue(name.c_str(), static_cast<void *>(&value));
    if (err < 0) {
      std::cout << "addDeviceSetting: " << name << " [FAILED]" << std::endl;
      return err;
    }
  }

  int32_t ret =
      settings_.addSetting(name, value, setting_type, tooltip, accessmode);
  if (ret >= 0) {
    notifySettingsChanged(SettingsChangeKind::Added, "setting added: " + name);
  }

  return 0;
}

template <typename T>
int32_t Filter::setSettingValue(std::string key_name, T value)
{
  std::optional<AccessType> access_mode = settings_.settingAccessMode(key_name);
  std::size_t pos = key_name.find_last_of('.');

  std::string name;
  if (pos != std::string::npos) {
    name = key_name.substr(pos + 1);
  }
  else {
    name = key_name;  // No dot found; whole string is the key
  }

  if (access_mode) {
    if (access_allowed_in_state(access_mode.value(), state_)) {
      if (settings_.isDeviceSetting(key_name)) {
        int32_t ret{-1};
        if constexpr (std::is_same<T, std::string>::value) {
          ret = setDeviceSettingValueStr(name.c_str(), value.c_str());
        }
        else {
          ret = setDeviceSettingValue(name.c_str(), &value);
        }
        if (ret >= 0) {
          ret = settings_.setValue<T>(key_name, value);
          if (ret >= 0) {
            notifySettingsChanged(SettingsChangeKind::Updated,
                                  "setting updated: " + key_name);
          }
        }
        return ret;
      }
      else {
        int32_t ret = settings_.setValue<T>(key_name, value);
        if (ret < 0) {
          std::cout << "setSettingValue: failed." << std::endl;
        }
        else {
          notifySettingsChanged(SettingsChangeKind::Updated,
                                "setting updated: " + key_name);
        }
        return ret;
      }
    }
    else {
      std::cout << "Not allowed to set setting " << key_name
                << " in \"state_==" << state2string(state_) << "\"."
                << std::endl;
      return -1;
    }
  }
  else {
    std::cout << "setSettingValue:: Setting \"" << key_name << "\" not found."
              << std::endl;
    return -1;
  }
}

template <typename T>
const T *Filter::settingValue(std::string full_key)
{
  T value;
  if (settings_.isDeviceSetting(full_key)) {
    // derive plain setting name before accessing the device
    std::string name = setting_simple_name(full_key);
    if constexpr (std::is_same<T, std::string>::value) {
      char placeholder[256];
      deviceSettingValue(name.c_str(), placeholder);
      value = std::string(placeholder);
    }
    else {
      deviceSettingValue(name.c_str(), &value);
    }
    // cache the read value without invoking device write path again
    settings_.setValue<T>(full_key, value);
  }
  // return cached value
  return settings_.value<T>(full_key);
}
