// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "error_logger.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace epf {

ErrorLogger::ErrorLogger()
{
  log_file_.open("epf_error.log", std::ios::app);
}

ErrorLogger::~ErrorLogger()
{
  if (log_file_.is_open()) {
    log_file_.close();
  }
}

ErrorLogger &ErrorLogger::instance()
{
  static ErrorLogger instance;
  return instance;
}

void ErrorLogger::log(LogLevel level, const std::string &message)
{
  std::lock_guard<std::mutex> lock(mtx_);

  auto t = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  std::ostringstream time_stream;
  time_stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

  std::string level_str;
  switch (level) {
    case LogLevel::INFO:
      level_str = "INFO";
      break;
    case LogLevel::WARNING:
      level_str = "WARNING";
      break;
    case LogLevel::ERROR:
      level_str = "ERROR";
      break;
  }

  std::string log_line =
      time_stream.str() + " [" + level_str + "] " + message + "\n";

  std::cerr << log_line;
  if (log_file_.is_open()) {
    log_file_ << log_line;
    log_file_.flush();
  }
}

}  // namespace epf
