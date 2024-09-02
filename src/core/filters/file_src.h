// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef FILESOURCE_H
#define FILESOURCE_H

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "core.h"

namespace ep {
class FileSrc : public Filter {
  public:
    FileSrc(const YAML::Node &config); 
    ~FileSrc();                        

  protected:
    int32_t _job();   
    int32_t _open();  
    int32_t _close(); 
    int32_t _set();   
    int32_t _reset(); 
    int32_t _start(); 
    int32_t _stop();  

  private:
    YAML::Node yaml_config_;

    // Settings
    uint32_t timeout_;
    std::string data_filename_;
    std::string hdr_filename_;
    bool loop_;
    std::vector<std::string> data_files_;
    std::vector<std::string> hdr_files_;
    int file_ptr_;

    FILE *data_file_;
    FILE *hdr_file_;
    int32_t remaining_msgs_;
    int32_t batch_;

    QueueWriter *w_;

    int openFiles();
    int closeFiles();
};
}  // namespace ep

#endif  // FILESOURCE_H
