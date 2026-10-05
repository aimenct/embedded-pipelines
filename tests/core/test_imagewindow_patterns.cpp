// Interactive visual smoke test for ImageWindow upload/display.
//
// This is intentionally NOT a normal CI gtest. It opens a GLUT window and lets
// a user step through synthetic patterns to visually verify color/channel
// handling.
//
// Controls:
//   Space / n : next pattern
//   b         : previous pattern
//   q / Esc   : quit
//
// Formats covered:
//   Mono8, Mono16, BIP32f mono, RGB8, BGR8
//
// Build this as a separate manual test/demo target, not with gtest_main.

#include <GL/freeglut.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

constexpr int K_PATTERN_WIDTH = 384;
constexpr int K_PATTERN_HEIGHT = 256;

struct PatternCase {
    std::string name;
    PixelFormat pixel_format;
    int channels = 1;
    std::unique_ptr<Queue> queue;
    std::unique_ptr<ImageWindow> window;
    std::vector<uint8_t> bytes;
};

std::vector<PatternCase> g_cases;
int g_index = 0;

uint8_t u8_ramp_x(int x, int width)
{
  return static_cast<uint8_t>((x * 255) / std::max(width - 1, 1));
}

uint16_t u16_ramp_x(int x, int width)
{
  return static_cast<uint16_t>((x * 65535u) /
                               static_cast<unsigned>(std::max(width - 1, 1)));
}

float f32_ramp_x(int x, int width)
{
  return static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
}

PatternCase make_mono8()
{
  PatternCase c;
  c.name = "Mono8 horizontal ramp + checker";
  c.pixel_format = PixelFormat::Mono8;
  c.channels = 1;
  c.bytes.resize(K_PATTERN_WIDTH * K_PATTERN_HEIGHT);

  for (int y = 0; y < K_PATTERN_HEIGHT; ++y) {
    for (int x = 0; x < K_PATTERN_WIDTH; ++x) {
      uint8_t v = u8_ramp_x(x, K_PATTERN_WIDTH);
      if (((x / 32) + (y / 32)) % 2 == 0) {
        v = static_cast<uint8_t>(std::min<int>(255, v + 32));
      }
      c.bytes[y * K_PATTERN_WIDTH + x] = v;
    }
  }

  return c;
}

PatternCase make_mono16()
{
  PatternCase c;
  c.name = "Mono16 horizontal ramp";
  c.pixel_format = PixelFormat::Mono16;
  c.channels = 1;
  c.bytes.resize(K_PATTERN_WIDTH * K_PATTERN_HEIGHT * sizeof(uint16_t));

  auto *dst = reinterpret_cast<uint16_t *>(c.bytes.data());
  for (int y = 0; y < K_PATTERN_HEIGHT; ++y) {
    for (int x = 0; x < K_PATTERN_WIDTH; ++x) {
      dst[y * K_PATTERN_WIDTH + x] = u16_ramp_x(x, K_PATTERN_WIDTH);
    }
  }

  return c;
}

PatternCase make_float32_mono()
{
  PatternCase c;
  c.name = "BIP32f mono ramp 0.0 to 1.0";
  c.pixel_format = PixelFormat::BIP32f;
  c.channels = 1;
  c.bytes.resize(K_PATTERN_WIDTH * K_PATTERN_HEIGHT * sizeof(float));

  auto *dst = reinterpret_cast<float *>(c.bytes.data());
  for (int y = 0; y < K_PATTERN_HEIGHT; ++y) {
    for (int x = 0; x < K_PATTERN_WIDTH; ++x) {
      dst[y * K_PATTERN_WIDTH + x] = f32_ramp_x(x, K_PATTERN_WIDTH);
    }
  }

  return c;
}

PatternCase make_rgb8()
{
  PatternCase c;
  c.name = "RGB8: red, green, blue bands + white diagonal";
  c.pixel_format = PixelFormat::RGB8;
  c.channels = 3;
  c.bytes.resize(K_PATTERN_WIDTH * K_PATTERN_HEIGHT * c.channels);

  for (int y = 0; y < K_PATTERN_HEIGHT; ++y) {
    for (int x = 0; x < K_PATTERN_WIDTH; ++x) {
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;

      if (x < K_PATTERN_WIDTH / 3) {
        r = 255;
      }
      else if (x < 2 * K_PATTERN_WIDTH / 3) {
        g = 255;
      }
      else {
        b = 255;
      }

      // White diagonal marker. Helps detect vertical flips/skew.
      if (std::abs(x - (y * K_PATTERN_WIDTH / K_PATTERN_HEIGHT)) < 4) {
        r = g = b = 255;
      }

      const int off = (y * K_PATTERN_WIDTH + x) * 3;
      c.bytes[off + 0] = r;
      c.bytes[off + 1] = g;
      c.bytes[off + 2] = b;
    }
  }

  return c;
}

PatternCase make_bgr8()
{
  PatternCase c;
  c.name =
      "BGR8 memory: should display red, green, blue bands + white diagonal";
  c.pixel_format = PixelFormat::BGR8;
  c.channels = 3;
  c.bytes.resize(K_PATTERN_WIDTH * K_PATTERN_HEIGHT * c.channels);

  for (int y = 0; y < K_PATTERN_HEIGHT; ++y) {
    for (int x = 0; x < K_PATTERN_WIDTH; ++x) {
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;

      if (x < K_PATTERN_WIDTH / 3) {
        r = 255;
      }
      else if (x < 2 * K_PATTERN_WIDTH / 3) {
        g = 255;
      }
      else {
        b = 255;
      }

      // White diagonal marker. Helps detect vertical flips/skew.
      if (std::abs(x - (y * K_PATTERN_WIDTH / K_PATTERN_HEIGHT)) < 4) {
        r = g = b = 255;
      }

      const int off = (y * K_PATTERN_WIDTH + x) * 3;
      c.bytes[off + 0] = b;
      c.bytes[off + 1] = g;
      c.bytes[off + 2] = r;
    }
  }

  return c;
}

void publish_pattern(PatternCase &pattern)
{
  QueueWriter writer(pattern.queue.get());
  if (writer.startWrite() < 0) {
    std::cerr << "Failed to publish pattern.\n";
    return;
  }
  auto *node = dynamic_cast<ObjectNode *>(writer.dataSchema()->item(0));
  if (node == nullptr) {
    writer.endWrite();
    return;
  }
  ImageObject image(node);
  std::memcpy(image.data(), pattern.bytes.data(), pattern.bytes.size());
  writer.endWrite();
}

void initialize_pattern(PatternCase &pattern, int position)
{
  ImageObject schema(pattern.name, K_PATTERN_WIDTH, K_PATTERN_HEIGHT,
                     pattern.channels, pattern.pixel_format);
  auto message = std::make_unique<Message>();
  message->addItem(schema.copyNode());
  pattern.queue = std::make_unique<Queue>();
  pattern.queue->init(1, std::move(message));
  pattern.window = std::make_unique<ImageWindow>(
      pattern.name, K_PATTERN_WIDTH, K_PATTERN_HEIGHT, 100 + position * 30,
      100 + position * 30);
  pattern.window->setDataSource(pattern.queue.get(), 0);
  pattern.window->setBlocking(false);
  pattern.window->start();
  publish_pattern(pattern);
}

void show_current_pattern()
{
  if (g_cases.empty()) return;
  PatternCase &pattern = g_cases[g_index];
  std::cout << "\n[" << (g_index + 1) << "/" << g_cases.size() << "] "
            << pattern.name
            << "\nPixelFormat: " << to_string(pattern.pixel_format)
            << ", channels: " << pattern.channels << "\n";
  for (auto &item : g_cases) {
    glutSetWindow(item.window->id());
    glutHideWindow();
  }
  glutSetWindow(pattern.window->id());
  glutShowWindow();
  glutPostRedisplay();
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

    case 27:  // Esc
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
  std::cout << "Interactive ImageWindow visual pattern test\n"
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

  g_cases.push_back(make_mono8());
  g_cases.push_back(make_mono16());
  g_cases.push_back(make_float32_mono());
  g_cases.push_back(make_rgb8());
  g_cases.push_back(make_bgr8());

  for (int i = 0; i < static_cast<int>(g_cases.size()); ++i) {
    initialize_pattern(g_cases[i], i);
  }

  for (auto &pattern : g_cases) {
    glutSetWindow(pattern.window->id());
    glutKeyboardFunc(keyboard_callback);
  }

  show_current_pattern();

  glutMainLoop();

  return 0;
}
