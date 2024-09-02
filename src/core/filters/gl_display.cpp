// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gl_display.h"

using namespace ep;
using namespace std;

GLenum pixelFormatToGLType(PixelFormat pf, unsigned int w, unsigned int h,
                           unsigned int c);

GLenum pixelFormatToGLType(PixelFormat pf, unsigned int w, unsigned int h,
                           unsigned int c)
{
  PixelFormatInfo nfo = get_image_info(pf, w, h, c);

  switch (nfo.baseType) {
    case EP_8U:
      printf("GL_UNSIGNED_BYTE\n");
      return GL_UNSIGNED_BYTE;
    case EP_16S:
      printf("GL_SHORT\n");
      return GL_SHORT;
    case EP_16U:
      printf("GL_UNSIGNED_SHORT\n");
      return GL_UNSIGNED_SHORT;
    case EP_32S:
      printf("GL_INT\n");
      return GL_INT;
    case EP_32F:
      printf("GL_FLOAT\n");
      return GL_FLOAT;
    case EP_64F:
      printf("GL_DOUBLE\n");
      return GL_DOUBLE;
    default:
      printf("GL_NONE\n");
      return GL_NONE;  // Unknown type or unsupported
  }
}

GLenum pixelFormatToGLFormat(PixelFormat pf, unsigned int w, unsigned int h,
                             unsigned int c);

GLenum pixelFormatToGLFormat(PixelFormat pf, unsigned int w, unsigned int h,
                             unsigned int c)
{
  PixelFormatInfo nfo = get_image_info(pf, w, h, c);

  // Single-Component Formats:

  switch (pf) {
      // Mono
    case Mono8:
      //      printf("GL_R8\n");
      //      return GL_R8;
      return GL_LUMINANCE;
    case Mono10:
      // printf("GL_R16\n");
      // return GL_R16;
      return GL_LUMINANCE;
    case Mono12:
      // printf("GL_R16\n");
      // return GL_R16;
      return GL_LUMINANCE;
    case Mono16:
      // printf("GL_R16\n");
      // return GL_R16;
      return GL_LUMINANCE;
    case BIP32f:
      if (c == 1) {
        // printf("GL_R32F\n");
        // return GL_R32F;
        return GL_LUMINANCE;
      }
      else
        return GL_NONE;
      // color
    case RGB8:
      printf("GL_RGB\n");
      return GL_RGB;
    case RGB10:
      printf("GL_RGB\n");
      return GL_RGB;
    case RGB12:
      printf("GL_RGB\n");
      return GL_RGB;
    case RGBa8:
      printf("GL_RGBA\n");
      return GL_RGBA;
    case RGBa12:
      printf("GL_RGBA\n");
      return GL_RGBA;
    case BGRa8:
      printf("GL_BGRA\n");
      return GL_NONE;
    default:
      printf("GL_NONE\n");
      return GL_NONE;  // Unknown type or unsupported
  }
}

bool GlDisplay::glut_initialized_ = false;

GlDisplay *p;

void w_render_texture()
{
  p->render_texture();
}

void GlDisplay::renderFrame()
{
  //  bool any = false;
  for (size_t i = 0; i < img_reader_.size(); ++i) {
    if (img_reader_[i] && img_reader_[i]->startRead() >= 0) {
      update_texture(static_cast<int>(i));
      img_reader_[i]->endRead();
    }
  }
  render_texture();
  usleep(p->timeout_);
  //  return any ? 0 : -1;
}

void w_renderFrame()
{
  p->renderFrame();
}
void w_reshape(int width, int height)
{
  p->reshape(width, height);
}
void w_motion(int x, int y)
{
  p->motion(x, y);
}
// void w_motion_over(int x, int y) {
//   p->motion_over(x,y);
// }
void w_zoom(float f, int x1, int y1)
{
  p->zoom(f, x1, y1);
}
void w_mouse(int button, int state, int x, int y)
{
  p->mouse(button, state, x, y);
}
// void w_keyboard(unsigned char key,int x=0,int y=0) {
//   p->keyboard(key,x,y);
// }

GlDisplay::GlDisplay()
    : Filter()
{
  p = this;

  name_ = "GlDisplay";
  type_ = "GlDisplay";

  name_ = "myDisplay";

  job_execution_model_ = MAIN_LOOP;

  max_sinks_ = 0;
  max_sources_ = 10;

  timeout_ = 40000;

  // initialized
  max_num_images = 10;

  images_.clear();
}

GlDisplay::GlDisplay(const YAML::Node &config)
    : Filter()
{
  yaml_config_ = config;

  name_ = "GlDisplay";
  type_ = "GlDisplay";

  p = this;

  job_execution_model_ = MAIN_LOOP;

  max_sinks_ = 0;
  max_sources_ = 10;

  timeout_ = 40000;

  // initialized
  max_num_images = 10;

  images_.clear();

  //  if (config["name"]) name_ = config["name"].as<std::string>();
  addSetting("timeout", timeout_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);
}

GlDisplay::~GlDisplay()
{
  cout << "GlDisplay destructor" << endl;

  for (unsigned int i = 0; i < images_.size(); i++) {
    delete images_[i];
    images_[i] = nullptr;
  }
  images_.clear();

  windows_.clear();
}

int32_t GlDisplay::_set()
{
  int images_found = 0;

  if (reader(0) != nullptr)
    printf("items count %ld\n", reader(0)->dataSchema()->itemCount());

  // item 1 - Image Object
  for (int i = 0; i < sourcePorts(); i++) {
    if (sourceQueue(i) != nullptr)
      for (int j = 0;
           j < static_cast<int>(reader(i)->dataSchema()->itemCount());
           j++) {  // check object images

        //        printf("GlDisplay looking for images: queue %d item %d\n", i,
        //        j);

        Node2 *n = reader(i)->dataSchema()->item(j);
        if (n->isObjectNode()) {
          ep::ObjectNode *onode = static_cast<ep::ObjectNode *>(n);

          if (onode->objecttype() == ep::EP_IMAGE_OBJ) {
            cout << "GlDisplay: set() image found Queue: " << i
                 << ", Item: " << j << endl;

            images_.push_back(new ImageObject(n));
            reader(i)->setBlockingCalls(false);
            img_reader_.push_back(reader(i));

            // InitWindow
            // WindowProperties p = {.zoom_f = 1.0,
            //                       .window_w = 256,
            //                       .window_h = 256,
            //                       .aspect = 1.0,
            //                       .raster_x = 0,
            //                       .raster_y = 0,
            //                       .x_prev = 0,
            //                       .y_prev = 0,
            //                       .img_w = 256,
            //                       .img_h = 256,
            //                       .texture = 0,
            //                       .data_type = 0,
            //                       .internal_data_format = 0,
            //                       .data_format = 0};

            WindowProperties p = {1.0, 256, 256, 1.0, 0, 0, 0,
                                  0,   256, 256, 0,   0, 0, 0};

            windows_.push_back(p);
            WindowProperties *prop = &windows_[images_found];
            ImageObject *img = images_[images_found];

            prop->window_w = static_cast<unsigned int>(
                std::round(img->width() * prop->zoom_f));

            prop->window_h = static_cast<unsigned int>(
                std::round(img->height() * prop->zoom_f));

            prop->aspect = static_cast<float>(img->width()) /
                           static_cast<float>(img->height());
            prop->aspect = 1.0;

            prop->data_type = pixelFormatToGLType(
                images_[i]->pixelFormat(), images_[i]->width(),
                images_[i]->height(), images_[i]->channels());

            prop->internal_data_format = pixelFormatToGLFormat(
                images_[i]->pixelFormat(), images_[i]->width(),
                images_[i]->height(), images_[i]->channels());

            prop->data_format = prop->internal_data_format;

            char name[64];
            std::snprintf(name, 64, "Queue: %d, Item: %d", i, j);
            glutInitWindowSize(prop->window_w, prop->window_h);
            glutInitWindowPosition(prop->window_w, prop->window_h);

            glutCreateWindow(name);

            glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

            // initialize window
            glClearColor(1.0, 1.0, 1.0, 0.0);
            glViewport(0, 0, prop->window_w, prop->window_h);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            if (glGetError() != GL_NO_ERROR) {
              printf("error init openGl");
              return -1;
            }
            // enable texture
            glEnable(GL_TEXTURE_2D);
            gluOrtho2D(-prop->raster_x,
                       prop->window_w / prop->zoom_f - prop->raster_x,
                       -prop->raster_y,
                       prop->window_h / prop->zoom_f - prop->raster_y);

            // glutDisplayFunc(display2); // Set display callback for window 2
            glGenTextures(1, &prop->texture);
            // add callbacks
            glutDisplayFunc(w_render_texture);
            glutReshapeFunc(w_reshape);
            //  glutKeyboardFunc(w_keyboard);
            glutMouseFunc(w_mouse);
            glutMotionFunc(w_motion);
            //  glutPassiveMotionFunc(motion_over);
            //  glutIdleFunc(w_renderFrame);
            glutIdleFunc(w_renderFrame);  // replace by job

            // glutCreateMenu(menu);
            // glutAddMenuEntry("[o] Open", 'o'); // open connected cameras ->
            // open command glutAddMenuEntry("[l] List", 'l'); // list connected
            // cameras glutAddMenuEntry("[s] Start/Stop", 's'); // stop
            // glutAddMenuEntry("[p] Play/Pause", 'p'); // play/pause
            // glutAddMenuEntry("[c] Close", 'c'); // close connected cameras
            // glutAddMenuEntry("[b] Calibrate", 'b'); // calibrate
            // glutAddMenuEntry("[r] Record", 'r'); // record
            // glutAddMenuEntry("[f] Record fast", 'f'); // record max speed
            // glutAddMenuEntry("[x] Open/Close Shutters", 'x'); //
            // open_shutters glutAddMenuEntry("[i] Info ON/OFF", 'i');
            // glutAddMenuEntry("[Esc] Exit", 27);
            // glutAttachMenu(GLUT_RIGHT_BUTTON);
            /* END OPENGL*/

            images_found++;
          }
        }
      }
  }

  return 0;
}

int32_t GlDisplay::_job()
{
  mainLoop();
  return 0;
}

int32_t GlDisplay::_open()
{
  glutInit();

  return 0;
}
int32_t GlDisplay::_close()
{
  return 0;
}

int32_t GlDisplay::_start()
{
  return 0;
}

int32_t GlDisplay::_stop()
{
  return 0;
}

int32_t GlDisplay::_reset()
{
  for (unsigned int i = 0; i < images_.size(); i++) {
    delete images_[i];
    images_[i] = nullptr;
  }
  images_.clear();
  img_reader_.clear();
  return 0;
}

void GlDisplay::render_texture()
{
  for (int i = 0; i < static_cast<int>(images_.size()); i++) {
    WindowProperties *prop = &windows_[i];
    ImageObject *img = images_[i];

    glutSetWindow(i + 1);  // Switch to window 2

    prop->img_w = img->width();
    prop->img_h = img->height();

    GLfloat y = prop->img_h > prop->img_w
                    ? 1.0f
                    : static_cast<GLfloat>(1.0 / prop->aspect);

    GLfloat x = prop->img_w > prop->img_h
                    ? 1.0f
                    : static_cast<GLfloat>(1.0 / prop->aspect);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1, 1, 1, 1);

    glBindTexture(GL_TEXTURE_2D,
                  prop->texture);  // Tell GL to use the first texture

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);  // new
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);  // new

    glBegin(GL_QUADS);  // image 0

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(x, 0);
    glVertex2f(static_cast<float>(img->width()), 0);
    glTexCoord2f(x, y);
    glVertex2f(static_cast<float>(img->width()),
               static_cast<float>(img->height()));
    glTexCoord2f(0, y);
    glVertex2f(0, static_cast<float>(img->height()));

    glEnd();

    glFlush();
    glutSwapBuffers();  // new
  }
}

void GlDisplay::reshape(int width, int height)
{
  int id = glutGetWindow();
  WindowProperties *prop = &windows_[id - 1];

  prop->window_w = width;
  prop->window_h = height;
  glViewport(0, 0, prop->window_w, prop->window_h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluOrtho2D(-prop->raster_x, prop->window_w / prop->zoom_f - prop->raster_x,
             -prop->raster_y, prop->window_h / prop->zoom_f - prop->raster_y);
  render_texture();
  // glutPostRedisplay();
}

void GlDisplay::motion(int x, int y)
{
  int id = glutGetWindow();
  WindowProperties *prop = &windows_[id - 1];

  y = prop->window_h - y - 1;
  double result = x / prop->zoom_f - prop->x_prev;
  prop->raster_x = static_cast<int>(result);
  result = y / prop->zoom_f - prop->y_prev;
  prop->raster_y = static_cast<int>(result);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluOrtho2D(-prop->raster_x, prop->window_w / prop->zoom_f - prop->raster_x,
             -prop->raster_y, prop->window_h / prop->zoom_f - prop->raster_y);
  render_texture();
  // glutPostRedisplay();
}

void GlDisplay::zoom(float f, int /*x1*/, int /*y1*/)
{
  int id = glutGetWindow();
  WindowProperties *prop = &windows_[id - 1];

  double zoom_f_prev = prop->zoom_f;
  prop->zoom_f *= f;
  double dx =
      (prop->window_w / prop->zoom_f - prop->window_w / zoom_f_prev) * 0.5;
  double dy =
      (prop->window_h / prop->zoom_f - prop->window_h / zoom_f_prev) * 0.5;
  prop->raster_x += int(dx);
  prop->raster_y += int(dy);
  reshape(prop->window_w, prop->window_h);
}

void GlDisplay::mouse(int button, int state, int x, int y)
{
  int id = glutGetWindow();
  WindowProperties *prop = &windows_[id - 1];

  y = prop->window_h - y - 1;

  if (state == GLUT_DOWN) {
    double result = x / prop->zoom_f - prop->raster_x;
    prop->x_prev = static_cast<int>(result);
    result = y / prop->zoom_f - prop->raster_y;
    prop->y_prev = static_cast<int>(result);
  }
  else if (button == 3) {  // Zoom in
    zoom(1.3f, x, y);
  }
  else if (button == 4) {  // Zoom out
    zoom(1 / 1.3f, x, y);
  }
  return;
}

void GlDisplay::update_texture(int i)
{
  WindowProperties *prop = &windows_[i];
  glutSetWindow(i + 1);  // Switch to window 2

  glBindTexture(GL_TEXTURE_2D, prop->texture);

  // if (i == 2) {
  //   if (prop->internal_data_format == GL_R16) printf("----------GL_R16\n");
  //   if (prop->data_format == GL_RED) printf("----------GL_RED\n");
  //   if (prop->data_type == GL_UNSIGNED_SHORT)
  //     printf("----------GL_UNSIGNED_SHORT\n");
  // }

  glTexImage2D(GL_TEXTURE_2D, 0, prop->internal_data_format,
               images_[i]->width(), images_[i]->height(), 0, prop->data_format,
               prop->data_type, images_[i]->data());
}
