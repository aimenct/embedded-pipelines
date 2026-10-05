#include <gtest/gtest.h>

#include <string>

#include "core.h"

using namespace epf;

TEST(MessageTest, ItemsCanBeRetrievedByIndexAndName)
{
  Message message;
  message.addItem(std::make_unique<DataNode>("first", EP_32S,
                                             std::vector<size_t>{1}, nullptr));
  message.addItem(std::make_unique<DataNode>("second", EP_32S,
                                             std::vector<size_t>{1}, nullptr));

  ASSERT_EQ(message.itemCount(), 2u);
  ASSERT_NE(message.item(0), nullptr);
  EXPECT_EQ(message.item(0)->name(), "first");
  ASSERT_NE(message.item("second"), nullptr);
  EXPECT_EQ(message.item("second")->name(), "second");

  testing::internal::CaptureStderr();
  Node *missing_item = message.item("does_not_exist");
  std::string error = testing::internal::GetCapturedStderr();
  EXPECT_EQ(missing_item, nullptr);
  EXPECT_NE(error.find("not found"), std::string::npos);
}

TEST(MessageTest, AssignmentRebuildsStreamedNodeMapWithoutDuplicates)
{
  Message source;

  auto streamed_a = std::make_unique<DataNode>(
      "streamed_a", EP_8U, std::vector<size_t>{4}, nullptr, false);
  auto non_streamed = std::make_unique<DataNode>("non_streamed", EP_32S,
                                                 std::vector<size_t>{1});
  auto streamed_b = std::make_unique<DataNode>(
      "streamed_b", EP_8U, std::vector<size_t>{6}, nullptr, false);

  source.addItem(std::move(streamed_a));
  source.addItem(std::move(non_streamed));
  source.addItem(std::move(streamed_b));

  Message target;
  target.addItem(std::make_unique<DataNode>(
      "legacy_streamed", EP_8U, std::vector<size_t>{10}, nullptr, false));

  target = source;

  ASSERT_EQ(target.itemCount(), source.itemCount());
  EXPECT_EQ(target.size(), static_cast<size_t>(10));

  auto *assigned_streamed_a =
      static_cast<DataNode *>(target.item("streamed_a"));
  auto *assigned_non_streamed =
      static_cast<DataNode *>(target.item("non_streamed"));
  auto *assigned_streamed_b =
      static_cast<DataNode *>(target.item("streamed_b"));
  ASSERT_NE(assigned_streamed_a, nullptr);
  ASSERT_NE(assigned_non_streamed, nullptr);
  ASSERT_NE(assigned_streamed_b, nullptr);

  EXPECT_EQ(target.streamedNodeOffset(assigned_streamed_a),
            static_cast<size_t>(0));
  EXPECT_EQ(target.streamedNodeOffset(assigned_streamed_b),
            static_cast<size_t>(4));

  testing::internal::CaptureStderr();
  EXPECT_EQ(target.streamedNodeOffset(assigned_non_streamed),
            static_cast<size_t>(0));
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_NE(err.find("Streamed node not found"), std::string::npos);
}
