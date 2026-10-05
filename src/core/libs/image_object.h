// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef IMAGE_OBJECT_H
#define IMAGE_OBJECT_H

#include "image_formats.h"
#include "node.h"

namespace epf {

// Image object as node tree:
// ObjectNode “Image”
//  ├─ DataNode “width”         (int32_t)
//  ├─ DataNode “height”        (int32_t)
//  ├─ DataNode “channels”      (int32_t)
//  ├─ DataNode “pixelFormat”   (int32_t ← PixelFormat enum)
//  └─ DataNode “data”          (unsigned char [] ←
//  width*height*channels*bytesPerPixel)

// ObjectNode “Image”
//  ├─ DataNode “width”         (int32_t)
//  ├─ DataNode “height”        (int32_t)
//  ├─ DataNode “channels”      (int32_t)
//  ├─ DataNode “imageEncoding” (int32_t ← ImageEncoding enum)
//  ├─ DataNode “size”          (int64_t ← encoded size)
//  └─ DataNode “data”          (unsigned char[])

// Examples:

// ObjectNode “Image”:
//   width:         1280            # DataNode<int32_t>
//   height:        720             # DataNode<int32_t>
//   channels:      1               # DataNode<int32_t>
//   pixelFormat:   Mono10          # DataNode<int32_t> (PixelFormat::Mono10)
//   imageEncoding: Bitstream       # DataNode<int32_t>
//   (ImageEncoding::Bitstream) data:           <byte[?]>      #
//   DataNode<unsigned char[]> # size should be: 1280*720*2 bytes (Mono10 packed
//   to 16‐bit words)

// ObjectNode “Image”:
//  width:         640
//  height:        480
//  channels:      3
//  pixelFormat:   RGB8            # the decoded pixel format, even though the
//  file is compressed imageEncoding: JPEG            # DataNode<int32_t>
//  (ImageEncoding::JPEG) data:           <jpeg_bytes>   # the full JPEG file
//  payload

// ObjectNode “Image”:
//  width:         1920
//  height:        1080
//  channels:      3
//  pixelFormat:   YUV420p        # chroma‐subsampled 4:2:0 planar
//  imageEncoding: H264           # DataNode<int32_t> (ImageEncoding::H264)
//  data:           <h264_nal>     # one or more NALUs (bitstream)

class ImageObject {
  private:
    // --- Node ownership ---
    std::unique_ptr<ObjectNode> owned_root_;  // if we own it
    ObjectNode *raw_root_{};  // if we reference an external node
    bool owns_node_{false};

    // --- Pointers into the tree ---
    DataNode *width_node_{};         // int32
    DataNode *height_node_{};        // int32
    DataNode *channels_node_{};      // int32
    DataNode *pixel_format_node_{};  // int32 ← PixelFormat
    DataNode *data_node_{};          // byte[]

    // --- Cached values ---
    // Avoid cached copies of node data. All values are read directly from the
    // underlying DataNodes when required.

    void update_variables();  // initialize node pointers and validate sizes

  public:
    // --- ctors/dtors ---
    ImageObject();                                    // makes an empty tree
    explicit ImageObject(ObjectNode *external_root);  // todo Node
    ImageObject(const std::string &name, int32_t width, int32_t height,
                int32_t channels, PixelFormat pf);
    ImageObject(const std::string &name, int32_t width, int32_t height,
                int32_t channels, PixelFormat pf, uint8_t *prealloc_data);

    ImageObject(const ImageObject &other);
    ImageObject &operator=(const ImageObject &other);

    ImageObject(ImageObject &&) noexcept;
    ImageObject &operator=(ImageObject &&) noexcept;

    ~ImageObject();

    // --- Tree export ---
    std::unique_ptr<ObjectNode> moveNode();
    std::unique_ptr<ObjectNode> copyNode() const;

    // --- Accessors ---
    int32_t width() const
    {
      return *static_cast<int32_t *>(width_node_->value());
    }
    int32_t height() const
    {
      return *static_cast<int32_t *>(height_node_->value());
    }
    int32_t channels() const
    {
      return *static_cast<int32_t *>(channels_node_->value());
    }
    PixelFormat pixelFormat() const
    {
      return *static_cast<PixelFormat *>(pixel_format_node_->value());
    }
    void *data() const
    {
      return data_node_->value();
    }
    float bytesPerPixel() const
    {
      auto info = get_image_info(pixelFormat(), width(), height(), channels());
      return info.bytesPerPixel;
    }
    size_t bufferSize() const
    {
      auto info = get_image_info(pixelFormat(), width(), height(), channels());
      return info.size;
    }
    BaseType baseType() const
    {
      auto info = get_image_info(pixelFormat(), width(), height(), channels());
      return info.baseType;
    }

    DataNode *widthNode() const
    {
      return width_node_;
    }
    DataNode *heightNode() const
    {
      return height_node_;
    }
    DataNode *channelsNode() const
    {
      return channels_node_;
    }
    DataNode *pixelFormatNode() const
    {
      return pixel_format_node_;
    }
    DataNode *dataNode() const
    {
      return data_node_;
    }
};

}  // namespace epf

#endif  // IMAGE_OBJECT_H
