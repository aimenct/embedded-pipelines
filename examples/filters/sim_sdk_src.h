// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _SimSdkSrc
#define _SimSdkSrc

#include "../libs/sensor_sdk.h"
#include "core.h"

class SimSdkSrc : public ep::Filter {
  public:
    SimSdkSrc(const YAML::Node &config);  // create settings
    ~SimSdkSrc();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // start filters' loop
    int32_t _stop();   // start filters' loop

  private:
    // Node
    YAML::Node yaml_config_;

    // virtual functions
    int32_t setDeviceSettingValue(const char *key, const void *value);
    int32_t setDeviceSettingValueStr(const char *key, const char *value);
    int32_t deviceSettingValue(const char *key, void *value);

    SensorSDK oscilloscope_;

    // filter settings
    std::string com_port_;
    uint32_t timeout_;

    // Information modelling of queue data
    ep::DataNode *counter_datanode_;
    ep::DataNode *measurement_datanode_;
    ep::DataNode *timestamp_datanode_;
};

#endif
