// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaml-cpp/yaml.h>

#include "modbus.h"

#define MAX_STR 256

int main(int /*argc*/, char **/*argv[]*/)
{
  using namespace ep;

  /* check command line arguments */
  // if (argc != 2) {
  //   std::cout << "Usage: " << argv[0] << std::endl;
  //   std::cout << "[../tests/basic-io/ConfigModbusTest.yml]" << std::endl;
  //   ;
  //   exit(0);
  // }

  std::string fname("../../../tests/modbus/test_modbus_client.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  std::cout << "Here's the input YAML:\n" << config << std::endl;
  printf("press key to continue\n");
  getchar();


  YAML::Node node = config["filters"][0];
  ModbusClient *modbus_client = new ModbusClient(node);

  int threads = 1;
  Pipeline pipe(threads);

  pipe.add(modbus_client);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.assignTask(0, modbus_client);
  
  printf("pres enter to start\n");
  getchar();

  pipe.run();
  printf("pres enter to stop\n");
  getchar();

  pipe.halt();
}
