// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "file_sink.h"

using namespace ep;
using namespace std;

// Helper function to get the current timestamp as a string
std::string getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
  return ss.str();
}

FileSink::FileSink(const YAML::Node& config)
    : Filter()
{
  std::cout << "FileSink constructor " << std::endl;
  yaml_config_ = config;

  name_ = "my_name";
  max_sources_ = 10;
  max_sinks_ = 0;
  folder_path_ = "./";
  timeout_ = 0;
  max_file_size_ = 500000000;  // maximum file size in bytes

  if (config["name"]) name_ = config["name"].as<std::string>();

  // if (config["filter_settings"]) {
  //   printf("FILTER SETTINGS\n");
  //   getchar();
  // }

  // if (config["filter_commands"]) {
  //   printf("FILTER COMMANDS\n");
  //   getchar();
  // }

  // if (config["device_settings"]) {
  //   printf("DEVICE SETTINGS\n");
  //   getchar();
  // }

  // if (config["queue_settings"]) {
  //   printf("QUEUE SETTINGS\n");
  //   getchar();
  // }

  addSetting("path", folder_path_);
  // addSetting("max sources", max_src_queues_, false, "queues", W); // todo -
  // change status addSetting("max sinks", max_sink_queues_, false, "queues",
  // W);  // todo - change status
  addSetting("timeout", timeout_);
  addSetting("max file size", max_file_size_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);

  // // yujuu
  // // todo - move to another place
  // if (yaml_settings["queue_settings"]) {
  //   if (yaml_settings["queue_settings"]["max sources"])
  //     max_src_queues_ =
  //         yaml_settings["queue_settings"]["max sources"].as<int>();
  //   if (yaml_settings["queue_settings"]["max sinks"])
  //     max_sink_queues_ = yaml_settings["queue_settings"]["max
  //     sinks"].as<int>();

  //   sink_lengths_.clear();
  //   if (yaml_settings["queue_settings"]["sink length"])
  //     sink_lengths_ =
  //         yaml_settings["queue_settings"]["sink
  //         length"].as<std::vector<int>>();
  //   else
  //     // Initialize with default values
  //     sink_lengths_ = std::vector<int>(max_sink_queues_, 10);

  //   std::cout << "max sources: " << max_src_queues_ << std::endl;
  //   std::cout << "max sinks: " << max_sink_queues_ << std::endl;
  //   std::cout << "sink lengths: ";
  //   for (const int& length : sink_lengths_) {
  //     std::cout << length << " ";
  //   }
  //   std::cout << std::endl;
  // }
  // else {
  //   std::cout << "'sink length' not found." << std::endl;
  //   sink_lengths_ = std::vector<int>(max_sink_queues_, 10);
  //}

  //  settings_.print();
}

FileSink::~FileSink()
{
  cout << "FileSink destructor" << endl;
}

int FileSink::openFile(FileData& file_data)
{
  // Construct the new filename with timestamp and .bin suffix for data files
  std::string data_filename = folder_path_ + "/" + timestamp_ + "_" +
                              std::to_string(file_data.msg_count) + "_" +
                              file_data.filename + "_dat.bin";

  file_data.data_file = std::fopen(data_filename.c_str(), "wb");
  if (!file_data.data_file) {
    std::cerr << "Error opening file: " << file_data.filename << std::endl;
    // Ensure we close any previously opened files in case of an error
    std::fclose(file_data.data_file);
    return -1;
  }

  // data_filename =
  //     folder_path_ + "/" + timestamp + "_" + file_data.filename + ".yml";
  // message_to_yaml(*(file_data.reader->msg()), data_filename);

  // Construct the new filename with timestamp and .yml suffix for hdr files
  if (file_data.reader->hdrSchema() != nullptr) {
    std::string hdr_filename = folder_path_ + "/" + timestamp_ + "_" +
                               std::to_string(file_data.msg_count) + "_" +
                               file_data.filename + "_hdr.bin";

    file_data.hdr_file = std::fopen(hdr_filename.c_str(), "w");
    if (!file_data.hdr_file) {
      std::cerr << "Error opening file: " << hdr_filename << std::endl;
      // Ensure we close any previously opened files in case of an error
      std::fclose(file_data.hdr_file);
      return -1;
    }
  }

  // hdr_filename =
  //     folder_path_ + "/" + timestamp + "_" + file_data.filename + "_hdr.yml";
  // message_to_yaml(*(file_data.reader->hdr()), hdr_filename);

  return 0;
}

int FileSink::closeFile(FileData& f)
{
  if (f.data_file) {
    std::fclose(f.data_file);
    f.data_file = nullptr;
  }
  if (f.hdr_file) {
    std::fclose(f.hdr_file);
    f.hdr_file = nullptr;
  }

  f.file_size = 0;

  return 0;
}

int FileSink::openFiles()
{
  // Get the current timestamp
  timestamp_ = getCurrentTimestamp();
  for (auto& file_data : files_) {
    // Construct the new filename with timestamp and .bin suffix for data files
    std::string data_filename = folder_path_ + "/" + timestamp_ + "_0_" +
                                file_data.filename + "_dat.bin";

    file_data.data_file = std::fopen(data_filename.c_str(), "wb");
    if (!file_data.data_file) {
      std::cerr << "Error opening file: " << data_filename << std::endl;
      // Ensure we close any previously opened files in case of an error
      closeFiles();
      return -1;
    }
    data_filename =
        folder_path_ + "/" + timestamp_ + "_" + file_data.filename + "_dat.yml";
    message_to_yaml(*(file_data.reader->dataSchema()), data_filename);

    // Construct the new filename with timestamp and .yml suffix for hdr files
    if (file_data.reader->hdrSchema() != nullptr) {
      std::string hdr_filename = folder_path_ + "/" + timestamp_ + "_0_" +
                                 file_data.filename + "_hdr.bin";
      file_data.hdr_file = std::fopen(hdr_filename.c_str(), "w");
      if (!file_data.hdr_file) {
        std::cerr << "Error opening file: " << hdr_filename << std::endl;
        // Ensure we close any previously opened files in case of an error
        closeFiles();
        return -1;
      }

      hdr_filename = folder_path_ + "/" + timestamp_ + "_" +
                     file_data.filename + "_hdr.yml";
      message_to_yaml(*(file_data.reader->hdrSchema()), hdr_filename);
    }
  }
  return 0;
}

int FileSink::closeFiles()
{
  for (auto& file_data : files_) {
    if (file_data.data_file) {
      std::fclose(file_data.data_file);
      file_data.data_file = nullptr;
    }
    if (file_data.hdr_file) {
      std::fclose(file_data.hdr_file);
      file_data.hdr_file = nullptr;
    }
  }
  return 0;
}

int FileSink::_set()
{
  std::cout << "FileSink Set" << std::endl;

  std::string fname = "q";

  // check active queues / readers
  auto sources = connectedSources();
  for (auto& i : sources) {
    fname += std::to_string(i);
    // Add a FileData struct for each filename
    files_.emplace_back(fname, reader(i));
  }
  return 0;
}

int32_t FileSink::_job()
{
  bool any = false;
  int last_err = 0;

  for (auto& file : files_) {
    QueueReader* reader = file.reader;
    int err = reader->startRead(reader->batchSize(), reader->newPerBatch());

    if (err >= 0) {
      any = true;

      auto data_size = reader->dataSchema()->size();
      fwrite(reader->dataPtrA(), 1, reader->lenA() * data_size, file.data_file);
      fwrite(reader->dataPtrB(), 1, reader->lenB() * data_size, file.data_file);

      // Write header data if hdr_file is not nullptr
      if (file.hdr_file) {
        auto header_size = reader->hdrSchema()->size();
        fwrite(reader->hdrPtrA(), 1, reader->lenA() * header_size,
               file.hdr_file);
        fwrite(reader->hdrPtrB(), 1, reader->lenB() * header_size,
               file.hdr_file);
      }

      file.msg_count += reader->batchSize();
      file.file_size += data_size;

      if (file.file_size > max_file_size_) {
        std::cout << "file size " << file.file_size << std::endl;
        closeFile(file);
        openFile(file);
      }

      reader->endRead();
    }
    else {
      last_err = err;  // Capture the last error encountered
    }
  }

  usleep(timeout_);
  return any ? 0 : last_err;
}

// int FileSink::job()
// {
//   bool any = false;

//   int err = 0;
//   for (auto& f : files_) {
//     QueueReader* r = f.reader;
//     err = r->startRead(r->batchSize(), r->newPerBatch());

//     if (err >= 0) {
//       any = true;

//       fwrite(r->dataPtrA(), 1, r->lenA() * r->msg()->size(), f.data_file);
//       fwrite(r->dataPtrB(), 1, r->lenB() * r->msg()->size(), f.data_file);

//       // Access the pointers (hdrA, hdrB) in the circular buffer
//       // pointing to start of the header messages block
//       if (f.hdr_file != nullptr) {
//         fwrite(r->hdrPtrA(), 1, r->lenA() * r->hdr()->size(), f.hdr_file);
//         fwrite(r->hdrPtrB(), 1, r->lenB() * r->hdr()->size(), f.hdr_file);
//       }

//       f.msg_count += r->batchSize();
//       f.file_size += r->msg()->size();

//       if (f.file_size > max_file_size_) {
//         std::cout << "file size " << f.file_size << std::endl;
//         closeFile(f);
//         openFile(f);
//       }

//       r->endRead();
//     }
//     // else
//     // 	printf("filesink do job err\n");
//   }
//   if (any) return 0;
//   usleep(timeout_);
//   return err;
// }

int FileSink::_open()
{
  std::cout << "FileSink open" << std::endl;
  return 0;
}
int FileSink::_close()
{
  std::cout << "FileSink close" << std::endl;
  return 0;
}

int FileSink::_start()
{
  std::cout << "FileSink start" << std::endl;
  openFiles();
  return 0;
}

int FileSink::_stop()
{
  std::cout << "FileSink stop" << std::endl;
  closeFiles();
  return 0;
}

int FileSink::_reset()
{
  std::cout << "FileSink reset" << std::endl;
  files_.clear();
  return 0;
}
