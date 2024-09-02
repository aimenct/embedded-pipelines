// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <time.h>

#include "camera.h"
#include "opcua.h"
#include "core.h"

int main(int /*argc*/, char** /*argv[]*/)
{
  using namespace ep;

  // /* check command line arguments */
  // if (argc!=2) {
  //     std::cout << "Usage: " << argv[0] << std::endl;
  //     std::cout << "[../tests/basic_io/config_opcua_server.yml]" <<
  //     std::endl; exit(0);
  // }

  std::string fname("../../examples/camera_pipeline.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  //
  YAML::Node node0 = config["filters"][0];
  YAML::Node node1 = config["filters"][1];
  YAML::Node node2 = config["filters"][2];

  ArvCam *camera = new ArvCam(node0);
  OPCUAserver *server = new OPCUAserver(node1);
  GlDisplay *display = new GlDisplay(node2);

  int threads = 2;

  Pipeline pipe(threads);

  pipe.add(camera);
  pipe.add(server);
  pipe.add(display);

  pipe.connect(camera, 0, server, 0);
  pipe.connect(camera, 0, display, 0);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.assignTask(0, camera);
  pipe.assignTask(1, server);
  pipe.assignTask(2, display);

  printf("pres enter to start\n");
  getchar();

  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  pipe.halt();

  
}
