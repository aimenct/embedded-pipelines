// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "image_formats.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace epf {

struct FmtDesc {
    std::vector<int32_t> depth;  // one entry means all channels use same depth
    float bytesPerSample{0.0f};  // bytes per stored sample for ordinary layouts
    int32_t fixedChannels = 0;   // 0 = dynamic channel count
    BaseType baseType = BaseType::EP_8U;
    float bytesPerPixelOverride{
        0.0f};  // bytes/pixel for packed/subsampled layouts
    bool packed = false;
    bool planar = false;
    bool colorConversion = false;
};

static const std::unordered_map<PixelFormat, FmtDesc> fmt_table = {
    // Mono
    {PixelFormat::Mono8, {{8}, 1.0f, 1, BaseType::EP_8U}},
    {PixelFormat::Mono10, {{10}, 2.0f, 1, BaseType::EP_16U}},
    {PixelFormat::Mono12, {{12}, 2.0f, 1, BaseType::EP_16U}},
    {PixelFormat::Mono10p,
     {{10}, 1.0f, 1, BaseType::EP_8U, 10.0f / 8.0f, true}},
    {PixelFormat::Mono10g40,
     {{10}, 1.0f, 1, BaseType::EP_8U, 10.0f / 8.0f, true}},
    {PixelFormat::Mono12g24,
     {{12}, 1.0f, 1, BaseType::EP_8U, 12.0f / 8.0f, true}},
    {PixelFormat::Mono16, {{16}, 2.0f, 1, BaseType::EP_16U}},

    // Bayer: memory is one sample per pixel; pattern is EPF metadata, not an
    // OpenGL format.
    {PixelFormat::BayerRG8, {{8}, 1.0f, 1, BaseType::EP_8U}},
    {PixelFormat::BayerRG10, {{10}, 2.0f, 1, BaseType::EP_16U}},
    {PixelFormat::BayerRG12, {{12}, 2.0f, 1, BaseType::EP_16U}},
    {PixelFormat::BayerRG10p,
     {{10}, 1.0f, 1, BaseType::EP_8U, 10.0f / 8.0f, true}},
    {PixelFormat::BayerRG12p,
     {{12}, 1.0f, 1, BaseType::EP_8U, 12.0f / 8.0f, true}},
    {PixelFormat::BayerRG10g40,
     {{10}, 1.0f, 1, BaseType::EP_8U, 10.0f / 8.0f, true}},
    {PixelFormat::BayerGR8, {{8}, 1.0f, 1, BaseType::EP_8U}},
    {PixelFormat::BayerGR10, {{10}, 2.0f, 1, BaseType::EP_16U}},
    {PixelFormat::BayerGR12, {{12}, 2.0f, 1, BaseType::EP_16U}},

    // RGB / BGR
    {PixelFormat::RGB8, {{8}, 1.0f, 3, BaseType::EP_8U}},
    {PixelFormat::RGB10, {{10}, 2.0f, 3, BaseType::EP_16U}},
    {PixelFormat::RGB12, {{12}, 2.0f, 3, BaseType::EP_16U}},
    {PixelFormat::RGB10p32, {{10}, 1.0f, 3, BaseType::EP_8U, 4.0f, true}},
    {PixelFormat::BGR8, {{8}, 1.0f, 3, BaseType::EP_8U}},
    {PixelFormat::BGR10, {{10}, 2.0f, 3, BaseType::EP_16U}},
    {PixelFormat::BGR12, {{12}, 2.0f, 3, BaseType::EP_16U}},
    {PixelFormat::BGR10p32, {{10}, 1.0f, 3, BaseType::EP_8U, 4.0f, true}},

    // RGBA / BGRA
    {PixelFormat::RGBA8, {{8}, 1.0f, 4, BaseType::EP_8U}},
    {PixelFormat::RGBA12, {{12}, 2.0f, 4, BaseType::EP_16U}},
    {PixelFormat::BGRA8, {{8}, 1.0f, 4, BaseType::EP_8U}},
    {PixelFormat::BGRA10, {{10}, 2.0f, 4, BaseType::EP_16U}},
    {PixelFormat::BGRA12, {{12}, 2.0f, 4, BaseType::EP_16U}},

    // YUV: bytesPerPixelOverride is total stored bytes per luma pixel.
    {PixelFormat::YUV420p,
     {{8}, 1.0f, 3, BaseType::EP_8U, 1.5f, false, true, true}},
    {PixelFormat::YUV422p,
     {{8}, 1.0f, 3, BaseType::EP_8U, 2.0f, false, true, true}},
    {PixelFormat::YUV444p,
     {{8}, 1.0f, 3, BaseType::EP_8U, 3.0f, false, true, true}},
    {PixelFormat::NV12,
     {{8}, 1.0f, 3, BaseType::EP_8U, 1.5f, false, true, true}},
    {PixelFormat::YUYV422,
     {{8}, 1.0f, 3, BaseType::EP_8U, 2.0f, false, false, true}},
    {PixelFormat::YUV422_8_UYVY,
     {{8}, 1.0f, 3, BaseType::EP_8U, 2.0f, false, false, true}},

    // Mosaic/hyperspectral macro-pixel formats. The override is total bytes per
    // mosaic cell.
    {PixelFormat::Mosaic3u8, {{8}, 1.0f, 9, BaseType::EP_8U, 9.0f}},
    {PixelFormat::Mosaic4u8, {{8}, 1.0f, 16, BaseType::EP_8U, 16.0f}},
    {PixelFormat::Mosaic5u8, {{8}, 1.0f, 25, BaseType::EP_8U, 25.0f}},
    {PixelFormat::Mosaic3u10, {{10}, 2.0f, 9, BaseType::EP_16U, 18.0f}},
    {PixelFormat::Mosaic4u10, {{10}, 2.0f, 16, BaseType::EP_16U, 32.0f}},
    {PixelFormat::Mosaic5u10, {{10}, 2.0f, 25, BaseType::EP_16U, 50.0f}},
    {PixelFormat::Mosaic3u12, {{12}, 2.0f, 9, BaseType::EP_16U, 18.0f}},
    {PixelFormat::Mosaic4u12, {{12}, 2.0f, 16, BaseType::EP_16U, 32.0f}},
    {PixelFormat::Mosaic5u12, {{12}, 2.0f, 25, BaseType::EP_16U, 50.0f}},

    // Dynamic interleaved
    {PixelFormat::BIP8, {{8}, 1.0f, 0, BaseType::EP_8U}},
    {PixelFormat::BIP10, {{10}, 2.0f, 0, BaseType::EP_16U}},
    {PixelFormat::BIP12, {{12}, 2.0f, 0, BaseType::EP_16U}},
    {PixelFormat::BIP16, {{16}, 2.0f, 0, BaseType::EP_16U}},
    {PixelFormat::BIP8s, {{8}, 1.0f, 0, BaseType::EP_8S}},
    {PixelFormat::BIP10s, {{10}, 2.0f, 0, BaseType::EP_16S}},
    {PixelFormat::BIP12s, {{12}, 2.0f, 0, BaseType::EP_16S}},
    {PixelFormat::BIP16s, {{16}, 2.0f, 0, BaseType::EP_16S}},
    {PixelFormat::BIP32s, {{32}, 4.0f, 0, BaseType::EP_32S}},
    {PixelFormat::BIP32f, {{32}, 4.0f, 0, BaseType::EP_32F}},
    {PixelFormat::BIP64f, {{64}, 8.0f, 0, BaseType::EP_64F}},

    // Dynamic planar. Not directly uploadable as a single ordinary 2D texture
    // when channels > 1.
    {PixelFormat::BIL8, {{8}, 1.0f, 0, BaseType::EP_8U, 0.0f, false, true}},
    {PixelFormat::BIL10, {{10}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}},
    {PixelFormat::BIL12, {{12}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}},
    {PixelFormat::BIL16, {{16}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}},
    {PixelFormat::BSQ8, {{8}, 1.0f, 0, BaseType::EP_8U, 0.0f, false, true}},
    {PixelFormat::BSQ10, {{10}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}},
    {PixelFormat::BSQ12, {{12}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}},
    {PixelFormat::BSQ16, {{16}, 2.0f, 0, BaseType::EP_16U, 0.0f, false, true}}};

static const FmtDesc &desc_for(PixelFormat pf)
{
  auto it = fmt_table.find(pf);
  if (it == fmt_table.end()) {
    throw std::runtime_error("Unsupported PixelFormat");
  }
  return it->second;
}

PixelFormatInfo get_image_info(PixelFormat pf, int32_t width, int32_t height,
                               int32_t channels)
{
  const auto &d = desc_for(pf);
  PixelFormatInfo out;

  out.numChannels = (d.fixedChannels > 0) ? d.fixedChannels : channels;
  out.bytesPerPixel =
      (d.bytesPerPixelOverride > 0.0f)
          ? d.bytesPerPixelOverride
          : d.bytesPerSample * static_cast<float>(std::max(out.numChannels, 0));

  if (d.depth.size() == 1) {
    out.depth.assign(std::max(out.numChannels, 0), d.depth[0]);
  }
  else {
    out.depth = d.depth;
  }

  out.baseType = d.baseType;
  const double expected_size = static_cast<double>(width) *
                               static_cast<double>(height) *
                               static_cast<double>(out.bytesPerPixel);
  out.size = static_cast<int64_t>(std::ceil(expected_size - 1e-9));

  if (d.fixedChannels > 0 && channels > 0 && out.numChannels != channels) {
    std::cerr << "Warning: PixelFormat " << to_string(pf) << " has "
              << d.fixedChannels << " channels but requested " << channels
              << "\n";
  }

  return out;
}

std::string to_string(PixelFormat pf)
{
  static const std::unordered_map<PixelFormat, std::string> names = {
      {PixelFormat::Mono8, "Mono8"},
      {PixelFormat::Mono10, "Mono10"},
      {PixelFormat::Mono12, "Mono12"},
      {PixelFormat::Mono10p, "Mono10p"},
      {PixelFormat::Mono10g40, "Mono10g40"},
      {PixelFormat::Mono12g24, "Mono12g24"},
      {PixelFormat::Mono16, "Mono16"},
      {PixelFormat::BayerRG8, "BayerRG8"},
      {PixelFormat::BayerRG10, "BayerRG10"},
      {PixelFormat::BayerRG12, "BayerRG12"},
      {PixelFormat::BayerRG10p, "BayerRG10p"},
      {PixelFormat::BayerRG12p, "BayerRG12p"},
      {PixelFormat::BayerRG10g40, "BayerRG10g40"},
      {PixelFormat::BayerGR8, "BayerGR8"},
      {PixelFormat::BayerGR10, "BayerGR10"},
      {PixelFormat::BayerGR12, "BayerGR12"},
      {PixelFormat::RGB8, "RGB8"},
      {PixelFormat::RGB10, "RGB10"},
      {PixelFormat::RGB12, "RGB12"},
      {PixelFormat::RGB10p32, "RGB10p32"},
      {PixelFormat::BGR8, "BGR8"},
      {PixelFormat::BGR10, "BGR10"},
      {PixelFormat::BGR12, "BGR12"},
      {PixelFormat::BGR10p32, "BGR10p32"},
      {PixelFormat::RGBA8, "RGBA8"},
      {PixelFormat::RGBA12, "RGBA12"},
      {PixelFormat::BGRA8, "BGRA8"},
      {PixelFormat::BGRA10, "BGRA10"},
      {PixelFormat::BGRA12, "BGRA12"},
      {PixelFormat::YUV420p, "YUV420p"},
      {PixelFormat::YUV422p, "YUV422p"},
      {PixelFormat::YUV444p, "YUV444p"},
      {PixelFormat::NV12, "NV12"},
      {PixelFormat::YUYV422, "YUYV422"},
      {PixelFormat::YUV422_8_UYVY, "YUV422_8_UYVY"},
      {PixelFormat::Mosaic3u8, "Mosaic3u8"},
      {PixelFormat::Mosaic4u8, "Mosaic4u8"},
      {PixelFormat::Mosaic5u8, "Mosaic5u8"},
      {PixelFormat::Mosaic3u10, "Mosaic3u10"},
      {PixelFormat::Mosaic4u10, "Mosaic4u10"},
      {PixelFormat::Mosaic5u10, "Mosaic5u10"},
      {PixelFormat::Mosaic3u12, "Mosaic3u12"},
      {PixelFormat::Mosaic4u12, "Mosaic4u12"},
      {PixelFormat::Mosaic5u12, "Mosaic5u12"},
      {PixelFormat::BIP8, "BIP8"},
      {PixelFormat::BIP10, "BIP10"},
      {PixelFormat::BIP12, "BIP12"},
      {PixelFormat::BIP16, "BIP16"},
      {PixelFormat::BIP8s, "BIP8s"},
      {PixelFormat::BIP10s, "BIP10s"},
      {PixelFormat::BIP12s, "BIP12s"},
      {PixelFormat::BIP16s, "BIP16s"},
      {PixelFormat::BIP32s, "BIP32s"},
      {PixelFormat::BIP32f, "BIP32f"},
      {PixelFormat::BIP64f, "BIP64f"},
      {PixelFormat::BIL8, "BIL8"},
      {PixelFormat::BIL10, "BIL10"},
      {PixelFormat::BIL12, "BIL12"},
      {PixelFormat::BIL16, "BIL16"},
      {PixelFormat::BSQ8, "BSQ8"},
      {PixelFormat::BSQ10, "BSQ10"},
      {PixelFormat::BSQ12, "BSQ12"},
      {PixelFormat::BSQ16, "BSQ16"}};
  auto it = names.find(pf);
  return (it != names.end() ? it->second : "Unknown");
}

std::string to_string(ImageEncoding enc)
{
  static const std::unordered_map<ImageEncoding, std::string> names = {
      {ImageEncoding::JPEG, "JPEG"},
      {ImageEncoding::PNG, "PNG"},
      {ImageEncoding::TIF, "TIF"},
      {ImageEncoding::WEBP, "WEBP"},
      {ImageEncoding::MJPEG, "MJPEG"},
      {ImageEncoding::H264, "H264"},
      {ImageEncoding::HEVC, "HEVC"},
      {ImageEncoding::VP8, "VP8"},
      {ImageEncoding::VP9, "VP9"},
      {ImageEncoding::AV1, "AV1"},
      {ImageEncoding::Bitstream, "Bitstream"}};
  auto it = names.find(enc);
  return (it != names.end() ? it->second : "Unknown");
}

PixelFormat from_string_pixel_format(const std::string &name)
{
  static const std::unordered_map<std::string, PixelFormat> rev = {
      {"Mono8", PixelFormat::Mono8},
      {"Mono10", PixelFormat::Mono10},
      {"Mono12", PixelFormat::Mono12},
      {"Mono10p", PixelFormat::Mono10p},
      {"Mono10g40", PixelFormat::Mono10g40},
      {"Mono12g24", PixelFormat::Mono12g24},
      {"Mono16", PixelFormat::Mono16},
      {"BayerRG8", PixelFormat::BayerRG8},
      {"BayerRG10", PixelFormat::BayerRG10},
      {"BayerRG12", PixelFormat::BayerRG12},
      {"BayerRG10p", PixelFormat::BayerRG10p},
      {"BayerRG12p", PixelFormat::BayerRG12p},
      {"BayerRG10g40", PixelFormat::BayerRG10g40},
      {"BayerGR8", PixelFormat::BayerGR8},
      {"BayerGR10", PixelFormat::BayerGR10},
      {"BayerGR12", PixelFormat::BayerGR12},
      {"RGB8", PixelFormat::RGB8},
      {"RGB10", PixelFormat::RGB10},
      {"RGB12", PixelFormat::RGB12},
      {"RGB10p32", PixelFormat::RGB10p32},
      {"BGR8", PixelFormat::BGR8},
      {"BGR10", PixelFormat::BGR10},
      {"BGR12", PixelFormat::BGR12},
      {"BGR10p32", PixelFormat::BGR10p32},
      {"RGBA8", PixelFormat::RGBA8},
      {"RGBA12", PixelFormat::RGBA12},
      {"BGRA8", PixelFormat::BGRA8},
      {"BGRA10", PixelFormat::BGRA10},
      {"BGRA12", PixelFormat::BGRA12},
      {"YUV420p", PixelFormat::YUV420p},
      {"YUV422p", PixelFormat::YUV422p},
      {"YUV444p", PixelFormat::YUV444p},
      {"NV12", PixelFormat::NV12},
      {"YUYV422", PixelFormat::YUYV422},
      {"YUV422_8_UYVY", PixelFormat::YUV422_8_UYVY},
      {"Mosaic3u8", PixelFormat::Mosaic3u8},
      {"Mosaic4u8", PixelFormat::Mosaic4u8},
      {"Mosaic5u8", PixelFormat::Mosaic5u8},
      {"Mosaic3u10", PixelFormat::Mosaic3u10},
      {"Mosaic4u10", PixelFormat::Mosaic4u10},
      {"Mosaic5u10", PixelFormat::Mosaic5u10},
      {"Mosaic3u12", PixelFormat::Mosaic3u12},
      {"Mosaic4u12", PixelFormat::Mosaic4u12},
      {"Mosaic5u12", PixelFormat::Mosaic5u12},
      {"BIP8", PixelFormat::BIP8},
      {"BIP10", PixelFormat::BIP10},
      {"BIP12", PixelFormat::BIP12},
      {"BIP16", PixelFormat::BIP16},
      {"BIP8s", PixelFormat::BIP8s},
      {"BIP10s", PixelFormat::BIP10s},
      {"BIP12s", PixelFormat::BIP12s},
      {"BIP16s", PixelFormat::BIP16s},
      {"BIP32s", PixelFormat::BIP32s},
      {"BIP32f", PixelFormat::BIP32f},
      {"BIP64f", PixelFormat::BIP64f},
      {"BIL8", PixelFormat::BIL8},
      {"BIL10", PixelFormat::BIL10},
      {"BIL12", PixelFormat::BIL12},
      {"BIL16", PixelFormat::BIL16},
      {"BSQ8", PixelFormat::BSQ8},
      {"BSQ10", PixelFormat::BSQ10},
      {"BSQ12", PixelFormat::BSQ12},
      {"BSQ16", PixelFormat::BSQ16},

      // Common aliases.
      {"BIP32F", PixelFormat::BIP32f},
      {"BIP64F", PixelFormat::BIP64f},
  };

  auto it = rev.find(name);
  if (it == rev.end()) {
    throw std::invalid_argument("Unknown PixelFormat: " + name);
  }
  return it->second;
}

ImageEncoding from_string_image_encoding(const std::string &name)
{
  static const std::unordered_map<std::string, ImageEncoding> rev = {
      {"JPEG", ImageEncoding::JPEG},
      {"PNG", ImageEncoding::PNG},
      {"TIF", ImageEncoding::TIF},
      {"WEBP", ImageEncoding::WEBP},
      {"MJPEG", ImageEncoding::MJPEG},
      {"H264", ImageEncoding::H264},
      {"HEVC", ImageEncoding::HEVC},
      {"VP8", ImageEncoding::VP8},
      {"VP9", ImageEncoding::VP9},
      {"AV1", ImageEncoding::AV1},
      {"Bitstream", ImageEncoding::Bitstream},
      {"TIFF", ImageEncoding::TIF},
      {"tif", ImageEncoding::TIF},
      {"tiff", ImageEncoding::TIF}};
  auto it = rev.find(name);
  if (it == rev.end())
    throw std::invalid_argument("Unknown ImageEncoding: " + name);
  return it->second;
}

static bool is_bip(PixelFormat pf)
{
  switch (pf) {
    case PixelFormat::BIP8:
    case PixelFormat::BIP10:
    case PixelFormat::BIP12:
    case PixelFormat::BIP16:
    case PixelFormat::BIP8s:
    case PixelFormat::BIP10s:
    case PixelFormat::BIP12s:
    case PixelFormat::BIP16s:
    case PixelFormat::BIP32s:
    case PixelFormat::BIP32f:
    case PixelFormat::BIP64f:
      return true;
    default:
      return false;
  }
}

static GLenum normalized_format_for_channels(unsigned int channels)
{
  switch (channels) {
    case 1:
      return GL_RED;
    case 2:
      return GL_RG;
    case 3:
      return GL_RGB;
    case 4:
      return GL_RGBA;
    default:
      return GL_NONE;
  }
}

static GLenum integer_format_for_channels(unsigned int channels)
{
  switch (channels) {
    case 1:
      return GL_RED_INTEGER;
    case 2:
      return GL_RG_INTEGER;
    case 3:
      return GL_RGB_INTEGER;
    case 4:
      return GL_RGBA_INTEGER;
    default:
      return GL_NONE;
  }
}

static GLenum unsigned_internal_format(unsigned int bits, unsigned int channels)
{
  if (bits <= 8) {
    switch (channels) {
      case 1:
        return GL_R8;
      case 2:
        return GL_RG8;
      case 3:
        return GL_RGB8;
      case 4:
        return GL_RGBA8;
      default:
        return GL_NONE;
    }
  }

  // 10/12-bit EPF formats are stored in 16-bit containers for OpenGL upload.
  switch (channels) {
    case 1:
      return GL_R16;
    case 2:
      return GL_RG16;
    case 3:
      return GL_RGB16;
    case 4:
      return GL_RGBA16;
    default:
      return GL_NONE;
  }
}

static GLenum signed_integer_internal_format(unsigned int bits,
                                             unsigned int channels)
{
  if (bits <= 8) {
    switch (channels) {
      case 1:
        return GL_R8I;
      case 2:
        return GL_RG8I;
      case 3:
        return GL_RGB8I;
      case 4:
        return GL_RGBA8I;
      default:
        return GL_NONE;
    }
  }
  if (bits <= 16) {
    switch (channels) {
      case 1:
        return GL_R16I;
      case 2:
        return GL_RG16I;
      case 3:
        return GL_RGB16I;
      case 4:
        return GL_RGBA16I;
      default:
        return GL_NONE;
    }
  }

  switch (channels) {
    case 1:
      return GL_R32I;
    case 2:
      return GL_RG32I;
    case 3:
      return GL_RGB32I;
    case 4:
      return GL_RGBA32I;
    default:
      return GL_NONE;
  }
}

static GLenum float_internal_format(unsigned int bits, unsigned int channels)
{
  if (bits == 32) {
    switch (channels) {
      case 1:
        return GL_R32F;
      case 2:
        return GL_RG32F;
      case 3:
        return GL_RGB32F;
      case 4:
        return GL_RGBA32F;
      default:
        return GL_NONE;
    }
  }

  // GL 64-bit float texture internal formats are not generally available as
  // ordinary sampled texture formats. Keep upload unsupported by default.
  return GL_NONE;
}

GLPixelFormatInfo to_gl_info(PixelFormat pf, unsigned int, unsigned int,
                             unsigned int channels)
{
  GLPixelFormatInfo gl;
  const auto &d = desc_for(pf);

  if (d.packed) {
    gl.requiresUnpack = true;
    return gl;
  }

  if (d.colorConversion) {
    gl.requiresColorConversion = true;
    gl.requiresPlanarUpload = d.planar;
    return gl;
  }

  if (d.planar && channels > 1) {
    gl.requiresPlanarUpload = true;
    return gl;
  }

  unsigned int gl_channels = channels;
  if (d.fixedChannels > 0) {
    gl_channels = static_cast<unsigned int>(d.fixedChannels);
  }
  if (gl_channels == 0) {
    return gl;
  }

  const unsigned int bits =
      d.depth.empty() ? 0U : static_cast<unsigned int>(d.depth[0]);

  switch (pf) {
    // Single-sample mono and raw Bayer formats can be uploaded as one-channel
    // data.
    case PixelFormat::Mono8:
    case PixelFormat::Mono10:
    case PixelFormat::Mono12:
    case PixelFormat::Mono16:
    case PixelFormat::BayerRG8:
    case PixelFormat::BayerRG10:
    case PixelFormat::BayerRG12:
    case PixelFormat::BayerGR8:
    case PixelFormat::BayerGR10:
    case PixelFormat::BayerGR12:
      gl.externalFormat = GL_RED;
      gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      gl.internalFormat = unsigned_internal_format(bits, 1);
      gl.directlyUploadable = gl.internalFormat != GL_NONE;
      return gl;

    case PixelFormat::RGB8:
    case PixelFormat::RGB10:
    case PixelFormat::RGB12:
      gl.externalFormat = GL_RGB;
      gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      gl.internalFormat = unsigned_internal_format(bits, 3);
      gl.directlyUploadable = gl.internalFormat != GL_NONE;
      return gl;

    case PixelFormat::BGR8:
    case PixelFormat::BGR10:
    case PixelFormat::BGR12:
      gl.externalFormat = GL_BGR;
      gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      gl.internalFormat = unsigned_internal_format(bits, 3);
      gl.directlyUploadable = gl.internalFormat != GL_NONE;
      return gl;

    case PixelFormat::RGBA8:
    case PixelFormat::RGBA12:
      gl.externalFormat = GL_RGBA;
      gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      gl.internalFormat = unsigned_internal_format(bits, 4);
      gl.directlyUploadable = gl.internalFormat != GL_NONE;
      return gl;

    case PixelFormat::BGRA8:
    case PixelFormat::BGRA10:
    case PixelFormat::BGRA12:
      gl.externalFormat = GL_BGRA;
      gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      gl.internalFormat = unsigned_internal_format(bits, 4);
      gl.directlyUploadable = gl.internalFormat != GL_NONE;
      return gl;

    default:
      break;
  }

  if (is_bip(pf) || ((pf == PixelFormat::BIL8 || pf == PixelFormat::BIL10 ||
                      pf == PixelFormat::BIL12 || pf == PixelFormat::BIL16 ||
                      pf == PixelFormat::BSQ8 || pf == PixelFormat::BSQ10 ||
                      pf == PixelFormat::BSQ12 || pf == PixelFormat::BSQ16) &&
                     gl_channels == 1)) {
    switch (d.baseType) {
      case BaseType::EP_8U:
      case BaseType::EP_16U:
        gl.externalFormat = normalized_format_for_channels(gl_channels);
        gl.externalType = (bits <= 8) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
        gl.internalFormat = unsigned_internal_format(bits, gl_channels);
        break;

      case BaseType::EP_8S:
      case BaseType::EP_16S:
      case BaseType::EP_32S:
        gl.externalFormat = integer_format_for_channels(gl_channels);
        if (d.baseType == BaseType::EP_8S)
          gl.externalType = GL_BYTE;
        else if (d.baseType == BaseType::EP_16S)
          gl.externalType = GL_SHORT;
        else
          gl.externalType = GL_INT;
        gl.internalFormat = signed_integer_internal_format(bits, gl_channels);
        break;

      case BaseType::EP_32F:
        gl.externalFormat = normalized_format_for_channels(gl_channels);
        gl.externalType = GL_FLOAT;
        gl.internalFormat = float_internal_format(32, gl_channels);
        break;

      case BaseType::EP_64F:
        // GL_DOUBLE may be valid as an upload type, but there is no ordinary
        // GL_R64F/GL_RGBA64F sampled texture internal format in core OpenGL.
        gl.externalFormat = GL_NONE;
        gl.externalType = GL_NONE;
        gl.internalFormat = GL_NONE;
        break;

      default:
        break;
    }

    gl.directlyUploadable = gl.externalFormat != GL_NONE &&
                            gl.externalType != GL_NONE &&
                            gl.internalFormat != GL_NONE;
    return gl;
  }

  // Mosaic and other higher-dimensional layouts require interpretation before
  // an ordinary 2D texture upload.
  return gl;
}

GLenum to_gl_type(PixelFormat pf, unsigned int width, unsigned int height,
                  unsigned int channels)
{
  return to_gl_info(pf, width, height, channels).externalType;
}

GLenum to_gl_format(PixelFormat pf, unsigned int width, unsigned int height,
                    unsigned int channels)
{
  return to_gl_info(pf, width, height, channels).externalFormat;
}

GLenum to_gl_internal_format(PixelFormat pf, unsigned int width,
                             unsigned int height, unsigned int channels)
{
  return to_gl_info(pf, width, height, channels).internalFormat;
}

PixelFormat from_gl_format(GLenum externalFormat, GLenum externalType,
                           unsigned int channels, unsigned int bitDepth)
{
  // Accept legacy luminance as one-channel input.
  if (externalFormat == GL_LUMINANCE) {
    externalFormat = GL_RED;
    channels = 1;
  }

  auto effective_channels = channels;
  switch (externalFormat) {
    case GL_RED:
    case GL_RED_INTEGER:
      effective_channels = 1;
      break;
    case GL_RG:
    case GL_RG_INTEGER:
      effective_channels = 2;
      break;
    case GL_RGB:
    case GL_BGR:
    case GL_RGB_INTEGER:
      effective_channels = 3;
      break;
    case GL_RGBA:
    case GL_BGRA:
    case GL_RGBA_INTEGER:
      effective_channels = 4;
      break;
    default:
      throw std::invalid_argument("Unsupported OpenGL external format");
  }

  switch (externalType) {
    case GL_UNSIGNED_BYTE:
      if (effective_channels == 1) return PixelFormat::Mono8;
      if (effective_channels == 3)
        return externalFormat == GL_BGR ? PixelFormat::BGR8 : PixelFormat::RGB8;
      if (effective_channels == 4)
        return externalFormat == GL_BGRA ? PixelFormat::BGRA8
                                         : PixelFormat::RGBA8;
      return PixelFormat::BIP8;

    case GL_UNSIGNED_SHORT:
      if (effective_channels == 1) {
        if (bitDepth == 10) return PixelFormat::Mono10;
        if (bitDepth == 12) return PixelFormat::Mono12;
        return PixelFormat::Mono16;
      }

      if (effective_channels == 3) {
        if (externalFormat == GL_BGR) {
          if (bitDepth == 10) return PixelFormat::BGR10;
          if (bitDepth == 12) return PixelFormat::BGR12;
        }
        else {
          if (bitDepth == 10) return PixelFormat::RGB10;
          if (bitDepth == 12) return PixelFormat::RGB12;
        }
        return PixelFormat::BIP16;
      }

      if (effective_channels == 4) {
        if (externalFormat == GL_BGRA) {
          if (bitDepth == 10) return PixelFormat::BGRA10;
          if (bitDepth == 12) return PixelFormat::BGRA12;
        }
        else if (bitDepth == 12) {
          return PixelFormat::RGBA12;
        }
        return PixelFormat::BIP16;
      }

      if (bitDepth == 10) return PixelFormat::BIP10;
      if (bitDepth == 12) return PixelFormat::BIP12;
      return PixelFormat::BIP16;

    case GL_BYTE:
      return PixelFormat::BIP8s;

    case GL_SHORT:
      if (bitDepth == 10) return PixelFormat::BIP10s;
      if (bitDepth == 12) return PixelFormat::BIP12s;
      return PixelFormat::BIP16s;

    case GL_INT:
      return PixelFormat::BIP32s;

    case GL_FLOAT:
      return PixelFormat::BIP32f;

    case GL_DOUBLE:
      return PixelFormat::BIP64f;

    default:
      throw std::invalid_argument("Unsupported OpenGL external type");
  }
}

}  // namespace epf
