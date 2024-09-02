// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filters/sim_sdk_src.h"
#include "filters/sink_template.h"

int main()
{
  using namespace ep;

  std::string fname("../../examples/tutorial_03.yml");
  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  // check input YAML
  YAML::Node config = YAML::LoadFile(fname);

  // Validate configuration nodes
  if (!config["filters"] || config["filters"].size() < 2) {
    std::cerr << "Error: Bad or incomplete Yaml File\n" << std::endl;
    return -1;  // Exit if the expected structure is not found
  }

  SimSdkSrc *source = new SimSdkSrc(config["filters"][0]);
  SinkTemplate *sink = new SinkTemplate(config["filters"][1]);

  int32_t threads = 2;
  Pipeline pipe(threads);

  // Add both filters to the pipeline (pipeline acquires ownership and
  // automatically manage the memory)
  pipe.add(source);
  pipe.add(sink);

  // Connect sink queues
  pipe.connect(source, 0, sink, 0);

  // Assign each filter to run in each thread
  pipe.assignTask(0, source);
  pipe.assignTask(1, sink);

  // Start pipeline
  //  pipe.run();

  YAML::Node my_settings;
  source->writeSettings(my_settings);
  std::cout << my_settings << std::endl;

  std::cout << "press enter to open \n" << std::endl;
  getchar();
  pipe.open();

  my_settings = YAML::Node();
  source->writeSettings(my_settings);
  std::cout << my_settings << std::endl;

  std::cout << "press enter to set \n" << std::endl;
  getchar();

  pipe.set();

  std::cout << "press enter to start pipeline execution\n" << std::endl;
  getchar();

  pipe.start();
  pipe.launch();

  std::cout << "press enter to call stop command \n" << std::endl;
  getchar();
  source->runCommand("stop");

  std::cout << "press enter to call reset command\n" << std::endl;
  getchar();
  source->runCommand("OscilloReset");

  my_settings = YAML::Node();
  source->writeSettings(my_settings);
  std::cout << my_settings << std::endl;

  std::cout << "press enter to call start command\n" << std::endl;
  getchar();
  source->runCommand("start");

  std::cout << "press enter to stop\n" << std::endl;
  getchar();

  // Stop pipeline
  pipe.halt();

  YAML::Node node = YAML::Node();
  pipe.writeSettings(node);
  std::cout << "pipelines settings after halt:" << std::endl;
  std::cout << node << std::endl;
  //  pipe.saveSettings("my_new_settings_tutorial03.yml");
}
