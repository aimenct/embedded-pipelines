// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sink_template.h"

using namespace ep;

SinkTemplate::SinkTemplate(const YAML::Node &config)
    : Filter()
{
  // Base filter setting will be overwritten by readSettings if found in config
  // YAML
  name_ = "MySinkTemplate";
  type_ = "SinkTemplate";
  max_sources_ = 5;
  max_sinks_ = 0;

  // My filter settings
  timeout_ = 0;
  // Add some parameters as settings (they will be exposed)
  addSetting("Timeout", timeout_, FILTER_SETTING, "job() timeout in us");

  // update settings from YAML
  readSettings(config);
}

SinkTemplate::~SinkTemplate()
{
  std::cout << "SinkTemplate destructor" << std::endl;
}

int32_t SinkTemplate::_set()
{
  // check compatibility - at least one source should be connected
  connected_sources_ = connectedSources();
  if (connected_sources_.size() <= 0) {  // not source queue conected
    std::cerr << "SinkTemplate _set fail" << std::endl;
    return -1;
  }

  return 0;
}

int32_t SinkTemplate::_job()
{
  bool any = false;
  // loop through all connected sources and copy data to corresponding sinks
  for (auto id : connected_sources_) {
    // try to get a buffer from the source queue
    int err = 0;
    if ((err = reader(id)->startRead()) >= 0) {
      any = true;
      reader(id)->dataMsg(0).print();
      reader(id)->endRead();
    }
  }
  if (any) return 0;
  // wait us
  usleep(timeout_);
  return -1;
}

int32_t SinkTemplate::_open()
{
  return 0;
}
int32_t SinkTemplate::_close()
{
  return 0;
}

int32_t SinkTemplate::_start()
{
  return 0;
}

int32_t SinkTemplate::_stop()
{
  return 0;
}

int32_t SinkTemplate::_reset()
{
  return 0;
}
