// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "image_processing.h"

namespace epf {

std::vector<std::vector<float>> jetColormap;

int32_t compute_histogram256(const ImageObject &image,
                             std::vector<float> &histogram_f, float max_f)
{
  return compute_histogram256(image, histogram_f, nullptr, nullptr, max_f);
}

int32_t compute_histogram256(const ImageObject &image,
                             std::vector<float> &histogram_f, double *hist_min,
                             double *hist_max, float max_f)
{
  /* Compute normalized histogram across all channels.
   *
   * If hist_min/hist_max are provided, they receive the value domain used
   * for binning. This is especially important for EP_32F, where the range
   * may be inferred from finite image values.
   */

  auto set_hist_range = [&](double min_v, double max_v) {
    if (hist_min) *hist_min = min_v;
    if (hist_max) *hist_max = max_v;
  };

  // Validate pixel format and dimensions.
  size_t bytes_per_pixel = static_cast<size_t>(image.bytesPerPixel());
  if (bytes_per_pixel == 0 || !image.data()) {
    std::cerr << "compute_histogram failure - invalid image data " << std::endl;
    set_hist_range(0.0, 0.0);
    return -1;
  }

  if (histogram_f.size() != 256) {
    std::cerr << "invalid histogram size " << std::endl;
    set_hist_range(0.0, 0.0);
    return -1;
  }

  uint32_t histogram[256];
  memset(histogram, 0, sizeof(histogram));

  const int total_pixels =
      static_cast<int>(image.width() * image.height() * image.channels());

  if (total_pixels <= 0) {
    std::fill(histogram_f.begin(), histogram_f.end(), 0.0f);
    set_hist_range(0.0, 0.0);
    return -1;
  }

  double base_min = 0.0;
  double base_max = 0.0;
  basetype_range(image.baseType(), base_min, base_max);

  switch (image.baseType()) {
    case EP_8U: {
      set_hist_range(0.0, 255.0);

      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(image.data());

      int i = 0;
      for (; i <= total_pixels - 4; i += 4) {
        histogram[ptr[i]]++;
        histogram[ptr[i + 1]]++;
        histogram[ptr[i + 2]]++;
        histogram[ptr[i + 3]]++;
      }

      for (; i < total_pixels; i++) {
        histogram[ptr[i]]++;
      }

      break;
    }

    case EP_16U: {
      set_hist_range(0.0, 65535.0);

      const uint16_t *ptr = reinterpret_cast<const uint16_t *>(image.data());

      for (int i = 0; i < total_pixels; i++) {
        histogram[ptr[i] >> 8]++;
      }

      break;
    }

    case EP_32F: {
      const float *ptr = reinterpret_cast<const float *>(image.data());

      float min_v = 0.0f;
      float max_v = max_f;

      // If max_f is explicitly provided, use range [0, max_f].
      // Otherwise infer from finite values.
      if (!(max_f > 0.0f)) {
        min_v = std::numeric_limits<float>::infinity();
        max_v = -std::numeric_limits<float>::infinity();

        for (int i = 0; i < total_pixels; ++i) {
          const float v = ptr[i];
          if (!std::isfinite(v)) continue;

          min_v = std::min(min_v, v);
          max_v = std::max(max_v, v);
        }

        if (!std::isfinite(min_v) || !std::isfinite(max_v)) {
          min_v = 0.0f;
          max_v = 1.0f;
        }
        else if (!(max_v > min_v)) {
          // Constant finite image. Use a tiny range around the value so the
          // histogram domain remains meaningful for marker->value conversion.
          const float center = min_v;
          const float eps = std::max(std::abs(center) * 1e-6f, 1e-6f);
          min_v = center - eps;
          max_v = center + eps;
        }
      }

      set_hist_range(static_cast<double>(min_v), static_cast<double>(max_v));

      const float denom = std::max(max_v - min_v, 1e-12f);
      const float scale = 256.0f / denom;

      for (int i = 0; i < total_pixels; ++i) {
        const float v = ptr[i];
        if (!std::isfinite(v)) continue;

        int idx = static_cast<int>((v - min_v) * scale);
        idx = std::clamp(idx, 0, 255);
        histogram[idx]++;
      }

      break;
    }

    default:
      std::cerr << "compute_histogram failure - unsupported pixel format "
                << std::endl;
      set_hist_range(base_min, base_max);
      return -2;
  }

  // Normalize for visualization.
  const uint32_t max_value = *std::max_element(histogram, histogram + 256);

  if (max_value == 0) {
    std::fill(histogram_f.begin(), histogram_f.end(), 0.0f);
    return 0;
  }

  for (int i = 0; i < 256; i++) {
    histogram_f[i] =
        static_cast<float>(histogram[i]) / static_cast<float>(max_value);
  }

  return 0;
}

int32_t normalize_image(const ImageObject &input_image,
                        ImageObject &output_image, float min_value,
                        float max_value)
{
  // Validate input image
  if (!input_image.data() || input_image.bytesPerPixel() == 0) {
    return -1;  // Invalid input image
  }

  // Validate output image dimensions and format
  if (output_image.width() != input_image.width() ||
      output_image.height() != input_image.height() ||
      output_image.channels() != input_image.channels() ||
      output_image.pixelFormat() != input_image.pixelFormat()) {
    return -2;  // Mismatch in output image properties
  }

  // Access input and output data
  const void *input_data = input_image.data();
  void *output_data = output_image.data();
  size_t total_pixels =
      input_image.width() * input_image.height() * input_image.channels();
  BaseType base_type = input_image.baseType();

  // Calculate scaling factor and offset for normalization
  float range = max_value - min_value;
  if (range <= 0.0f) {
    return -3;  // Invalid range
  }

  switch (base_type) {
    case EP_8U: {
      const uint8_t *in = static_cast<const uint8_t *>(input_data);
      uint8_t *out = static_cast<uint8_t *>(output_data);
      for (size_t i = 0; i < total_pixels; ++i) {
        float normalized = (in[i] - min_value) / range;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        out[i] =
            static_cast<uint8_t>(normalized * 255.0f);  // Scale to 8-bit range
      }
      break;
    }
    case EP_16U: {
      const uint16_t *in = static_cast<const uint16_t *>(input_data);
      uint16_t *out = static_cast<uint16_t *>(output_data);
      for (size_t i = 0; i < total_pixels; ++i) {
        float normalized = (in[i] - min_value) / range;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        out[i] = static_cast<uint16_t>(normalized *
                                       65535.0f);  // Scale to 16-bit range
      }
      break;
    }
    case EP_32F: {
      const float *in = static_cast<const float *>(input_data);
      float *out = static_cast<float *>(output_data);
      for (size_t i = 0; i < total_pixels; ++i) {
        float normalized = (in[i] - min_value) / range;
        out[i] = std::clamp(normalized, 0.0f, 1.0f);
      }
      break;
    }
    default:
      return -4;  // Unsupported pixel format
  }

  return 0;  // Success
}

int32_t convert_uyvy422_to_rgb8(const ImageObject &input_image,
                                ImageObject &output_image)
{
  if (input_image.pixelFormat() != PixelFormat::YUV422_8_UYVY) {
    return -1;
  }

  if (output_image.pixelFormat() != PixelFormat::RGB8 ||
      output_image.channels() != 3 ||
      output_image.width() != input_image.width() ||
      output_image.height() != input_image.height()) {
    return -2;
  }

  const int32_t width = input_image.width();
  const int32_t height = input_image.height();
  if ((width % 2) != 0) {
    return -3;
  }

  const uint8_t *src = static_cast<const uint8_t *>(input_image.data());
  uint8_t *dst = static_cast<uint8_t *>(output_image.data());
  if (!src || !dst) {
    return -4;
  }

  auto clamp_u8 = [](int32_t v) -> uint8_t {
    return static_cast<uint8_t>(std::max(0, std::min(255, v)));
  };

  const int32_t pixel_count = width * height;
  const int32_t pair_count = pixel_count / 2;

  for (int32_t pair = 0; pair < pair_count; ++pair) {
    const int32_t i = pair * 4;
    const int32_t u = static_cast<int32_t>(src[i + 0]) - 128;
    const int32_t y0 = static_cast<int32_t>(src[i + 1]);
    const int32_t v = static_cast<int32_t>(src[i + 2]) - 128;
    const int32_t y1 = static_cast<int32_t>(src[i + 3]);

    const int32_t c0 = y0 - 16;
    const int32_t c1 = y1 - 16;

    const int32_t r0 = (298 * c0 + 409 * v + 128) >> 8;
    const int32_t g0 = (298 * c0 - 100 * u - 208 * v + 128) >> 8;
    const int32_t b0 = (298 * c0 + 516 * u + 128) >> 8;

    const int32_t r1 = (298 * c1 + 409 * v + 128) >> 8;
    const int32_t g1 = (298 * c1 - 100 * u - 208 * v + 128) >> 8;
    const int32_t b1 = (298 * c1 + 516 * u + 128) >> 8;

    const int32_t o = pair * 6;
    dst[o + 0] = clamp_u8(r0);
    dst[o + 1] = clamp_u8(g0);
    dst[o + 2] = clamp_u8(b0);
    dst[o + 3] = clamp_u8(r1);
    dst[o + 4] = clamp_u8(g1);
    dst[o + 5] = clamp_u8(b1);
  }

  return 0;
}

void convertToFloat(float *data_out, char *data_in, BaseType datatype, size_t L,
                    size_t offset)
{
  // Cast the data from the DataNode and convert accordingly
  switch (datatype) {
    case EP_8U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<uint8_t *>(&data_in[ii]));
      break;
    }
    case EP_8S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<int8_t *>(&data_in[ii]));
      break;
    }
    case EP_16U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<uint16_t *>(&data_in[ii]));
      break;
    }
    case EP_16S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset) {
        data_out[i] =
            static_cast<float>(*reinterpret_cast<int16_t *>(&data_in[ii]));
      }
      break;
    }
    case EP_32U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<uint32_t *>(&data_in[ii]));
      break;
    }
    case EP_32S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<int32_t *>(&data_in[ii]));
      break;
    }
    case EP_64S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<int64_t *>(&data_in[ii]));
      break;
    }
    case EP_64U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<uint64_t *>(&data_in[ii]));
      break;
    }
    case EP_32F: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<float *>(&data_in[ii]));
      break;
    }
    case EP_64F: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<float>(*reinterpret_cast<double *>(&data_in[ii]));
      break;
    }
    default:
      //      std::cerr << "Unsupported data type encountered: " <<
      //      node->datatype()
      std::cerr << "Unsupported data type encountered: " << datatype
                << std::endl;
      break;
  }
}

void convertToDouble(double *data_out, char *data_in, BaseType datatype,
                     size_t L, size_t offset)
{
  // Cast the data from the DataNode and convert accordingly
  switch (datatype) {
    case EP_8U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<uint8_t *>(&data_in[ii]));
      break;
    }
    case EP_8S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<int8_t *>(&data_in[ii]));
      break;
    }
    case EP_16U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<uint16_t *>(&data_in[ii]));
      break;
    }
    case EP_16S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset) {
        data_out[i] =
            static_cast<double>(*reinterpret_cast<int16_t *>(&data_in[ii]));
      }
      break;
    }
    case EP_32U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<uint32_t *>(&data_in[ii]));
      break;
    }
    case EP_32S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<int32_t *>(&data_in[ii]));
      break;
    }
    case EP_64S: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<int64_t *>(&data_in[ii]));
      break;
    }
    case EP_64U: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<uint64_t *>(&data_in[ii]));
      break;
    }
    case EP_32F: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<float *>(&data_in[ii]));
      break;
    }
    case EP_64F: {
      for (size_t i = 0, ii = 0; i < L; i++, ii += offset)
        data_out[i] =
            static_cast<double>(*reinterpret_cast<double *>(&data_in[ii]));
      break;
    }
    default:
      //      std::cerr << "Unsupported data type encountered: " <<
      //      node->datatype()
      std::cerr << "Unsupported data type encountered: " << datatype
                << std::endl;
      break;
  }
}

std::vector<std::vector<float>> createJetColormapLUT(int num_colors)
{
  std::vector<std::vector<float>> colormap(
      static_cast<size_t>(num_colors), std::vector<float>(3));  // RGB colors

  for (int i = 0; i < num_colors; ++i) {
    float t = static_cast<float>(i) /
              static_cast<float>(num_colors - 1);  // Normalize to [0, 1]

    // Jet colormap formula
    if (t < 0.125f) {
      colormap[static_cast<size_t>(i)][0] = 0.0f;
      colormap[static_cast<size_t>(i)][1] = 0.0f;
      colormap[static_cast<size_t>(i)][2] = 0.5f + 0.5f * (t / 0.125f);
    }
    else if (t < 0.375f) {
      colormap[static_cast<size_t>(i)][0] = 0.0f;
      colormap[static_cast<size_t>(i)][1] = (t - 0.125f) / 0.25f;
      colormap[static_cast<size_t>(i)][2] = 1.0f;
    }
    else if (t < 0.625f) {
      colormap[static_cast<size_t>(i)][0] = (t - 0.375f) / 0.25f;
      colormap[static_cast<size_t>(i)][1] = 1.0f;
      colormap[static_cast<size_t>(i)][2] = 1.0f - (t - 0.375f) / 0.25f;
    }
    else if (t < 0.875f) {
      colormap[static_cast<size_t>(i)][0] = 1.0f;
      colormap[static_cast<size_t>(i)][1] = 1.0f - (t - 0.625f) / 0.25f;
      colormap[static_cast<size_t>(i)][2] = 0.0f;
    }
    else {
      colormap[static_cast<size_t>(i)][0] = 1.0f - 0.5f * (t - 0.875f) / 0.125f;
      colormap[static_cast<size_t>(i)][1] = 0.0f;
      colormap[static_cast<size_t>(i)][2] = 0.0f;
    }
  }

  return colormap;
}

void initializeColormap()
{
  jetColormap.clear();
  jetColormap = createJetColormapLUT(256);
}

std::vector<float> getColorFromLUT(
    float normalized_value, const std::vector<std::vector<float>> &colormap)
{
  if (!std::isfinite(normalized_value)) {
    normalized_value = 0.0f;
  }

  normalized_value = std::clamp(normalized_value, 0.0f, 1.0f);

  if (colormap.empty()) {
    return {normalized_value, normalized_value, normalized_value};
  }

  const int num_colors = static_cast<int>(colormap.size());

  int index =
      static_cast<int>(normalized_value * static_cast<float>(num_colors - 1));

  index = std::clamp(index, 0, num_colors - 1);

  if (colormap[index].size() < 3) {
    return {normalized_value, normalized_value, normalized_value};
  }

  return colormap[index];
}

}  // namespace epf
