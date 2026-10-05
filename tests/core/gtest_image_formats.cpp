#include <gtest/gtest.h>

#include "core.h"

using namespace epf;

TEST(ImageFormatsGLTest, DirectlyUploadableSimpleFormats)
{
  const auto mono8 = to_gl_info(PixelFormat::Mono8, 8, 8, 1);
  EXPECT_EQ(mono8.externalFormat, GL_RED);
  EXPECT_EQ(mono8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(mono8.internalFormat, GL_R8);
  EXPECT_TRUE(mono8.directlyUploadable);

  const auto mono16 = to_gl_info(PixelFormat::Mono16, 8, 8, 1);
  EXPECT_EQ(mono16.externalFormat, GL_RED);
  EXPECT_EQ(mono16.externalType, GL_UNSIGNED_SHORT);
  EXPECT_EQ(mono16.internalFormat, GL_R16);
  EXPECT_TRUE(mono16.directlyUploadable);

  const auto rgb8 = to_gl_info(PixelFormat::RGB8, 8, 8, 3);
  EXPECT_EQ(rgb8.externalFormat, GL_RGB);
  EXPECT_EQ(rgb8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(rgb8.internalFormat, GL_RGB8);
  EXPECT_TRUE(rgb8.directlyUploadable);

  const auto bgr8 = to_gl_info(PixelFormat::BGR8, 8, 8, 3);
  EXPECT_EQ(bgr8.externalFormat, GL_BGR);
  EXPECT_EQ(bgr8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(bgr8.internalFormat, GL_RGB8);
  EXPECT_TRUE(bgr8.directlyUploadable);

  const auto rgba8 = to_gl_info(PixelFormat::RGBA8, 8, 8, 4);
  EXPECT_EQ(rgba8.externalFormat, GL_RGBA);
  EXPECT_EQ(rgba8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(rgba8.internalFormat, GL_RGBA8);
  EXPECT_TRUE(rgba8.directlyUploadable);

  const auto bgra8 = to_gl_info(PixelFormat::BGRA8, 8, 8, 4);
  EXPECT_EQ(bgra8.externalFormat, GL_BGRA);
  EXPECT_EQ(bgra8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(bgra8.internalFormat, GL_RGBA8);
  EXPECT_TRUE(bgra8.directlyUploadable);
}

TEST(ImageFormatsGLTest, BIP32fUploadByChannelCount)
{
  const auto c1 = to_gl_info(PixelFormat::BIP32f, 8, 8, 1);
  EXPECT_EQ(c1.externalFormat, GL_RED);
  EXPECT_EQ(c1.externalType, GL_FLOAT);
  EXPECT_EQ(c1.internalFormat, GL_R32F);
  EXPECT_TRUE(c1.directlyUploadable);

  const auto c2 = to_gl_info(PixelFormat::BIP32f, 8, 8, 2);
  EXPECT_EQ(c2.externalFormat, GL_RG);
  EXPECT_EQ(c2.externalType, GL_FLOAT);
  EXPECT_EQ(c2.internalFormat, GL_RG32F);
  EXPECT_TRUE(c2.directlyUploadable);

  const auto c3 = to_gl_info(PixelFormat::BIP32f, 8, 8, 3);
  EXPECT_EQ(c3.externalFormat, GL_RGB);
  EXPECT_EQ(c3.externalType, GL_FLOAT);
  EXPECT_EQ(c3.internalFormat, GL_RGB32F);
  EXPECT_TRUE(c3.directlyUploadable);

  const auto c4 = to_gl_info(PixelFormat::BIP32f, 8, 8, 4);
  EXPECT_EQ(c4.externalFormat, GL_RGBA);
  EXPECT_EQ(c4.externalType, GL_FLOAT);
  EXPECT_EQ(c4.internalFormat, GL_RGBA32F);
  EXPECT_TRUE(c4.directlyUploadable);

  const auto c5 = to_gl_info(PixelFormat::BIP32f, 8, 8, 5);
  EXPECT_FALSE(c5.directlyUploadable);
}

TEST(ImageFormatsGLTest, BayerSingleChannelUpload)
{
  const auto rg8 = to_gl_info(PixelFormat::BayerRG8, 8, 8, 1);
  EXPECT_EQ(rg8.externalFormat, GL_RED);
  EXPECT_EQ(rg8.externalType, GL_UNSIGNED_BYTE);
  EXPECT_EQ(rg8.internalFormat, GL_R8);
  EXPECT_TRUE(rg8.directlyUploadable);

  const auto rg12 = to_gl_info(PixelFormat::BayerRG12, 8, 8, 1);
  EXPECT_EQ(rg12.externalFormat, GL_RED);
  EXPECT_EQ(rg12.externalType, GL_UNSIGNED_SHORT);
  EXPECT_EQ(rg12.internalFormat, GL_R16);
  EXPECT_TRUE(rg12.directlyUploadable);

  const auto reversed = from_gl_format(GL_RED, GL_UNSIGNED_BYTE, 1);
  EXPECT_EQ(reversed, PixelFormat::Mono8);
}

TEST(ImageFormatsGLTest, PackedFormatsRequireUnpack)
{
  for (auto pf :
       {PixelFormat::Mono10p, PixelFormat::BayerRG10p, PixelFormat::RGB10p32}) {
    const auto gl = to_gl_info(pf, 8, 8, 1);
    EXPECT_FALSE(gl.directlyUploadable);
    EXPECT_TRUE(gl.requiresUnpack);
  }
}

TEST(ImageFormatsGLTest, YUVFormatsRequireColorConversion)
{
  const auto yuv420p = to_gl_info(PixelFormat::YUV420p, 8, 8, 3);
  EXPECT_FALSE(yuv420p.directlyUploadable);
  EXPECT_TRUE(yuv420p.requiresColorConversion);
  EXPECT_TRUE(yuv420p.requiresPlanarUpload);

  const auto nv12 = to_gl_info(PixelFormat::NV12, 8, 8, 3);
  EXPECT_FALSE(nv12.directlyUploadable);
  EXPECT_TRUE(nv12.requiresColorConversion);
  EXPECT_TRUE(nv12.requiresPlanarUpload);

  const auto yuyv422 = to_gl_info(PixelFormat::YUYV422, 8, 8, 3);
  EXPECT_FALSE(yuyv422.directlyUploadable);
  EXPECT_TRUE(yuyv422.requiresColorConversion);
  EXPECT_FALSE(yuyv422.requiresPlanarUpload);
}

TEST(ImageFormatsGLTest, PlanarHyperspectralUploadRules)
{
  const auto bil8c3 = to_gl_info(PixelFormat::BIL8, 8, 8, 3);
  EXPECT_FALSE(bil8c3.directlyUploadable);
  EXPECT_TRUE(bil8c3.requiresPlanarUpload);

  const auto bsq16c5 = to_gl_info(PixelFormat::BSQ16, 8, 8, 5);
  EXPECT_FALSE(bsq16c5.directlyUploadable);
  EXPECT_TRUE(bsq16c5.requiresPlanarUpload);

  const auto bil8c1 = to_gl_info(PixelFormat::BIL8, 8, 8, 1);
  EXPECT_TRUE(bil8c1.directlyUploadable);
  EXPECT_EQ(bil8c1.externalFormat, GL_RED);
}

TEST(ImageFormatsGLTest, ReverseMappingSimpleFormats)
{
  EXPECT_EQ(from_gl_format(GL_RED, GL_UNSIGNED_BYTE, 1), PixelFormat::Mono8);
  EXPECT_EQ(from_gl_format(GL_RGB, GL_UNSIGNED_BYTE, 3), PixelFormat::RGB8);
  EXPECT_EQ(from_gl_format(GL_BGR, GL_UNSIGNED_BYTE, 3), PixelFormat::BGR8);
  EXPECT_EQ(from_gl_format(GL_RGBA, GL_UNSIGNED_BYTE, 4), PixelFormat::RGBA8);
  EXPECT_EQ(from_gl_format(GL_BGRA, GL_UNSIGNED_BYTE, 4), PixelFormat::BGRA8);

  EXPECT_EQ(from_gl_format(GL_RED, GL_FLOAT, 1), PixelFormat::BIP32f);
  EXPECT_EQ(from_gl_format(GL_RG, GL_FLOAT, 2), PixelFormat::BIP32f);
  EXPECT_EQ(from_gl_format(GL_RGB, GL_FLOAT, 3), PixelFormat::BIP32f);
  EXPECT_EQ(from_gl_format(GL_RGBA, GL_FLOAT, 4), PixelFormat::BIP32f);
}

TEST(ImageFormatsGLTest, ReverseMappingUnsignedShortBitDepthAndAmbiguity)
{
  EXPECT_EQ(from_gl_format(GL_RED, GL_UNSIGNED_SHORT, 1, 10),
            PixelFormat::Mono10);
  EXPECT_EQ(from_gl_format(GL_RGB, GL_UNSIGNED_SHORT, 3, 10),
            PixelFormat::RGB10);
  EXPECT_EQ(from_gl_format(GL_BGR, GL_UNSIGNED_SHORT, 3, 10),
            PixelFormat::BGR10);

  EXPECT_EQ(from_gl_format(GL_RGB, GL_UNSIGNED_SHORT, 3), PixelFormat::BIP16);
}
