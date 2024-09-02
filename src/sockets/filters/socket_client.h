// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_SOCKET_CLIENT_H
#define _EP_SOCKET_CLIENT_H

#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdlib>

#include "core.h"

namespace ep {
namespace sChrono = std::chrono;
using sClock = std::chrono::steady_clock;

class SocketClient : public Filter {
  public:
    SocketClient(const YAML::Node& config);  // create settings
    ~SocketClient();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _set();
    int32_t _reset();
    int32_t _close();
    int32_t _start();
    int32_t _stop();

    void reconnect();
    int socketCfg();

  private:
    YAML::Node yaml_config_;

    // Configured in Set()
    Queue* out_queue_;
    unsigned int in_buff_size_, in_buff_scaling_;
    int ctr_errors_;
    bool enable_shifts_;
    std::vector<int> in_buff_shifts_;
    unsigned int out_buff_size_;
    std::vector<std::vector<int32_t>> out_buff_items_;

    /* Add Filter settings */
    // General settings
    short queue_length_ = 16;
    int timeout_;  // thread function timeout ms

    // Communication settings
    int queue_read_timeout_;
    int socket_read_timeout_;
    int retries_to_reconnect_;

    // Socket settings
    int connection_timeout_;  // thread function timeout ms
    std::string ip_;
    uint16_t port_;

    /* User variables*/
    struct sockaddr_in serv_addr_;
    int sock_, client_fd_;
};

}  // namespace ep

#endif  // _EP_SOCKET_CLIENT_H
