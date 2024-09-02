// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/////////////////////////////////////////
//        IMAGE SOURCE FILTER          //
/////////////////////////////////////////
// Read & stream images from a folder  //
/////////////////////////////////////////

#ifndef IMGSRC_H
#define IMGSRC_H

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core.h"

using namespace ep;

class ImgSrc : public Filter {
  public:
    ImgSrc(const YAML::Node &config);  // create settings
    ~ImgSrc();                         // delete settings

    std::vector<fteng::signal<void(std::string name, void *value)> >
        filter_signals;

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // close connection with device
    int32_t _stop();   // create queues and allocate memory

  private:
    YAML::Node yaml_config_;

    // Settings
    uint32_t timeout_;
    unsigned int queue_length_;  // output queue lenght
    unsigned int queue_type_;    // fifo-0 , lifo-1
    std::string folder_path_;
    char loop_;

    // Internal variables
    ImageObject *img_obj_;
    std::string file_;
    std::vector<std::string> files_;
    size_t image_index_;
};

#endif  // IMGSRC_H
