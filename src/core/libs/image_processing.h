// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <math.h>

#include <vector>

#include "epf_types.h"
#include "image_object.h"

namespace epf {

int32_t compute_histogram256(const ImageObject &image,
                             std::vector<float> &histogram_f,
                             float max_f = 1.0);

int32_t compute_histogram256(const ImageObject &image,
                             std::vector<float> &histogram_f, double *hist_min,
                             double *hist_max, float max_f = 0.0f);

int32_t normalize_image(const ImageObject &input_image,
                        ImageObject &output_image, float min_value,
                        float max_value);

int32_t convert_uyvy422_to_rgb8(const ImageObject &input_image,
                                ImageObject &output_image);

void convertToFloat(float *data_out, char *data_in, BaseType datatype, size_t L,
                    size_t offset);

void convertToDouble(double *data_out, char *data_in, BaseType datatype,
                     size_t L, size_t offset);

extern std::vector<std::vector<float>> jetColormap;

void initializeColormap();
std::vector<std::vector<float>> createJetColormapLUT(int numColors = 256);
std::vector<float> getColorFromLUT(
    float normalizedValue, const std::vector<std::vector<float>> &colormap);

}  // namespace epf

#endif  // IMAGE_PROCESSING_H
