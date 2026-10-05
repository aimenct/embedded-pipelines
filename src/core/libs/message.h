// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef MESSAGE_H
#define MESSAGE_H

#include "node_tree.h"

namespace epf {

class Message : public NodeTree {
  public:
    Message()
        : NodeTree(){};

    /* Constructor with a NodeTree */
    Message(const ObjectNode &node,
            const std::vector<int32_t> streamed_nodes = {});

    /* Copy constructor */
    Message(const Message &msg);

    /* Retrive rootNode */
    const epf::ObjectNode &rootNode();

    /* Get serialized size of the messages */
    std::size_t itemCount() const;

    /* Get serialized size of the messages */
    std::size_t size() const;

    /* Get Node of each item with updated ptr by hierarchical index */
    epf::Node *item(const std::size_t item_index) const;

    /* Get Node of each item with updated ptr by hierarchical signed index */
    epf::Node *item(int32_t item_index) const;

    /* Get Node of each item with updated ptr by name - probably not unique
     */
    epf::Node *item(std::string name) const;

    /* Assignment operator */
    const Message &operator=(const Message &obj);

    /**
     * @brief Add new item
     * */
    void addItem(std::unique_ptr<epf::Node> node,
                 epf::RefType reference_type = epf::EP_HAS_CHILD);

    /**
     * @brief Get the byte offset of a streamed DataNode in the payload.
     *
     * @param node Streamed DataNode whose payload offset is requested.
     * */
    size_t streamedNodeOffset(const DataNode *node) const;

  private:
    std::vector<int32_t> streamed_nodes_;

  public:
    void updateMessage(char *message_pointer);

    std::size_t size_ = 0;
};

}  // namespace epf

#endif  // MESSAGE_H
