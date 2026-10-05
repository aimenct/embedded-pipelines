// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef IMAGE_FORMATS_H
#define IMAGE_FORMATS_H

#include <GL/freeglut.h>

#include <cstdint>
#include <string>
#include <vector>

#include "epf_types.h"

namespace epf {

// Uncompressed raw pixel formats
enum class PixelFormat {
  Mono8,
  Mono10,
  Mono12,
  Mono10p,
  Mono10g40,
  Mono12g24,
  Mono16,
  BayerRG8,
  BayerRG10,
  BayerRG12,
  BayerRG10p,
  BayerRG12p,
  BayerRG10g40,
  BayerGR8,
  BayerGR10,
  BayerGR12,
  RGB8,
  RGB10,
  RGB12,
  RGB10p32,
  BGR8,
  BGR10,
  BGR12,
  BGR10p32,
  RGBA8,
  RGBA12,
  BGRA8,
  BGRA10,
  BGRA12,
  YUV420p,
  YUV422p,
  YUV444p,
  NV12,
  YUYV422,
  YUV422_8_UYVY,
  Mosaic3u8,
  Mosaic4u8,
  Mosaic5u8,
  Mosaic3u10,
  Mosaic4u10,
  Mosaic5u10,
  Mosaic3u12,
  Mosaic4u12,
  Mosaic5u12,
  BIP8,
  BIP10,
  BIP12,
  BIP16,
  BIP8s,
  BIP10s,
  BIP12s,
  BIP16s,
  BIP32s,
  BIP32f,
  BIP64f,
  BIL8,
  BIL10,
  BIL12,
  BIL16,
  BSQ8,
  BSQ10,
  BSQ12,
  BSQ16
};

// Compressed image/video encodings
enum class ImageEncoding {
  JPEG,
  PNG,
  TIF,
  WEBP,
  MJPEG,
  H264,
  HEVC,
  VP8,
  VP9,
  AV1,
  Bitstream
};

// Metadata for a pixel format
struct PixelFormatInfo {
    std::vector<int32_t> depth;  // Bits per channel
    float bytesPerPixel =
        0.0f;  // Total stored bytes per pixel, across channels/planes
    int32_t numChannels = 0;  // 0 for dynamic if caller passes 0
    BaseType baseType = BaseType::EP_8U;
    int64_t size = 0;  // ceil(width*height*bytesPerPixel)
};

// Metadata for OpenGL upload.
// externalFormat/externalType/internalFormat are intended for glTexImage2D.
// directlyUploadable=false means the EPF memory layout is valid but needs an
// unpack/color-conversion/plane-handling step before ordinary 2D texture
// upload.
struct GLPixelFormatInfo {
    GLenum externalFormat =
        GL_NONE;                    // GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_BGR...
    GLenum externalType = GL_NONE;  // GL_UNSIGNED_BYTE, GL_FLOAT...
    GLenum internalFormat = GL_NONE;  // GL_R8, GL_R32F, GL_RGBA16...
    bool directlyUploadable = false;
    bool requiresUnpack = false;
    bool requiresColorConversion = false;
    bool requiresPlanarUpload = false;
};

// Format introspection
PixelFormatInfo get_image_info(PixelFormat pf, int32_t width, int32_t height,
                               int32_t channels);

// String conversions
std::string to_string(PixelFormat pf);
std::string to_string(ImageEncoding enc);
PixelFormat from_string_pixel_format(const std::string &name);
ImageEncoding from_string_image_encoding(const std::string &name);

// OpenGL conversions.
//
// Prefer to_gl_info() for new code because it returns the external format,
// external type, internal format, and whether the EPF layout is directly
// uploadable as a normal OpenGL 2D texture.
GLPixelFormatInfo to_gl_info(PixelFormat pf, unsigned int width,
                             unsigned int height, unsigned int channels);

// Legacy helpers retained for existing callers. These return GL_NONE when the
// format is not directly uploadable without unpacking/conversion.
GLenum to_gl_type(PixelFormat pf, unsigned int width, unsigned int height,
                  unsigned int channels);
GLenum to_gl_format(PixelFormat pf, unsigned int width, unsigned int height,
                    unsigned int channels);
GLenum to_gl_internal_format(PixelFormat pf, unsigned int width,
                             unsigned int height, unsigned int channels);

// Best-effort reverse mapping for simple, unpacked OpenGL layouts.
//
// This cannot recover packed formats, Bayer pattern identity, planar layouts,
// YUV layouts, or whether GL_RGB + GL_UNSIGNED_SHORT was originally RGB10,
// RGB12, BIP10, BIP12, or BIP16 unless bitDepth is provided. Use bitDepth=10
// or 12 when that distinction is available; otherwise 16-bit unsigned dynamic
// formats are returned as BIP16 for ambiguous cases.
PixelFormat from_gl_format(GLenum externalFormat, GLenum externalType,
                           unsigned int channels, unsigned int bitDepth = 0);

}  // namespace epf

#endif  // IMAGE_FORMATS_H
