// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filters/src_template.h"

int main()
{
  using namespace ep;

  std::string fname("../../examples/tutorial_01.yml");
  if (access(fname.c_str(), F_OK) == -1) {
    std::cerr << "Error: File not found: " << fname << std::endl;
    return -1;
  }

  // Load the YAML configuration
  YAML::Node config = YAML::LoadFile(fname);

  if (!config["filters"] || !config["filters"][0]) {
    std::cerr << "Error: Bad Yaml File or missing filters section\n";
    return -1;
  }

  // Initialize source filter from YAML configuration
  SrcTemplate *source = new SrcTemplate(config["filters"][0]);

  // Create a pipeline
  Pipeline pipe;

  // Add source filter to the pipeline (pipeline manages the memory)
  pipe.add(source);

  std::cout << "Press Enter to start pipeline execution\n";
  getchar();

  // Start the pipeline
  pipe.run();

  // Retrieve and process messages from the source filter's sink queue
  if (pipe.filter(0)->sinkQueue(0)) {
    QueueReader reader(pipe.filter(0)->sinkQueue(0));

    for (int i = 0; i < 100; ++i) {
      reader.startRead();
      // print the entire dataMsg
      std::cout<<"retrived message "<<i<<":"<<std::endl;
      reader.dataMsg(0).print(); 
      std::cout<<std::endl;
      reader.endRead();
    }
  }
  else {
    std::cerr << "Error: No sink queue available for the filter\n";
  }

  std::cout << "Press Enter to stop\n";
  getchar();

  // Stop the pipeline
  pipe.halt();

  return 0;
}
