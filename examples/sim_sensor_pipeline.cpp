// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "opcua.h"
#include "filters/src_template.h"

int main(/*int argc, char *argv[]*/)
{
  using namespace ep;

  // /* check command line arguments */
  // if (argc!=2) {
  //     std::cout << "Usage: " << argv[0] << std::endl;
  //     std::cout << "[../tests/basic_io/config_opcua_server.yml]" <<
  //     std::endl; exit(0);
  // }

  std::string fname("../../examples/sim_sensor_pipeline.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  //
  YAML::Node node0 = config["filters"][0];
  YAML::Node node1 = config["filters"][1];

  SrcTemplate *src = new SrcTemplate(node0);
  OPCUAserver *server = new OPCUAserver(node1);

  // Configure how many threads you want for the pipeline. Usually is good one
  // thread per filter
  int32_t threads = 2;
  Pipeline pipe(threads);

  // Add both filters to the pipeline (pipeline acquires ownership and
  // automatically manage the memory)
  pipe.add(src);
  pipe.add(server);

  // Connect sink queues from SrcExample to OPCUAserver sources
  pipe.connect(src, 0, server, 0);

  // Assign each filter to run in each thread
  pipe.assignTask(0, src);
  pipe.assignTask(1, server);

  printf("pres enter to start\n");
  getchar();

  // Start pipeline
  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  // Stop pipeline
  pipe.halt();
}
