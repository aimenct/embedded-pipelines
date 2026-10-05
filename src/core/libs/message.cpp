// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "message.h"

using namespace epf;

Message::Message(const ObjectNode &node, std::vector<int32_t> streamed_nodes)
    : NodeTree(node),
      size_(0)
{
  for (auto index : streamed_nodes) {
    if (index >= 0 && index < length()) {
      Node *node = operator[](index);
      if (node->isDataNode()) {
        epf::DataNode *data_node = static_cast<epf::DataNode *>(node);
        if (!data_node->memMgmt()) {
          streamed_nodes_.push_back(index);
          size_ += data_node->size();
        }
      }
    }
  }
}

Message::Message(const Message &msg)
    : NodeTree(msg)
{
  *this = msg;
}

const epf::ObjectNode &Message::rootNode()
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

epf::Node *Message::item(const std::size_t item_index) const
{
  const auto &refs = root().references();

  if (item_index >= refs.size()) {
    std::cerr << "Message: Item index is out of bounds" << std::endl;
    return nullptr;
  }

  epf::Node *node = refs[item_index].address();
  if (!node) {
    std::cerr << "Message: Address returned a nullptr" << std::endl;
    return nullptr;
  }

  return node;
}

epf::Node *Message::item(int32_t item_index) const
{
  if (item_index < 0) {
    std::cerr << "Message: Item index is out of bounds" << std::endl;
    return nullptr;
  }

  return item(static_cast<std::size_t>(item_index));
}

epf::Node *Message::item(std::string name) const
{
  const auto &refs = root().references();

  for (const auto &ref : refs) {
    epf::Node *node = ref.address();
    if (!node) {
      std::cerr << "Message: Address returned a nullptr" << std::endl;
      continue;
    }

    if (node->name() == name) {
      return node;
    }
  }

  std::cerr << "Message: Item with name '" << name << "' not found"
            << std::endl;
  return nullptr;
}

const Message &Message::operator=(const Message &obj)
{
  NodeTree::operator=(obj);

  streamed_nodes_ = obj.streamed_nodes_;
  size_ = obj.size_;
  return *this;
}

void Message::addItem(std::unique_ptr<epf::Node> node,
                      epf::RefType reference_type)
{
  int32_t parent_index = add(std::move(node), reference_type);
  for (int32_t i = parent_index; i < length(); i++) {
    Node *added_node = operator[](i);
    if (added_node->isDataNode()) {
      epf::DataNode *data_node = static_cast<epf::DataNode *>(added_node);
      if (!data_node->memMgmt()) {
        streamed_nodes_.push_back(i);
        size_ += data_node->size();
      }
    }
  }
}

size_t Message::streamedNodeOffset(const DataNode *node) const
{
  int32_t offset = 0;
  for (auto index : this->streamed_nodes_) {
    // node->setPtrMsg(pointer);
    DataNode *data_node = static_cast<DataNode *>(operator[](index));

    if (data_node == node) {
      return offset;
    }
    offset += static_cast<int32_t>(data_node->size());
  }

  std::cerr << "[Message] Streamed node not found." << std::endl;

  return 0;
}

void Message::updateMessage(char *pointer)
{
  int32_t offset = 0;
  for (auto index : this->streamed_nodes_) {
    // node->setPtrMsg(pointer);
    DataNode *data_node = static_cast<DataNode *>(operator[](index));
    data_node->setValue(offset + pointer);
    offset += static_cast<int32_t>(data_node->size());
  }
}
