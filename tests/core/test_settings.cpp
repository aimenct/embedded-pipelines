// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <core.h>

struct DummyFilter {
    // filter_settings
    bool push_mode = true;
    int32_t queue_length = 5;
    int32_t arv_num_buffers = 65;
    uint32_t output_interleave = 1;
    int32_t output_bands = 3;

    // device_settings
    std::string pixel_format = "MONO8";
    float exposure_time = 3000.;

    ep::Settings2 settings;

    DummyFilter()
    {
      settings = ep::Settings2("filter");
    }

    void createSettings()
    {
      settings.addSetting("push_mode", push_mode);
      settings.addSetting("queue_length", queue_length);
      settings.addSetting("arv_num_buffers", arv_num_buffers);
      settings.addSetting("output_interleave", output_interleave);
      settings.addSetting("output_bands", output_bands);

      settings.addSetting("ExposureTime", exposure_time);
      settings.addSetting("PixelFormat", pixel_format);

      settings.addCommand("IncreaseExposure()",
                          std::bind(&DummyFilter::method, this));
    }

    int32_t method()
    {
      exposure_time = exposure_time + 10;
      return 1;
    }
};

int main()
{
  std::string fname = "../../../tests/core/test_settings.yml";
  YAML::Node config = YAML::LoadFile(fname)["filters"][0];

  std::cout << "aa" << std::endl;

  DummyFilter filter;

  filter.createSettings();

  filter.settings.fromYAML(config);

  {
    ep::Settings2 alt_settings = filter.settings;
    alt_settings.setValue("push_mode", true);
  }

  // settings.setValue("push_mode", true);
  filter.settings.setFilterName("camera_filter");

  filter.settings.print();

  std::cout << std::endl;

  filter.settings.setValue<std::string>("PixelFormat", "RGB8");

  const std::string* p_format =
      filter.settings.value<std::string>("PixelFormat");
  const int32_t* w_p_format = filter.settings.value<int32_t>("PixelFormat");
  const float* exposure = filter.settings.value<float>("ExposureTime");

  std::cout << "PixelFormat-> " << *p_format << std::endl;
  std::cout << "wPixelFormat-> " << w_p_format << std::endl;
  std::cout << "ExposureTime-> " << *exposure << std::endl;

  std::cout << std::endl;
  getchar();

  filter.settings.runCommand("IncreaseExposure()");
  std::cout << filter.exposure_time << std::endl;
  filter.settings.print();

  return 0;
}
