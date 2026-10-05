// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// image_object.cpp

#include "image_object.h"

#include <iostream>

using namespace epf;

void ImageObject::update_variables()
{
  // assume children were added in precisely this order:
  //  0: width, 1: height, 2: channels, 3: pixel_format, 4: data
  auto &refs = raw_root_->references();
  width_node_ = static_cast<DataNode *>(refs[0].address());
  height_node_ = static_cast<DataNode *>(refs[1].address());
  channels_node_ = static_cast<DataNode *>(refs[2].address());
  pixel_format_node_ = static_cast<DataNode *>(refs[3].address());
  data_node_ = static_cast<DataNode *>(refs[4].address());

  auto width = *static_cast<int32_t *>(width_node_->value());
  auto height = *static_cast<int32_t *>(height_node_->value());
  auto ch = *static_cast<int32_t *>(channels_node_->value());
  auto pf = *static_cast<PixelFormat *>(pixel_format_node_->value());

  auto info = get_image_info(pf, width, height, ch);
  if (data_node_->size() != static_cast<std::size_t>(info.size)) {
    std::cerr << "Warning: data buffer size (" << data_node_->size()
              << ") does not match expected size (" << info.size << ")\n";
  }
}

// --- ctors/dtors ---

ImageObject::ImageObject() = default;

ImageObject::ImageObject(ObjectNode *external)
    : raw_root_(external),
      owns_node_(false)
{
  if (!raw_root_ || raw_root_->objecttype() != EP_IMAGE_RAW) {
    throw std::invalid_argument("ImageObject: invalid external node");
  }
  update_variables();
}

ImageObject::ImageObject(const std::string &name, int32_t width, int32_t height,
                         int32_t channels, PixelFormat pf)
    : owned_root_(std::make_unique<ObjectNode>(name, EP_IMAGE_RAW)),
      raw_root_(owned_root_.get()),
      owns_node_(true)
{
  // helper to add a single int32_t DataNode
  auto make_scalar = [&](const char *key, int32_t v) {
    auto n = std::make_unique<DataNode>(key, EP_32S, std::vector<size_t>{1});
    n->write(&v);
    return n;
  };
  // add the four required children
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("width", width));
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("height", height));
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("channels", channels));
  owned_root_->addReference(
      EP_HAS_CHILD, make_scalar("pixel_format", static_cast<int32_t>(pf)));

  // data buffer (uninitialized)
  size_t buf = get_image_info(pf, width, height, channels).size;
  auto data_nd =
      std::make_unique<DataNode>("data", EP_8U, std::vector<size_t>{buf});
  owned_root_->addReference(EP_HAS_CHILD, std::move(data_nd));

  update_variables();
}

// image_object.cpp

// #include "image_object.h"
// using namespace epf;

ImageObject::ImageObject(const std::string &name, int32_t width, int32_t height,
                         int32_t channels, PixelFormat pf,
                         uint8_t *prealloc_data)
    : owned_root_(std::make_unique<ObjectNode>(name, EP_IMAGE_RAW)),
      raw_root_(owned_root_.get()),
      owns_node_(true)
{
  // Helper for a single‐element int32 DataNode
  auto make_scalar = [&](const char *key, int32_t v) {
    auto n = std::make_unique<DataNode>(key, EP_32S, std::vector<size_t>{1});
    n->write(&v);
    return n;
  };

  // Add the four scalars: width, height, channels, pixel_format
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("width", width));
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("height", height));
  owned_root_->addReference(EP_HAS_CHILD, make_scalar("channels", channels));
  owned_root_->addReference(
      EP_HAS_CHILD, make_scalar("pixel_format", static_cast<int32_t>(pf)));

  // Allocate the data buffer node, using the pre‐allocated pointer
  size_t buf_bytes = get_image_info(pf, width, height, channels).size;
  auto data_nd = std::make_unique<DataNode>(
      "data", EP_8U, std::vector<size_t>{buf_bytes}, prealloc_data);
  owned_root_->addReference(EP_HAS_CHILD, std::move(data_nd));

  // Populate all the cached members from the tree
  update_variables();
}

ImageObject::ImageObject(const ImageObject &o)
{
  if (o.owns_node_) {
    //    owned_root_ = o.owned_root_->clone();
    owned_root_ = std::make_unique<ObjectNode>(*o.owned_root_);
    raw_root_ = owned_root_.get();
    owns_node_ = true;
  }
  else {
    raw_root_ = o.raw_root_;
    owns_node_ = false;
  }
  update_variables();
}

ImageObject &ImageObject::operator=(const ImageObject &o)
{
  if (&o == this) return *this;
  owned_root_.reset();
  if (o.owns_node_) {
    //    owned_root_ = o.owned_root_->clone();
    owned_root_ = std::make_unique<ObjectNode>(*o.owned_root_);
    raw_root_ = owned_root_.get();
    owns_node_ = true;
  }
  else {
    raw_root_ = o.raw_root_;
    owns_node_ = false;
  }
  update_variables();
  return *this;
}

ImageObject::ImageObject(ImageObject &&o) noexcept
    : owned_root_(std::move(o.owned_root_)),
      raw_root_(o.raw_root_),
      owns_node_(o.owns_node_),

      width_node_(o.width_node_),
      height_node_(o.height_node_),
      channels_node_(o.channels_node_),
      pixel_format_node_(o.pixel_format_node_),
      data_node_(o.data_node_)
{
  o.raw_root_ = nullptr;
  o.owns_node_ = false;
}

ImageObject &ImageObject::operator=(ImageObject &&o) noexcept
{
  if (&o == this) return *this;
  owned_root_ = std::move(o.owned_root_);
  raw_root_ = o.raw_root_;
  owns_node_ = o.owns_node_;

  width_node_ = o.width_node_;
  height_node_ = o.height_node_;
  channels_node_ = o.channels_node_;
  pixel_format_node_ = o.pixel_format_node_;
  data_node_ = o.data_node_;

  o.raw_root_ = nullptr;
  o.owns_node_ = false;
  return *this;
}

ImageObject::~ImageObject() = default;

std::unique_ptr<ObjectNode> ImageObject::moveNode()
{
  if (!owns_node_) {
    throw std::runtime_error("ImageObject::move_node: no ownership");
  }
  auto tmp = std::move(owned_root_);
  raw_root_ = nullptr;
  owns_node_ = false;
  return tmp;
}

std::unique_ptr<ObjectNode> ImageObject::copyNode() const
{
  if (!raw_root_) return nullptr;
  //  return raw_root_->clone();
  return std::make_unique<ObjectNode>(*raw_root_);
}
