// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "socket_client.h"

namespace ep {
/* Filter constructor - create settings */
SocketClient::SocketClient(const YAML::Node &config)
    : Filter()
{
  std::cout << "SocketClient constructor " << std::endl;

  name_ = "SocketClient";
  if (config["name"]) {
    name_ = config["name"].as<std::string>();
  }
  type_ = "SocketClient";

  /******************* EDIT HERE **********************************/
  // General settings

  timeout_ = 1000;
  addSetting("timeout", timeout_, FILTER_SETTING, "thread timeout [us]");

  // Communication settings
  queue_read_timeout_ = 1000;
  addSetting(
      "queue read timeout", queue_read_timeout_, FILTER_SETTING,
      "timeout for each input queue [us] after which a queue will be skipped");

  socket_read_timeout_ = 1000000;
  addSetting("socket read timeout", socket_read_timeout_, FILTER_SETTING,
             "timeout used when reading msg through TCP socket [us]");

  retries_to_reconnect_ = 5;
  addSetting(
      "retries to reconnect", retries_to_reconnect_, FILTER_SETTING,
      "uppon error, send & recv data N times before attempting to reconnect");

  // Socket settings
  connection_timeout_ = 1000000;
  addSetting("connection timeout", connection_timeout_, FILTER_SETTING,
             "timeout for connection retry [us]");

  ip_ = "127.0.0.1";
  addSetting("IP", ip_, FILTER_SETTING,
             "IP address of the server to which the client"
             "wants to establish a connection");

  port_ = 0;
  addSetting("port", port_, FILTER_SETTING, "TCP port to use");

  // Other initializations
  sock_ = 0;
  client_fd_ = 0;
  enable_shifts_ = true;
  in_buff_scaling_ = 20;
  in_buff_size_ = 0;
  out_buff_size_ = 0;
  ctr_errors_ = 0;

  max_sinks_ = 1;
  max_sources_ = 2;
  /******************* EDIT END  ***********************************/

  /* Add control methods */

  yaml_config_ = config;
  readSettings(config);
}

/* Filter destructor*/
SocketClient::~SocketClient()
{
  std::cout << "SocketClient destructor " << std::endl;
}

int32_t SocketClient::_open()
{
  if (socketCfg() < 0) {
    return -1;
  }

  while ((client_fd_ = connect(sock_, (struct sockaddr *)&serv_addr_,
                               sizeof(serv_addr_))) < 0) {
    std::cerr << "SocketClient::open: Connection Failed" << std::endl;
    usleep(connection_timeout_);
  }
  return 0;
}

int32_t SocketClient::_close()
{
  if (::close(client_fd_) < 0) {
    std::cerr << "SocketClient::open: Disconnection Failed" << std::endl;
    return -1;
  }
  return 0;
}

int32_t SocketClient::_set()
{
  std::cout << "SocketClient::set" << std::endl;

  Message *msg = new Message();

  out_buff_items_.resize(max_sources_);

  if (yaml_config_["type"]) {
    std::string filter_type = yaml_config_["type"].as<std::string>();
    if (filter_type != "FilterSocketClient") {
      std::cerr
          << "SocketClient::set error: socket filter config not found in YAML"
          << std::endl;
      return -1;
    }
  }

  // Data TCP->Queue: read YAML file and set up the qOut DataSchema.
  YAML::Node node_read = yaml_config_["TCP read"];
  if (node_read.Type() != YAML::NodeType::Sequence) {
    std::cerr << "SocketClient error in YAML: read register definition"
              << std::endl;
  }
  // Set up the output Queue Dataschema
  enable_shifts_ = false;
  if (node_read[0]["total size"]) {
    in_buff_size_ = node_read[0]["total size"].as<int>();
  }
  else {
    std::cerr << "SocketClient error in YAML: read register size not defined"
              << std::endl;
  }

  unsigned int in_buff_size_check = 0;
  for (size_t i = 1; i < node_read.size(); i++) {
    YAML::Node node_i = node_read[i];
    std::string node_i_name;
    if (node_i["name"]) {
      node_i_name = node_i["name"].as<std::string>();
    }
    else {
      node_i_name = std::string("Node ");
      node_i_name.append(std::to_string(i));
    }
    BaseType node_i_type = string_to_basetype(node_i["type"].as<std::string>());
    int node_i_type_size = type_size(node_i_type);
    DataNode *socketnode = new DataNode(node_i_name, node_i_type, {1}, nullptr);
    msg->addItem(socketnode);

    in_buff_size_check += node_i_type_size;
    int shift_i = 0;
    if (node_i["shift"]) {
      enable_shifts_ = true;
      shift_i = node_i["shift"].as<int>();
      in_buff_size_check += shift_i;
    }
    in_buff_shifts_.push_back(shift_i);
  }
  if (in_buff_size_check > in_buff_size_) {
    std::cerr << "SocketClient error in YAML: total input buffer can't "
                 "be smaller than the sum of its parts"
              << std::endl;
  }
  else if (in_buff_size_check < in_buff_size_) {
    std::cout << "SocketClient: some variables from the input buffer "
                 "are not output to the queue"
              << std::endl;
  }

  // Data Queues->TCP: read YAML file and set up the info needed.
  YAML::Node node_write = yaml_config_["TCP write"];
  if (node_write.Type() != YAML::NodeType::Sequence) {
    std::cerr << "SocketClient error in YAML: write register definition"
              << std::endl;
  }
  // Set up the info needed to copy data from the Queues to the TCP buffer
  for (size_t i = 0; i < node_write.size(); i++) {
    YAML::Node node_i = node_write[i];
    if (node_i["src_port"]) {
      int port_idx = node_i["src_port"].as<int>();
      Message *queue_i_schema = reader(port_idx)->dataSchema();
      // Does the queue with id "queueId" exist?
      if (port_idx < static_cast<int>(out_buff_items_.size())) {
        if (node_i["message"].IsSequence()) {
          for (int j = 0; j < static_cast<int>(node_i["message"].size()); j++) {
            YAML::Node node_j = node_i["message"][j];
            if (node_j["item"]) {
              int node_j_position = node_j["item"].as<int>();
              ep::Node2 *edge_node_j = queue_i_schema->item(node_j_position);
              // Does the variable in "nodeJPosition" position exist?
              if (edge_node_j != NULL && edge_node_j->isDataNode()) {
                DataNode *edge_data_node_j = (DataNode *)edge_node_j;
                out_buff_items_[port_idx].push_back(node_j_position);
                // queue_i_vars.push_back(std::make_pair(edge_node_j->address(),
                //                                       edge_node_j->size()));
                out_buff_size_ +=
                    static_cast<unsigned int>(edge_data_node_j->size());
              }
              else {
                std::cerr
                    << "FilterSocketClient error in YAML: node in position "
                    << node_j_position << " doesn't exist in queue of size "
                    << queue_i_schema->itemCount() << std::endl;
              }
            }
          }
        }
        else if (node_i["message"].IsScalar()) {
          std::string message = node_i["message"].as<std::string>();
          if (message == "all") {
            if (reader(port_idx) != nullptr) {
              size_t queue_i_size = reader(port_idx)->queue()->dataSize();

              for (int idx = 0;
                   idx < static_cast<int>(queue_i_schema->itemCount()); idx++) {
                out_buff_items_[port_idx].push_back(idx);
              }
              out_buff_size_ += static_cast<unsigned int>(queue_i_size);
            }
            else {
              std::cerr << "FilterSocketClient error in YAML: queue in port "
                        << port_idx << " is null " << std::endl;
            }
          }
        }
        else {
          std::cout << "FilterSocketClient error in YAML: message not defined"
                    << std::endl;
        }
      }
      else {
        std::cerr << "SocketClient error in YAML: queue in position "
                  << port_idx << " not ready. " << std::endl;
      }
    }
    else {
      std::cerr << "SocketClient error in YAML: write register definition"
                << std::endl;
    }
  }

  addSinkQueue(0, msg);

  return 0;
}

int32_t SocketClient::_reset()
{
  in_buff_size_ = 0;
  enable_shifts_ = true;
  in_buff_shifts_.clear();
  out_buff_size_ = 0;
  for (auto items : out_buff_items_) {
    items.clear();
  }
  out_buff_items_.clear();

  std::cout << "SocketClient reset" << std::endl;
  getchar();
  return 0;
}

/* Thread function implementation */
int32_t SocketClient::_job()
{
  int err = 0;

  // pointers to write to the output queue

  // vectors with the pointers to read from input queues
  std::vector<bool> in_queue_locked;

  // For each queue that will be read, set calls on non-blocking mode
  // and add an entry to store the buffer pointers
  for (int port = 0; port < static_cast<int>(out_buff_items_.size()); port++) {
    // int in_queue_id = out_buff_info_[i].id;
    reader(port)->setBlockingCalls(false);
    in_queue_locked.push_back(false);
  }
  ///////////////// Optimization required ////////////////////////
  std::vector<char> st_in_buff(in_buff_size_ * in_buff_scaling_);
  char *in_buff = st_in_buff.data();
  // ssize_t sockErrPrev = 0;
  // ssize_t* inBuffPrev = new ssize_t[buffInSize];

  std::vector<char> st_out_buff(out_buff_size_);
  char *out_buff = st_out_buff.data();
  //////////////////////////////////////////////////////////////

  // Get buffer/s only from the input queue/s used in the TCP socket
  for (int port = 0; port < static_cast<int>(out_buff_items_.size()); port++) {
    sClock::time_point time_prev = sClock::now();

    in_queue_locked[port] = true;
    err = reader(port)->startRead();

    if (err < 0) {
      if (err == QE_DISABLED) {
        in_queue_locked[port] = false;

        for (int j = 0; j < port; j++) {
          if (in_queue_locked[j]) {
            reader(port)->endRead();
          }
        }
        std::cerr << "SocketClient: src QE_DISABLED" << std::endl;
      }
      else if (state_ != RUNNING) {
        in_queue_locked[port] = false;
        for (int j = 0; j < port; j++) {
          if (in_queue_locked[j]) {
            reader(port)->endRead();
          }
        }
        std::cerr << "SocketClient: exit status!=runnnig" << std::endl;
      }
      sClock::time_point time_curr = sClock::now();
      sChrono::microseconds time_microseconds =
          sChrono::duration_cast<sChrono::microseconds>(time_curr - time_prev);
      long int microseconds_diff = time_microseconds.count();
      if (microseconds_diff > queue_read_timeout_) {
        in_queue_locked[port] = false;
        break;
      }
    }
  }

  /* process data */
  int buff_ctr = 0;
  for (int i = 0; i < static_cast<int>(out_buff_items_.size()); i++) {
    if (in_queue_locked[i]) {
      for (int j = 0; j < static_cast<int>(out_buff_items_[i].size()); j++) {
        Node2 *out_buff_node = reader(i)->dataMsg().item(j);
        if (out_buff_node->isDataNode()) {
          DataNode *out_buff_data_node = (DataNode *)out_buff_node;
          if (out_buff_data_node->value() != nullptr) {
            memcpy(out_buff + buff_ctr, out_buff_data_node->value(),
                   out_buff_data_node->size());
            buff_ctr += static_cast<int>(out_buff_data_node->size());
          }
        }
      }
    }
    else {
      for (int j = 0; j < static_cast<int>(out_buff_items_[i].size()); j++) {
        Node2 *out_buff_node = reader(i)->dataMsg().item(j);
        if (out_buff_node->isDataNode()) {
          DataNode *out_buff_data_node = (DataNode *)out_buff_node;
          // memcpy(out_buff + buff_ctr, 0, out_buff_data_node->size());
          buff_ctr += static_cast<int>(out_buff_data_node->size());
        }
      }
    }
  }
  // Update input buffer/s - release buffer/s
  for (int i = 0; i < static_cast<int>(out_buff_items_.size()); i++) {
    if (in_queue_locked[i]) {
      reader(i)->endRead();
    }
  }

  bool flag_write_ok = false, flag_read_ok = false;
  // Send output message.
  // MSG_NOSIGNAL is meant to avoid the program from shutting down on a
  // disconnection.
  ssize_t bytes_sent = send(sock_, out_buff, out_buff_size_, MSG_NOSIGNAL);
  if (bytes_sent == out_buff_size_) {
    flag_write_ok = true;
    std::cout << "Bytes sent = " << bytes_sent << std::endl;
    // // Print sent values
    // ////////////////////////////////////////////////////////////
    // // change this
    // ////////////////////////////////////////////////////////////
    // std::cout << "Values sent = ";
    // print_float((float*) outBuff, 5);
  }
  else if (bytes_sent < 0) {
    std::cerr << "FilterSocketClient: writting error == " << strerror(errno)
              << std::endl;
  }
  else {
    std::cerr << "FilterSocketClient: writting error == partially sent."
              << std::endl;
    std::cerr << "Bytes sent = " << bytes_sent << std::endl;
    // // Print sent values
    // ////////////////////////////////////////////////////////////
    // // change this
    // ////////////////////////////////////////////////////////////
    // std::cout << "Values sent = ";
    // print_float((float*) outBuff, sockErr/4);
  }

  // Listen to the server
  ssize_t bytes_recv =
      recv(sock_, in_buff, in_buff_size_ * in_buff_scaling_, MSG_NOSIGNAL);
  // Enable Compose partial messages
  // if ((sockErr + sockErrPrev) >= buffInSize)
  // Check of a full new message has been read
  if (bytes_recv >= in_buff_size_) {
    flag_read_ok = true;
    // New data stored in the tail of the received message
    // Enable Compose partial messages
    // if (sockErr < buffInSize)
    // {
    //     char buffTmp[sockErr];
    //     memcpy(buffTmp, inBuffTail, sockErr);
    //     memcpy(inBuff, inBuffPrev, sockErrPrev);
    //     memcpy(inBuff + sockErrPrev, buffTmp, sockErr);
    //     inBuffTail = inBuff;
    // }
    // Write data into the queue
    // Get empty buffer/s in output queue
    if ((err = writer(0)->startWrite()) < 0) {
      if (err == QE_DISABLED) {
        std::cout << "SocketClient: sink QE_DISABLED" << std::endl;
      }
      else if (state_ != RUNNING) {
        std::cout << "FilterSocketClient: exit status!=runnnig" << std::endl;
      }
    }
    // Copy socket data into the ep::message
    Message *out_msg = &writer(0)->dataMsg();
    for (int i = 0; i < static_cast<int>(out_msg->itemCount()); i++) {
      if (out_msg->item(i)->isDataNode()) {
        DataNode *out_data_node = (DataNode *)out_msg->item(i);
        if (in_buff_shifts_[i] > 0) {
          in_buff += in_buff_shifts_[i];
        }
        out_data_node->write(in_buff);
        in_buff += out_data_node->size();
      }
    }

    // Print the answer
    std::cout << "Bytes received = " << bytes_recv << std::endl;

    std::cout << "Values received = [ " << std::endl;
    for (int i = 0; i < static_cast<int>(out_msg->itemCount()); ++i) {
      if (out_msg->item(i)->isDataNode()) {
        DataNode *print_node = (DataNode *)out_msg->item(i);
        print_node->print();
        // std::cout << print_node->name() << " : " << print_node->value();
      }
    }
    std::cout << "]" << std::endl;
    // ////////////////////////////////////////////////////////////
    // // change this
    // ////////////////////////////////////////////////////////////
    // std::cout << "Values received = ";
    // print_float((float *)dataOut, sink_queue[0]->dataSchema().length());

    // Update output buffer - release buffer
    writer(0)->endWrite();
    // out_queue_.endWrite(producer_id);

    // sockErrPrev = 0;
  }
  else if (bytes_recv < 0) {
    std::cerr << "SocketClient: reading error == " << strerror(errno)
              << std::endl;
  }
  else if (bytes_recv == 0) {
    std::cerr << "SocketClient: reading error == no data received" << std::endl;
  }
  else {
    std::cerr << "SocketClient: reading error ==  missing information"
              << std::endl;
    std::cout << "Bytes received = " << bytes_recv << std::endl;
    // Enable Compose partial messages
    // memcpy(inBuffPrev + sockErrPrev, inBuff, sockErr);
    // sockErrPrev += sockErr;
  }

  if (!flag_read_ok || !flag_write_ok) {
    ctr_errors_++;
  }
  else  // both read and write were ok
  {
    ctr_errors_ = 0;
  }

  if (ctr_errors_ > retries_to_reconnect_) {
    this->reconnect();
    ctr_errors_ = 0;
  }

  usleep(timeout_);
  return 0;
}

void SocketClient::reconnect()
{
  std::cout << "SocketClient: attempting to reconnect... " << std::endl;
  if (::close(client_fd_) < 0) {
    std::cerr << "SocketClient: Disconnection Failed" << std::endl;
  }

  while (socketCfg() < 0) {
    sleep(10);  // sleep for 10 seconds if the socket cloudn't be created
  }

  while (state_ == RUNNING) {
    if ((client_fd_ = connect(sock_, (struct sockaddr *)&serv_addr_,
                              sizeof(serv_addr_))) < 0) {
      std::cout << "SocketClient: Connection Failed" << std::endl;
    }
    else {
      std::cout << "SocketClient: Reconnected" << std::endl;
      break;
    }
    usleep(connection_timeout_);
  }
  return;
}

int SocketClient::socketCfg()
{
  if ((sock_ = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "SocketClient: Socket creation error" << std::endl;
    return -1;
  }

  serv_addr_.sin_family = AF_INET;
  serv_addr_.sin_port = htons(port_);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip_.c_str(), &serv_addr_.sin_addr) <= 0) {
    std::cerr << "SocketClient: Invalid address/ Address not supported"
              << std::endl;
    return -1;
  }

  struct timeval tv;
  tv.tv_sec = socket_read_timeout_ / 1000000;
  tv.tv_usec = socket_read_timeout_ % 1000000;
  if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                 sizeof(tv)) < 0) {
    std::cerr << "SocketClient: unable to set receive timeout" << std::endl;
    return -1;
  }
  return 0;
}

int32_t SocketClient::_start()
{
  return 0;
}

int32_t SocketClient::_stop()
{
  return 0;
}

}  // namespace ep
