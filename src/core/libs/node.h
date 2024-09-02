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

#include "ep_types.h"

namespace ep {
class Node2;

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
    Reference(ep::RefType type, ep::Node2* address);

    /** @brief Function that returns the type of the reference.
     *  @return a integer associated with type.
     */
    ep::RefType type() const;

    /** @brief Function that returns the address of the Node to which it is
     * directed
     *  @returns a pointer with the address of the Node.
     */
    ep::Node2* address() const;

  private:
    ep::RefType type_;
    std::unique_ptr<ep::Node2> address_;
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
class Node2 {
  protected:
    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param nodetype: type of the node described by a type constant from
     * ep_types.h.
     * @param tooltip provides a functional description of the node
     */
    Node2(std::string name, ep::NodeType nodetype, std::string tooltip = "")
        : id_(-1),
          name_(name),
          nodetype_(nodetype),
          tooltip_(tooltip)
    {
      visibility_ = EP_BEGINNER;
    };

    /**
     * @brief Copy constructor
     */
    Node2(const Node2& obj);

    const ep::Node2& operator=(const ep::Node2& obj);

  public:
    /**
     * @brief Destructor
     */
    virtual ~Node2();

    /** @brief Returns the name of the Node
     *  @return std::string name */
    const int& id() const;

    /** @brief Returns the name of the Node
     *  @return std::string name */
    std::string name() const;

    /** @brief Return the type of the Node
     *  @return type from ep_types.h */
    ep::NodeType nodetype() const;

    /** @brief Returns the tooltip associated with the parameter
     *  @return std::string of the tooltip parameter */
    std::string tooltip() const;

    /** @brief Returns the visibility associated with the parameter
     *  @return ep::type associated with the visibility
     */
    ep::VisibilityType visibility() const;

    /** @brief Adds a new reference in the node
     *  @param referencetype specifies the type of the reference
     *  @param value specifies the receptor of the reference */
    void addReference(ep::RefType referencetype, ep::Node2* value);

    void removeReference(std::size_t index);

    void removeReference(ep::Node2* node);

    /** @brief Returns the references that the Node has
     *  @return a copy of the vector that contains the references of the Node
     */
    const std::vector<ep::Reference>& references() const;

    /** @brief Set the tooltip associated with the parameter
     *  @param string string associated with the tooltip */
    void setTooltip(const std::string& tooltip);

    /** @brief Set the visibility asociated with the parameter
     *  @param visibility type associated with the visibility
     */
    void setVisibility(const ep::VisibilityType& visibility);

    void setId(const int id);

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

  protected:
    void printNodeTreeChild(int x, bool full) const;

    void copyNodeChilds(const ep::Node2& node);

  private:
    int32_t id_;
    std::string name_;
    ep::NodeType nodetype_;
    std::string tooltip_ = "";
    std::vector<ep::Reference> references_;
    ep::VisibilityType visibility_;
};

/** @brief Derived class from the universal class Node. This derived class is
 *  intended to contain other different Nodes.
 */
class ObjectNode : public Node2 {
  private:
    ep::ObjectType objecttype_;

  public:
    ObjectNode(ep::ObjectType objecttype = EP_OBJ)
        : Node2("unamed", EP_OBJECTNODE),
          objecttype_(objecttype){};

    ObjectNode(std::string name, ep::ObjectType objecttype = EP_OBJ,
               std::string tooltip = "")
        : Node2(name, EP_OBJECTNODE, tooltip),
          objecttype_(objecttype){};

    ObjectNode(const ObjectNode& obj);

    const ep::ObjectNode& operator=(const ep::ObjectNode& obj);

    ~ObjectNode(){};

    ep::ObjectType objecttype() const;
};

/** @brief Derived class from the universal class Node. This derived class is
 *  intended to contain strings as datatypes
 */
class StringNode : public Node2 {
  public:
    /**
     * @brief Default constructor with "unamed" name key.
     */
    StringNode()
        : Node2("unamed", EP_STRINGNODE){};

    /**
     * @brief Constructor with custom name key.
     *
     * @param name: name of the node.
     */
    StringNode(std::string name)
        : Node2(name, EP_STRINGNODE){};

    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param value: address of the value being described. The user is
     * responsible for managing this memory.
     */
    StringNode(std::string name, std::string value, std::string tooltip = "")
        : Node2(name, EP_STRINGNODE, tooltip),
          value_(value){};

    StringNode(const StringNode& obj);

    const ep::StringNode& operator=(const ep::StringNode& obj);

    /** @brief Returns the current value in the Node
     *  @return value that was assigned in the Node */
    const std::string& value() const;
    void setValue(std::string value);

    void print() const;

  private:
    std::string value_;
};

/** @brief Derived class from the universal class Node. This derived class is
 * intended to contain diferents types of data as datatypes.
 */
class DataNode : public Node2 {
  public:
    DataNode()
        : Node2("unamed", EP_DATANODE)
    {
      streamed_ = 0;
      memory_mgmt_ = 0;
      value_ = nullptr;
    };

    /**
     * @brief Constructor
     *
     * @param name: name of the node.
     * @param type: type of the node described by a type constant from
     * ep_types.h.
     * @param value: address of the value being described. The user is
     * responsible for managing this memory.
     * @param arraydim Indicates the dimensions of the array.
     * @param accessmode Describes the access mode of the Node. Usually
     * indicates if is writteable or readable
     */
    DataNode(std::string name, ep::BaseType datatype,
             std::vector<size_t> arraydim, void* value, bool memory_managed,
             std::string tooltip = "", bool streamed = false,
             ep::AccessType accessmode = W)
        : Node2(name, EP_DATANODE, tooltip),
          datatype_(datatype),
          arraydimensions_(arraydim),
          memory_mgmt_(memory_managed),
          streamed_(streamed),
          accessmode_(accessmode)
    {
      assert(!(memory_managed && streamed));
      assert(!(streamed && (datatype_ == EP_STRING)));

      rank_ = 0;
      size_ = type_size(datatype);
      elements_ = 1;
      for (size_t dim : arraydimensions_) {
        if (dim > 0) {
          rank_++;
          size_ *= dim;
          elements_ *= static_cast<int32_t>(dim);
        }
      }

      if (rank_ == 0) {
        size_ = 0;
        elements_ = 0;
      }

      if (memory_managed) {
        if (datatype == EP_STRING) {
          value_ = (void*)new std::string[elements_];
          if (value != nullptr) {  // Deep copy of std::string
            for (int i = 0; i < elements_; i++) {
              ((std::string*)value_)[i] = ((std::string*)value)[i];
            }
          }
        }
        else {
          value_ = (void*)new char[size_];
          if (value != nullptr) {
            memcpy(value_, value, size_);
          }
        }
      }
      else {
        value_ = value;
      }
    }

    // Regular memory managed DataNode
    DataNode(std::string name, ep::BaseType datatype,
             std::vector<size_t> arraydim = {}, std::string tooltip = "",
             ep::AccessType accessmode = W)
        : DataNode(name, datatype, arraydim, nullptr, true, tooltip, false,
                   accessmode)
    {
      memset(value_, 0, size_);
      offset_ = 0;
      ptr_msg_ = nullptr;
    };

    // Data Node can be static or streamed.
    // if "value" =  nullptr stramed, otherwise static
    DataNode(std::string name, ep::BaseType datatype,
             std::vector<size_t> arraydim, void* value,
             std::string tooltip = "", ep::AccessType accessmode = W)
        : DataNode(name, datatype, arraydim, value, false, tooltip,
                   value == nullptr, accessmode)
    {
      offset_ = 0;
      ptr_msg_ = nullptr;
    };

    /**
     * @brief Copy constructor
     */
    DataNode(const DataNode& obj);

    /**
     * @brief Assignment operator constructor.
     */
    const ep::DataNode& operator=(const ep::DataNode& obj);

    ~DataNode()
    {
      if (memory_mgmt_) {
        if (value_ != nullptr) {
          if (datatype_ == EP_STRING) {
            delete[](std::string*) value_;
            value_ = nullptr;
          }
          else {
            delete[](char*) value_;
            value_ = nullptr;
          }
        }
      }
    };

    /** @brief Returns the current value in the Node
     *  @return value that was assigned in the Node */
    void* value() const;

    ep::BaseType datatype() const;

    /** @brief Returns the access mode that is assigned
     *  @return a char of the current access mode */
    ep::AccessType accessMode() const;

    /** @brief Returns the size that will be associated in the memory
     *  @return Integer of the size parameter */
    std::size_t size() const;

    /** @brief Returns the rank associated with the data */
    int rank() const;

    /** @brief Returns the number of elements in the array */
    int arrayelements() const;

    /** @brief Returns the array dimension associated with the data*/
    std::vector<std::size_t> arraydimensions() const;

    /** @brief Set the value of the DataNode
     *  @param value that will be allocated in the DataNode
     */
    void setValue(void* invalue);

    /** @brief Set the size that will be used in memory
     *  @param sizein size in size_t */
    void setSize(size_t sizein);

    /** @brief Set the accessmode of the node
     *  @param accessmode char of the type of accessmode */
    void setAccessMode(ep::AccessType accessmode);

    /** @brief Set the datatype of the node
     *  @param datatype type of the storedge data
     */
    void setDatatype(ep::BaseType datatype);

    /** @brief Set the rank of the node
     *  @param rank indicates if the data is an scalar or an array
     */
    void setRank(int rank);

    /** @brief Write (mem copy) value to the DataNode value pointer
     *  @param value that will be copied in the DataNode
     */
    void write(void* value)
    {
      if ((this->value_ == nullptr) || (value == nullptr)) {
        std::cerr << "DataNode write error value == nullptr \n" << std::endl;
        exit(-1);
      }

      if (this->datatype_ == EP_STRING)
        for (int i = 0; i < elements_; i++)
          ((std::string*)value_)[i] = ((std::string*)value)[i];
      else
        memcpy(this->value_, value, this->size_);
    }

    /** @brief Read (mem copy) value from DataNode to value pointer
     *  @param value where DataNode value will be copied
     */
    void read(void* value)
    {
      if ((this->value_ == nullptr) || (value == nullptr)) {
        std::cerr << "DataNode read error value == nullptr \n" << std::endl;
        exit(-1);
      }

      if (this->datatype_ == EP_STRING)
        for (int i = 0; i < elements_; i++)
          ((std::string*)value)[i] = ((std::string*)value_)[i];
      else
        memcpy(value, this->value_, this->size_);
    }

    void print() const;

    size_t offset() const
    {
      return offset_;
    };
    void setOffset(size_t offset)
    {
      offset_ = offset;
    };

    void* ptrMsg() const
    {
      return ptr_msg_;
    };

    void setPtrMsg(void* ptr_msg)
    {
      ptr_msg_ = ptr_msg;
    };

    bool isStreamed()
    {
      return streamed_;
    }

    bool memMgmt()
    {
      return memory_mgmt_;
    }

  private:
    ep::BaseType datatype_ = ep::EP_8C;
    std::vector<size_t> arraydimensions_;
    void* value_ = nullptr;
    int32_t rank_ = static_cast<int32_t>(arraydimensions_.size());
    int elements_;
    bool memory_mgmt_;
    bool streamed_;
    ep::AccessType accessmode_ = ep::W;

    size_t size_ = 0;
    size_t offset_;
    void* ptr_msg_;
};

/** @brief Derived class from the universal class Node. This Node is
 * intended to manage the Nodes that needs callbacks
 */
class CommandNode : public Node2 {
  public:
    CommandNode()
        : Node2("unnamed_command()", EP_COMMANDNODE){};

    CommandNode(std::string name, std::function<int()> command,
                std::string tooltip = "", AccessType accessmode = W)
        : Node2(name, EP_COMMANDNODE, tooltip)
    {
      command_ = command;
      accessmode_ = accessmode;
    };

    /**
     * @brief Copy constructor
     */
    CommandNode(const CommandNode& obj);

    /**
     * @brief Assignment operator constructor.
     */
    const ep::CommandNode& operator=(const ep::CommandNode& obj);

    /** @brief Returns the access mode that is assigned
     *  @return a char of the current access mode */
    ep::AccessType accessMode() const;

    int32_t run() const;

  private:
    std::function<int()> command_;
    ep::AccessType accessmode_ = W;
};

}  // namespace ep

#endif  // NODE_H
