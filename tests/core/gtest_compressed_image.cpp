#include <gtest/gtest.h>

#include <cstring>

#include "core.h"

using namespace epf;

TEST(CompressedImageObjectTest, ManagedMemory)
{
  uint8_t src[4] = {1, 2, 3, 4};
  auto obj = CompressedImageObject::CreateWithManagedMemory(
      "img", ImageEncoding::JPEG, 4, 10, src);

  EXPECT_EQ(obj.imageEncoding(), ImageEncoding::JPEG);
  EXPECT_EQ(obj.encodedSize(), static_cast<size_t>(4));
  EXPECT_EQ(obj.bufferSize(), static_cast<size_t>(10));
  EXPECT_TRUE(obj.dataNode()->memMgmt());
  EXPECT_EQ(std::memcmp(obj.data(), src, 4), 0);
}

TEST(CompressedImageObjectTest, ExternalBuffer)
{
  uint8_t buffer[8] = {0};
  auto obj = CompressedImageObject::CreateWithExternalBuffer(
      "img", ImageEncoding::PNG, 6, sizeof(buffer), buffer);

  EXPECT_FALSE(obj.dataNode()->memMgmt());
  EXPECT_EQ(obj.data(), buffer);
  buffer[0] = 9;
  EXPECT_EQ(static_cast<uint8_t *>(obj.data())[0], 9);
}

TEST(CompressedImageObjectTest, Streamed)
{
  auto obj =
      CompressedImageObject::CreateStreamed("img", ImageEncoding::MJPEG, 20);

  // EXPECT_TRUE(obj.dataNode()->isStreamed());
  EXPECT_FALSE(obj.dataNode()->memMgmt());
  EXPECT_EQ(obj.encodedSize(), static_cast<size_t>(0));
  EXPECT_EQ(obj.bufferSize(), static_cast<size_t>(20));
}

TEST(CompressedImageObjectTest, CopyAndAssignmentKeepIndependentNodeTrees)
{
  uint8_t src[5] = {1, 2, 3, 4, 5};
  auto original = CompressedImageObject::CreateWithManagedMemory(
      "img", ImageEncoding::JPEG, 5, 8, src);

  CompressedImageObject copied(original);
  CompressedImageObject assigned;
  assigned = original;

  ASSERT_NE(copied.dataNode(), original.dataNode());
  ASSERT_NE(assigned.dataNode(), original.dataNode());
  ASSERT_NE(copied.sizeNode(), original.sizeNode());
  ASSERT_NE(assigned.sizeNode(), original.sizeNode());

  original.setEncodedSize(3);
  EXPECT_EQ(original.encodedSize(), static_cast<size_t>(3));
  EXPECT_EQ(copied.encodedSize(), static_cast<size_t>(5));
  EXPECT_EQ(assigned.encodedSize(), static_cast<size_t>(5));
}
