// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>

#include "core.h"
//#include "../../examples/filters/image_src.h"

using namespace ep;
using namespace std;

// Filter2 *f;
// void slot_setting(std::string name, void *value)
// {
//   //  std::string value = "../../../data/samples/tif1";
//   f->setSettingValue(name, value);
// }

// void test_signal_filter_setting()
// {
//   //  Define YAML data inline as a string
//   std::string config_src = R"(
//           Filters:
//           - name: Image1
//             type: ImgSrc
//             filter_settings:
//               queue length: 10
//               queue type: lifo
//               timeout: 40000
//               folder path: ../../../data/samples/tif1
//     )";

//   std::string config_disp = R"(
//           Filters:
//           - name: Display
//             type: GlDisplay
//             filter_settings:
//               timeout: 40000
//               maximum images: 10
//     )";

//   YAML::Node cfg_src = YAML::Load(config_src);
//   // YAML::Node config_node = YAML::LoadFile("config_imgsrc.yml");
//   YAML::Node cfg_disp = YAML::Load(config_disp);
//   // YAML::Node config_node = YAML::LoadFile("config_gldisplay.yml");

//   // YAML::Emitter emitter;
//   // emitter << config_node;
//   // std::cout << emitter.c_str() << std::endl;
//   //  config.clear();

//   ImgSrc *src = new ImgSrc(cfg_src["Filters"][0]);
//   GlDisplay *disp = new GlDisplay();
//   // f = src;

//   fteng::signal<void(std::string delta)> update;  // 1
//   //  fteng::signal<void(void *delta)> update;  // 2

//   std::string settname = "folder path";

//   for (int i = 0; i < 10; i++) {
//     update.connect([&src, &settname](std::string delta) {
//       src->setSettingValue(settname, delta);

//       // update.connect([&f, &settname](void *delta) {
//       //   f->setSettingValue(settname, delta);

//       // printf("delta %s %s\n", settname.c_str(), delta.c_str());
//       const std::string *d = src->settingValue<std::string>("folder path");
//       std::cout << "folder path read " << *d << endl;
//     });
//   }

//   update("my_folder");
//   // std::string svar = "my_folder";
//   // update(svar);
//   // update(static_cast<void *>(&svar));
//   getchar();

//   update("my_folder 1");
//   // svar = "my_folder 1";
//   // update(static_cast<void *>(&svar));
//   // update(svar);
//   getchar();

//   delete src;
//   delete disp;

//   return;
// }

void slot_void(void *delta)
{
  printf("valor %d\n", *(int *)delta);
  return;
}

void test_signal_void()
{
  fteng::signal<void(void *delta)> signal_;
  //  signal_.connect(slot_void);

  int c = 8;
  signal_((void *)&c);

  int *d = new int;
  *d = 16;
  signal_((void *)d);

  int *t = new int;
  *t = 19;
  delete d;

  signal_.connect(
      [&t](void *delta) { printf("slot: %d %d\n", *t, *(int *)delta); });

  signal_((void *)d);

  delete d;
  delete t;

  return;
}

int main()
{
  //  test_signal_filter_setting();
  test_signal_void();

  return 0;
}
