// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "file_sink.h"

#include <cerrno>
#include <cstring>
#include <system_error>

using namespace epf;
using namespace std;

// Helper function to get the current timestamp as a string
std::string get_current_timestamp()
{
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
  return ss.str();
}

FileSink::FileSink(const YAML::Node &config)
    : Filter(config, 10, 0)
{
  std::cout << "[FileSink] constructor" << std::endl;
  yaml_config_ = config;

  folder_path_ = "./";
  timeout_ = 0;
  max_file_size_ = 500000000;  // maximum file size in bytes

  // You can also add some filter specific commands
  // addCommand("start", std::bind(&FileSink::start, this));
  // addCommand("stop", std::bind(&FileSink::stop, this));

  addSetting("path", folder_path_);
  addSetting("timeout", timeout_);
  addSetting("max file size", max_file_size_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);
  //  settings_.print();
}

FileSink::~FileSink()
{
  closeFiles();
  cout << "[FileSink] destructor" << endl;
}

int FileSink::openFile(FileData &file_data)
{
  if (ensureOutputDirectory() != 0) {
    return -1;
  }

  // Construct the new filename with timestamp and .bin suffix for data files
  std::string data_filename = folder_path_ + "/" + timestamp_ + "_" +
                              std::to_string(file_data.msg_count) + "_" +
                              file_data.filename + "_dat.bin";

  file_data.data_file = std::fopen(data_filename.c_str(), "wb");
  if (!file_data.data_file) {
    std::cerr << "[FileSink] Error opening data file: '" << data_filename
              << "'." << std::endl;
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

    file_data.hdr_file = std::fopen(hdr_filename.c_str(), "wb");
    if (!file_data.hdr_file) {
      std::cerr << "[FileSink] Error opening header file: '" << hdr_filename
                << "'." << std::endl;
      std::fclose(file_data.data_file);
      file_data.data_file = nullptr;
      return -1;
    }
  }

  // hdr_filename =
  //     folder_path_ + "/" + timestamp + "_" + file_data.filename + "_hdr.yml";
  // message_to_yaml(*(file_data.reader->hdr()), hdr_filename);

  return 0;
}

int FileSink::closeFile(FileData &f)
{
  std::string data_filename;
  std::string hdr_filename;
  std::string data_schema_filename;
  std::string hdr_schema_filename;
  if (f.file_size == 0) {
    data_filename = folder_path_ + "/" + timestamp_ + "_" +
                    std::to_string(f.msg_count) + "_" + f.filename + "_dat.bin";
    data_schema_filename =
        folder_path_ + "/" + timestamp_ + "_" + f.filename + "_dat.yml";
    if (f.hdr_file) {
      hdr_filename = folder_path_ + "/" + timestamp_ + "_" +
                     std::to_string(f.msg_count) + "_" + f.filename +
                     "_hdr.bin";
      hdr_schema_filename =
          folder_path_ + "/" + timestamp_ + "_" + f.filename + "_hdr.yml";
    }
  }

  if (f.data_file) {
    std::fclose(f.data_file);
    f.data_file = nullptr;
  }
  if (f.hdr_file) {
    std::fclose(f.hdr_file);
    f.hdr_file = nullptr;
  }

  if (!data_filename.empty()) {
    std::error_code ec;
    std::filesystem::remove(data_filename, ec);
    if (!hdr_filename.empty()) std::filesystem::remove(hdr_filename, ec);
    if (!data_schema_filename.empty())
      std::filesystem::remove(data_schema_filename, ec);
    if (!hdr_schema_filename.empty())
      std::filesystem::remove(hdr_schema_filename, ec);
  }

  f.file_size = 0;

  return 0;
}

int FileSink::openFiles()
{
  if (ensureOutputDirectory() != 0) {
    return -1;
  }

  // Get the current timestamp
  timestamp_ = get_current_timestamp();
  for (auto &file_data : files_) {
    // Construct the new filename with timestamp and .bin suffix for data files
    std::string data_filename = folder_path_ + "/" + timestamp_ + "_0_" +
                                file_data.filename + "_dat.bin";

    file_data.data_file = std::fopen(data_filename.c_str(), "wb");
    if (!file_data.data_file) {
      std::cerr << "[FileSink] Error opening data file: '" << data_filename
                << "'." << std::endl;
      // Ensure we close any previously opened files in case of an error
      closeFiles();
      return -1;
    }
    data_filename =
        folder_path_ + "/" + timestamp_ + "_" + file_data.filename + "_dat.yml";
    serialize_message(*(file_data.reader->dataSchema()), data_filename);

    // Construct the new filename with timestamp and .yml suffix for hdr files
    if (file_data.reader->hdrSchema() != nullptr) {
      std::string hdr_filename = folder_path_ + "/" + timestamp_ + "_0_" +
                                 file_data.filename + "_hdr.bin";
      file_data.hdr_file = std::fopen(hdr_filename.c_str(), "wb");
      if (!file_data.hdr_file) {
        std::cerr << "[FileSink] Error opening header file: '" << hdr_filename
                  << "'." << std::endl;
        // Ensure we close any previously opened files in case of an error
        closeFiles();
        return -1;
      }

      hdr_filename = folder_path_ + "/" + timestamp_ + "_" +
                     file_data.filename + "_hdr.yml";
      serialize_message(*(file_data.reader->hdrSchema()), hdr_filename);
    }
  }
  return 0;
}

int FileSink::closeFiles()
{
  for (auto &file_data : files_) {
    closeFile(file_data);
  }
  return 0;
}

int FileSink::ensureOutputDirectory() const
{
  if (folder_path_.empty()) {
    std::cerr << "Error: recording path is empty." << std::endl;
    return -1;
  }

  std::error_code ec;
  std::filesystem::path output_dir(folder_path_);
  if (std::filesystem::exists(output_dir, ec)) {
    if (ec) {
      std::cerr << "Error checking output directory '" << folder_path_
                << "': " << ec.message() << std::endl;
      return -1;
    }
    if (!std::filesystem::is_directory(output_dir, ec)) {
      std::cerr << "Error: output path is not a directory: " << folder_path_
                << std::endl;
      return -1;
    }
    return 0;
  }

  if (ec) {
    std::cerr << "Error checking output directory '" << folder_path_
              << "': " << ec.message() << std::endl;
    return -1;
  }

  if (!std::filesystem::create_directories(output_dir, ec)) {
    // The directory may have been created by another thread/process between
    // exists() and create_directories(); accept that case.
    if (!ec && std::filesystem::exists(output_dir) &&
        std::filesystem::is_directory(output_dir)) {
      return 0;
    }
    if (ec) {
      std::cerr << "Error creating output directory '" << folder_path_
                << "': " << ec.message() << std::endl;
    }
    else {
      std::cerr << "Error creating output directory '" << folder_path_ << "'."
                << std::endl;
    }
    return -1;
  }

  return 0;
}

int FileSink::_set()
{
  std::cout << "[FileSink] set" << std::endl;
  closeFiles();
  files_.clear();

  std::string fname = "q";

  // check active queues / readers
  //  auto sources = connectedSources();
  auto sources = connectedSources();
  for (auto &i : sources) {
    fname += std::to_string(i);
    // Add a FileData struct for each filename
    files_.emplace_back(fname, sourcePort(i)->reader());
  }
  return 0;
}

int32_t FileSink::_job()
{
  bool any = false;
  int last_err = 0;

  for (auto &file : files_) {
    QueueReader *reader = file.reader;
    int err =
        reader->startRead(reader->messageWindow(), reader->messageStride());

    if (err >= 0) {
      auto data_size = reader->dataSchema()->size();
      if (!file.data_file) {
        std::cerr << "[FileSink] ERROR! Data file handle is null for '"
                  << file.filename << "'." << std::endl;
        reader->endRead();
        last_err = -1;
        continue;
      }

      const size_t expected_data_a = reader->lenA() * data_size;
      const size_t expected_data_b = reader->lenB() * data_size;
      size_t written_data_a =
          fwrite(reader->dataPtrA(), 1, expected_data_a, file.data_file);
      size_t written_data_b =
          fwrite(reader->dataPtrB(), 1, expected_data_b, file.data_file);
      if (written_data_a != expected_data_a ||
          written_data_b != expected_data_b) {
        std::cerr << "[FileSink] ERROR! Failed writing data for '"
                  << file.filename << "'. A: " << written_data_a << "/"
                  << expected_data_a << ", B: " << written_data_b << "/"
                  << expected_data_b << ", errno=" << errno << " ("
                  << std::strerror(errno) << ")" << std::endl;
        reader->endRead();
        return -1;
      }
      any = true;

      // Write header data if hdr_file is not nullptr
      if (file.hdr_file) {
        auto header_size = reader->hdrSchema()->size();
        const size_t expected_hdr_a = reader->lenA() * header_size;
        const size_t expected_hdr_b = reader->lenB() * header_size;
        size_t written_hdr_a =
            fwrite(reader->hdrPtrA(), 1, expected_hdr_a, file.hdr_file);
        size_t written_hdr_b =
            fwrite(reader->hdrPtrB(), 1, expected_hdr_b, file.hdr_file);
        if (written_hdr_a != expected_hdr_a ||
            written_hdr_b != expected_hdr_b) {
          std::cerr << "[FileSink] ERROR! Failed writing header for '"
                    << file.filename << "'. A: " << written_hdr_a << "/"
                    << expected_hdr_a << ", B: " << written_hdr_b << "/"
                    << expected_hdr_b << ", errno=" << errno << " ("
                    << std::strerror(errno) << ")" << std::endl;
          reader->endRead();
          return -1;
        }
      }

      file.msg_count += reader->messageWindow();
      file.file_size += data_size;

      if (file.file_size > max_file_size_) {
        std::cout << "[FileSink] rotating file at size " << file.file_size
                  << std::endl;
        closeFile(file);
        if (openFile(file) != 0) {
          std::cerr << "[FileSink] Error reopening rotated file for '"
                    << file.filename << "'." << std::endl;
          reader->endRead();
          return -1;
        }
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

int FileSink::_open()
{
  std::cout << "[FileSink] open" << std::endl;
  return 0;
}
int FileSink::_close()
{
  std::cout << "[FileSink] close" << std::endl;
  closeFiles();
  return 0;
}

int FileSink::_start()
{
  std::cout << "[FileSink] start" << std::endl;
  for (auto &file_data : files_) {
    file_data.msg_count = 0;
    file_data.file_size = 0;
  }
  return openFiles();
}

int FileSink::_stop()
{
  std::cout << "[FileSink] stop" << std::endl;
  closeFiles();
  return 0;
}

int FileSink::_reset()
{
  std::cout << "[FileSink] reset" << std::endl;
  closeFiles();
  files_.clear();
  return 0;
}
