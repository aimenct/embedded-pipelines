// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "image_utils.h"

namespace ep {

PixelFormatInfo get_image_info(PixelFormat pf, int32_t w, int32_t h, int32_t c)
{
  PixelFormatInfo nfo{{}, -1.0f, 1, EP_8U, 0};

  switch (pf) {
    case Mono8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 1;
      nfo.numChannels = 1;
      nfo.baseType = EP_8U;
      break;

    case Mono10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

    case Mono12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

    case Mono10p:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = static_cast<float>(nfo.depth[0]) / 8.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

      // case Mono10g40:
      //   nfo.depth.push_back(10);
      //   nfo.bytesPerPixel = (nfo.depth / 8.0f);
      //   nfo.numChannels = 1;
      //   nfo.baseType = EP_8U;
      //   break;

      // case Mono12g24:
      //   nfo.depth.push_back(10);
      //   nfo.bytesPerPixel = 5.0f;
      //   nfo.numChannels = 1;
      //   nfo.baseType = EP_8U;
      //   break;

    case Mono16:
      nfo.depth.push_back(16);
      nfo.bytesPerPixel = 2;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

      // raw bayer
    case BayerRG8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 1;
      nfo.numChannels = 1;
      nfo.baseType = EP_8U;
      break;

    case BayerRG10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 1;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

    case BayerRG10p:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = static_cast<float>(nfo.depth[0]) / 8.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

    case BayerRG12p:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = static_cast<float>(nfo.depth[0]) / 8.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

      // case BayerRG10g40:
      //   nfo.depth.push_back(10);
      //   nfo.bytesPerPixel = 5.0f;
      //   nfo.numChannels = 1;
      //   break;

    case BayerGR8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 1.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_8U;
      break;

    case BayerGR10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

    case BayerGR12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2.0f;
      nfo.numChannels = 1;
      nfo.baseType = EP_16U;
      break;

      // color
    case RGB8:
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_8U;
      break;

    case RGB10:
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

    case RGB12:
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

      // case RGB10p32:
      //   nfo.depth.push_back(10);
      //   nfo.bytesPerPixel = 5.0f;
      //   nfo.numChannels = 1;
      //   break;

    case BGR8:
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_8U;
      break;

    case BGR10:
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

    case BGR12:
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * 3;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

      // case BGR10p32:
      //   nfo.depth.push_back(10);
      //   nfo.depth.push_back(10);
      //   nfo.depth.push_back(10);
      //   nfo.bytesPerPixel = 1.0f;
      //         nfo.numChannels = static_cast<int32_t>( nfo.depth.size()
      //         );
      //   break;

    case RGBa8:
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 4;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_8U;
      break;

    case RGBa12:
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * 4;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

    case BGRa8:
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 4;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_8U;
      break;

    case BGRa10:
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 4;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

    case BGRa12:
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * 4;
      nfo.numChannels = static_cast<int32_t>(nfo.depth.size());
      nfo.baseType = EP_16U;
      break;

    case Mosaic3u8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 9.0f;
      nfo.numChannels = 9;
      nfo.baseType = EP_8U;
      break;

    case Mosaic4u8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 16.0f;
      nfo.numChannels = 16;
      nfo.baseType = EP_8U;
      break;

    case Mosaic5u8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = 25.0f;
      nfo.numChannels = 25;
      nfo.baseType = EP_8U;
      break;

    case Mosaic3u10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 25;
      nfo.numChannels = 9;
      nfo.baseType = EP_16U;
      break;

    case Mosaic4u10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 16;
      nfo.numChannels = 16;
      nfo.baseType = EP_16U;
      break;

    case Mosaic5u10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * 25;
      nfo.numChannels = 25;
      nfo.baseType = EP_16U;
      break;

    case Mosaic3u12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * 9;
      nfo.numChannels = 9;
      nfo.baseType = EP_16U;
      break;

    case Mosaic4u12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 16;
      nfo.numChannels = 16;
      nfo.baseType = EP_16U;
      break;

    case Mosaic5u12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 25;
      nfo.numChannels = 25;
      nfo.baseType = EP_16U;
      break;

    case BIP8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_8U;
      break;

    case BIP10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BIP12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = 2 * c;
      nfo.baseType = EP_16U;
      break;

    case BIP16:
      nfo.depth.push_back(16);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BIP8s:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_8S;
      break;

    case BIP16s:
      nfo.depth.push_back(16);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16S;
      break;

    case BIP32s:
      nfo.depth.push_back(32);
      nfo.bytesPerPixel = 4 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_32S;
      break;

    case BIP32f:
      nfo.depth.push_back(32);
      nfo.bytesPerPixel = 4 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_32F;
      break;

    case BIP64f:
      nfo.depth.push_back(64);
      nfo.bytesPerPixel = 8 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_64F;
      break;

    case BIL8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_8U;
      break;

    case BIL10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BIL12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BIL16:
      nfo.depth.push_back(16);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BSQ8:
      nfo.depth.push_back(8);
      nfo.bytesPerPixel = static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_8U;
      break;

    case BSQ10:
      nfo.depth.push_back(10);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BSQ12:
      nfo.depth.push_back(12);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    case BSQ16:
      nfo.depth.push_back(16);
      nfo.bytesPerPixel = 2 * static_cast<float>(c);
      nfo.numChannels = c;
      nfo.baseType = EP_16U;
      break;

    default:
      break;
  };

  if (nfo.bytesPerPixel == -1) {
    std::cout << "Image Utils Error - PixelFormat not supported " << std::endl;
    exit(0);
  }

  //  nfo.size = w * h * nfo.bytesPerPixel + 0.5f;
  nfo.size = static_cast<int64_t>(
      std::ceil(static_cast<double>(w) * static_cast<double>(h) *
                static_cast<double>(nfo.bytesPerPixel)));

  if (nfo.numChannels != c) {
    std::cout << "Image Utils Warning - Pixel Format Num Channels Corrected "
              << std::endl;
  }

  return nfo;
}

std::string pixelformat_to_string(PixelFormat pf)
{
  switch (pf) {
    case Mono8:
      return "Mono8";
    case Mono10:
      return "Mono10";
    case Mono12:
      return "Mono12";
    case Mono10p:
      return "Mono10p";
    case Mono10g40:
      return "Mono10g40";
    case Mono12g24:
      return "Mono12g24";
    case Mono16:
      return "Mono16";
    case BayerRG8:
      return "BayerRG8";
    case BayerRG10:
      return "BayerRG10";
    case BayerRG10p:
      return "BayerRG10p";
    case BayerRG12p:
      return "BayerRG12p";
    case BayerRG10g40:
      return "BayerRG10g40";
    case BayerGR8:
      return "BayerGR8";
    case BayerGR10:
      return "BayerGR10";
    case BayerGR12:
      return "BayerGR12";
    case RGB8:
      return "RGB8";
    case RGB10:
      return "RGB10";
    case RGB12:
      return "RGB12";
    case RGB10p32:
      return "RGB10p32";
    case BGR8:
      return "BGR8";
    case BGR10:
      return "BGR10";
    case BGR12:
      return "BGR12";
    case BGR10p32:
      return "BGR10p32";
    case RGBa8:
      return "RGBa8";
    case RGBa12:
      return "RGBa12";
    case BGRa8:
      return "BGRa8";
    case BGRa10:
      return "BGRa10";
    case BGRa12:
      return "BGRa12";
    case Mosaic3u8:
      return "Mosaic3u8";
    case Mosaic4u8:
      return "Mosaic4u8";
    case Mosaic5u8:
      return "Mosaic5u8";
    case Mosaic3u10:
      return "Mosaic3u10";
    case Mosaic4u10:
      return "Mosaic4u10";
    case Mosaic5u10:
      return "Mosaic5u10";
    case Mosaic3u12:
      return "Mosaic3u12";
    case Mosaic4u12:
      return "Mosaic4u12";
    case Mosaic5u12:
      return "Mosaic5u12";
    case BIP8:
      return "BIP8";
    case BIP10:
      return "BIP10";
    case BIP12:
      return "BIP12";
    case BIP16:
      return "BIP16";
    case BIP8s:
      return "BIP8s";
    case BIP10s:
      return "BIP10s";
    case BIP12s:
      return "BIP12s";
    case BIP16s:
      return "BIP16s";
    case BIP32s:
      return "BIP32s";
    case BIP32f:
      return "BIP32f";
    case BIP64f:
      return "BIP64f";
    case BIL8:
      return "BIL8";
    case BIL10:
      return "BIL10";
    case BIL12:
      return "BIL12";
    case BIL16:
      return "BIL16";
    case BSQ8:
      return "BSQ8";
    case BSQ10:
      return "BSQ10";
    case BSQ12:
      return "BSQ12";
    case BSQ16:
      return "BSQ16";
      // Add other cases as needed
  }
  return "Unknown Pixel Format";
}

// Map string representations to PixelFormat enum values
PixelFormat string_to_pixelformat(const std::string& format_name)
{
  static const std::unordered_map<std::string, PixelFormat> format_map = {
      {"Mono8", Mono8},
      {"Mono10", Mono10},
      {"Mono12", Mono12},
      {"Mono10p", Mono10p},
      {"Mono10g40", Mono10g40},
      {"Mono12g24", Mono12g24},
      {"Mono16", Mono16},
      {"BayerRG8", BayerRG8},
      {"BayerRG10", BayerRG10},
      {"BayerRG10p", BayerRG10p},
      {"BayerRG12p", BayerRG12p},
      {"BayerRG10g40", BayerRG10g40},
      {"BayerGR8", BayerGR8},
      {"BayerGR10", BayerGR10},
      {"BayerGR12", BayerGR12},
      {"RGB8", RGB8},
      {"RGB10", RGB10},
      {"RGB12", RGB12},
      {"RGB10p32", RGB10p32},
      {"BGR8", BGR8},
      {"BGR10", BGR10},
      {"BGR12", BGR12},
      {"BGR10p32", BGR10p32},
      {"RGBa8", RGBa8},
      {"RGBa12", RGBa12},
      {"BGRa8", BGRa8},
      {"BGRa10", BGRa10},
      {"BGRa12", BGRa12},
      {"Mosaic3u8", Mosaic3u8},
      {"Mosaic4u8", Mosaic4u8},
      {"Mosaic5u8", Mosaic5u8},
      {"Mosaic3u10", Mosaic3u10},
      {"Mosaic4u10", Mosaic4u10},
      {"Mosaic5u10", Mosaic5u10},
      {"Mosaic3u12", Mosaic3u12},
      {"Mosaic4u12", Mosaic4u12},
      {"Mosaic5u12", Mosaic5u12},
      {"BIP8", BIP8},
      {"BIP10", BIP10},
      {"BIP12", BIP12},
      {"BIP16", BIP16},
      {"BIP8s", BIP8s},
      {"BIP10s", BIP10s},
      {"BIP12s", BIP12s},
      {"BIP16s", BIP16s},
      {"BIP32s", BIP32s},
      {"BIP32f", BIP32f},
      {"BIP64f", BIP64f},
      {"BIL8", BIL8},
      {"BIL10", BIL10},
      {"BIL12", BIL12},
      {"BIL16", BIL16},
      {"BSQ8", BSQ8},
      {"BSQ10", BSQ10},
      {"BSQ12", BSQ12},
      {"BSQ16", BSQ16},
      // Add other mappings as needed
  };

  auto it = format_map.find(format_name);
  if (it != format_map.end()) {
    return it->second;
  }
  else {
    throw std::invalid_argument("Invalid PixelFormat name: " + format_name);
  }
}

}  // namespace ep
