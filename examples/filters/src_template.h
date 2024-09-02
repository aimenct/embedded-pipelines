// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _SrcTemplate
#define _SrcTemplate

#include <cmath>

#include "core.h"

class SrcTemplate : public ep::Filter {
  public:
    SrcTemplate(const YAML::Node& config);  // create settings
    ~SrcTemplate();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // start filters' loop
    int32_t _stop();   // stop filters' loop

    int32_t increaseFrequency();
    int32_t decreaseFrequency();

  private:
    // Values through queue
    int32_t time_ = 0;
    double sensor_;  // A*cos(f*t)

    // Information modelling of queue data
    ep::DataNode* sensor_datanode_;
    ep::DataNode* time_datanode_;

    // Settings of the data source
    double frequency_;
    double amplitude_;
    uint32_t timeout_;
};

#endif
