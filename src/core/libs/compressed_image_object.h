#ifndef COMPRESSED_IMAGE_OBJECT_H
#define COMPRESSED_IMAGE_OBJECT_H

#include "image_formats.h"
#include "node.h"

namespace epf {

// Compressed image object as node tree:
// ObjectNode "Image"
//   ├─ DataNode "size"   (uint64_t)
//   ├─ StringNode "image_encoding" (ImageEncoding name)
//   └─ DataNode "data"   (unsigned char[])
//
// "size" reflects the actual encoded image size. When data is streamed,
// this value must be streamed alongside the data buffer.

class CompressedImageObject {
  private:
    std::unique_ptr<ObjectNode> owned_root_;
    ObjectNode *raw_root_{};
    bool owns_node_{false};

    DataNode *data_node_{};
    DataNode *size_node_{};
    StringNode *encoding_node_{};

    // Values are obtained directly from the nodes when required.

    void update_variables();

    // Private constructor to force use of static factory methods
    CompressedImageObject(const std::string &name, ImageEncoding encoding,
                          size_t encoded_size, size_t buffer_size,
                          uint8_t *data, bool memory_managed, bool streamed);

  public:
    CompressedImageObject();
    explicit CompressedImageObject(ObjectNode *external_root);

    // Factory methods for different use cases
    /// Create an image object for streamed data (no internal buffer)
    static CompressedImageObject CreateStreamed(const std::string &name,
                                                ImageEncoding encoding,
                                                size_t buffer_size);

    /// Create an image object that manages its own memory (copies from source)
    //  ¿check encoding size - external dtaa?
    static CompressedImageObject CreateWithManagedMemory(
        const std::string &name, ImageEncoding encoding, size_t encoded_size,
        size_t buffer_size, const uint8_t *source_data);

    /// Create an image object using external memory (no copy, caller manages)
    // ¿check encoding size - external dtaa?
    static CompressedImageObject CreateWithExternalBuffer(
        const std::string &name, ImageEncoding encoding, size_t encoded_size,
        size_t buffer_size, uint8_t *external_data);

    // // Constructors
    // CompressedImageObject(const std::string& name, ImageEncoding encoding,
    //                       size_t buffer_size);  // Streamed
    // CompressedImageObject(const std::string& name, ImageEncoding encoding,
    //                       size_t encoding_size, size_t buffer_size,
    //                       uint8_t* data);  // get ownership of data memory
    // CompressedImageObject(const std::string& name, ImageEncoding encoding,
    //                       size_t encoding_size, size_t buffer_size,
    //                       uint8_t* prealloc_data);  // External data memory

    CompressedImageObject(const CompressedImageObject &);
    CompressedImageObject &operator=(const CompressedImageObject &);
    CompressedImageObject(CompressedImageObject &&) noexcept;
    CompressedImageObject &operator=(CompressedImageObject &&) noexcept;
    ~CompressedImageObject();

    std::unique_ptr<ObjectNode> moveNode();
    std::unique_ptr<ObjectNode> copyNode() const;

    ImageEncoding imageEncoding() const
    {
      return from_string_image_encoding(*encoding_node_->value());
    }
    void *data() const
    {
      return data_node_->value();
    }
    size_t encodedSize() const
    {
      if (!size_node_->value()) return 0;
      return *static_cast<uint64_t *>(size_node_->value());
    }
    void setEncodedSize(size_t size);
    size_t bufferSize() const
    {
      return data_node_->size();
    }

    DataNode *dataNode() const
    {
      return data_node_;
    }
    DataNode *sizeNode() const
    {
      return size_node_;
    }
    StringNode *encodingNode() const
    {
      return encoding_node_;
    }
};

}  // namespace epf

#endif  // COMPRESSED_IMAGE_OBJECT_H
