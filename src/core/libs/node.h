// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef NODE_H
#define NODE_H

#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "epf_types.h"

namespace epf {
class Node;

/** @brief Class to describe references between Nodes.
 *
 *  It describes the type of the reference and indicates to
 *  which Node is directed.
 */
class Reference {
  public:
    /** @brief Constructor
     *
     *  @param type Indicates what type of reference it is.
     *  @param address Indicates the address of the Node to which.
     *  it is directed
     */
    Reference(epf::RefType type, std::unique_ptr<epf::Node> address);

    /** @brief Function that returns the type of the reference.
     *  @return a integer associated with type.
     */
    epf::RefType type() const;

    /** @brief Function that returns the address of the Node to which it is
     * directed
     *  @returns a pointer with the address of the Node.
     */
    epf::Node *address() const;

  private:
    epf::RefType type_;
    std::unique_ptr<epf::Node> address_;
};

/**
 * @brief Class to describe properties of memory locations.
 *
 * Node class is an universal class to represent the properties o one or
 * more memory locations in the system with the ability to relate them with
 * a tree-like hierarchical dependence. It can be easily mapped to represent
 * a simple element list, a Genicam tree description of a camera, an Asset
 * Administration Shell, an OPC-UA server, a Python dictionary, etc.
 */
class Node {
  protected:
    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param nodetype: type of the node described by a type constant from
     * epf_types.h.
     * @param tooltip provides a functional description of the node
     */
    Node(std::string name, epf::NodeType nodetype, std::string tooltip = "",
         AccessType access_mode = epf::W)
        :  // id_(-1),
          name_(name),
          nodetype_(nodetype),
          tooltip_(tooltip),
          access_mode_(access_mode),
          visibility_(EP_BEGINNER){};

    /**
     * @brief Copy constructor
     */
    Node(const Node &obj);

    epf::Node &operator=(const epf::Node &obj);

  public:
    /**
     * @brief Destructor
     */
    virtual ~Node();

    // /** @brief Returns the name of the Node
    //  *  @return std::string name */
    // const int& id() const;

    /** @brief Returns the name of the Node
     *  @return std::string name */
    std::string name() const;

    /** @brief Return the type of the Node
     *  @return type from epf_types.h */
    epf::NodeType nodetype() const;

    /** @brief Returns the tooltip associated with the parameter
     *  @return std::string of the tooltip parameter */
    std::string tooltip() const;

    /** @brief Returns the visibility associated with the parameter
     *  @return epf::type associated with the visibility
     */
    epf::VisibilityType visibility() const;

    /**
     *
     */
    epf::AccessType accessMode() const;

    /** @brief Adds a new reference in the node
     *  @param referencetype specifies the type of the reference
     *  @param value specifies the receptor of the reference */
    void addReference(epf::RefType referencetype,
                      std::unique_ptr<epf::Node> value);

    void removeReference(std::size_t index);

    void removeReference(epf::Node *node);

    /** @brief Returns the references that the Node has
     *  @return a copy of the vector that contains the references of the Node
     */
    const std::vector<epf::Reference> &references() const;

    /** @brief Set the tooltip associated with the parameter
     *  @param string string associated with the tooltip */
    void setTooltip(const std::string &tooltip);

    /** @brief Set the visibility asociated with the parameter
     *  @param visibility type associated with the visibility
     */
    void setVisibility(const epf::VisibilityType &visibility);

    /** @brief Set the accessmode of the node
     *  @param accessmode char of the type of accessmode */
    void setAccessMode(epf::AccessType access_mode);

    // void setId(const int id);

    void setName(const std::string name);

    /** @brief Returns all the variables that the Node has
     *  @param oneline: If is it in true all the variables are printed in
     * oneline, otherwise the variables are printed with line breaks
     *  @param full If is it in true only the variables name and value are
     * printed, otherwise are printed all the variables available.
     *  @return A print of the actual variables of the Node.*/
    virtual void print() const;

    /** @brief Prints a tree of the actual Node with his childs
     */
    virtual void printTree() const;

    int isDataNode() const
    {
      if (nodetype_ == EP_DATANODE)
        return 1;
      else
        return 0;
    }
    int isObjectNode() const
    {
      if (nodetype_ == EP_OBJECTNODE)
        return 1;
      else
        return 0;
    }
    int isStringNode() const
    {
      if (nodetype_ == EP_STRINGNODE)
        return 1;
      else
        return 0;
    }
    int isCommandNode() const
    {
      if (nodetype_ == EP_COMMANDNODE)
        return 1;
      else
        return 0;
    }

  protected:
    void printNodeTreeChild(int x, bool full) const;

    void copyNodeChilds(const epf::Node &node);

  private:
    // int32_t id_;
    std::string name_;
    epf::NodeType nodetype_;
    std::string tooltip_ = "";
    std::vector<epf::Reference> references_;
    epf::AccessType access_mode_{epf::AccessType::W};
    epf::VisibilityType visibility_{epf::EP_BEGINNER};
};

/** @brief Derived class from the universal class Node. This derived class is
 *  intended to contain other different Nodes.
 */
class ObjectNode : public Node {
  private:
    epf::ObjectType objecttype_;

  public:
    ObjectNode(epf::ObjectType objecttype = EP_OBJ)
        : Node("unamed", EP_OBJECTNODE, "", epf::R),
          objecttype_(objecttype){};

    ObjectNode(std::string name, epf::ObjectType objecttype = EP_OBJ,
               std::string tooltip = "")
        : Node(name, EP_OBJECTNODE, tooltip, epf::R),
          objecttype_(objecttype){};

    ObjectNode(const ObjectNode &obj);

    epf::ObjectNode &operator=(const epf::ObjectNode &obj);

    ~ObjectNode(){};

    epf::ObjectType objecttype() const;
};

/** @brief Derived class from the universal class Node. This derived class is
 *  intended to contain strings as datatypes
 */
class StringNode : public Node {
  public:
    /**
     * @brief Default constructor with "unamed" name key.
     */
    StringNode()
        : Node("unamed", EP_STRINGNODE){};

    /**
     * @brief Constructor with custom name key.
     *
     * @param name: name of the node.
     */
    StringNode(std::string name)
        : Node(name, EP_STRINGNODE){};

    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param value: address of the value being described. The user is
     * responsible for managing this memory.
     * @param tooltip: description of the node functionality/information
     * containment.
     */
    StringNode(std::string name, std::string *value, AccessType access_mode = W,
               std::string tooltip = "")
        : Node(name, EP_STRINGNODE, tooltip, access_mode),
          value_(value){};

    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param value: address of the value being described.
     * @param tooltip: description of the node functionality/information
     * containment.
     */
    StringNode(std::string name, std::string value, AccessType access_mode = W,
               std::string tooltip = "")
        : Node(name, EP_STRINGNODE, tooltip, access_mode),
          managed_data_(value),
          value_(&managed_data_){};

    StringNode(const StringNode &obj);

    /** @brief Returns the access mode that is assigned
     *  @return a char of the current access mode */
    epf::AccessType accessMode() const;

    epf::StringNode &operator=(const epf::StringNode &obj);

    /** @brief Returns the current value in the Node
     *  @return value that was assigned in the Node */
    std::string *value() const;
    void setValue(std::string *value);

    void print() const;

  private:
    void copyFrom(const StringNode &obj);
    std::string managed_data_{};
    std::string *value_;
};

/** @brief Derived class from the universal class Node. This derived class is
 * intended to contain diferents types of data as datatypes.
 */
class DataNode : public Node {
  public:
    DataNode()
        : Node("unamed", EP_DATANODE)
    {
      // streamed_ = false;
      value_ = nullptr;
    };

    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param type: type of the node described by a type constant from
     * epf_types.h.
     * @param value: address of the value being described. The user is
     * responsible for managing this memory.
     * @param arraydim Indicates the dimensions of the array.
     * @param accessmode Describes the access mode of the Node. Usually
     * indicates if is writteable or readable
     */
    DataNode(std::string name, epf::BaseType datatype,
             std::vector<size_t> arraydim, void *value, bool streamed = false,
             std::string tooltip = "", epf::AccessType access_mode = W);

    // Regular memory managed DataNode
    DataNode(std::string name, epf::BaseType datatype,
             std::vector<size_t> arraydim = {}, std::string tooltip = "",
             epf::AccessType access_mode = W)
        : DataNode(name, datatype, arraydim, nullptr, true, tooltip,
                   access_mode){};

    /**
     * @brief Copy constructor
     */
    DataNode(const DataNode &obj);

    /**
     * @brief Assignment operator constructor.
     */
    epf::DataNode &operator=(const epf::DataNode &obj);

    /**
     * @brief Destructor.
     */
    ~DataNode(){};

    /** @brief Returns the current value in the Node
     *  @return value that was assigned in the Node */
    void *value() const;

    epf::BaseType datatype() const;

    /** @brief Returns the size that will be associated in the memory
     *  @return Integer of the size parameter */
    std::size_t size() const;

    /** @brief Returns the rank associated with the data */
    int rank() const;

    /** @brief Returns the number of elements in the array */
    size_t arrayelements() const;

    /** @brief Returns the array dimension associated with the data*/
    std::vector<std::size_t> arraydimensions() const;

    /** @brief Set the value of the DataNode
     *  @param value that will be allocated in the DataNode
     */
    void setValue(void *invalue);

    /** @brief Set the size that will be used in memory
     *  @param sizein size in size_t */
    void setSize(size_t sizein);

    /** @brief Set the datatype of the node
     *  @param datatype type of the storedge data
     */
    void setDatatype(epf::BaseType datatype);

    /** @brief Set the rank of the node
     *  @param rank indicates if the data is an scalar or an array
     */
    void setRank(int rank);

    /** @brief Write (mem copy) value to the DataNode value pointer
     *  @param value that will be copied in the DataNode
     */
    int32_t write(const void *value);

    /** @brief Read (mem copy) value from DataNode to value pointer
     *  @param value where DataNode value will be copied
     */
    int32_t read(void *value) const;

    void print() const;

    // bool isStreamed() const;

    bool memMgmt() const;

  private:
    void copyFrom(const DataNode &obj);
    void *value_{nullptr};
    std::unique_ptr<char[]> managed_data_{nullptr};
    epf::BaseType datatype_{epf::EP_8C};
    std::vector<size_t> arraydimensions_{};
    size_t size_{0};
    int32_t rank_{0};
    size_t elements_{0};
    // bool streamed_{false};
};

/** @brief Derived class from the universal class Node. This Node is
 * intended to manage the Nodes that needs callbacks
 */
class CommandNode : public Node {
  public:
    CommandNode()
        : Node("unnamed_command()", EP_COMMANDNODE, "", epf::W),
          command_([]() { return 0; })
    {
    }  // Default lambda

    CommandNode(std::string name, std::function<int()> command,
                std::string tooltip = "", AccessType access_mode = W)
        : Node(name, EP_COMMANDNODE, tooltip, access_mode),
          command_(std::move(command))
    {
    }

    /**
     * @brief Copy constructor
     */
    CommandNode(const CommandNode &obj);

    /**
     * @brief Assignment operator constructor.
     */
    const epf::CommandNode &operator=(const epf::CommandNode &obj);

    int32_t run() const;

  private:
    std::function<int()> command_;
};

}  // namespace epf

#endif  // NODE_H
