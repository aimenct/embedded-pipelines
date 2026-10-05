// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef ERROR_LOGGER_H
#define ERROR_LOGGER_H

#include <fstream>
#include <mutex>
#include <string>

namespace epf {

/**
 * @enum LogLevel
 * @brief Severity levels for log messages.
 */
enum class LogLevel { INFO, WARNING, ERROR };

/**
 * @class ErrorLogger
 * @brief Simple thread-safe logger that writes messages to stderr and a log
 * file.
 */
class ErrorLogger {
  public:
    /** Get the singleton instance. */
    static ErrorLogger &instance();

    /** Log a message with the given severity. */
    void log(LogLevel level, const std::string &message);

  private:
    ErrorLogger();
    ~ErrorLogger();

    ErrorLogger(const ErrorLogger &) = delete;
    ErrorLogger &operator=(const ErrorLogger &) = delete;

    std::ofstream log_file_;
    std::mutex mtx_;
};

}  // namespace epf

/** Convenience macro for logging an info message */
#define LOG_INFO(msg) \
  ::epf::ErrorLogger::instance().log(::epf::LogLevel::INFO, msg)

/** Convenience macro for logging a warning message */
#define LOG_WARNING(msg) \
  ::epf::ErrorLogger::instance().log(::epf::LogLevel::WARNING, msg)

/** Convenience macro for logging an error message */
#define LOG_ERROR(msg) \
  ::epf::ErrorLogger::instance().log(::epf::LogLevel::ERROR, msg)

#endif  // ERROR_LOGGER_H
