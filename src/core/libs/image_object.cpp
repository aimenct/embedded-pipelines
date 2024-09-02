// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "image_object.h"

using namespace ep;

ImageObject::ImageObject(ep::Node2 *node)
{
  if (!node->isObjectNode()) {
    printf("Error ImageObject Constructor - not object type\n");
    exit(0);
  }

  ep::ObjectNode *obj_node = static_cast<ep::ObjectNode *>(node);

  if (obj_node->objecttype() != ep::EP_IMAGE_OBJ) {
    printf("Error ImageObject Constructor - not Image Object type %d\n",
           obj_node->objecttype());
    exit(0);
  }

  root_node_ = obj_node;

  // Get pointers from root node
  ep::DataNode *dn =
      static_cast<ep::DataNode *>(root_node_->references()[0].address());
  width_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[1].address());
  height_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[2].address());
  channels_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[3].address());
  pixel_format_ = static_cast<ep::PixelFormat *>(dn->value());
  data_node_ =
      static_cast<ep::DataNode *>(root_node_->references()[4].address());

  PixelFormatInfo info =
      get_image_info(*pixel_format_, *width_, *height_, *channels_);

  size_ = info.size;

  node_tree_mem_mgmt_ = 0;
}

ImageObject::ImageObject(std::string name, int32_t width, int32_t height, int32_t channels,
            PixelFormat pixel_format, uint8_t *data)
{
  root_node_ = new ep::ObjectNode(name, ep::EP_IMAGE_OBJ);
  ep::DataNode *w_n = new ep::DataNode("width", ep::EP_32S, {1});
  ep::DataNode *h_n = new ep::DataNode("height", ep::EP_32S, {1});
  ep::DataNode *c_n = new ep::DataNode("channels", ep::EP_32S, {1});
  ep::DataNode *pf_n = new ep::DataNode("pixel format", ep::EP_32S, {1});

  PixelFormatInfo info = get_image_info(pixel_format, width, height, channels);

  size_t data_size = static_cast<size_t>(info.size);
  ep::DataNode *data_n = new ep::DataNode("data", ep::EP_8U, {data_size}, data);
  root_node_->addReference(ep::EP_HAS_CHILD, w_n);
  root_node_->addReference(ep::EP_HAS_CHILD, h_n);
  root_node_->addReference(ep::EP_HAS_CHILD, c_n);
  root_node_->addReference(ep::EP_HAS_CHILD, pf_n);
  root_node_->addReference(ep::EP_HAS_CHILD, data_n);

  // Get pointers from root node
  ep::DataNode *dn =
      static_cast<ep::DataNode *>(root_node_->references()[0].address());
  width_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[1].address());
  height_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[2].address());
  channels_ = static_cast<int32_t *>(dn->value());
  dn = static_cast<ep::DataNode *>(root_node_->references()[3].address());
  pixel_format_ = static_cast<PixelFormat *>(dn->value());
  data_node_ =
      static_cast<ep::DataNode *>(root_node_->references()[4].address());

  *width_ = width;
  *height_ = height;
  *channels_ = channels;
  *pixel_format_ = pixel_format;

  size_ = info.size;

  node_tree_mem_mgmt_ = 1;
}

ImageObject::~ImageObject()
{
  if (node_tree_mem_mgmt_) {
    delete root_node_;
    root_node_ = nullptr;
    node_tree_mem_mgmt_ = 0;
    width_ = nullptr;
    height_ = nullptr;
    channels_ = nullptr;
    pixel_format_ = nullptr;
    data_node_ = nullptr;
    size_ = 0;
  }
}

ObjectNode *ImageObject::transferNodeTree()
{
  if (node_tree_mem_mgmt_ == 0) {
    std::cerr << "Error: ImageNode - transferNodeTree not possible"
              << std::endl;
    return nullptr;  // exit(0);
  }

  ObjectNode *temp = new ObjectNode();
  *temp = *root_node_;
  delete root_node_;

  // clear this
  node_tree_mem_mgmt_ = 0;
  width_ = nullptr;
  height_ = nullptr;
  channels_ = nullptr;
  pixel_format_ = nullptr;
  data_node_ = nullptr;
  size_ = 0;

  return temp;
  //      return root_node_;
}

ObjectNode *ImageObject::copyNodeTree()
{
  ObjectNode *temp = new ObjectNode();
  *temp = *root_node_;
  return temp;
}

/* Get width */
int32_t ImageObject::width() const
{
  return *width_;
}

/* Get heigth */
int32_t ImageObject::height() const
{
  return *height_;
}
/* Get heigth */
int32_t ImageObject::channels() const
{
  return *channels_;
}

/* Get pixelFormat */
PixelFormat ImageObject::pixelFormat() const
{
  return *pixel_format_;
}

/* Get data */
void *ImageObject::data() const
{
  return data_node_->value();
}

/* bytesPerPixel */
float ImageObject::bytesPerPixel() const
{
  return static_cast<float>(size_) / static_cast<float>((*width_) * (*height_));
}


/* Get data */
size_t ImageObject::size() const
{
  return size_;
}
