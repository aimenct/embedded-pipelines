// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "file_src.h"

#include <cerrno>
#include <cstring>

using namespace epf;
using namespace std;

namespace fs = std::filesystem;

// Function to extract directory from a filename
std::string extract_directory(const std::string &filepath)
{
  return std::filesystem::path(filepath).parent_path().string();
}

// Function to extract the base filename without extension
std::string extract_base_filename(const std::string &filename)
{
  //  return std::filesystem::path(filename).stem().string();
  return std::filesystem::path(filename).filename().string();
}

// Function to add matching files to a vector
int add_matching_files(const std::string &filename,
                       std::vector<std::string> &filenames)
{
  std::string directory = extract_directory(filename);
  std::string base_filename = extract_base_filename(filename);

  // Regular expression to match filenames
  std::regex bin_pattern(R"((\d{8}_\d{6})_(\d+)_q(\d+)_(\w+)\.bin)");
  std::regex yml_pattern(R"((\d{8}_\d{6})_q(\d+)_(\w+).yml)");

  std::smatch match;
  if (!std::regex_match(base_filename, match, yml_pattern)) {
    std::cerr << "Filename format does not match the expected pattern."
              << std::endl;
    return -1;
  }

  std::string timestamp = match[1].str();
  std::string queue_index = match[2].str();
  std::string type = match[3].str();

  std::vector<std::string> matching_files;

  // Iterate over the files in the directory
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    std::string current_filename = entry.path().filename().string();
    if (std::regex_match(current_filename, match, bin_pattern)) {
      if (match[1].str() == timestamp && match[3].str() == queue_index &&
          match[4].str() == type) {
        matching_files.push_back(entry.path().string());
      }
    }
  }

  // Sort the matching files based on the count
  std::sort(matching_files.begin(), matching_files.end(),
            [&](const std::string &a, const std::string &b) {
              std::smatch match_a, match_b;
              std::string base_a = extract_base_filename(a);
              std::string base_b = extract_base_filename(b);
              if (std::regex_match(base_a, match_a, bin_pattern) &&
                  std::regex_match(base_b, match_b, bin_pattern)) {
                return std::stoi(match_a[2].str()) <
                       std::stoi(match_b[2].str());
              }
              else {
                // If the filenames do not match the pattern, keep the original
                // order
                return a < b;
              }
            });

  // Add sorted filenames to the provided vector
  filenames.insert(filenames.end(), matching_files.begin(),
                   matching_files.end());

  return 0;
}

//

FileSrc::FileSrc(const YAML::Node &config)
    : Filter(config, 0, 1),
      timeout_(0),
      data_filename_(),
      hdr_filename_(),
      loop_(false),
      data_files_(),
      hdr_files_(),
      file_ptr_(0),
      data_file_(nullptr),
      hdr_file_(nullptr),
      remaining_msgs_(0),
      batch_(0),
      w_(nullptr)
{
  std::cout << "[FileSrc] constructor" << std::endl;
  yaml_config_ = config;

  timeout_ = 0;

  addSetting("data_file", data_filename_);
  addSetting("hdr_file", hdr_filename_);
  addSetting("timeout", timeout_);
  addSetting("loop", loop_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);
  settings_.print();
}

FileSrc::~FileSrc()
{
  closeFiles();
  cout << "[FileSrc] destructor" << endl;
}

int FileSrc::_set()
{
  std::cout << "[FileSrc] set" << std::endl;
  closeFiles();
  data_files_.clear();
  hdr_files_.clear();
  file_ptr_ = 0;
  remaining_msgs_ = 0;
  batch_ = 0;
  w_ = nullptr;

  YAML::Node data_node = YAML::LoadFile(data_filename_);
  if (!data_node) {
    std::cerr << "[FileSrc] Failed to load data schema YAML file: '"
              << data_filename_ << "'." << std::endl;
    return -1;
  }
  //  Message* data_msg = message_from_yaml(data_node);
  auto data_msg = deserialize_message(data_node);
  if (add_matching_files(data_filename_, data_files_) < 0) return -1;

  // initialize queue
  YAML::Node hdr_node = YAML::LoadFile(hdr_filename_);
  if (!hdr_node) {
    std::cerr << "[FileSrc] Failed to load header schema YAML file: '"
              << hdr_filename_ << "'." << std::endl;
    return -1;
  }
  //  Message* hdr_msg = message_from_yaml(hdr_node);
  auto hdr_msg = deserialize_message(hdr_node);
  if (add_matching_files(hdr_filename_, hdr_files_) < 0) return -1;

  //  addSinkQueue(0, std::move(data_msg), std::move(hdr_msg));
  sinkPort(0)->activate(std::move(data_msg), std::move(hdr_msg));
  w_ = sinkPort(0)->writer();

  file_ptr_ = 0;
  batch_ = w_->batchSize();

  int32_t ret = openFiles();

  return ret;
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
  if (file_ptr_ < 0 || file_ptr_ >= static_cast<int>(data_files_.size()) ||
      file_ptr_ >= static_cast<int>(hdr_files_.size())) {
    std::cerr << "[FileSrc] File index out of range. index=" << file_ptr_
              << ", data_files=" << data_files_.size()
              << ", hdr_files=" << hdr_files_.size() << std::endl;
    return -1;
  }

  data_file_ = std::fopen(data_files_[file_ptr_].c_str(), "rb");
  if (!data_file_) {
    std::cerr << "[FileSrc] Error opening data file: '"
              << data_files_[file_ptr_] << "'." << std::endl;
    return -1;
  }
  // Get the total file size
  std::fseek(data_file_, 0, SEEK_END);
  uint64_t file_size = std::ftell(data_file_);
  std::fseek(data_file_, 0, SEEK_SET);
  std::cout << "Total file size: " << file_size << " bytes" << std::endl;
  std::cout << "Dataschema size: " << w_->dataSchema()->size() << std::endl;
  remaining_msgs_ =
      static_cast<int32_t>((file_size / w_->dataSchema()->size()));

  hdr_file_ = std::fopen(hdr_files_[file_ptr_].c_str(), "rb");
  if (!hdr_file_) {
    std::cerr << "[FileSrc] Error opening header file: '"
              << hdr_files_[file_ptr_] << "'." << std::endl;
    std::fclose(data_file_);
    data_file_ = nullptr;
    return -1;
  }

  std::cout << "[FileSrc] open files OK" << std::endl;
  return 0;
}

int32_t FileSrc::_job()
{
  int err = w_->startWrite(batch_);
  if (err < 0) {
    usleep(timeout_);
    return err;
  }

  auto read_data = [&](void *ptr, size_t size, size_t count, FILE *file,
                       const std::string &name) {
    size_t elements_read = std::fread(ptr, size, count, file);
    if (elements_read != count) {
      std::cerr << "[FileSrc] ERROR! Short read on " << name
                << ". read=" << elements_read << ", expected=" << count
                << ", element_size=" << size << ", errno=" << errno << " ("
                << std::strerror(errno) << ")" << std::endl;
    }
    return elements_read;
  };

  // Read data file
  if (data_file_) {
    read_data(w_->dataPtrA(), w_->dataSchema()->size(), w_->lenA(), data_file_,
              "data file (A)");
    read_data(w_->dataPtrB(), w_->dataSchema()->size(), w_->lenB(), data_file_,
              "data file (B)");
  }

  // Read header file
  if (hdr_file_) {
    read_data(w_->hdrPtrA(), w_->hdrSchema()->size(), w_->lenA(), hdr_file_,
              "header file (A)");
    read_data(w_->hdrPtrB(), w_->hdrSchema()->size(), w_->lenB(), hdr_file_,
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
        std::cout << "[FileSrc] job end - emit stop (feature not implemented)"
                  << std::endl;
        stop();
        return -1;
      }
    }
    openFiles();
  }

  usleep(timeout_);
  return 0;
}

int FileSrc::_open()
{
  std::cout << "[FileSrc] open" << std::endl;
  return 0;
}
int FileSrc::_close()
{
  std::cout << "[FileSrc] close" << std::endl;
  closeFiles();
  return 0;
}

int FileSrc::_start()
{
  std::cout << "[FileSrc] start" << std::endl;
  return 0;
}

int FileSrc::_stop()
{
  std::cout << "[FileSrc] stop" << std::endl;
  closeFiles();
  return 0;
}

int FileSrc::_reset()
{
  std::cout << "[FileSrc] reset" << std::endl;
  closeFiles();
  data_files_.clear();
  hdr_files_.clear();
  file_ptr_ = 0;
  remaining_msgs_ = 0;
  batch_ = 0;
  w_ = nullptr;
  return 0;
}
