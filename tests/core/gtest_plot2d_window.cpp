#include <GL/freeglut.h>
#include <gtest/gtest.h>

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

void clear_gl_errors()
{
  while (glGetError() != GL_NO_ERROR) {
  }
}

}  // namespace

TEST(Plot2DWindowPublicApiTest, RejectsNullDataSources)
{
  GlutInitializer glut;
  Plot2DWindow window("plot2d-null-sources", 320, 240, 0, 0, 100);

  EXPECT_EQ(window.setXSource(nullptr, 0), -1);
  EXPECT_EQ(window.setYSource(nullptr, 0), -1);
}

TEST(Plot2DWindowPublicApiTest, DegenerateRangeRendersWithoutGlError)
{
  GlutInitializer glut;
  Plot2DWindow window("plot2d-public-api", 320, 240, 0, 0, 100);

  window.setBatchSize(10, 2);
  window.setBatchSize(0, 1);
  window.setBatchSize(10, 0);

  window.setYRange(42.0f, 42.0f);

  clear_gl_errors();
  Window::reshape(640, 480);
  Window::display();

  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
