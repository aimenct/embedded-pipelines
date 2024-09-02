// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "file_src.h"

using namespace ep;
using namespace std;

namespace fs = std::filesystem;

// Function to extract directory from a filename
std::string extractDirectory(const std::string& filepath)
{
  return std::filesystem::path(filepath).parent_path().string();
}

// Function to extract the base filename without extension
std::string extractBaseFilename(const std::string& filename)
{
  //  return std::filesystem::path(filename).stem().string();
  return std::filesystem::path(filename).filename().string();
}

// Function to add matching files to a vector
int addMatchingFiles(const std::string& filename,
                     std::vector<std::string>& filenames)
{
  std::string directory = extractDirectory(filename);
  std::string baseFilename = extractBaseFilename(filename);

  std::cout << baseFilename << std::endl;
  getchar();

  // Regular expression to match filenames
  std::regex bin_pattern(R"((\d{8}_\d{6})_(\d+)_q(\d+)_(\w+)\.bin)");
  std::regex yml_pattern(R"((\d{8}_\d{6})_q(\d+)_(\w+).yml)");

  std::smatch match;
  if (!std::regex_match(baseFilename, match, yml_pattern)) {
    std::cerr << "Filename format does not match the expected pattern."
              << std::endl;
    return -1;
  }

  std::string timestamp = match[1].str();
  std::string queueIndex = match[2].str();
  std::string type = match[3].str();

  std::vector<std::string> matchingFiles;

  // Iterate over the files in the directory
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    std::string currentFilename = entry.path().filename().string();
    if (std::regex_match(currentFilename, match, bin_pattern)) {
      if (match[1].str() == timestamp && match[3].str() == queueIndex &&
          match[4].str() == type) {
        matchingFiles.push_back(entry.path().string());
      }
    }
  }

  for (auto& f : matchingFiles) std::cout << f << std::endl;
  getchar();

  // Sort the matching files based on the count
  std::sort(matchingFiles.begin(), matchingFiles.end(),
            [&](const std::string& a, const std::string& b) {
              std::smatch matchA, matchB;
              std::string baseA = extractBaseFilename(a);
              std::string baseB = extractBaseFilename(b);
              if (std::regex_match(baseA, matchA, bin_pattern) &&
                  std::regex_match(baseB, matchB, bin_pattern)) {
                return std::stoi(matchA[2].str()) < std::stoi(matchB[2].str());
              }
              else {
                // If the filenames do not match the pattern, keep the original
                // order
                return a < b;
              }
            });

  // Add sorted filenames to the provided vector
  filenames.insert(filenames.end(), matchingFiles.begin(), matchingFiles.end());

  return 0;
}

//

FileSrc::FileSrc(const YAML::Node& config)
    : Filter()
{
  std::cout << "FileSrc constructor " << std::endl;
  yaml_config_ = config;

  name_ = "my_name";
  max_sources_ = 0;
  max_sinks_ = 1;
  timeout_ = 0;

  if (config["name"]) name_ = config["name"].as<std::string>();

  addSetting("data file", data_filename_);
  addSetting("hdr file", hdr_filename_);
  addSetting("timeout", timeout_);
  addSetting("loop", loop_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);

  // // todo - move to another place
  // yuju
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
  //     sink_lengths_ = std::vector<int>(max_sink_queues_, 1);

  //   //    std::cout << "max sources: " << max_src_queues_ << std::endl;
  //   //    std::cout << "max sinks: " << max_sink_queues_ << std::endl;
  //   // std::cout << "src lengths: ";
  //   // for (const int& length : src_lengths_) {
  //   //   std::cout << length << " ";
  //   // }
  //   // std::cout << std::endl;
  // }
  // // else {
  // //   std::cout << "'sink length' not found." << std::endl;
  // //   sink_lengths_ = std::vector<int>(max_sink_queues_, 10);
  // // }

  settings_.print();
}

FileSrc::~FileSrc()
{
  cout << "FileSrc destructor" << endl;
}

int FileSrc::_set()
{
  std::cout << "FileSrc Set" << std::endl;

  YAML::Node data_node = YAML::LoadFile(data_filename_);
  if (!data_node) {
    std::cerr << "Failed to load YAML file: " << data_filename_ << std::endl;
    return -1;
  }
  Message* data_msg = message_from_yaml(data_node);
  if (addMatchingFiles(data_filename_, data_files_) < 0) return -1;

  // initialize queue
  YAML::Node hdr_node = YAML::LoadFile(hdr_filename_);
  if (!hdr_node) {
    std::cerr << "Failed to load YAML file: " << hdr_filename_ << std::endl;
    return -1;
  }
  Message* hdr_msg = message_from_yaml(hdr_node);
  if (addMatchingFiles(hdr_filename_, hdr_files_) < 0) return -1;

  for (auto& f : data_files_) std::cout << f << std::endl;

  for (auto& f : hdr_files_) std::cout << f << std::endl;

  getchar();

  data_msg->print();
  getchar();

  hdr_msg->print();
  getchar();

  addSinkQueue(0, data_msg, hdr_msg);
  w_ = writer(0);

  file_ptr_ = 0;
  batch_ = w_->batchSize();

  cout << "batch " << batch_ << endl;
  getchar();

  openFiles();

  return 0;
}

int FileSrc::closeFiles()
{
  if (data_file_) {
    std::fclose(data_file_);
    data_file_ = nullptr;
  }
  if (hdr_file_) {
    std::fclose(hdr_file_);
    hdr_file_ = nullptr;
  }
  return 0;
}

int FileSrc::openFiles()
{
  data_file_ = std::fopen(data_files_[file_ptr_].c_str(), "rb");
  if (!data_file_) {
    std::cerr << "Error opening file: " << data_files_[file_ptr_] << std::endl;
    return -1;
  }
  // Get the total file size
  std::fseek(data_file_, 0, SEEK_END);
  uint64_t fileSize = std::ftell(data_file_);
  std::fseek(data_file_, 0, SEEK_SET);
  std::cout << "Total file size: " << fileSize << " bytes" << std::endl;
  remaining_msgs_ = static_cast<int32_t>((fileSize / w_->dataSchema()->size()));

  hdr_file_ = std::fopen(hdr_files_[file_ptr_].c_str(), "rb");
  if (!hdr_file_) {
    std::cerr << "Error opening file: " << data_files_[file_ptr_] << std::endl;
    return -1;
  }

  std::cout << "Open Files OK" << std::endl;
  return 0;
}

int32_t FileSrc::_job()
{
  int err = w_->startWrite(batch_);
  if (err < 0) {
    usleep(timeout_);
    return err;
  }

  auto readData = [&](void* ptr, size_t size, size_t count, FILE* file,
                      const std::string& name) {
    size_t elementsRead = std::fread(ptr, size, count, file);
    if (elementsRead != count) {
      std::cerr << "Error: Only " << elementsRead << " elements read from "
                << name << ", expected " << count << std::endl;
    }
    return elementsRead;
  };

  // Read data file
  if (data_file_) {
    readData(w_->dataPtrA(), w_->dataSchema()->size(), w_->lenA(), data_file_,
             "data file (A)");
    readData(w_->dataPtrB(), w_->dataSchema()->size(), w_->lenB(), data_file_,
             "data file (B)");
  }

  // Read header file
  if (hdr_file_) {
    readData(w_->hdrPtrA(), w_->hdrSchema()->size(), w_->lenA(), hdr_file_,
             "header file (A)");
    readData(w_->hdrPtrB(), w_->hdrSchema()->size(), w_->lenB(), hdr_file_,
             "header file (B)");
  }

  w_->endWrite();

  // Handle remaining messages and batch updates
  remaining_msgs_ -= batch_;
  if (remaining_msgs_ >= w_->batchSize()) {
    batch_ = w_->batchSize();
  }
  else if (remaining_msgs_ > 0) {
    batch_ = remaining_msgs_;
  }
  else {
    closeFiles();
    ++file_ptr_;
    if (file_ptr_ >= static_cast<int>(data_files_.size())) {
      if (loop_) {
        file_ptr_ = 0;
      }
      else {
        std::cout << "FileSrc job end - emit stop (feature not implemented)"
                  << std::endl;
        exit(0);
      }
    }
    openFiles();
  }

  usleep(timeout_);
  return 0;
}

// int FileSrc::job()
// {

//   size_t elementsRead;

//   int err = w_->startWrite(batch_);
//   if (err >= 0) {
//     if (data_file_ != nullptr) {
//       elementsRead =
//           std::fread(w_->dataPtrA(), w_->msg()->size(), w_->lenA(),
//           data_file_);
//       if (elementsRead != static_cast<size_t>(w_->lenA())) {
//         std::cerr << "Error: Only " << elementsRead
//                   << " elements read, expected " << w_->lenA() << std::endl;
//       }
//       elementsRead =
//           std::fread(w_->dataPtrB(), w_->msg()->size(), w_->lenB(),
//           data_file_);
//       if (elementsRead != static_cast<size_t>(w_->lenB())) {
//         std::cerr << "Error: Only " << elementsRead
//                   << " elements read, expected " << w_->lenB() << std::endl;
//       }
//     }

//     if (hdr_file_ != nullptr) {
//       elementsRead =
//           std::fread(w_->hdrPtrA(), w_->hdr()->size(), w_->lenA(),
//           hdr_file_);
//       if (elementsRead != static_cast<size_t>(w_->lenA())) {
//         std::cerr << "Error: Only " << elementsRead
//                   << " elements read, expected " << w_->lenA() << std::endl;
//       }
//       elementsRead =
//           std::fread(w_->hdrPtrB(), w_->hdr()->size(), w_->lenB(),
//           hdr_file_);
//       if (elementsRead != static_cast<size_t>(w_->lenB())) {
//         std::cerr << "Error: Only " << elementsRead
//                   << " elements read, expected " << w_->lenB() << std::endl;
//       }
//     }

//     w_->endWrite();

//     // point to new batch
//     remaining_msgs_ -= batch_;
//     if (remaining_msgs_ >= w_->batchSize()) {
//       batch_ = w_->batchSize();
//       std::cout << "FileSrc job remaining msgs>=batchsize: " <<
//       remaining_msgs_
//                 << " batch " << batch_ << std::endl;
//     }
//     else if (remaining_msgs_ > 0) {
//       batch_ = remaining_msgs_;
//       std::cout << "FileSrc job remaining msgs>0:  " << remaining_msgs_
//                 << " batch " << batch_ << std::endl;
//     }
//     else {  // if eof close file
//       closeFiles();
//       file_ptr_++;
//       if ((file_ptr_ > static_cast<int>(data_files_.size())) && (loop_)) {
//         file_ptr_ = 0;
//         std::cout << "FileSrc job loop file_ptr_ " << file_ptr_ << std::endl;
//       }
//       else if ((file_ptr_ > static_cast<int>(data_files_.size())) &&
//       (!loop_)) {
//         // todo emit stop;
//         std::cout << "FileSrc job end - emit stop" << std::endl;
//         exit(0);
//       }
//       else {
//         std::cout << "FileSrc job file_ptr_ " << file_ptr_ << std::endl;
//         file_ptr_ = 0;
//         openFiles();
//       }
//     }
//     usleep(timeout_);
//     return 0;
//   }
//   usleep(timeout_);
//   return err;
// }

int FileSrc::_open()
{
  std::cout << "FileSrc open" << std::endl;
  return 0;
}
int FileSrc::_close()
{
  std::cout << "FileSrc close" << std::endl;
  return 0;
}

int FileSrc::_start()
{
  std::cout << "FileSrc start" << std::endl;
  return 0;
}

int FileSrc::_stop()
{
  std::cout << "FileSrc stop" << std::endl;
  return 0;
}

int FileSrc::_reset()
{
  std::cout << "FileSrc reset" << std::endl;
  closeFiles();
  return 0;
}
