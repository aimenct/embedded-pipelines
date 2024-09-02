// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include <basic_io.h>
#include <opcua.h>

int main(int argc, char *argv[])
{
  using namespace ep;

  /* check command line arguments */
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << std::endl;
    std::cout << "[../tests/basic_io/config_opcua_server.yml]" << std::endl;
    exit(0);
  }

  getchar();

  std::string fname(argv[1]);

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found " << fname << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  printf("press key to continue\n");
  getchar();

  ///* create FakeSrc object */
  YAML::Node node1 = config["filters"][0];
  FakeSrc *src1 = new FakeSrc(&node1);
  //
  ///* create FakeSrc object */
  YAML::Node node2 = config["filters"][1];
  FakeSrc *src2 = new FakeSrc(&node2);
  //
  ///* create PrintSink object */
  YAML::Node node3 = config["filters"][2];
  PrintSink *print = new PrintSink(&node3);

  YAML::Node node4 = config["filters"][3];
  OPCUAserver *srv = new OPCUAserver(&node4, (char *)fname.c_str());

  Pipeline pipe;

  pipe.add(src1);
  pipe.add(src2);
  pipe.add(print);
  //  pipe.add(srv);

  pipe.connect(src1,0,print,0);
  pipe.connect(src2,0,print,1);
  // pipe.connect(src1, 0,srv, 0);
  // pipe.connect(src2, 0,srv, 1);
  
  printf("Press enter to start \n");
  getchar();

  pipe.run();

  printf("Press enter to stop \n");
  getchar();

  pipe.halt();

}
