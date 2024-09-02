// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_MODBUS_CLIENT
#define _EP_MODBUS_CLIENT

#include <errno.h>
#include <modbus/modbus.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

#include "core.h"

namespace ep {
struct modbus_config_t {
    std::string name;  // name of the variable
    std::string type;  // type macros defined in ep_types.h
    int address;  // modbus address in the address space where the variable is
                  // placed out of the distinct 4 spaces
    int length;   // size in number of positions (16/1 bits for registers/coils)
    char accessmode;  // r/w to specify the operation to make.
                      // digital inputs and input registers can only be read
    void *value;      // extra information if needed for the given type
};

struct compare_by_address {
    bool operator()(const modbus_config_t &x, const modbus_config_t &y) const
    {
      return x.address < y.address;
    }
};

class ModbusClient : public Filter {
  public:
    ModbusClient(const YAML::Node &config);  // create settings
    ~ModbusClient();                         // delete settings

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _set();
    int32_t _reset();
    int32_t _close();
    int32_t _start();
    int32_t _stop();

  private:
    Queue *queue_out_;

    // internal variables
    float fps_;
    unsigned int queue_length_;  // output queue lenght

    // modbus client info
    modbus_t *ctx_;
    std::string ip_;
    int port_;

    std::vector<modbus_config_t> in_reg_, hold_reg_, in_dig_, coil_;
    int in_reg_index_, hold_reg_index_, in_dig_index_, coil_index_;

    // Blocks of memory needed
    // starting directions for the blocks
    std::vector<int> in_reg_srv_addr_;
    std::vector<int> hold_reg_srv_addr_;
    std::vector<int> in_dig_srv_addr_;
    std::vector<int> coil_srv_addr_;
    // number of bytes of the blocks
    std::vector<int> in_reg_size_;
    std::vector<int> hold_reg_size_;
    std::vector<int> in_dig_size_;
    std::vector<int> coil_size_;

    // 1 read command only:
    // starting directions for the commands
    int in_reg_srv_addr_once_;
    int hold_reg_srv_addr_once_;
    int in_dig_srv_addr_once_;
    int coil_srv_addr_once_;
    // number of bytes for the commands
    int in_reg_size_once_;
    int hold_reg_size_once_;
    int in_dig_size_once_;
    int coil_size_once_;
    // auxiliar registers to hold the whole data recieved, which then is reduced
    // to the requested variables and put into the queue buffer
    std::unique_ptr<uint16_t[]> in_reg_report_;
    uint16_t *hold_reg_report_;
    uint8_t *in_dig_report_;
    uint8_t *coil_report_;

    void readYamlReg(const YAML::Node *yaml_node, const char *key_name,
                     std::vector<modbus_config_t> &v_conf);

    /** @brief Configure the command parameters to use on modbus reading
     * operations, sorting the original vector by address to then search for
     * contigous blocks of memory, thus, reducing the number of total commands.
     *
     * It also modifies the original vector, sorting it and cleaning errors.
        @param vConf original vector containing the variables to be read.
        @param vAddr vector to store the starting addresses to be read.
        @param vNb vector to store the number of addresses to be read.*/
    void cfgRdComms(std::vector<modbus_config_t> &v_conf,
                    std::vector<int> &v_addr, std::vector<int> &v_nb);

    void addModbusRegToNode(std::vector<modbus_config_t> &v_conf,
                            Message &v_node, int &index_i, int lenght_to_bytes);
};

}  // namespace ep

#endif  //_EP_MODBUS_CLIENT
