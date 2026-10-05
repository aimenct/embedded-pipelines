// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef GLUT_DISPLAY_H
#define GLUT_DISPLAY_H

#include <GL/freeglut.h>

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "../libs/filter.h"
#include "../libs/glut_window.h"

class GlutDisplay : public epf::Filter {
  public:
    GlutDisplay();
    GlutDisplay(const YAML::Node &config);
    ~GlutDisplay();

    static void glutInit();
    static void mainLoop();
    void setKeyboardCallback(void (*callback)(unsigned char, int, int));

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();

  private:
    static std::atomic<bool> glut_initialized_;
    static std::mutex glut_mutex_;

    std::vector<std::unique_ptr<Window>> windows;

    void initialize();

    void parseWindowSettings(const YAML::Node &window_node, std::string &name,
                             std::string &type, int &update_interval);

    bool validateSource(const YAML::Node &source, int &port,
                        std::string &buffer, int &item);

    static Window::BufferType bufferTypeFromString(const std::string &buffer);

    static void onClose();
};

#endif  // GLUT_DISPLAY_H
