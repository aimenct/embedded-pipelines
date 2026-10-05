// Interactive visual smoke test for Plot2DWindow.
//
// This is intentionally NOT a normal CI gtest. It opens a GLUT window and lets
// a user step through synthetic 2D signals to visually verify Plot2DWindow
// rendering, autoscale-like ranges, panning/zooming, and flat-signal safety.
//
// Controls:
//   Space / n : next pattern
//   b         : previous pattern
//   q / Esc   : quit
//
// Mouse controls are the normal Plot2DWindow controls.

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

constexpr int K_COUNT = 512;
constexpr int K_WIDTH = 900;
constexpr int K_HEIGHT = 500;

struct PatternCase {
    std::string name;
    std::vector<float> x;
    std::vector<float> y;
};

std::vector<PatternCase> g_cases;
std::unique_ptr<Queue> g_queue;
std::unique_ptr<QueueWriter> g_writer;
std::unique_ptr<Plot2DWindow> g_window;
int g_index = 0;

void apply_pattern(const PatternCase &pattern)
{
  if (!g_window || !g_writer) return;

  const int count = std::min<int>(K_COUNT, static_cast<int>(pattern.x.size()));
  if (g_writer->startWrite(count) < 0) {
    std::cerr << "Failed to reserve Plot2D pattern queue buffers.\n";
    return;
  }

  for (int i = 0; i < count; ++i) {
    Message *message = g_writer->dataMsg(i);
    auto *x_node =
        dynamic_cast<DataNode *>(message ? message->item(0) : nullptr);
    auto *y_node =
        dynamic_cast<DataNode *>(message ? message->item(1) : nullptr);
    if (!x_node || !y_node || x_node->write(&pattern.x[i]) < 0 ||
        y_node->write(&pattern.y[i]) < 0) {
      g_writer->endWriteAbort();
      std::cerr << "Failed to populate Plot2D pattern queue buffers.\n";
      return;
    }
  }

  if (g_writer->endWrite() < 0) {
    std::cerr << "Failed to publish Plot2D pattern.\n";
    return;
  }

  Window::timerCallback(g_window->id());
  std::cout << "\n[" << (g_index + 1) << "/" << g_cases.size() << "] "
            << pattern.name << "\n";
}

PatternCase make_sine()
{
  PatternCase p;
  p.name = "Sine wave";
  p.x.resize(K_COUNT);
  p.y.resize(K_COUNT);

  for (int i = 0; i < K_COUNT; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(K_COUNT - 1);
    p.x[i] = t * 4.0f * 3.14159265359f;
    p.y[i] = std::sin(p.x[i]);
  }

  return p;
}

PatternCase make_ramp()
{
  PatternCase p;
  p.name = "Ramp";
  p.x.resize(K_COUNT);
  p.y.resize(K_COUNT);

  for (int i = 0; i < K_COUNT; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(K_COUNT - 1);
    p.x[i] = static_cast<float>(i);
    p.y[i] = 2.0f * t - 1.0f;
  }

  return p;
}

PatternCase make_step()
{
  PatternCase p;
  p.name = "Step signal";
  p.x.resize(K_COUNT);
  p.y.resize(K_COUNT);

  for (int i = 0; i < K_COUNT; ++i) {
    p.x[i] = static_cast<float>(i);
    p.y[i] = (i < K_COUNT / 3) ? -1.0f : ((i < 2 * K_COUNT / 3) ? 0.25f : 1.0f);
  }

  return p;
}

PatternCase make_flat()
{
  PatternCase p;
  p.name = "Flat signal, degenerate Y range test";
  p.x.resize(K_COUNT);
  p.y.resize(K_COUNT);

  for (int i = 0; i < K_COUNT; ++i) {
    p.x[i] = static_cast<float>(i);
    p.y[i] = 42.0f;
  }

  return p;
}

PatternCase make_spiky()
{
  PatternCase p;
  p.name = "Spiky signal";
  p.x.resize(K_COUNT);
  p.y.resize(K_COUNT);

  for (int i = 0; i < K_COUNT; ++i) {
    p.x[i] = static_cast<float>(i);
    p.y[i] = 0.15f * std::sin(static_cast<float>(i) * 0.15f);
  }

  for (int i = 64; i < K_COUNT; i += 96) {
    p.y[i] = 1.0f;
  }

  return p;
}

void show_current_pattern()
{
  if (g_cases.empty()) return;
  apply_pattern(g_cases[g_index]);
}

void keyboard_callback(unsigned char key, int, int)
{
  switch (key) {
    case ' ':
    case 'n':
    case 'N':
      g_index = (g_index + 1) % static_cast<int>(g_cases.size());
      show_current_pattern();
      break;

    case 'b':
    case 'B':
      g_index = (g_index + static_cast<int>(g_cases.size()) - 1) %
                static_cast<int>(g_cases.size());
      show_current_pattern();
      break;

    case 27:
    case 'q':
    case 'Q':
      glutLeaveMainLoop();
      break;

    default:
      break;
  }
}

void print_help()
{
  std::cout << "Interactive Plot2DWindow visual pattern test\n"
            << "Controls:\n"
            << "  Space / n : next pattern\n"
            << "  b         : previous pattern\n"
            << "  q / Esc   : quit\n";
}

}  // namespace

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

  print_help();

  g_cases.push_back(make_sine());
  g_cases.push_back(make_ramp());
  g_cases.push_back(make_step());
  g_cases.push_back(make_flat());
  g_cases.push_back(make_spiky());

  auto message = std::make_unique<Message>();
  message->addItem(
      std::make_unique<DataNode>("x", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("y", EP_32F, std::vector<size_t>{1}, nullptr));
  g_queue = std::make_unique<Queue>();
  g_queue->init(K_COUNT, std::move(message));
  g_writer = std::make_unique<QueueWriter>(g_queue.get());

  g_window = std::make_unique<Plot2DWindow>(
      "Plot2D manual visual test", K_WIDTH, K_HEIGHT, 100, 100, K_COUNT);
  if (g_window->setXSource(g_queue.get(), 0) < 0 ||
      g_window->setYSource(g_queue.get(), 1) < 0) {
    std::cerr << "Failed to configure Plot2D queue sources.\n";
    return 1;
  }
  g_window->setBatchSize(K_COUNT, 1);
  g_window->setBlocking(false);
  g_window->start();

  glutSetWindow(g_window->id());
  glutKeyboardFunc(keyboard_callback);

  show_current_pattern();

  glutMainLoop();

  g_window.reset();
  g_writer.reset();
  g_queue->free();
  g_queue.reset();
  return 0;
}
