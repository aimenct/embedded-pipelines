#include <GL/freeglut.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

struct GlutInitializer {
    GlutInitializer()
    {
      static bool init = false;
      if (!init) {
        int argc = 1;
        char name[] = "gtest";
        char *argv[] = {name, nullptr};
        glutInit(&argc, argv);
        init = true;
      }
    }
};

}  // namespace

TEST(HistWindowTest, ComputesDomainAndNormalizedHistogramAcrossFormats)
{
  GlutInitializer glut;
  HistWindow hist_window("hist-test", 320, 240, 0, 0);

  // Mono8: exact base domain [0, 255]
  {
    ImageObject img("mono8", 2, 2, 1, PixelFormat::Mono8);
    auto *ptr = static_cast<uint8_t *>(img.data());
    const std::array<uint8_t, 4> values = {0, 64, 128, 255};
    std::copy(values.begin(), values.end(), ptr);

    std::vector<float> histogram(256, 0.0f);
    double histogram_min = 0.0;
    double histogram_max = 0.0;
    EXPECT_EQ(
        compute_histogram256(img, histogram, &histogram_min, &histogram_max),
        0);
    EXPECT_DOUBLE_EQ(histogram_min, 0.0);
    EXPECT_DOUBLE_EQ(histogram_max, 255.0);

    hist_window.computeHistogram(img);

    EXPECT_DOUBLE_EQ(hist_window.minVal(), 0.0);
    EXPECT_DOUBLE_EQ(hist_window.maxVal(), 255.0);
    for (float bin : histogram) {
      EXPECT_GE(bin, 0.0f);
      EXPECT_LE(bin, 1.0f);
    }
  }

  // Mono16: exact base domain [0, 65535]
  {
    ImageObject img("mono16", 3, 1, 1, PixelFormat::Mono16);
    auto *ptr = static_cast<uint16_t *>(img.data());
    const std::array<uint16_t, 3> values = {0, 32768, 65535};
    std::copy(values.begin(), values.end(), ptr);

    std::vector<float> histogram(256, 0.0f);
    double histogram_min = 0.0;
    double histogram_max = 0.0;
    EXPECT_EQ(
        compute_histogram256(img, histogram, &histogram_min, &histogram_max),
        0);
    EXPECT_DOUBLE_EQ(histogram_min, 0.0);
    EXPECT_DOUBLE_EQ(histogram_max, 65535.0);

    hist_window.computeHistogram(img);

    EXPECT_DOUBLE_EQ(hist_window.minVal(), 0.0);
    EXPECT_DOUBLE_EQ(hist_window.maxVal(), 65535.0);
    for (float bin : histogram) {
      EXPECT_GE(bin, 0.0f);
      EXPECT_LE(bin, 1.0f);
    }
  }

  // BIP32f: NaN/Inf skipped, inferred finite domain is [-1, 1]
  {
    ImageObject img("bip32f", 6, 1, 1, PixelFormat::BIP32f);
    auto *ptr = static_cast<float *>(img.data());
    const std::array<float, 6> values = {
        -1.0f,
        0.0f,
        0.5f,
        1.0f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity()};
    std::copy(values.begin(), values.end(), ptr);

    std::vector<float> histogram(256, 0.0f);
    double histogram_min = 0.0;
    double histogram_max = 0.0;
    EXPECT_EQ(
        compute_histogram256(img, histogram, &histogram_min, &histogram_max),
        0);
    EXPECT_DOUBLE_EQ(histogram_min, -1.0);
    EXPECT_DOUBLE_EQ(histogram_max, 1.0);

    hist_window.computeHistogram(img);

    EXPECT_DOUBLE_EQ(hist_window.minVal(), -1.0);
    EXPECT_DOUBLE_EQ(hist_window.maxVal(), 1.0);

    const int non_zero_bins = static_cast<int>(std::count_if(
        histogram.begin(), histogram.end(), [](float v) { return v > 0.0f; }));
    EXPECT_EQ(non_zero_bins, 4);
    for (float bin : histogram) {
      EXPECT_GE(bin, 0.0f);
      EXPECT_LE(bin, 1.0f);
    }
  }

  // Constant float image: must not divide by zero, domain centered at value.
  {
    ImageObject img("const32f", 2, 2, 1, PixelFormat::BIP32f);
    auto *ptr = static_cast<float *>(img.data());
    const std::array<float, 4> values = {42.0f, 42.0f, 42.0f, 42.0f};
    std::copy(values.begin(), values.end(), ptr);

    std::vector<float> histogram(256, 0.0f);
    double histogram_min = 0.0;
    double histogram_max = 0.0;
    EXPECT_EQ(
        compute_histogram256(img, histogram, &histogram_min, &histogram_max),
        0);
    EXPECT_LT(histogram_min, 42.0);
    EXPECT_GT(histogram_max, 42.0);

    hist_window.computeHistogram(img);

    EXPECT_LT(hist_window.minVal(), 42.0);
    EXPECT_GT(hist_window.maxVal(), 42.0);

    const int non_zero_bins = static_cast<int>(std::count_if(
        histogram.begin(), histogram.end(), [](float v) { return v > 0.0f; }));
    EXPECT_EQ(non_zero_bins, 1);
    EXPECT_FLOAT_EQ(*std::max_element(histogram.begin(), histogram.end()),
                    1.0f);
    for (float bin : histogram) {
      EXPECT_GE(bin, 0.0f);
      EXPECT_LE(bin, 1.0f);
    }
  }
}
