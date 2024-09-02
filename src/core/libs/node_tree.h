// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_NODE_TREE_H
#define _EP_NODE_TREE_H

#include <cassert>
#include <iomanip>

#include "node.h"

namespace ep {

/* Manages the creation of a Node set with one ObjectNode root, and some child
 * nodes reference them. It also manages the indexing of the nodes of the tree*/
class NodeTree {
  protected:
    /* Default constructor*/
    NodeTree()
    {
      add2list(&root_node_);
    };

    /* Init with name */
    NodeTree(std::string name);

    /* Init with node */
    NodeTree(const ep::ObjectNode* node);

    /* Copy constructor (with its references)*/
    NodeTree(const NodeTree& obj);

    /* Assignment operator */
    const NodeTree& operator=(const NodeTree& obj);

    /* Adds a new Node or NodeTree to the ObjectNode type root Node.*/
    int add(ep::Node2* node, ep::RefType reference_type, int parent_index = 0);

    /* Removes one leaf Node */
    void remove(int index);
    void remove(ep::Node2* node);

  protected:
    std::size_t parentIndex(const ep::Node2* node) const;

    std::vector<ep::Node2*> node_list_;

  public:
    /* Retrives Node Index */
    std::size_t nodeIndex(const ep::Node2* node) const;

    const std::string& name() const;

    const ep::ObjectNode& root() const;

    ep::Node2* operator[](std::size_t index) const;

    /* Prints the NodeTree info */
    void print(int index = 0) const;

    int32_t length() const;

  private:
    void indentedPrint(int index, int indent) const;
    std::string name_ = " ";
    ep::ObjectNode root_node_ = ep::ObjectNode(name_);

    /* Get order equivalence of hierarchical walkthrough */
    std::vector<std::size_t> hierarchicalOrder() const;

    /** Adds node and its childs to the node_list_ index. Returns index assigned
     * to the parent node.*/
    int add2list(ep::Node2* node);

    void computeHierarchicalOrder(std::vector<std::size_t>& order,
                                  const ep::Node2* node) const;

    void removeFromList(ep::Node2* node);
};

}  // namespace ep

#endif  // _EP_NODE_TREE_H
