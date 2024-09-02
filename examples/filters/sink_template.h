// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _SinkTemplate
#define _SinkTemplate

#include "core.h"

class SinkTemplate : public ep::Filter {
  public:
    SinkTemplate(const YAML::Node& config);  // create settings
    ~SinkTemplate();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // enable filter's job execution
    int32_t _stop();  // disable filter's job execution

  private:
    // Settings
    uint32_t timeout_;

    // Private variables
    std::vector<int> connected_sources_;
};

#endif
