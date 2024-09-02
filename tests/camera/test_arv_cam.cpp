// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "camera.h"
#include "core.h"

int32_t main()
{
  using namespace ep;
  std::string fname = "../../../tests/camera/test_arv_cam.yml";

  YAML::Node config = YAML::LoadFile(fname)["filters"][0];

  ArvCam *src = new ArvCam(config);

  GlDisplay *disp = new GlDisplay();

  Pipeline pipe;

  pipe.add(src);
  pipe.add(disp);

  pipe.connect(src,0,disp,0);

  YAML::Node pi_config;
  pipe.writeSettings(pi_config);
  std::cout << "Settings before open " << std::endl;
  std::cout << pi_config << std::endl;

  std::cout<<" press to open pipeline"<<std::endl;
  getchar();
  pipe.open();

  if (src->state() == DISCONNECTED) {
    std::cout <<"Not camera found"<< std::endl;
    exit(0);
  }

  std::cout << "Settings after open " << std::endl;
  pi_config = YAML::Node();
  pipe.writeSettings(pi_config);
  std::cout << pi_config << std::endl <<std::endl;

  std::cout<<" press to launch pipeline"<<std::endl;
  getchar();
  pipe.set();
  pipe.start();
  pipe.launch();

  printf("halt\n");
  getchar();
  pipe.halt();

  return 0;
}
