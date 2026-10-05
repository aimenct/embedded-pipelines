#include <gtest/gtest.h>

#include "core.h"

using namespace epf;

TEST(ImageObjectTest, CopyAndAssignmentKeepIndependentNodeTrees)
{
  ImageObject original("img", 4, 3, 1, PixelFormat::Mono8);
  auto *orig_data = static_cast<uint8_t *>(original.data());
  orig_data[0] = 11;

  ImageObject copied(original);
  ImageObject assigned;
  assigned = original;

  ASSERT_NE(copied.dataNode(), original.dataNode());
  ASSERT_NE(assigned.dataNode(), original.dataNode());
  ASSERT_NE(copied.widthNode(), original.widthNode());
  ASSERT_NE(assigned.widthNode(), original.widthNode());

  auto *copied_data = static_cast<uint8_t *>(copied.data());
  auto *assigned_data = static_cast<uint8_t *>(assigned.data());
  EXPECT_EQ(copied_data[0], 11);
  EXPECT_EQ(assigned_data[0], 11);

  orig_data[0] = 42;
  EXPECT_EQ(copied_data[0], 11);
  EXPECT_EQ(assigned_data[0], 11);
}

TEST(ImageObjectTest, MoveAssignmentPreservesTreePointers)
{
  ImageObject original("img", 2, 2, 1, PixelFormat::Mono8);
  auto *original_data = static_cast<uint8_t *>(original.data());
  original_data[0] = 7;

  ImageObject moved_to;
  moved_to = std::move(original);

  ASSERT_NE(moved_to.dataNode(), nullptr);
  EXPECT_EQ(moved_to.width(), 2);
  EXPECT_EQ(moved_to.height(), 2);
  EXPECT_EQ(static_cast<uint8_t *>(moved_to.data())[0], 7);
}

TEST(ImageObjectTest, ExternalNodeAssignmentReferencesSameNodeTree)
{
  ImageObject owned("img", 3, 3, 1, PixelFormat::Mono8);
  auto copied_node = owned.copyNode();
  ObjectNode *external_root = copied_node.get();

  ImageObject external(external_root);
  ImageObject assigned;
  assigned = external;

  ASSERT_EQ(assigned.dataNode(), external.dataNode());
  ASSERT_EQ(assigned.widthNode(), external.widthNode());

  auto *external_data = static_cast<uint8_t *>(external.data());
  auto *assigned_data = static_cast<uint8_t *>(assigned.data());
  external_data[0] = 99;
  EXPECT_EQ(assigned_data[0], 99);
}
