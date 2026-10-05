#include <gtest/gtest.h>

#include "core.h"

using namespace epf;

TEST(DataNodeTest, ManagedStorageIsOwnedAndCopiedIndependently)
{
  DataNode original("value", EP_32S, std::vector<size_t>{1});
  int32_t original_value = 12;
  ASSERT_EQ(original.write(&original_value), 0);

  DataNode copy(original);
  int32_t replacement_value = 99;
  ASSERT_EQ(copy.write(&replacement_value), 0);

  int32_t unchanged_original_value = 0;
  ASSERT_EQ(original.read(&unchanged_original_value), 0);
  EXPECT_TRUE(original.memMgmt());
  EXPECT_TRUE(copy.memMgmt());
  EXPECT_EQ(unchanged_original_value, original_value);
  EXPECT_NE(original.value(), copy.value());
}

TEST(DataNodeTest, UnmanagedStorageRemainsExternallyShared)
{
  int32_t external_value = 5;
  DataNode original("value", EP_32S, std::vector<size_t>{1}, &external_value);
  DataNode copy(original);
  int32_t replacement_value = 42;

  ASSERT_EQ(copy.write(&replacement_value), 0);

  EXPECT_FALSE(original.memMgmt());
  EXPECT_FALSE(copy.memMgmt());
  EXPECT_EQ(external_value, replacement_value);
  EXPECT_EQ(original.value(), copy.value());
}

TEST(CommandNodeTest, CopiedNodesExecuteTheOriginalCommand)
{
  int execution_count = 0;
  CommandNode original_command("increment", [&execution_count]() {
    ++execution_count;
    return 0;
  });

  CommandNode copy_constructed_command(original_command);
  CommandNode copy_assigned_command = original_command;

  EXPECT_EQ(original_command.run(), 0);
  EXPECT_EQ(copy_constructed_command.run(), 0);
  EXPECT_EQ(copy_assigned_command.run(), 0);
  EXPECT_EQ(execution_count, 3);
}

TEST(ObjectNodeTest, AssignmentReplacesTheExistingChildTree)
{
  ObjectNode original_root("root");
  auto child_folder = std::make_unique<ObjectNode>("folder");
  child_folder->addReference(
      EP_HAS_CHILD,
      std::make_unique<DataNode>("value", EP_32S, std::vector<size_t>{1}));
  original_root.addReference(EP_HAS_CHILD, std::move(child_folder));

  ASSERT_EQ(original_root.references().size(), 1u);
  const Node *nested_folder = original_root.references().front().address();
  ASSERT_NE(nested_folder, nullptr);
  ASSERT_TRUE(nested_folder->isObjectNode());
  EXPECT_EQ(nested_folder->name(), "folder");
  EXPECT_EQ(nested_folder->references().size(), 1u);

  ObjectNode replacement_root("replacement");
  original_root = replacement_root;

  EXPECT_EQ(original_root.name(), "replacement");
  EXPECT_TRUE(original_root.references().empty());
}

TEST(DataNodeTest, SelfAssignmentPreservesManagedStorageAndValue)
{
  DataNode node("value", EP_32S, std::vector<size_t>{1});
  int32_t original_value = 73;
  ASSERT_EQ(node.write(&original_value), 0);

  node = node;  // NOLINT(clang-diagnostic-self-assign-overloaded)

  EXPECT_TRUE(node.memMgmt());
  int32_t value_after_assignment = 0;
  ASSERT_EQ(node.read(&value_after_assignment), 0);
  EXPECT_EQ(value_after_assignment, original_value);
}

TEST(NodeReferenceTest, RemovingReferenceErasesOwnedChild)
{
  ObjectNode parent("parent");
  auto child =
      std::make_unique<DataNode>("child", EP_32S, std::vector<size_t>{1});
  Node *child_address = child.get();
  parent.addReference(EP_HAS_CHILD, std::move(child));

  parent.removeReference(child_address);

  EXPECT_TRUE(parent.references().empty());
}

TEST(ObjectNodeTest, CopyPreservesMetadataAndOwnsIndependentChildren)
{
  ObjectNode original("root", EP_OBJ, "description");
  original.setAccessMode(R);
  original.setVisibility(EP_EXPERT);
  auto original_child =
      std::make_unique<DataNode>("value", EP_32S, std::vector<size_t>{1});
  int32_t original_value = 17;
  ASSERT_EQ(original_child->write(&original_value), 0);
  original.addReference(EP_HAS_PROPERTY, std::move(original_child));

  ObjectNode copy(original);
  ASSERT_EQ(copy.references().size(), 1u);
  auto *copied_child =
      static_cast<DataNode *>(copy.references().front().address());
  int32_t copied_value = 81;
  ASSERT_EQ(copied_child->write(&copied_value), 0);

  const auto *unchanged_child =
      static_cast<const DataNode *>(original.references().front().address());
  int32_t unchanged_value = 0;
  ASSERT_EQ(unchanged_child->read(&unchanged_value), 0);
  EXPECT_EQ(copy.name(), original.name());
  EXPECT_EQ(copy.tooltip(), original.tooltip());
  EXPECT_EQ(copy.visibility(), original.visibility());
  EXPECT_EQ(copy.accessMode(), original.accessMode());
  EXPECT_EQ(copy.references().front().type(), EP_HAS_PROPERTY);
  EXPECT_EQ(unchanged_value, original_value);
  EXPECT_NE(unchanged_child, copied_child);
}

TEST(StringNodeTest, CopyAndAssignmentRespectStorageOwnership)
{
  StringNode managed_source("managed", std::string("source"));
  StringNode managed_copy(managed_source);

  EXPECT_NE(managed_copy.value(), managed_source.value());
  *managed_copy.value() = "copy";
  EXPECT_EQ(*managed_source.value(), "source");

  StringNode managed_assignment_source("managed_assignment_source",
                                       std::string("source"));
  StringNode managed_assignment;
  managed_assignment = managed_assignment_source;
  EXPECT_NE(managed_assignment.value(), managed_assignment_source.value());
  *managed_assignment.value() = "assignment";
  EXPECT_EQ(*managed_assignment_source.value(), "source");

  std::string external_value = "external";
  StringNode external_source("external", &external_value);
  StringNode external_copy(external_source);
  StringNode external_assignment;
  external_assignment = external_source;

  EXPECT_EQ(external_copy.value(), &external_value);
  EXPECT_EQ(external_assignment.value(), &external_value);
  *external_copy.value() = "shared";
  EXPECT_EQ(external_value, "shared");
  EXPECT_EQ(*external_assignment.value(), "shared");
}
