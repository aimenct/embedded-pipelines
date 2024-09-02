// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef FilterTemplate_H
#define FilterTemplate_H

#include "core.h"

using namespace ep;

class FilterTemplate : public Filter {
  public:
    FilterTemplate(const YAML::Node &config);  // create settings
    ~FilterTemplate();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device, conf number of filter
                   // sinks, sources, readers writers
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // close connection with device
    int32_t _stop();   // create queues and allocate memory

  private:
    // Settings
    uint32_t timeout_;

    // Private variables
    std::vector<int> connected_sources_;
};

#endif
