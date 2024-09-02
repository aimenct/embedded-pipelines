// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filters/filter_template.h"
#include "filters/sink_template.h"
#include "filters/src_template.h"

int main()
{
  using namespace ep;

  std::string fname("../../examples/tutorial_02.yml");
  if (access(fname.c_str(), F_OK) == -1) {
    std::cerr << "Error: File not found: " << fname << std::endl;
    return -1;
  }

  // check input YAML
  YAML::Node config = YAML::LoadFile(fname);

  // Validate configuration nodes
  if (!config["filters"] || config["filters"].size() < 4) {
    std::cerr << "Error: Bad or incomplete Yaml File\n" << std::endl;
    return -1;  // Exit if the expected structure is not found
  }

  SrcTemplate *source = new SrcTemplate(config["filters"][0]);
  FilterTemplate *proc = new FilterTemplate(config["filters"][1]);
  SinkTemplate *sink_1 = new SinkTemplate(config["filters"][2]);
  SinkTemplate *sink_2 = new SinkTemplate(config["filters"][3]);

  int32_t threads = 3;
  Pipeline pipe(threads);

  // Add both filters to the pipeline (pipeline acquires ownership and
  // automatically manage the memory)
  pipe.add(source);
  pipe.add(proc);
  pipe.add(sink_1);
  pipe.add(sink_2);

  // Connect sink queues 
  pipe.connect(source, 0, proc, 0);
  pipe.connect(proc, 0, sink_1, 0);
  pipe.connect(source, 0, sink_2, 0);

  // Assign each filter to run in each thread
  pipe.assignTask(0, source);
  pipe.assignTask(1, proc);
  pipe.assignTask(2, sink_1);
  pipe.assignTask(2, sink_2);

  std::cout << "press enter to start pipeline execution\n" << std::endl;
  getchar();

  // Start pipeline
  pipe.run();

  std::cout << "press enter to stop\n" << std::endl;
  getchar();

  // Stop pipeline
  pipe.halt();
}
