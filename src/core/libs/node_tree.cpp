// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "node_tree.h"

using namespace epf;

NodeTree::NodeTree()
{
  root_node_ = epf::ObjectNode("unamed");
  add2list(&root_node_);
}

NodeTree::NodeTree(std::string name)
{
  root_node_ = epf::ObjectNode(name);
  add2list(&root_node_);
}

NodeTree::NodeTree(const ObjectNode &node)
{
  root_node_ = node;
  add2list(&root_node_);
}

NodeTree::NodeTree(const NodeTree &obj)
{
  *this = obj;
}

const NodeTree &NodeTree::operator=(const NodeTree &obj)
{
  root_node_ = obj.root_node_;

  node_list_.clear();

  // Add list of nodes in hierarchical order
  add2list(&root_node_);

  // Reorder them to match the original NodeTree
  std::vector<std::size_t> order = obj.hierarchicalOrder();
  std::vector<epf::Node *> copy_node_list = node_list_;

  for (std::size_t i = 0; i < node_list_.size(); i++) {
    node_list_[i] = copy_node_list[order[i]];

    // set ID
    // node_list_[i]->setId(static_cast<int32_t>(i));
  }

  return *this;
}

void NodeTree::setName(std::string name)
{
  root_node_.setName(name);
}

std::string NodeTree::name() const
{
  return root_node_.name();
}

const epf::ObjectNode &NodeTree::root() const
{
  return root_node_;
}

epf::Node *NodeTree::operator[](std::size_t index) const
{
  if (index >= node_list_.size()) {
    std::cout << "NodeTree::operator[]: index out of range" << std::endl;
    return nullptr;
  }
  return node_list_[index];
}

int NodeTree::add(std::unique_ptr<epf::Node> node, epf::RefType reference_type,
                  int parent_index)
{
  if (parent_index < 0) {
    std::cerr << "[NodeTree] " << node->name()
              << " could not be added, parent index must be >=0." << std::endl;
    return -1;
  }
  if (parent_index >= length()) {
    std::cerr << "[NodeTree] " << node->name()
              << " could not be added, parent index " << parent_index
              << " is invalid. NodeTree has " << length() << " elements."
              << std::endl;
    return -1;
  }
  size_t u_index = static_cast<size_t>(parent_index);
  node_list_.at(u_index)->addReference(reference_type, std::move(node));

  Node *saved_node_addr = node_list_.at(u_index)->references().back().address();

  return add2list(saved_node_addr);
}

void NodeTree::remove(int index)
{
  assert(index >= 0);
  size_t u_index = static_cast<size_t>(index);
  epf::Node *node = node_list_.at(u_index);
  remove(node);
}

void NodeTree::remove(epf::Node *node)
{
  std::size_t parent_index = parentIndex(node);
  epf::Node *parent_node = node_list_.at(parent_index);

  removeFromList(node);
  parent_node->removeReference(node);
}

void NodeTree::removeFromList(epf::Node *node)
{
  int32_t index = nodeIndex(node);
  if (index < 0) {
    std::cout << "NodeTree::removeFromList: node not found in node_list_: "
              << (node ? node->name() : "<null>") << std::endl;
    return;
  }
  auto it = node_list_.begin();
  std::advance(it, index);
  node_list_.erase(it);

  for (const epf::Reference &ref : node->references()) {
    removeFromList(ref.address());
  }
}

int32_t NodeTree::nodeIndex(const epf::Node *node) const
{
  for (std::size_t i = 0; i < node_list_.size(); i++) {
    if (node_list_[i] == node) {
      return static_cast<int32_t>(i);
    }
  }
  std::cout << "NodeTree::nodeIndex: node not found in node_list_: "
            << (node ? node->name() : "<null>") << std::endl;
  return -1;
}

std::vector<std::size_t> NodeTree::hierarchicalOrder() const
{
  std::vector<std::size_t> order;

  const epf::Node *node = &root_node_;
  computeHierarchicalOrder(order, node);

  return order;
}

void NodeTree::print(int index) const
{
  assert(index >= 0);
  int indent = 1;
  std::cout << std::setw(3) << index << "|"
            << std::string(static_cast<size_t>(indent), ' ');
  indentedPrint(index, indent);
}

int32_t NodeTree::length() const
{
  return static_cast<int32_t>(node_list_.size());
}

void NodeTree::indentedPrint(int index, int indent) const
{
  assert(index >= 0);
  assert(indent >= 0);

  size_t u_index = static_cast<size_t>(index);
  epf::Node *node = node_list_[u_index];
  node->print();
  indent += 5;  // Supposing node name of 10 chars

  for (const epf::Reference &ref : node->references()) {
    int32_t new_index = nodeIndex(ref.address());
    if (new_index < 0) {
      std::cout << "NodeTree::indentedPrint: missing reference from "
                << node->name() << " to "
                << (ref.address() ? ref.address()->name() : "<null>")
                << std::endl;
      continue;
    }
    std::cout << std::setw(3) << new_index << "|" << std::string(1, ' ');
    std::cout << std::string(static_cast<size_t>(indent), ' ') << "└--->";
    // Add additional indent to account for the arrow
    indentedPrint(new_index, indent + 5);
  }
}

int32_t NodeTree::parentIndex(const epf::Node *node) const
{
  for (std::size_t i = 0; i < node_list_.size(); i++) {
    for (const epf::Reference &ref : node_list_[i]->references()) {
      if (ref.address() == node) {
        return static_cast<int32_t>(i);
      }
    }
  }
  return -1;
}

int NodeTree::add2list(epf::Node *node)
{
  int32_t index = static_cast<int32_t>(node_list_.size());
  node_list_.push_back(node);

  // set id
  // node->setId(index);

  for (const epf::Reference &ref : node->references()) {
    add2list(ref.address());
  }

  return index;
}

void NodeTree::computeHierarchicalOrder(std::vector<std::size_t> &order,
                                        const epf::Node *node) const
{
  int32_t index = nodeIndex(node);
  if (index < 0) return;
  order.push_back(index);

  for (const epf::Reference &ref : node->references()) {
    computeHierarchicalOrder(order, ref.address());
  }

  return;
}
