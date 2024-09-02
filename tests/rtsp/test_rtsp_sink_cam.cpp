// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "camera.h"
#include "rtsp.h"

int main(int /*argc*/, char ** /*argv[]*/)
{
  using namespace ep;

  std::string fname("../../../tests/rtsp/test_rtsp_sink_cam.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);

  ArvCam *camera = new ArvCam( config["filters"][0] );
  RtspSink *rtsp = new RtspSink( config["filters"][1] );

  Pipeline pipe;

  pipe.add(camera);
  pipe.add(rtsp);

  pipe.connect(camera, 0, rtsp, 0);

  printf("pres enter to start\n");
  getchar();

  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  pipe.halt();
}
