// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src_template.h"

using namespace ep;

/* Filter constructor - create settings */
SrcTemplate::SrcTemplate(const YAML::Node &config)
    : Filter()
{
  // Base filter setting will be overwritten by readSettings if found in config
  // YAML
  name_ = "MyCosineSensor";
  type_ = "SrcTemplate";
  max_sinks_ = 1;

  // My filter settings
  amplitude_ = 10;
  frequency_ = 2;
  timeout_ = 10;

  // Add some parameters as settings (they will be exposed)
  addSetting("Frequency", frequency_, FILTER_SETTING,
             "Frequency (Hz) value of the sinusoidal function.");

  addSetting("Amplitude", amplitude_, FILTER_SETTING,
             "Amplitude value of the sinusoidal function.");

  addSetting("Timeout", timeout_, FILTER_SETTING, "Timeout in us.");

  // You can also add custom commands
  addCommand("increaseFrequency()",
             std::bind(&SrcTemplate::increaseFrequency, this));
  addCommand("decreaseFrequency()",
             std::bind(&SrcTemplate::decreaseFrequency, this));

  // update settings from YAML
  readSettings(config);
}

/* Filter destructor*/
SrcTemplate::~SrcTemplate()
{
  std::cout << "SrcTemplate destructor " << std::endl;
}

int32_t SrcTemplate::_open()
{
  return 0;
}

int32_t SrcTemplate::_close()
{
  return 0;
}

int32_t SrcTemplate::_set()
{
  // Create a new message to define the data schema for the queue
  Message *msg = new Message();

  // Create data nodes for the sensor and time data
  DataNode *sensor_datanode =
      new DataNode("Sensor", EP_64F, {1}, &sensor_, false, "Data source", true);
  DataNode *time_datanode =
      new DataNode("Time", EP_32S, {1}, &time_, false, "Time counter", true);

  // Add the data nodes as items to the message (message takes ownership of the
  // data nodes)
  msg->addItem(sensor_datanode);
  msg->addItem(time_datanode);

  // Create a queue based on the message's data schema and connect it to sink
  // port 0
  addSinkQueue(0, msg);

  return 0;
}

int32_t SrcTemplate::_start()
{
  // Associate modelling with the writer
  sensor_datanode_ = static_cast<DataNode *>(writer(0)->dataSchema()->item(0));
  time_datanode_ = static_cast<DataNode *>(writer(0)->dataSchema()->item(1));
  return 0;
}

int32_t SrcTemplate::_stop()
{
  return 0;
}

int32_t SrcTemplate::_reset()
{
  return 0;
}

/* Thread function implementation */
int32_t SrcTemplate::_job()
{
  // Retrieve sensor data
  time_++;
  sensor_ = amplitude_ * std::cos(frequency_ * time_);

  // Copy it to queue
  int32_t ptr = writer(0)->startWrite();
  if (ptr >= 0) {
    sensor_datanode_->write(&sensor_);
    time_datanode_->write(&time_);
    writer(0)->endWrite();
    usleep(timeout_);
    return 0;
  }
  usleep(timeout_);
  return ptr;
}

int32_t SrcTemplate::increaseFrequency()
{
  frequency_ += 10;
  return 0;
}

int32_t SrcTemplate::decreaseFrequency()
{
  frequency_ -= 10;
  return 0;
}
