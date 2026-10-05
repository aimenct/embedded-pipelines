// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "core.h"

using namespace epf;

namespace {
std::unique_ptr<Queue> *g_bgr_queue = nullptr;
std::unique_ptr<Queue> *g_f32_queue = nullptr;
std::unique_ptr<Queue> *g_mono8_queue = nullptr;
std::vector<uint8_t> *g_bgr_pattern = nullptr;
std::vector<float> *g_float_pattern = nullptr;
std::vector<uint8_t> *g_mono8_pattern = nullptr;

std::unique_ptr<Queue> create_image_queue(const std::string &name, int width,
                                          int height, int channels,
                                          PixelFormat pixel_format)
{
  ImageObject schema(name, width, height, channels, pixel_format);
  auto schema_msg = std::make_unique<Message>();
  schema_msg->addItem(schema.copyNode());

  auto q = std::make_unique<Queue>();
  q->init(1, std::move(schema_msg));
  return q;
}

void fill_bgr_test_pattern(std::vector<uint8_t> &buffer, int width, int height)
{
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int idx = (y * width + x) * 3;
      const uint8_t xb = static_cast<uint8_t>((255 * x) / (width - 1));
      const uint8_t yb = static_cast<uint8_t>((255 * y) / (height - 1));
      const uint8_t cb =
          static_cast<uint8_t>((((x / 16) + (y / 16)) % 2) * 255);

      buffer[idx + 0] = xb;  // B
      buffer[idx + 1] = yb;  // G
      buffer[idx + 2] = cb;  // R
    }
  }
}

void fill_float32_test_pattern(std::vector<float> &buffer, int width,
                               int height)
{
  constexpr float PI = 3.14159265358979323846f;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float fx = static_cast<float>(x) / static_cast<float>(width - 1);
      const float fy = static_cast<float>(y) / static_cast<float>(height - 1);
      const float wave =
          0.5f + 0.5f * std::sin(8.0f * PI * fx) * std::cos(6.0f * PI * fy);
      buffer[y * width + x] = wave;
    }
  }
}

void fill_mono8_test_pattern(std::vector<uint8_t> &buffer, int width,
                             int height)
{
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float fx = static_cast<float>(x) / static_cast<float>(width - 1);
      const float fy = static_cast<float>(y) / static_cast<float>(height - 1);
      const float radial =
          std::sqrt((fx - 0.5f) * (fx - 0.5f) + (fy - 0.5f) * (fy - 0.5f));
      const float rings = 0.5f + 0.5f * std::cos(40.0f * radial);
      buffer[y * width + x] =
          static_cast<uint8_t>(std::clamp(rings, 0.0f, 1.0f) * 255.0f);
    }
  }
}

void publish_frame(Queue *q, const void *src, size_t size)
{
  QueueWriter writer(q);
  if (writer.startWrite() < 0) {
    std::cerr << "Failed to write test frame." << std::endl;
    return;
  }

  auto on = dynamic_cast<ObjectNode *>(writer.dataSchema()->item(0));
  if (!on) {
    std::cerr << "Invalid image schema node." << std::endl;
    writer.endWrite();
    return;
  }
  ImageObject image(on);
  std::memcpy(image.data(), src, size);
  writer.endWrite();
}

void publish_all_frames_timer(int)
{
  if (g_bgr_queue && g_bgr_pattern)
    publish_frame(g_bgr_queue->get(), g_bgr_pattern->data(),
                  g_bgr_pattern->size() * sizeof(uint8_t));
  if (g_f32_queue && g_float_pattern)
    publish_frame(g_f32_queue->get(), g_float_pattern->data(),
                  g_float_pattern->size() * sizeof(float));
  if (g_mono8_queue && g_mono8_pattern)
    publish_frame(g_mono8_queue->get(), g_mono8_pattern->data(),
                  g_mono8_pattern->size() * sizeof(uint8_t));

  glutTimerFunc(50, publish_all_frames_timer, 0);
}

void keyboard_callback(unsigned char key, int, int)
{
  if (key == 27 || key == 'q' || key == 'Q') {
    glutLeaveMainLoop();
  }
}

}  // namespace

int main(int argc, char **argv)
{
  constexpr int WIDTH = 512;
  constexpr int HEIGHT = 512;

  glutInit(&argc, argv);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

  auto bgr_queue = create_image_queue("bgr8_pattern", WIDTH, HEIGHT, 3,
                                      epf::PixelFormat::BGR8);
  auto f32_queue = create_image_queue("float32_pattern", WIDTH, HEIGHT, 1,
                                      epf::PixelFormat::BIP32f);
  auto mono8_queue = create_image_queue("mono8_pattern", WIDTH, HEIGHT, 1,
                                        epf::PixelFormat::Mono8);

  std::vector<uint8_t> bgr_pattern(WIDTH * HEIGHT * 3);
  std::vector<float> float_pattern(WIDTH * HEIGHT);
  std::vector<uint8_t> mono8_pattern(WIDTH * HEIGHT);

  fill_bgr_test_pattern(bgr_pattern, WIDTH, HEIGHT);
  fill_float32_test_pattern(float_pattern, WIDTH, HEIGHT);
  fill_mono8_test_pattern(mono8_pattern, WIDTH, HEIGHT);

  ImageWindow bgr_window("BGR8 Test Pattern", WIDTH, HEIGHT, 0, 0);
  ImageWindow float_window("Float32 Test Pattern", WIDTH, HEIGHT, WIDTH + 40,
                           0);
  ImageWindow mono8_window("Mono8 Test Pattern", WIDTH, HEIGHT,
                           2 * (WIDTH + 40), 0);

  bgr_window.setDataSource(bgr_queue.get(), 0);
  float_window.setDataSource(f32_queue.get(), 0);
  mono8_window.setDataSource(mono8_queue.get(), 0);

  for (int window_id :
       {bgr_window.id(), float_window.id(), mono8_window.id()}) {
    glutSetWindow(window_id);
    glutKeyboardFunc(keyboard_callback);
  }

  // Publish after readers are attached, otherwise first frame can be missed.
  publish_frame(bgr_queue.get(), bgr_pattern.data(),
                bgr_pattern.size() * sizeof(uint8_t));
  publish_frame(f32_queue.get(), float_pattern.data(),
                float_pattern.size() * sizeof(float));
  publish_frame(mono8_queue.get(), mono8_pattern.data(),
                mono8_pattern.size() * sizeof(uint8_t));
  bgr_window.setBlocking(false);
  float_window.setBlocking(false);
  mono8_window.setBlocking(false);

  bgr_window.start();
  float_window.start();
  mono8_window.start();

  g_bgr_queue = &bgr_queue;
  g_f32_queue = &f32_queue;
  g_mono8_queue = &mono8_queue;
  g_bgr_pattern = &bgr_pattern;
  g_float_pattern = &float_pattern;
  g_mono8_pattern = &mono8_pattern;
  glutTimerFunc(50, publish_all_frames_timer, 0);

  std::cout << "Showing GLUT test windows:\n"
            << " - BGR8 checker/gradient pattern\n"
            << " - Float32 sinusoidal pattern\n"
            << " - Mono8 concentric-rings pattern\n"
            << "Press ESC in a window to close." << std::endl;

  glutMainLoop();
  return 0;
}
