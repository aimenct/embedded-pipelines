// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "../core/filters/src_filter.h"
#include "opcua.h"

using namespace epf;

int main(/*int argc, char *argv[]*/)
{
  using namespace epf;

  std::string fname("/edge-computing/tests/opcua/config_opcua_server.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  printf("press key to continue\n");
  getchar();

  YAML::Node node4 = config["filters"][3];

  int threads = 2;

  Pipeline pipe(threads);

  auto src = pipe.add<SrcFilter>();
  auto server = pipe.add<OPCUAserver>(node4);

  pipe.connect(src, 0, server, 0);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.assignTask(0, src);
  pipe.assignTask(1, server);

  printf("pres enter to start\n");
  getchar();

  // start pipeline
  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  // stop pipeline
  pipe.halt();
}
