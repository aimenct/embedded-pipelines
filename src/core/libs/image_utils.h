// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <cmath>
#include <iostream>
#include <vector>

#include "ep_types.h"

namespace ep {

enum PixelFormat {
  // mono
  Mono8,
  Mono10,
  Mono12,
  Mono10p,
  Mono10g40,
  Mono12g24,
  Mono16,
  // raw bayer
  BayerRG8,
  BayerRG10,
  BayerRG10p,
  BayerRG12p,
  BayerRG10g40,
  BayerGR8,
  BayerGR10,
  BayerGR12,
  // color
  RGB8,
  RGB10,
  RGB12,
  RGB10p32,
  BGR8,
  BGR10,
  BGR12,
  BGR10p32,
  RGBa8,
  RGBa12,
  BGRa8,
  BGRa10,
  BGRa12,
  // multispectral / hyperspectral
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

struct PixelFormatInfo {
    std::vector<int32_t> depth;
    float bytesPerPixel;
    int32_t numChannels;
    BaseType baseType;
    int64_t size;
};

PixelFormatInfo get_image_info(PixelFormat pf, int32_t w, int32_t h, int32_t c);
std::string pixelformat_to_string(PixelFormat pf);
PixelFormat string_to_pixelformat(const std::string& format_name);

// /* OPENGL CONVERSION */

// /* GSTREAMER CONVERSION */
// #ifdef EP_GSTREAMER
// int to_gstreamer(PixelFormat pixelFormat, unsigned int channels = 0);
// #endif

}  // namespace ep

#endif  // IMAGE_UTILS_H
