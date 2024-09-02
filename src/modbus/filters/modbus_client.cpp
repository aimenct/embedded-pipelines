// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "modbus_client.h"

// #define DEBUG
namespace ep {
ModbusClient::ModbusClient(const YAML::Node &config)
    : Filter()
{
  std::cout << "ModbusClient constructor " << std::endl;

  name_ = "ModbusClient";
  if (config["name"]) name_ = config["name"].as<std::string>();
  type_ = "ModbusClient";
  YAML::Node yaml_settings = config["settings"];

  /* initialize filter variables */
  // push_mode_ = 'y';
  fps_ = 10;

  // IP = new char[MAX_CHAR256_SIZE];
  ip_ = "127.0.0.1";
  port_ = 1502;
  
  max_sinks_ = 1;

  /* build filter settings - add public settings */
  // addSetting("push mode",push_mode_,FILTER_SETTING,"select pull/push mode");

  addSetting("fps", fps_, FILTER_SETTING,
             "frames per second. 1000.000/fps == timeout");

  addSetting("IP", ip_, FILTER_SETTING,
             "IP address of the server to which the client"
             "wants to establish a connection");

  addSetting("port", port_, FILTER_SETTING, "TCP port to use");

  /* parse YAML file */
  readSettings(config);

  std::cout << "read modbus registers from the yaml file" << std::endl;
  readYamlReg(&config, "input registers", in_reg_);
  readYamlReg(&config, "hold registers", hold_reg_);
  readYamlReg(&config, "digital inputs", in_dig_);
  readYamlReg(&config, "coils", coil_);

#ifdef DEBUG
  if (inputReg.size() > 0) {
    std::cout << std::endl << "Input Registers" << std::endl;
    for (unsigned int i = 0; i < inputReg.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Name:     " << inputReg[i].name << std::endl;
      std::cout << "Address:  " << inputReg[i].address << std::endl;
      std::cout << "Size:     " << inputReg[i].length << std::endl;
      std::cout << "Type:     " << inputReg[i].type << std::endl;
    }
  }
  if (holdReg.size() > 0) {
    std::cout << std::endl << "Hold registers to read" << std::endl;
    for (unsigned int i = 0; i < holdReg.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Name:     " << holdReg[i].name << std::endl;
      std::cout << "Address:  " << holdReg[i].address << std::endl;
      std::cout << "Size:     " << holdReg[i].length << std::endl;
      std::cout << "Acc mode: " << holdReg[i].accessmode << std::endl;
      std::cout << "Type:     " << holdReg[i].type << std::endl;
    }
  }
  if (inputDig.size() > 0) {
    std::cout << std::endl << "Digital inputs" << std::endl;
    for (unsigned int i = 0; i < inputDig.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Name:     " << inputDig[i].name << std::endl;
      std::cout << "Address:  " << inputDig[i].address << std::endl;
      std::cout << "Size:     " << inputDig[i].length << std::endl;
    }
  }
  if (coil.size() > 0) {
    std::cout << std::endl << "Coils to read" << std::endl;
    for (unsigned int i = 0; i < coil.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Name:     " << coil[i].name << std::endl;
      std::cout << "Address:  " << coil[i].address << std::endl;
      std::cout << "Size:     " << coil[i].length << std::endl;
      std::cout << "Acc mode: " << coil[i].accessmode << std::endl;
    }
  }
#endif
  // Check if there is something to read
  if (in_reg_.empty() && hold_reg_.empty() && in_dig_.empty() &&
      coil_.empty() && true) {
    std::cout << "Modbus process failed: no variables requested" << std::endl;
    exit(-1);
  }
}

ModbusClient::~ModbusClient()
{
  std::cout << "ModbusClient destructor " << std::endl;

  in_reg_.clear();
  hold_reg_.clear();
  in_dig_.clear();
  coil_.clear();
}

int32_t ModbusClient::_open()
{
  // Start connection
  ctx_ = modbus_new_tcp(ip_.c_str(), port_);

  if (modbus_connect(ctx_) == -1) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(ctx_);
    exit(-1);
  }

  return 0;
}

int32_t ModbusClient::_set()
{
  std::cout << "ModbusClient: set " << std::endl;

  // Check input queues - only 1 output queue allowed?
  auto connected_sources =  connectedSources();
  if (connected_sources.size() != 0) return -1;

  Message *msg = new Message();

  int index = 0;
  in_reg_index_ = index;
  addModbusRegToNode(in_reg_, *msg, index, 2);
  hold_reg_index_ = index;
  addModbusRegToNode(hold_reg_, *msg, index, 2);
  in_dig_index_ = index;
  addModbusRegToNode(in_dig_, *msg, index, 1);
  coil_index_ = index;
  addModbusRegToNode(coil_, *msg, index, 1);

  // 1 read command only:
  // starting directions for the commands
  in_reg_srv_addr_once_ = 0;
  hold_reg_srv_addr_once_ = 0;
  in_dig_srv_addr_once_ = 0;
  coil_srv_addr_once_ = 0;
  // number of bytes for the commands
  in_reg_size_once_ = 0;
  hold_reg_size_once_ = 0;
  in_dig_size_once_ = 0;
  coil_size_once_ = 0;

  if (in_reg_.size() > 0) {
    in_reg_srv_addr_once_ = in_reg_.front().address;
    in_reg_size_once_ = in_reg_.back().address - in_reg_.front().address +
                        in_reg_.back().length;
  }
  cfgRdComms(in_reg_, in_reg_srv_addr_, in_reg_size_);
  in_reg_report_[in_reg_size_once_];
  in_reg_report_ = std::make_unique<uint16_t[]>(in_reg_size_once_);

  if (hold_reg_.size() > 0) {
    hold_reg_srv_addr_once_ = hold_reg_.front().address;
    hold_reg_size_once_ = hold_reg_.back().address - hold_reg_.front().address +
                          hold_reg_.back().length;
  }
  cfgRdComms(hold_reg_, hold_reg_srv_addr_, hold_reg_size_);
  // hold_reg_report_[hold_reg_size_once_];

  if (in_dig_.size() > 0) {
    in_dig_srv_addr_once_ = in_dig_.front().address;
    in_dig_size_once_ = in_dig_.back().address - in_dig_.front().address +
                        in_dig_.back().length;
  }
  cfgRdComms(in_dig_, in_dig_srv_addr_, in_dig_size_);
  // in_dig_report_[in_dig_size_once_];

  if (coil_.size() > 0) {
    coil_srv_addr_once_ = coil_.front().address;
    coil_size_once_ =
        coil_.back().address - coil_.front().address + coil_.back().length;
  }
  cfgRdComms(coil_, coil_srv_addr_, coil_size_);
  // coil_report_[coil_size_once_];

  addSinkQueue(0,msg);

  return 0;
}


int32_t ModbusClient::_job()
{

#ifdef DEBUG
  if (inputRegSrvAddr.size() > 0) {
    std::cout << std::endl
              << "Read input registers: command parameters" << std::endl;
    for (unsigned int i = 0; i < inputRegSrvAddr.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Address:             " << inputRegSrvAddr[i] << std::endl;
      std::cout << "Number of registers: " << inputRegNAddr[i] << std::endl;
    }
  }
  if (inputReg.size() > 0) {
    std::cout << std::endl
              << "Input Registers, checking if the entry was removed correctly"
              << std::endl;
    for (unsigned int i = 0; i < inputReg.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Name:     " << inputReg[i].name << std::endl;
      std::cout << "Address:  " << inputReg[i].address << std::endl;
      std::cout << "Size:     " << inputReg[i].length << std::endl;
      std::cout << "Type:     " << inputReg[i].type << std::endl;
    }
  }

  if (holdRegSrvAddrR.size() > 0) {
    std::cout << std::endl
              << "Read hold registers: command parameters" << std::endl;
    for (unsigned int i = 0; i < holdRegSrvAddrR.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Address:             " << holdRegSrvAddrR[i] << std::endl;
      std::cout << "Number of registers: " << holdRegNAddrR[i] << std::endl;
    }
  }
  if (inputDigSrvAddr.size() > 0) {
    std::cout << std::endl
              << "Read digital input: command parameters" << std::endl;
    for (unsigned int i = 0; i < inputDigSrvAddr.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Address:             " << inputDigSrvAddr[i] << std::endl;
      std::cout << "Number of bits:      " << inputDigNAddr[i] << std::endl;
    }
  }
  if (coilSrvAddrR.size() > 0) {
    std::cout << std::endl << "Read coils: command parameters" << std::endl;
    for (unsigned int i = 0; i < coilSrvAddrR.size(); i++) {
      std::cout << "Nº: " << i << std::endl;
      std::cout << "Address:             " << coilSrvAddrR[i] << std::endl;
      std::cout << "Number of bits:      " << coilNAddrR[i] << std::endl;
    }
  }
#endif

  int err = writer(0)->startWrite();
  if (err >= 0) {
    Message *msg = &writer(0)->dataMsg();
    std::vector<char> modbus_data(msg->size());
    char* data_ptr_r = modbus_data.data();

    std::cout << "The message size is " << msg->itemCount() << std::endl;

    int rc;
    unsigned int addr_index = in_reg_index_;
    if (in_reg_.size() > 0) {
      rc = modbus_read_input_registers(ctx_, in_reg_srv_addr_once_,
                                       in_reg_size_once_, in_reg_report_.get());
      if (rc != in_reg_size_once_) {
        std::cout << "ERROR reading digital inputs: " << rc << std::endl;
        std::cout << "Address = " << in_reg_srv_addr_once_
                  << ", nb = " << in_reg_size_once_ << std::endl;
      }
      for (unsigned int i = 0; i < in_reg_srv_addr_.size(); i++) {
        memcpy(data_ptr_r + addr_index,
               in_reg_report_.get() +
                   (in_reg_srv_addr_[i] - in_reg_srv_addr_[0]) * 2,
               in_reg_size_[i] * 2);
        addr_index += in_reg_size_[i] * 2;
      }
      
    }

    addr_index = 0;
    int data_ptr_size = static_cast<int>(msg->size()); 

    std::cout << "msg->size() = " << data_ptr_size << std::endl;

    for (int i = 0; i < static_cast<int>(msg->itemCount()) ; i++) {
      DataNode *queue_datanode = (DataNode *)msg->item(i);
      int node_size = static_cast<int>(queue_datanode->size());

      std::cout << "Processing DataNode " << i << ": size = " << node_size
                << ", addr_index = " << addr_index << std::endl;

      // Verificación antes de la escritura
      if (static_cast<int>(addr_index + node_size) > data_ptr_size) {
        std::cerr << "Error: attempt to write outside the memory bounds. "
                  << "addr_index = " << addr_index
                  << ", node_size = " << node_size
                  << ", total_size = " << data_ptr_size << std::endl;
        return -1; 
      }

      queue_datanode->write(data_ptr_r + addr_index);
      addr_index += node_size;

      std::cout << "addr_index after writing: " << addr_index << std::endl;
    }
    // addr_index = 0;
    // for (int i = 0; i < msg->itemCount(); i++) {
    //   DataNode *queue_datanode = (DataNode *)msg->item(i);
    //   queue_datanode->write(data_ptr_r.data());
    //   // addr_index += queue_datanode->size();
    // }

    writer(0)->endWrite();
  }
  usleep(static_cast<unsigned int>(1000000.0 / fps_));
  return 0;
}

int32_t ModbusClient::_close()
{
  modbus_close(ctx_);
  modbus_free(ctx_);
  return 0;
}

int32_t ModbusClient::_reset()
{
  if (in_reg_.size() > 0) {
    in_reg_srv_addr_.clear();
    in_reg_size_.clear();
  }
  if (hold_reg_.size() > 0) {
    hold_reg_srv_addr_.clear();
    hold_reg_size_.clear();
  }
  if (in_dig_.size() > 0) {
    in_dig_srv_addr_.clear();
    in_dig_size_.clear();
  }
  if (coil_.size() > 0) {
    coil_srv_addr_.clear();
    coil_size_.clear();
  }
  return 0;
}

void ModbusClient::readYamlReg(const YAML::Node *yaml_node, const char *key_name,
                               std::vector<modbus_config_t> &v_conf)
{
  if ((*yaml_node)[key_name]) {
    for (unsigned int i = 0; i < (*yaml_node)[key_name].size(); i++) {
      const YAML::Node &reg = (*yaml_node)[key_name][i];

      modbus_config_t config_aux;

      config_aux.name = reg["name"].as<std::string>();

      if (reg["type"]) {
        config_aux.type = reg["type"].as<std::string>();
      }
      else {
        config_aux.type = EP_8U;
      }
      config_aux.address = reg["address"].as<int>();
      // length is represented in the size of the address space
      config_aux.length = reg["size"].as<int>();
      if (reg["access mode"]) {
        config_aux.accessmode = reg["access mode"].as<char>();
      }
      else {
        config_aux.accessmode = 'r';
      }

      if (reg["value"]) {
        config_aux.value = nullptr;
      }
      else {
        config_aux.value = nullptr;
      }

      v_conf.push_back(config_aux);
    }

    // Sort the requested registers and
    // remove any redundant/overlapping register
    if (v_conf.empty() == false) {
      std::sort(v_conf.begin(), v_conf.end(), compare_by_address());

      int addr_expected = -1;
      for (std::vector<modbus_config_t>::iterator it = v_conf.begin();
           it != v_conf.end(); it++) {
        if ((*it).address >= addr_expected) {
          addr_expected = (*it).address + (*it).length;
        }
        // addr < than expected: remove the entry on the original vector
        // as it corresponds to the same variable (optional).
        else {
          v_conf.erase(it--);
        }
      }
    }
  }
  else {
    std::cout << "node " << key_name << " does not exist" << std::endl;
  }
}

void ModbusClient::cfgRdComms(std::vector<modbus_config_t> &v_conf,
                              std::vector<int> &v_addr, std::vector<int> &v_nb)
{
  // build commands
  int addr_expected = -1;
  for (unsigned int i = 0; i < v_conf.size(); i++) {
    if (v_conf[i].accessmode == 'r') {
      int addr_current = v_conf[i].address;
      int size = v_conf[i].length;

      // addr > than expected: create a new block
      if (addr_current > addr_expected) {
        v_addr.push_back(addr_current);
        v_nb.push_back(size);
        addr_expected = addr_current + size;
      }
      // addr = as the exected: add the variable to the block and continue
      else if (addr_current == addr_expected) {
        v_nb.back() += size;
        addr_expected += size;
      }
    }
  }
}

void ModbusClient::addModbusRegToNode(std::vector<modbus_config_t> &v_conf,
                                      Message &v_node, int &index_i,
                                      int lenght_to_bytes)
{
  int addr_expected = -1;
  for (unsigned int i = 0; i < v_conf.size(); i++) {
    if ((v_conf[i].accessmode = 'r') && (v_conf[i].address >= addr_expected)) {
      int size = v_conf[i].length * lenght_to_bytes;
      index_i += size;
      addr_expected = v_conf[i].address + v_conf[i].length;

      ep::DataNode *v_conf_dnode =
          new DataNode(v_conf[i].name, string_to_basetype(v_conf[i].type), {1},
                       v_conf[i].value);

      v_node.addItem(v_conf_dnode);
    }
  }
}


int32_t ModbusClient::_start() {
  return 0;
}

int32_t ModbusClient::_stop() {
  return 0;
}

}  // namespace ep
