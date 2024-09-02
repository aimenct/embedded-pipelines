// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef GLDISPLAY_H
#define GLDISPLAY_H

#include <GL/glut.h>

#include "core.h"

namespace ep {

class GlDisplay : public Filter {
  public:
    GlDisplay();
    GlDisplay(const YAML::Node &config);
    ~GlDisplay();

    static void glutInit()
    {
      int argc = 0;
      char *argv = nullptr;
      if (!glut_initialized_) {
        ::glutInit(&argc, &argv);
        glut_initialized_ = true;
      }
      else {
        std::cerr << "Error: GLUT has already been initialized.\n";
      }
    }

    static void mainLoop()
    {
      if (glut_initialized_) {
        ::glutMainLoop();
      }
      else {
        std::cerr << "Error: GLUT has not been initialized.\n";
      }
    }

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();

  private:
    uint32_t timeout_;

    YAML::Node yaml_config_;

    struct WindowProperties {
        double zoom_f;  // Pixel zoom, for both x & y
        unsigned int window_w;
        unsigned int window_h;
        float aspect;
        int raster_x;
        int raster_y;
        int x_prev;
        int y_prev;
        // variables
        int img_w;
        int img_h;
        GLuint texture;
        GLenum data_type;
        GLenum internal_data_format;
        GLenum data_format;
    };

    int max_num_images;

    std::vector<WindowProperties> windows_;
    std::vector<ImageObject *> images_;
    std::vector<QueueReader *> img_reader_;

    // Glut
    static bool glut_initialized_;

  public:
    void update_texture(int i);
    void render_texture();
    void renderFrame();
    void reshape(int width, int height);
    void motion(int x, int y);
    void motion_over(int x, int y);
    void zoom(float f, int x1, int y1);
    void mouse(int button, int state, int x, int y);
    void keyboard(unsigned char key, int x = 0, int y = 0);
};
}  // namespace ep

#endif  // GL_DISPLAY_H
