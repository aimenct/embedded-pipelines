// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef IMAGE_OBJECT_H
#define IMAGE_OBJECT_H

#include "image_utils.h"
#include "node.h"

namespace ep {

// Images in EP are represented as objects (NodeTrees):
// Name: Object Node
// - Width: DataNode ( int32_t )
// - Height: DataNode ( int32_t )
// - Channels: DataNode ( int32_t )
// - PixelFormat: DataNode ( int32_t / enum  )
// - data: DataNode ( unsigned char array )

class ImageObject {
  private:
    ep::ObjectNode *root_node_;  // pointer
    char node_tree_mem_mgmt_;

    int32_t *width_;
    int32_t *height_;
    int32_t *channels_;
    PixelFormat *pixel_format_;
    DataNode *data_node_;
    size_t size_;

  public:
    ImageObject(){};
    ImageObject(ep::Node2 *node);
    ImageObject(std::string name, int32_t width, int32_t height,
                int32_t channels, PixelFormat pixel_format, uint8_t *data);
    ~ImageObject();

    /* Generate node tree */
    ObjectNode *transferNodeTree();

    ObjectNode *copyNodeTree();

    /* Get width */
    int32_t width() const;

    /* Get heigth */
    int32_t height() const;

    /* Get heigth */
    int32_t channels() const;

    /* Get pixelFormat */
    PixelFormat pixelFormat() const;

    /* Get data */
    void *data() const;

    /* bytesPerPixel */
    float bytesPerPixel() const;

    /* size */
    size_t size() const;
};

}  // namespace ep

#endif  // IMAGE_OBJECT_H
