// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EPF_NODE_TREE_H
#define _EPF_NODE_TREE_H

#include <cassert>
#include <iomanip>

#include "node.h"

namespace epf {

/* Manages the creation of a Node set with one ObjectNode root, and some child
 * nodes reference them. It also manages the indexing of the nodes of the tree*/
class NodeTree {
  protected:
    /* Default constructor*/
    NodeTree();

    /* Init with name */
    NodeTree(std::string name);

    /* Init with node */
    NodeTree(const ObjectNode &node);

    /* Copy constructor (with its references)*/
    NodeTree(const NodeTree &obj);

    /* Assignment operator */
    const NodeTree &operator=(const NodeTree &obj);

    /* Adds a new Node or NodeTree to the ObjectNode type root Node.*/
    int add(std::unique_ptr<epf::Node> node, epf::RefType reference_type,
            int parent_index = 0);

    /* Removes one leaf Node */
    void remove(int index);
    void remove(epf::Node *node);

    // Returns -1 if not found
    int32_t parentIndex(const epf::Node *node) const;

    std::vector<epf::Node *> node_list_;

    std::string name() const;

  public:
    void setName(std::string name);

    /* Retrives Node Index */
    int32_t nodeIndex(const epf::Node *node) const;

    const epf::ObjectNode &root() const;

    /**
     * @brief Accessing by index.
     * @param index
     */
    epf::Node *operator[](std::size_t index) const;

    /* Prints the NodeTree info */
    void print(int index = 0) const;

    int32_t length() const;

  private:
    void indentedPrint(int index, int indent) const;

    epf::ObjectNode root_node_;

    /* Get order equivalence of hierarchical walkthrough */
    std::vector<std::size_t> hierarchicalOrder() const;

    /** Adds node and its childs to the node_list_ index. Returns index assigned
     * to the parent node.*/
    int add2list(epf::Node *node);

    void computeHierarchicalOrder(std::vector<std::size_t> &order,
                                  const epf::Node *node) const;

    void removeFromList(epf::Node *node);
};

}  // namespace epf

#endif  // _EPF_NODE_TREE_H
