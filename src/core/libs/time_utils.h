// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <chrono>
#include <cstdint>

namespace epf {

/* Function to get current timestamp in nanoseconds */
inline uint64_t get_timestamp_ns()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

/* Function to get current timestamp in milliseconds */
inline uint64_t get_timestamp_ms()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

/* Function to get current unix timestamp in milliseconds */
inline uint64_t get_unix_timestamp_ms()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

/* Get current time in seconds */
inline double get_timestamp_sec()
{
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace epf

#endif  // IMAGE_UTILS_H
