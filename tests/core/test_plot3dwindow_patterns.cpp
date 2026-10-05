// Interactive visual/manual smoke test for Plot3DWindow.
//
// Publishes a synthetic surface through the same Queue APIs used in production.
//
// Controls:
//   q / Esc : quit
//
// Mouse:
//   left   : pan
//   middle : autoscale/reset
//   right  : rotate
//   wheel  : zoom

#include <GL/freeglut.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

constexpr int K_GRID_N = 40;
constexpr int K_COUNT = K_GRID_N * K_GRID_N;
constexpr int K_WIDTH = 1024;
constexpr int K_HEIGHT = 768;

std::unique_ptr<Queue> g_queue;
std::unique_ptr<QueueWriter> g_writer;
std::unique_ptr<Plot3DWindow> g_window;

std::unique_ptr<Message> make_schema()
{
  auto message = std::make_unique<Message>();
  message->addItem(
      std::make_unique<DataNode>("x", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("y", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("z", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("v", EP_32F, std::vector<size_t>{1}, nullptr));
  return message;
}

bool publish_surface(bool use_radius_for_v)
{
  if (!g_writer || g_writer->startWrite(K_COUNT) < 0) return false;

  for (int iy = 0; iy < K_GRID_N; ++iy) {
    for (int ix = 0; ix < K_GRID_N; ++ix) {
      const int index = iy * K_GRID_N + ix;
      const float x = -3.0f + 6.0f * static_cast<float>(ix) /
                                  static_cast<float>(K_GRID_N - 1);
      const float y = -3.0f + 6.0f * static_cast<float>(iy) /
                                  static_cast<float>(K_GRID_N - 1);
      const float z = std::sin(x) * std::cos(y);
      const float v = use_radius_for_v ? std::sqrt(x * x + y * y) : z;

      Message *message = g_writer->dataMsg(index);
      auto *x_node =
          dynamic_cast<DataNode *>(message ? message->item(0) : nullptr);
      auto *y_node =
          dynamic_cast<DataNode *>(message ? message->item(1) : nullptr);
      auto *z_node =
          dynamic_cast<DataNode *>(message ? message->item(2) : nullptr);
      auto *v_node =
          dynamic_cast<DataNode *>(message ? message->item(3) : nullptr);
      if (!x_node || !y_node || !z_node || !v_node || x_node->write(&x) < 0 ||
          y_node->write(&y) < 0 || z_node->write(&z) < 0 ||
          v_node->write(&v) < 0) {
        g_writer->endWriteAbort();
        return false;
      }
    }
  }

  return g_writer->endWrite() == 0;
}

void keyboard_callback(unsigned char key, int, int)
{
  if (key == 27 || key == 'q' || key == 'Q') {
    glutLeaveMainLoop();
  }
}

void print_help(bool use_radius_for_v)
{
  std::cout
      << "Manual Plot3D pattern started.\n"
      << "Surface: z = sin(x) * cos(y), grid=" << K_GRID_N << "x" << K_GRID_N
      << "\n"
      << "Color source: " << (use_radius_for_v ? "radius" : "z") << "\n"
      << "Mouse: left=pan, middle=autoscale/reset, right=rotate, wheel=zoom\n"
      << "Press q or ESC in window to close.\n";
}

}  // namespace

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

  const bool use_radius_for_v = true;
  g_queue = std::make_unique<Queue>();
  g_queue->init(K_COUNT, make_schema());
  g_writer = std::make_unique<QueueWriter>(g_queue.get());
  g_window = std::make_unique<Plot3DWindow>("Plot3D pattern (manual)", K_WIDTH,
                                            K_HEIGHT, 50, 50, K_COUNT);

  if (g_window->setXSource(g_queue.get(), 0) < 0 ||
      g_window->setYSource(g_queue.get(), 1) < 0 ||
      g_window->setZSource(g_queue.get(), 2) < 0 ||
      g_window->setVSource(g_queue.get(), 3) < 0) {
    std::cerr << "Failed to configure Plot3D queue sources.\n";
    return 1;
  }
  g_window->setBatchSize(K_COUNT, 1);
  g_window->setBlocking(false);
  g_window->start();
  glutSetWindow(g_window->id());
  glutKeyboardFunc(keyboard_callback);

  if (!publish_surface(use_radius_for_v)) {
    std::cerr << "Failed to publish Plot3D surface.\n";
    return 1;
  }
  Window::timerCallback(g_window->id());
  print_help(use_radius_for_v);

  glutMainLoop();

  g_window.reset();
  g_writer.reset();
  g_queue->free();
  g_queue.reset();
  return 0;
}
