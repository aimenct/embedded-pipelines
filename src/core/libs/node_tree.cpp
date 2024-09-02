// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "node_tree.h"

using namespace ep;

NodeTree::NodeTree(std::string name)
{
  root_node_.setName(name);
  add2list(&root_node_);
}

NodeTree::NodeTree(const ep::ObjectNode* node)
{
  root_node_ = *node;

  add2list(&root_node_);
}

NodeTree::NodeTree(const NodeTree& obj)
{
  *this = obj;
}

const NodeTree& NodeTree::operator=(const NodeTree& obj)
{
  root_node_ = obj.root_node_;

  node_list_.clear();

  // Add list of nodes in hierarchical order
  add2list(&root_node_);

  // Reorder them to match the original NodeTree
  std::vector<std::size_t> order = obj.hierarchicalOrder();
  std::vector<ep::Node2*> copy_node_list = node_list_;

  for (std::size_t i = 0; i < node_list_.size(); i++) {
    node_list_[i] = copy_node_list[order[i]];

    // set ID
    node_list_[i]->setId(static_cast<int32_t>(i));
  }

  order.clear();
  return *this;
}

const std::string& NodeTree::name() const
{
  return name_;
}

const ep::ObjectNode& NodeTree::root() const
{
  return root_node_;
}

ep::Node2* NodeTree::operator[](std::size_t index) const
{
  return node_list_[index];
}

int NodeTree::add(ep::Node2* node, ep::RefType reference_type, int parent_index)
{
  node_list_[parent_index]->addReference(reference_type, node);

  return add2list(node);
}

void NodeTree::remove(int index)
{
  ep::Node2* node = node_list_[index];
  remove(node);
}

void NodeTree::remove(ep::Node2* node)
{
  std::size_t parent_index = parentIndex(node);
  ep::Node2* parent_node = node_list_[parent_index];

  parent_node->removeReference(node);
  removeFromList(node);
}

void NodeTree::removeFromList(ep::Node2* node)
{
  size_t index = nodeIndex(node);
  node_list_.erase(node_list_.begin() + index);

  for (const ep::Reference& ref : node->references()) {
    removeFromList(ref.address());
  }
}

std::size_t NodeTree::nodeIndex(const ep::Node2* node) const
{
  for (std::size_t i = 0; i < node_list_.size(); i++) {
    if (node_list_[i] == node) {
      return i;
    }
  }
  std::cout << "Node not found!!" << std::endl;
  throw;
}

std::vector<std::size_t> NodeTree::hierarchicalOrder() const
{
  std::vector<std::size_t> order;

  const ep::Node2* node = &root_node_;
  computeHierarchicalOrder(order, node);

  return order;
}

void NodeTree::print(int index) const
{
  int indent = 1;
  std::cout << std::setw(3) << index << "|" << std::string(indent, ' ');
  indentedPrint(index, indent);
}

int32_t NodeTree::length() const
{
  return static_cast<int32_t>(node_list_.size());
}

void NodeTree::indentedPrint(int index, int indent) const
{
  ep::Node2* node = node_list_[index];
  node->print();
  indent += 5;  // Supposing node name of 10 chars

  for (const ep::Reference& ref : node->references()) {
    int32_t new_index = static_cast<int32_t>(nodeIndex(ref.address()));
    std::cout << std::setw(3) << new_index << "|" << std::string(1, ' ');
    std::cout << std::string(indent, ' ') << "└--->";
    // Add additional indent to account for the arrow
    indentedPrint(new_index, indent + 5);
  }
}

std::size_t NodeTree::parentIndex(const ep::Node2* node) const
{
  for (std::size_t i = 0; i < node_list_.size(); i++) {
    for (const ep::Reference& ref : node_list_[i]->references()) {
      if (ref.address() == node) {
        return i;
      }
    }
  }

  throw;
}

int NodeTree::add2list(ep::Node2* node)
{
  int32_t index;
  node_list_.push_back(node);

  index = static_cast<int32_t>(node_list_.size());
  // set id
  node->setId(index);

  for (const ep::Reference& ref : node->references()) {
    add2list(ref.address());
  }

  return index;
}

void NodeTree::computeHierarchicalOrder(std::vector<std::size_t>& order,
                                        const ep::Node2* node) const
{
  std::size_t index = nodeIndex(node);
  order.push_back(index);

  for (const ep::Reference& ref : node->references()) {
    computeHierarchicalOrder(order, ref.address());
  }

  return;
}
