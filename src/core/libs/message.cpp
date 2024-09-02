// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "message.h"

using namespace ep;

Message::Message(const ep::ObjectNode* node)
    : NodeTree(node),
      size_(0)
{
  queue_node_list_.clear();
  for (auto n : node_list_) {
    if (n->isDataNode()) {
      ep::DataNode* data_node = static_cast<ep::DataNode*>(n);
      if (data_node->isStreamed()) {
        data_node->setOffset(size_);
        queue_node_list_.push_back(data_node);
        size_ += data_node->size();
      }
    }
  }
}

Message::Message(const Message& msg)
    : NodeTree(msg)
{
  *this = msg;
}

const ep::ObjectNode& Message::rootNode()
{
  return root();
}

std::size_t Message::itemCount() const
{
  return root().references().size();
}

std::size_t Message::size() const
{
  return size_;
}

ep::Node2* Message::item(const std::size_t item_index) const
{
  // printf"item \n");
  ep::Node2* node = root().references()[item_index].address();
  return node;
}

const Message& Message::operator=(const Message& obj)
{
  NodeTree::operator=(obj);

  // add to QueuedNoded
  size_ = 0;
  for (auto n : node_list_)
    if (n->isDataNode()) {  // ¿TODO do this check in a MACRO?
      ep::DataNode* node = static_cast<ep::DataNode*>(n);
      if (node->isStreamed()) {
        node->setOffset(size_);
        queue_node_list_.push_back(node);
        size_ += node->size();
      }
    }

  return *this;
}

void Message::addItem(ep::Node2* node, ep::RefType reference_type)
{
  add(node, reference_type);

  queue_node_list_.clear();

  // add to QueuedNoded
  size_ = 0;
  for (auto n : node_list_)
    if (n->isDataNode()) {
      ep::DataNode* node = static_cast<ep::DataNode*>(n);
      if (node->isStreamed()) {
        node->setOffset(size_);
        queue_node_list_.push_back(node);
        size_ += node->size();
      }
    }
}

void Message::updateMessage(char* pointer)
{
  for (auto node : this->queue_node_list_) {
    node->setPtrMsg(pointer);
    node->setValue(node->offset() + pointer);
  }
}
