// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef MESSAGE_H
#define MESSAGE_H

#include "node_tree.h"

namespace ep {

class Message : public NodeTree {
  public:
    Message()
        : NodeTree(){};

    /* Constructor with a NodeTree */
    Message(const ep::ObjectNode* node);

    /* Copy constructor */
    Message(const Message& msg);

    /* Retrive rootNode */
    const ep::ObjectNode& rootNode();

    /* Get serialized size of the messages */
    std::size_t itemCount() const;

    /* Get serialized size of the messages */
    std::size_t size() const;

    /* Get Node of each item with updated ptr by hierarchical index */
    ep::Node2* item(const std::size_t item_index) const;
    /* Get Node of each item with updated ptr by name - probably not unique
     */
    ep::Node2* item(std::string name) const;

    /* Assignment operator */
    const Message& operator=(const Message& obj);

    /* Add new item - ¿Pending how to implement this? */
    void addItem(ep::Node2* node,
                 ep::RefType reference_type = ep::EP_HAS_CHILD);

  private:
    std::vector<ep::DataNode*> queue_node_list_;

  public:
    void updateMessage(char* message_pointer);

    std::size_t size_ = 0;
};

}  // namespace ep

#endif  // MESSAGE_H
