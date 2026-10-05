#include <gtest/gtest.h>

#include <stdexcept>

#include "core.h"

using namespace epf;

namespace {

class TestNodeTree : public NodeTree {
  public:
    explicit TestNodeTree(const std::string &name)
        : NodeTree(name)
    {
    }
    TestNodeTree(const TestNodeTree &other)
        : NodeTree(other)
    {
    }

    using NodeTree::add;
    using NodeTree::name;
    using NodeTree::parentIndex;
    using NodeTree::remove;
};

}  // namespace

TEST(NodeTreeTest, HierarchyMutationMaintainsReferencesAndIndices)
{
  TestNodeTree tree("root");
  int removed_branch_index =
      tree.add(std::make_unique<ObjectNode>("removed_branch"), EP_HAS_CHILD);
  int removed_leaf_index =
      tree.add(std::make_unique<DataNode>("removed_leaf", EP_32S,
                                          std::vector<size_t>{1}),
               EP_HAS_CHILD, removed_branch_index);
  int surviving_branch_index =
      tree.add(std::make_unique<ObjectNode>("surviving_branch"), EP_HAS_CHILD);
  Node *surviving_branch = tree[static_cast<size_t>(surviving_branch_index)];

  ASSERT_EQ(removed_branch_index, 1);
  ASSERT_EQ(removed_leaf_index, 2);
  ASSERT_EQ(surviving_branch_index, 3);
  ASSERT_EQ(tree.length(), 4);
  EXPECT_EQ(tree.parentIndex(tree[static_cast<size_t>(removed_leaf_index)]),
            removed_branch_index);
  EXPECT_EQ(tree.nodeIndex(surviving_branch), surviving_branch_index);

  tree.remove(removed_branch_index);

  ASSERT_EQ(tree.length(), 2);
  EXPECT_EQ(tree[1], surviving_branch);
  EXPECT_EQ(tree.nodeIndex(surviving_branch), 1);
  EXPECT_EQ(tree.parentIndex(surviving_branch), 0);
  ASSERT_EQ(tree.root().references().size(), 1u);
  EXPECT_EQ(tree.root().references().front().address(), surviving_branch);
}

TEST(NodeTreeTest, InvalidMutationsLeaveTreeUnchanged)
{
  TestNodeTree tree("root");

  EXPECT_EQ(tree.add(std::make_unique<DataNode>("rejected_leaf", EP_32S,
                                                std::vector<size_t>{1}),
                     EP_HAS_CHILD, 99),
            -1);
  EXPECT_THROW(tree.remove(42), std::out_of_range);
  EXPECT_THROW(tree.remove(0), std::out_of_range);

  EXPECT_EQ(tree.length(), 1);
  EXPECT_TRUE(tree.root().references().empty());
}

TEST(NodeTreeTest, CopyOwnsAnIndependentHierarchy)
{
  TestNodeTree original("root");
  int branch_index =
      original.add(std::make_unique<ObjectNode>("branch"), EP_HAS_CHILD);
  original.add(
      std::make_unique<DataNode>("leaf", EP_32S, std::vector<size_t>{1}),
      EP_HAS_CHILD, branch_index);

  TestNodeTree copy(original);
  copy[1]->setName("renamed_branch");
  copy.remove(2);

  EXPECT_EQ(original.length(), 3);
  EXPECT_EQ(original[1]->name(), "branch");
  EXPECT_EQ(original[2]->name(), "leaf");
  EXPECT_EQ(copy.length(), 2);
  EXPECT_EQ(copy[1]->name(), "renamed_branch");
  EXPECT_NE(original[1], copy[1]);
}
