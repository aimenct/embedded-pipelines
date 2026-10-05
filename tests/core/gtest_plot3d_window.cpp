#include <GL/freeglut.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

struct GlutInitializer {
    GlutInitializer()
    {
      static bool initialized = false;
      if (!initialized) {
        int argc = 1;
        char name[] = "gtest";
        char *argv[] = {name, nullptr};
        glutInit(&argc, argv);
        initialized = true;
      }
    }
};

std::unique_ptr<Queue> make_xyzv_queue()
{
  auto message = std::make_unique<Message>();
  message->addItem(
      std::make_unique<DataNode>("x", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("y", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("z", EP_32F, std::vector<size_t>{1}, nullptr));
  message->addItem(
      std::make_unique<DataNode>("v", EP_16U, std::vector<size_t>{1}, nullptr));

  auto queue = std::make_unique<Queue>();
  queue->init(8, std::move(message));
  return queue;
}

}  // namespace

TEST(Plot3DWindowPublicApiTest, ConfiguresValidDataSources)
{
  GlutInitializer glut;
  auto queue = make_xyzv_queue();
  ASSERT_NE(queue, nullptr);

  {
    Plot3DWindow window("plot3d-sources", 16, 16, 0, 0, 32);

    EXPECT_EQ(window.setXSource(queue.get(), 0, Window::BufferType::Data), 0);
    EXPECT_EQ(window.setYSource(queue.get(), 1, Window::BufferType::Data), 0);
    EXPECT_EQ(window.setZSource(queue.get(), 2, Window::BufferType::Data), 0);
    EXPECT_EQ(window.setVSource(queue.get(), 3, Window::BufferType::Data), 0);
  }

  queue->free();
}

TEST(Plot3DWindowPublicApiTest, RejectsInvalidDataSources)
{
  GlutInitializer glut;
  {
    Plot3DWindow window("plot3d-null-sources", 16, 16, 0, 0, 32);

    EXPECT_EQ(window.setXSource(nullptr, 0), -1);
    EXPECT_EQ(window.setYSource(nullptr, 0), -1);
    EXPECT_EQ(window.setZSource(nullptr, 0), -1);
    EXPECT_EQ(window.setVSource(nullptr, 0), -1);
  }

  auto queue = make_xyzv_queue();
  ASSERT_NE(queue, nullptr);
  {
    Plot3DWindow window("plot3d-invalid-item", 16, 16, 0, 0, 32);
    EXPECT_EQ(window.setXSource(queue.get(), 99), -1);
  }
  queue->free();
}
