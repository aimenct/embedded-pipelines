// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filter_template.h"

using namespace ep;

FilterTemplate::FilterTemplate(const YAML::Node &config)
    : Filter()
{
  // Base filter setting will be overwritten by readSettings if found in config
  // YAML
  name_ = "MyFilter";
  type_ = "FilterTemplate";
  max_sinks_ = 10;
  max_sources_ = 10;

  // My filter settings
  timeout_ = 0;

  // Add some parameters as settings (they will be exposed)
  addSetting("Timeout", timeout_, FILTER_SETTING, "job() timeout in us");

  // update settings from YAML
  readSettings(config);
}

FilterTemplate::~FilterTemplate()
{
  std::cout << "FilterTemplate destructor" << std::endl;
}

int FilterTemplate::_set()
{
  // check compatibility - at least one source should be connected
  connected_sources_ = connectedSources();
  if (connected_sources_.size() <= 0) {  // not source queue conected
    std::cerr << "FilterTemplate _set fail" << std::endl;
    return -1;
  }

  // create as many sources as sinks with same data schema
  for (auto id : connected_sources_) {
    // create data message schema of the sink queue with the same stucture as
    // source queue
    Message *data_msg_schema = new Message(*(reader(id)->dataSchema()));

    // create queue and connect to source port 0
    addSinkQueue(id, data_msg_schema);
  }

  return 0;
}

int32_t FilterTemplate::_job()
{
  bool any = false;
  int err = 0;

  // Loop through all connected sources and copy data to corresponding sinks
  for (auto id : connected_sources_) {
    // Attempt to get a buffer from the source queue
    err = reader(id)->startRead();
    if (err < 0) {
      continue;  // Skip to the next source if reading fails
    }

    // Attempt to get a buffer from the sink queue
    err = writer(id)->startWrite();
    if (err < 0) {
      reader(id)->endReadAbort();
      continue;  // Skip to the next source if writing fails
    }

    // Copy data: the complete message's streamed data
    std::memcpy(writer(id)->dataPtrA(), reader(id)->dataPtrA(),
                reader(id)->dataSchema()->size());

    // Finalize the read and write operations
    writer(id)->endWrite();
    reader(id)->endRead();
    any = true;
  }

  // Wait if no operations were performed successfully
  if (!any) {
    usleep(timeout_);
  }

  // Return 0 if any operation succeeded, otherwise return the last error code
  return any ? 0 : err;
}

int32_t FilterTemplate::_open()
{
  return 0;
}
int32_t FilterTemplate::_close()
{
  return 0;
}

int32_t FilterTemplate::_start()
{
  return 0;
}

int32_t FilterTemplate::_stop()
{
  return 0;
}

int32_t FilterTemplate::_reset()
{
  return 0;
}
