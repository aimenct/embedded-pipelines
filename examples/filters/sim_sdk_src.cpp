// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sim_sdk_src.h"

using namespace ep;

/* Filter constructor - create settings */
SimSdkSrc::SimSdkSrc(const YAML::Node &config)
    : Filter()
{
  // Queue - settings Base filter setting
  name_ = "SimSdkSensor";
  type_ = "SimSdkSrc";

  max_sinks_ = 1;
  max_sources_ = 0;

  // Add some filter specific settings
  com_port_ = "COM2";
  addSetting("COM Port", com_port_, FILTER_SETTING,
             "device communication port.", W_D);
  setSettingValue<std::string>("COM Port", "COM3");

  timeout_ = 0;
  addSetting("Timeout", timeout_);

  // You can also add some filter specific commands
  addCommand("start", std::bind(&SimSdkSrc::start, this));
  addCommand("stop", std::bind(&SimSdkSrc::stop, this));

  // read yaml config and update settings accordingly
  yaml_config_ = config;  // save a copy used for device settings after open
  readSettings(yaml_config_);
}

/* Filter destructor*/
SimSdkSrc::~SimSdkSrc()
{
  std::cout << "SimSdkSrc destructor " << std::endl;
}

int32_t SimSdkSrc::_open()
{
  if (oscilloscope_.connect(com_port_)) {
    // device setting
    // for adding a device setting, you need to provide:
    // - the virtual methods to set the settings in the device:
    //   setDeviceSetttingValue(const char *key, const void *value) -
    //   setDeviceSettingValueStr(const char *key, const char *value) -
    //   deviceSettingValue(const char *key, void *value) -
    // - declare the settings as DEVICE_SETTINGS with the proper type.
    //   for this, you need to get the data type of the setting from the
    //   sdk or device API (see, ArvCam.h/cpp example).

    // add specific device settings
    double bias;
    addSetting("Bias", bias, DEVICE_SETTING, "", W);
    double amplitude;
    addSetting("Amplitude", amplitude, DEVICE_SETTING, "", W);
    double frequency;
    addSetting("Frequency", frequency, DEVICE_SETTING, "", W);

    // you can alo add filter commands bind to device commands
    addCommand("OscilloIncreaseFrequency",
               std::bind(&SensorSDK::increaseFrequency, &oscilloscope_));
    addCommand("OscilloDecreaseFrequency",
               std::bind(&SensorSDK::decreaseFrequency, &oscilloscope_));
    addCommand("OscilloReset", std::bind(&SensorSDK::reset, &oscilloscope_),
               "reset", W_S);

    // read and update settings from YAML config 
    readDeviceSettings(yaml_config_);
    return 0;
  }
  else
    return -1;
}

int32_t SimSdkSrc::_close()
{
  if (oscilloscope_.disconnect())
    return 0;
  else
    return -1;
}

int32_t SimSdkSrc::_set()
{
  // Create data message schema that goes through queue
  Message *msg = new Message();

  // Instantiate  Counter - streamed datanode
  DataNode *sensor_counter_datanode = new DataNode(
      "counter", EP_32S, {1}, nullptr, false, "Data counter", true);

  // Instantiate Measurement Array -  streamed datanode
  uint64_t array_length = oscilloscope_.dataSize();
  DataNode *sensor_measurement_datanode =
      new DataNode("measurement", EP_64F, {array_length}, nullptr, false,
                   "measurement", true);
  // (alternative interface)
  // DataNode *sensor_measurement_datanode =
  //     new DataNode("measurement", EP_64F, {oscilloscope_.dataSize()},
  //     nullptr, "measurement");

  // Instantiate Units - static datanode
  std::string units = "mW";
  DataNode *units_datanode =
      new DataNode("units", EP_STRING, {1}, (void *)&units, true,
                   "Engineering Units", false);
  // (alternative interface)
  // DataNode units_datanode("my node", EP_STRING, {1});
  // *(std::string *)node.value() = a;

  // Assign units datanode as a property (child) of the measurement datanode
  sensor_measurement_datanode->addReference(EP_HAS_PROPERTY, units_datanode);

  // Instantiate  Timestamp - streamed datanode
  DataNode *timestamp_datanode = new DataNode(
      "time stamp", EP_64S, {1}, nullptr, false, "Data counter", true);

  // Add items to the message (Message acquires ownership of the dynamic
  // allocated nodes)
  msg->addItem(sensor_counter_datanode);
  msg->addItem(sensor_measurement_datanode);
  msg->addItem(timestamp_datanode);

  // Instantiate sink queue with the number of messages and the message schema
  // assigning it to sink port 0
  int err = addSinkQueue(0, msg);

  return err;
}

int32_t SimSdkSrc::_reset()
{
  return 0;
}

int32_t SimSdkSrc::_start()
{
  // get the Associate modelling with the writer
  counter_datanode_ = static_cast<DataNode *>(writer(0)->dataSchema()->item(0));
  measurement_datanode_ =
      static_cast<DataNode *>(writer(0)->dataSchema()->item(1));
  timestamp_datanode_ =
      static_cast<DataNode *>(writer(0)->dataSchema()->item(2));

  return 0;
}

int32_t SimSdkSrc::_stop()
{
  return 0;
}

/* job implementation */
int32_t SimSdkSrc::_job()
{
  int32_t err = writer(0)->startWrite();
  if (err >= 0) {
    auto [counter, newData] = oscilloscope_.readData();

    if (counter >= 0) {
      counter_datanode_->write(&counter);
      measurement_datanode_->write(newData);
      int64_t timestamp_us =
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      timestamp_datanode_->write(&timestamp_us);
      writer(0)->endWrite();
      usleep(timeout_);
      return 0;
    }
    else {  // error read from oscilloscope, abort writing
      writer(0)->endWriteAbort();
      usleep(timeout_);
      return counter;
    }
  }
  usleep(timeout_);
  return err;
}

// virtual functions
int32_t SimSdkSrc::setDeviceSettingValue(const char *key, const void *value)
{
  double v = *static_cast<const double *>(value);
  return oscilloscope_.setParameter(key, v);
}

int32_t SimSdkSrc::setDeviceSettingValueStr(const char *key, const char *value)
{
  char *end;
  double result = std::strtod(value, &end);
  if (value != end) {
    oscilloscope_.setParameter(key, result);
    return 0;
  }
  else
    return -1;
}
int32_t SimSdkSrc::deviceSettingValue(const char *key, void *value)
{
  return oscilloscope_.getParameter(key, *static_cast<double *>(value));
}
