#include "compressed_image_object.h"

#include <cstring>
#include <iostream>

namespace epf {

void CompressedImageObject::update_variables()
{
  auto &refs = raw_root_->references();
  if (refs.size() < 3) return;

  size_node_ = static_cast<DataNode *>(refs[0].address());
  encoding_node_ = static_cast<StringNode *>(refs[1].address());
  data_node_ = static_cast<DataNode *>(refs[2].address());

  // Values will be read directly from these nodes when required.
}

CompressedImageObject::CompressedImageObject()
    : owned_root_(std::make_unique<ObjectNode>("Image", EP_IMAGE_COMPRESSED)),
      raw_root_(owned_root_.get()),
      owns_node_(true)
{
  auto size_nd =
      std::make_unique<DataNode>("size", EP_64U, std::vector<size_t>{1});
  *static_cast<uint64_t *>(size_nd->value()) = 0;

  auto enc_nd = std::make_unique<StringNode>("image_encoding",
                                             to_string(ImageEncoding::JPEG));

  auto data_nd =
      std::make_unique<DataNode>("data", EP_8U, std::vector<size_t>{0});

  owned_root_->addReference(EP_HAS_CHILD, std::move(size_nd));
  owned_root_->addReference(EP_HAS_CHILD, std::move(enc_nd));
  owned_root_->addReference(EP_HAS_CHILD, std::move(data_nd));

  update_variables();
}

CompressedImageObject::CompressedImageObject(ObjectNode *external)
    : raw_root_(external),
      owns_node_(false)
{
  if (!raw_root_ || raw_root_->objecttype() != EP_IMAGE_COMPRESSED) {
    throw std::invalid_argument("CompressedImageObject: invalid external node");
  }
  update_variables();
}

CompressedImageObject::CompressedImageObject(const std::string &name,
                                             ImageEncoding encoding,
                                             size_t encoded_size,
                                             size_t buffer_size, uint8_t *data,
                                             bool memory_managed, bool streamed)
    : owned_root_(std::make_unique<ObjectNode>(name, EP_IMAGE_COMPRESSED)),
      raw_root_(owned_root_.get()),
      owns_node_(true)
{
  std::unique_ptr<DataNode> size_nd;
  // auto size_nd =
  //     std::make_unique<DataNode>("size", EP_64U, std::vector<size_t>{1});
  // *static_cast<uint64_t*>(size_nd->value()) =
  //     static_cast<uint64_t>(encoded_size);

  auto enc_nd =
      std::make_unique<StringNode>("image_encoding", to_string(encoding));

  std::unique_ptr<DataNode> data_nd;
  if (streamed) {
    size_nd = std::make_unique<DataNode>("size", EP_64U, std::vector<size_t>{1},
                                         nullptr, false);

    data_nd = std::make_unique<DataNode>(
        "data", EP_8U, std::vector<size_t>{buffer_size}, nullptr, false);
  }
  else if (memory_managed) {
    size_nd =
        std::make_unique<DataNode>("size", EP_64U, std::vector<size_t>{1});
    *static_cast<uint64_t *>(size_nd->value()) =
        static_cast<uint64_t>(encoded_size);

    data_nd = std::make_unique<DataNode>("data", EP_8U,
                                         std::vector<size_t>{buffer_size});
    if (data && encoded_size > 0) {
      std::memcpy(data_nd->value(), data, std::min(encoded_size, buffer_size));
    }
  }
  else {
    size_nd =
        std::make_unique<DataNode>("size", EP_64U, std::vector<size_t>{1});
    *static_cast<uint64_t *>(size_nd->value()) =
        static_cast<uint64_t>(encoded_size);

    data_nd = std::make_unique<DataNode>(
        "data", EP_8U, std::vector<size_t>{buffer_size}, data);
  }

  owned_root_->addReference(EP_HAS_CHILD, std::move(size_nd));
  owned_root_->addReference(EP_HAS_CHILD, std::move(enc_nd));
  owned_root_->addReference(EP_HAS_CHILD, std::move(data_nd));

  update_variables();
}

CompressedImageObject CompressedImageObject::CreateStreamed(
    const std::string &name, ImageEncoding encoding, size_t buffer_size)
{
  return CompressedImageObject(name, encoding, 0, buffer_size, nullptr,
                               /*memory_managed=*/false, /*streamed=*/true);
}

CompressedImageObject CompressedImageObject::CreateWithManagedMemory(
    const std::string &name, ImageEncoding encoding, size_t encoded_size,
    size_t buffer_size, const uint8_t *source_data)
{
  return CompressedImageObject(name, encoding, encoded_size, buffer_size,
                               const_cast<uint8_t *>(source_data),
                               /*memory_managed=*/true, /*streamed=*/false);
}

CompressedImageObject CompressedImageObject::CreateWithExternalBuffer(
    const std::string &name, ImageEncoding encoding, size_t encoded_size,
    size_t buffer_size, uint8_t *external_data)
{
  return CompressedImageObject(name, encoding, encoded_size, buffer_size,
                               external_data, /*memory_managed=*/false,
                               /*streamed=*/false);
}

CompressedImageObject::CompressedImageObject(const CompressedImageObject &o)
{
  if (o.owns_node_) {
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

CompressedImageObject &CompressedImageObject::operator=(
    const CompressedImageObject &o)
{
  if (&o == this) return *this;
  owned_root_.reset();
  if (o.owns_node_) {
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

CompressedImageObject::CompressedImageObject(CompressedImageObject &&o) noexcept
    : owned_root_(std::move(o.owned_root_)),
      raw_root_(o.raw_root_),
      owns_node_(o.owns_node_),
      data_node_(o.data_node_),
      size_node_(o.size_node_),
      encoding_node_(o.encoding_node_)
{
  o.raw_root_ = nullptr;
  o.owns_node_ = false;
}

CompressedImageObject &CompressedImageObject::operator=(
    CompressedImageObject &&o) noexcept
{
  if (&o == this) return *this;
  owned_root_ = std::move(o.owned_root_);
  raw_root_ = o.raw_root_;
  owns_node_ = o.owns_node_;
  data_node_ = o.data_node_;
  size_node_ = o.size_node_;
  encoding_node_ = o.encoding_node_;

  o.raw_root_ = nullptr;
  o.owns_node_ = false;
  return *this;
}

CompressedImageObject::~CompressedImageObject() = default;

std::unique_ptr<ObjectNode> CompressedImageObject::moveNode()
{
  if (!owns_node_) {
    throw std::runtime_error("CompressedImageObject::moveNode: no ownership");
  }
  auto tmp = std::move(owned_root_);
  raw_root_ = nullptr;
  owns_node_ = false;
  return tmp;
}

std::unique_ptr<ObjectNode> CompressedImageObject::copyNode() const
{
  if (!raw_root_) return nullptr;
  return std::make_unique<ObjectNode>(*raw_root_);
}

void CompressedImageObject::setEncodedSize(size_t size)
{
  if (!size_node_) return;
  *static_cast<uint64_t *>(size_node_->value()) = static_cast<uint64_t>(size);
}

}  // namespace epf
