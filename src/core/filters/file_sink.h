// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef FILESINK_H
#define FILESINK_H

#include <stdio.h>
#include <stdlib.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core.h"

namespace ep {

struct FileData;

class FileSink : public Filter {
  public:
    FileSink(const YAML::Node &config);  
    ~FileSink();                         

  protected:
    int _job();
    int _open();   
    int _close();  
    int _set();    
    int _reset();  
    int _start();  
    int _stop();   

  private:
    YAML::Node yaml_config_;

    // Settings
    uint32_t timeout_;
    std::string folder_path_;
    uint64_t max_file_size_;
    std::vector<FileData> files_;

    std::string timestamp_;

    int openFile(FileData &file_data);
    int closeFile(FileData &f);
    int openFiles();
    int closeFiles();
};

struct FileData {
    std::string filename;
    FILE *data_file;
    FILE *hdr_file;
    unsigned int msg_count;
    size_t file_size;
    QueueReader *reader;

    // Constructor to initialize the struct
    FileData(const std::string &fname, QueueReader *r)
        : filename(fname),
          data_file(nullptr),
          hdr_file(nullptr),
          msg_count(0),
          file_size(0),
          reader(r)
    {
    }
};
}  // namespace ep

#endif  // FILESINK_H
