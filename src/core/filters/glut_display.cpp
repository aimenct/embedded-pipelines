// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "glut_display.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace epf;
using namespace std;

std::atomic<bool> GlutDisplay::glut_initialized_ = false;
std::mutex GlutDisplay::glut_mutex_;

GlutDisplay::GlutDisplay()
    : Filter()
{
  initialize();
}

GlutDisplay::GlutDisplay(const YAML::Node &config)
    : Filter(config, 10, 0)
{
  yaml_config_ = config;
  initialize();
}

void GlutDisplay::initialize()
{
  type_ = "GlutDisplay";
  job_execution_model_ = MAIN_LOOP;
}

GlutDisplay::~GlutDisplay()
{
  _close();
  std::cout << "GlutDisplay destructor" << std::endl;
}

void GlutDisplay::glutInit()
{
  static int argc = 1;
  static char *argv[] = {const_cast<char *>("dummy"), nullptr};

  std::lock_guard<std::mutex> lock(glut_mutex_);

  if (glut_initialized_) {
    return;
  }

  ::glutInit(&argc, argv);
  glut_initialized_ = true;
}

void GlutDisplay::mainLoop()
{
  {
    std::lock_guard<std::mutex> lock(glut_mutex_);

    if (!glut_initialized_) {
      std::cerr << "Error: GLUT has not been initialized.\n";
      return;
    }

    glutCloseFunc(onClose);

#ifdef GLUT_ACTION_ON_WINDOW_CLOSE
#ifdef GLUT_ACTION_GLUTMAINLOOP_RETURNS
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,
                  GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#else
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
#endif
#endif
  }

  ::glutMainLoop();

  std::cout << "GlutDisplay: leaving main loop.\n";
}

void GlutDisplay::onClose()
{
  std::cout << "Window is closing. Exiting GLUT main loop...\n";
  glutLeaveMainLoop();
}

int32_t GlutDisplay::_open()
{
  glutInit();
  return 0;
}

int32_t GlutDisplay::_close()
{
  _stop();
  windows.clear();
  return 0;
}

void GlutDisplay::parseWindowSettings(const YAML::Node &window_node,
                                      std::string &name, std::string &type,
                                      int &update_interval)
{
  name = window_node["name"] ? window_node["name"].as<std::string>()
                             : "Unnamed Window";

  type =
      window_node["type"] ? window_node["type"].as<std::string>() : "Unknown";

  update_interval = window_node["update interval"]
                        ? window_node["update interval"].as<int>()
                        : 100000;

  if (update_interval <= 0) {
    std::cerr << "Warning: invalid update interval for window '" << name
              << "'. Using default 100000 ms.\n";
    update_interval = 100000;
  }
}

Window::BufferType GlutDisplay::bufferTypeFromString(const std::string &buffer)
{
  return (buffer == "data") ? Window::BufferType::Data
                            : Window::BufferType::Header;
}

bool GlutDisplay::validateSource(const YAML::Node &source, int &port,
                                 std::string &buffer, int &item)
{
  if (!source || !source.IsSequence() || source.size() != 3) {
    std::cerr
        << "Error: source must be [source_port, item_index, data|header].\n";
    return false;
  }

  try {
    port = source[0].as<int>();
    item = source[1].as<int>();
    buffer = source[2].as<std::string>();
  }
  catch (const std::exception &e) {
    std::cerr << "Error: invalid source entry: " << e.what() << std::endl;
    return false;
  }

  if (buffer != "data" && buffer != "header") {
    std::cerr << "Error: source buffer must be 'data' or 'header', got '"
              << buffer << "'.\n";
    return false;
  }

  if (port < 0 || port >= maxSources()) {
    std::cerr << "Error: source port " << port << " is out of range [0, "
              << maxSources() << ").\n";
    return false;
  }

  if (!sourcePort(port) || !sourcePort(port)->isConnected()) {
    std::cerr << "Error: source port " << port << " is not connected.\n";
    return false;
  }

  if (item < 0) {
    std::cerr << "Error: source item index must be non-negative.\n";
    return false;
  }

  return true;
}

int32_t GlutDisplay::_set()
{
  if (!glut_initialized_) {
    glutInit();
  }

  windows.clear();

  if (!yaml_config_["windows"]) {
    std::cerr << "Error: 'windows' key missing in the configuration."
              << std::endl;
    return EXIT_FAILURE;
  }

  if (!yaml_config_["windows"].IsSequence()) {
    std::cerr << "Error: 'windows' must be a sequence." << std::endl;
    return EXIT_FAILURE;
  }

  const int screen_width = std::max(1, glutGet(GLUT_SCREEN_WIDTH));
  const int screen_height = std::max(1, glutGet(GLUT_SCREEN_HEIGHT));

  const int num_windows = static_cast<int>(yaml_config_["windows"].size());
  if (num_windows == 0) {
    std::cerr << "Error: No windows defined in the configuration." << std::endl;
    return EXIT_FAILURE;
  }

  const int cols =
      std::max(1, static_cast<int>(std::ceil(std::sqrt(num_windows))));
  const int rows = std::max(1, (num_windows + cols - 1) / cols);

  int window_width = std::max(400, screen_width / cols - 50);
  int window_height = std::max(300, screen_height / rows - 50);

  window_width = std::min(640, window_width);
  window_height = std::min(480, window_height);

  const int x_spacing =
      std::max(0, (screen_width - cols * window_width) / (cols + 1));
  const int y_spacing =
      std::max(0, (screen_height - rows * window_height) / (rows + 1));

  int window_index = 0;
  int created_windows = 0;

  for (const auto &window_node : yaml_config_["windows"]) {
    const int row = window_index / cols;
    const int col = window_index % cols;
    const int x_pos = x_spacing + col * (window_width + x_spacing);
    const int y_pos = y_spacing + row * (window_height + y_spacing);

    ++window_index;

    std::string name;
    std::string type;
    int update_interval = 100000;
    parseWindowSettings(window_node, name, type, update_interval);

    if (name.empty() || type.empty()) {
      std::cerr << "Error: Invalid window settings. Skipping.\n";
      continue;
    }

    std::unique_ptr<Window> win;

    if (type == "Plot2") {
      if (!window_node["source"] || !window_node["source"]["y"]) {
        std::cerr << "Error: Missing required source.y for Plot2 window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      int x_port = -1;
      int y_port = -1;
      int x_item = -1;
      int y_item = -1;
      std::string x_buffer;
      std::string y_buffer;

      const bool has_x_data = window_node["source"]["x"].IsDefined();

      if (has_x_data && !validateSource(window_node["source"]["x"], x_port,
                                        x_buffer, x_item)) {
        std::cerr << "Error: Invalid source.x for Plot2 window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      if (!validateSource(window_node["source"]["y"], y_port, y_buffer,
                          y_item)) {
        std::cerr << "Error: Invalid source.y for Plot2 window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      auto plot = std::make_unique<Plot2DWindow>(
          name, window_width, window_height, x_pos, y_pos, 1000);

      if (has_x_data) {
        const int32_t err =
            plot->setXSource(sourcePort(x_port)->queue(), x_item,
                             bufferTypeFromString(x_buffer));

        if (err < 0) {
          std::cerr << "Error: Failed to bind Plot2 source.x for window '"
                    << name << "'. Skipping.\n";
          continue;
        }
      }

      const int32_t err = plot->setYSource(sourcePort(y_port)->queue(), y_item,
                                           bufferTypeFromString(y_buffer));

      if (err < 0) {
        std::cerr << "Error: Failed to bind Plot2 source.y for window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      win = std::move(plot);
    }
    else if (type == "Image") {
      if (!window_node["source"]) {
        std::cerr << "Error: Missing source for Image window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      int port = -1;
      int item = -1;
      std::string buffer;

      if (!validateSource(window_node["source"], port, buffer, item)) {
        std::cerr << "Error: Invalid source for Image window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      auto image = std::make_unique<ImageWindow>(name, window_width,
                                                 window_height, x_pos, y_pos);

      const bool float_autoscale =
          window_node["float autoscale"]
              ? window_node["float autoscale"].as<bool>()
              : true;

      image->setFloatAutoscale(float_autoscale);

      const int32_t err = image->setDataSource(sourcePort(port)->queue(), item,
                                               bufferTypeFromString(buffer));

      if (err < 0) {
        std::cerr << "Error: Failed to bind Image source for window '" << name
                  << "'. Skipping.\n";
        continue;
      }

      bool probe_enabled = false;
      bool probe_hover = false;
      bool probe_click = false;
      std::string probe_units;
      int probe_precision = 2;
      int probe_throttle_ms = 100;

      if (window_node["probe"]) {
        const auto probe = window_node["probe"];

        probe_enabled = probe["enabled"] ? probe["enabled"].as<bool>() : false;

        const std::string mode =
            probe["mode"] ? probe["mode"].as<std::string>() : "click";

        probe_hover = (mode == "hover" || mode == "both");
        probe_click = (mode == "click" || mode == "both");

        probe_units = probe["units"] ? probe["units"].as<std::string>() : "";
        probe_precision = probe["precision"] ? probe["precision"].as<int>() : 2;
        probe_throttle_ms =
            probe["throttle ms"] ? probe["throttle ms"].as<int>() : 100;
      }

      image->setProbeOptions(probe_enabled, probe_hover, probe_click,
                             probe_units, probe_precision, probe_throttle_ms);

      if (probe_enabled && window_node["probe source"]) {
        int probe_port = -1;
        int probe_item = -1;
        std::string probe_buffer;

        if (validateSource(window_node["probe source"], probe_port,
                           probe_buffer, probe_item)) {
          if (probe_port != port) {
            std::cerr
                << "Warning: Image probe source for window '" << name
                << "' must use the same source port as the display image. "
                   "Probe source ignored."
                << std::endl;
          }
          else {
            image->setProbeSource(sourcePort(probe_port)->queue(), probe_item,
                                  bufferTypeFromString(probe_buffer));
          }
        }
      }

      win = std::move(image);
    }
    else if (type == "Plot3") {
      if (!window_node["source"] || !window_node["source"]["x"] ||
          !window_node["source"]["y"] || !window_node["source"]["z"]) {
        std::cerr << "Error: Missing required x/y/z source data for Plot3 "
                     "window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      int x_port = -1;
      int y_port = -1;
      int z_port = -1;
      int v_port = -1;
      int x_item = -1;
      int y_item = -1;
      int z_item = -1;
      int v_item = -1;
      std::string x_buffer;
      std::string y_buffer;
      std::string z_buffer;
      std::string v_buffer;

      if (!validateSource(window_node["source"]["x"], x_port, x_buffer,
                          x_item) ||
          !validateSource(window_node["source"]["y"], y_port, y_buffer,
                          y_item) ||
          !validateSource(window_node["source"]["z"], z_port, z_buffer,
                          z_item)) {
        std::cerr << "Error: Invalid required source data for Plot3 window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      const bool has_v_data = window_node["source"]["v"].IsDefined();

      if (has_v_data && !validateSource(window_node["source"]["v"], v_port,
                                        v_buffer, v_item)) {
        std::cerr << "Error: Invalid optional source.v for Plot3 window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      const int step = window_node["step"] ? window_node["step"].as<int>() : 1;
      const int batch =
          window_node["batch"] ? window_node["batch"].as<int>() : 500;
      const int buffer_length = window_node["buffer length"]
                                    ? window_node["buffer length"].as<int>()
                                    : 1000;

      if (step <= 0 || batch <= 0 || buffer_length <= 0) {
        std::cerr << "Error: Plot3 step, batch, and buffer length must be "
                     "positive for window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      auto plot3 = std::make_unique<Plot3DWindow>(
          name, window_width, window_height, x_pos, y_pos, buffer_length);

      plot3->setBatchSize(batch, step);

      if (plot3->setXSource(sourcePort(x_port)->queue(), x_item,
                            bufferTypeFromString(x_buffer)) < 0 ||
          plot3->setYSource(sourcePort(y_port)->queue(), y_item,
                            bufferTypeFromString(y_buffer)) < 0 ||
          plot3->setZSource(sourcePort(z_port)->queue(), z_item,
                            bufferTypeFromString(z_buffer)) < 0) {
        std::cerr << "Error: Failed to bind required Plot3 sources for window '"
                  << name << "'. Skipping.\n";
        continue;
      }

      if (has_v_data && plot3->setVSource(sourcePort(v_port)->queue(), v_item,
                                          bufferTypeFromString(v_buffer)) < 0) {
        std::cerr
            << "Error: Failed to bind optional Plot3 source.v for window '"
            << name << "'. Skipping.\n";
        continue;
      }

      win = std::move(plot3);
    }
    else {
      std::cerr << "Error: Unknown window type '" << type << "' for window '"
                << name << "'. Skipping.\n";
      continue;
    }

    if (win) {
      win->setTimeout(update_interval);
      win->start();
      windows.push_back(std::move(win));
      ++created_windows;
    }
  }

  if (created_windows == 0) {
    std::cerr << "Error: no GlutDisplay windows were created." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int32_t GlutDisplay::_reset()
{
  _close();
  return 0;
}

int32_t GlutDisplay::_start()
{
  for (auto &win : windows) {
    if (win) win->start();
  }

  return 0;
}

int32_t GlutDisplay::_stop()
{
  for (auto &win : windows) {
    if (win) win->stop();
  }

  return 0;
}

int32_t GlutDisplay::_job()
{
  mainLoop();
  return 0;
}

void GlutDisplay::setKeyboardCallback(void (*callback)(unsigned char, int, int))
{
  Window::setKeyboardCallback(callback);
}
