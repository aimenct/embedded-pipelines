// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <time.h>

#include <memory>

#include "camera.h"
#include "core.h"
#include "opcua.h"

int main(int /*argc*/, char ** /*argv[]*/)
{
  using namespace epf;

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

  Pipeline pipe;

  auto camera = pipe.add<ArvCam>(node0);
  auto display = pipe.add<GlutDisplay>(node1);
  auto server = std::make_unique<OPCUAserver>(node2);

  //  pipe.connect(camera, 0, server, 0);
  pipe.connect(camera, 0, display, 0);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  printf("pres enter to open\n");
  getchar();
  pipe.open();
  printf("pres enter to set\n");
  getchar();
  camera->saveSettings("");
  pipe.set();
  printf("pres enter to start\n");
  getchar();
  pipe.start();

  pipe.launch();

  printf("pres enter to stop\n");
  getchar();

  pipe.halt();
}
