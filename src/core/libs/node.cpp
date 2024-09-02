// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "node.h"

namespace ep {
Reference::Reference(ep::RefType type, ep::Node2* address)
    : address_(address)
{
  type_ = type;
}

ep::RefType Reference::type() const
{
  return type_;
}

Node2* Reference::address() const
{
  return address_.get();
}

Node2::Node2(const Node2& obj)
{
  *this = obj;
}

const ep::Node2& Node2::operator=(const ep::Node2& obj)
{
  references_.clear();

  name_ = obj.name_;
  nodetype_ = obj.nodetype_;
  tooltip_ = obj.tooltip_;
  visibility_ = obj.visibility_;

  return *this;
}

Node2::~Node2()
{
  references_.clear();
}

std::string Node2::name() const
{
  return name_;
}

ep::NodeType Node2::nodetype() const
{
  return nodetype_;
}

std::string Node2::tooltip() const
{
  return tooltip_;
}

ep::VisibilityType Node2::visibility() const
{
  return visibility_;
}

void Node2::addReference(ep::RefType referencetype, ep::Node2* address)
{
  Reference new_ref(referencetype, address);
  references_.push_back(std::move(new_ref));
}

void Node2::removeReference(std::size_t index)
{
  references_.erase(references_.begin() + index);
}

void Node2::removeReference(ep::Node2* node)
{
  for (std::size_t i = 0; i < references_.size(); i++) {
    if (references_[i].address() == node) {
      removeReference(i);
      return;
    }
  }
  throw;
}

const std::vector<ep::Reference>& Node2::references() const
{
  return references_;
}

void Node2::setTooltip(const std::string& b)
{
  tooltip_ = b;
}

void Node2::setVisibility(const ep::VisibilityType& visibility)
{
  visibility_ = visibility;
}

const int& Node2::id() const
{
  return id_;
}

void Node2::setId(const int id)
{
  id_ = id;
}

void Node2::setName(const std::string name)
{
  name_ = name;
}

void Node2::print() const
{
  std::cout << this->name_ << ": ";
  std::cout << "{nodetype: "
            << "(" << ep::nodetype_to_string(this->nodetype_) << ")}"
            << " ";
  std::cout << "{tooltip:" << this->tooltip_ << "}";
  std::cout << std::endl;
}

void visit_node(const ep::Node2* node, int& level)
{
  for (int i = 0; i < level; i++) std::cout << "\t";

  if (node->isDataNode()) {
    static_cast<const DataNode*>(node)->print();
  }
  else
    //    node->print(online, full);
    node->print();

  for (const Reference& ref : node->references()) {
    level = level + 1;
    //    visit_node(ref.address(), level, online, full);
    visit_node(ref.address(), level);
  }
  level = level - 1;
  return;
}

void Node2::printTree() const
{
  int level = 0;
  visit_node(this, level);
}

void Node2::copyNodeChilds(const ep::Node2& node)
{
  for (const Reference& ref : node.references()) {
    if (ref.address()->nodetype() == ep::EP_OBJECTNODE) {
      ep::ObjectNode* new_node = new ObjectNode();
      *new_node = *dynamic_cast<ep::ObjectNode*>(ref.address());
      addReference(ref.type(), new_node);
    }
    else if (ref.address()->nodetype() == ep::EP_STRINGNODE) {
      ep::StringNode* new_node = new StringNode();
      *new_node = *dynamic_cast<ep::StringNode*>(ref.address());
      addReference(ref.type(), new_node);
    }
    else if (ref.address()->nodetype() == ep::EP_DATANODE) {
      ep::DataNode* new_node = new DataNode();
      *new_node = *dynamic_cast<ep::DataNode*>(ref.address());
      addReference(ref.type(), new_node);
    }
    else if (ref.address()->nodetype() == ep::EP_COMMANDNODE) {
      ep::CommandNode* new_node = new CommandNode();
      *new_node = *dynamic_cast<ep::CommandNode*>(ref.address());
      addReference(ref.type(), new_node);
    }
  }
}

ObjectType ObjectNode::objecttype() const
{
  return objecttype_;
}

ObjectNode::ObjectNode(const ObjectNode& obj)
    : Node2(obj)
{
  *this = obj;
}

const ep::ObjectNode& ObjectNode::operator=(const ep::ObjectNode& obj)
{
  Node2::operator=(obj);
  objecttype_ = obj.objecttype_;
  copyNodeChilds(obj);

  return *this;
}

const std::string& StringNode::value() const
{
  return value_;
}

StringNode::StringNode(const StringNode& obj)
    : Node2(obj)
{
  *this = obj;
}

const ep::StringNode& StringNode::operator=(const ep::StringNode& obj)
{
  Node2::operator=(obj);

  value_ = obj.value_;

  copyNodeChilds(obj);

  return *this;
}

void StringNode::setValue(std::string value)
{
  value_ = value;
}

void StringNode::print() const
{
  std::cout << this->name() << ": ";
  std::cout << "{nodetype:" << ep::nodetype_to_string(this->nodetype()) << "} ";

  if (this->tooltip() != "") {
    std::cout << "{tooltip:" << this->tooltip() << "} ";
  }
  std::cout << "{value:" << this->value() << "} ";
  std::cout << "\n";
}

DataNode::DataNode(const DataNode& obj)
    : Node2(obj)
{
  streamed_ = 0;
  memory_mgmt_ = 0;
  value_ = nullptr;
  *this = obj;
}

const ep::DataNode& DataNode::operator=(const ep::DataNode& obj)
{
  Node2::operator=(obj);

  if ((this->memory_mgmt_ == 1) && (this->value_ != nullptr)) {
    // deb
    if (this->datatype_ == EP_STRING) {
      delete[](std::string*) value_;
    }
    else {
      delete[](char*) value_;
    }
    value_ = nullptr;
  }

  datatype_ = obj.datatype_;
  value_ = obj.value_;
  arraydimensions_ = obj.arraydimensions_;
  rank_ = obj.rank_;
  accessmode_ = obj.accessmode_;
  size_ = obj.size_;
  elements_ = obj.elements_;

  streamed_ = obj.streamed_;
  offset_ = obj.offset_;
  ptr_msg_ = obj.ptr_msg_;
  memory_mgmt_ = obj.memory_mgmt_;

  if (obj.memory_mgmt_) {
    if (datatype_ == EP_STRING) {
      value_ = (void*)new std::string[elements_];
      for (int i = 0; i < elements_; i++) {
        ((std::string*)value_)[i] = ((std::string*)obj.value_)[i];
      }
    }
    else {
      value_ = new char[size_];
      memcpy(this->value_, obj.value_, this->size_);
    }
  }

  copyNodeChilds(obj);

  return *this;
}

void* DataNode::value() const
{
  return value_;
}

ep::BaseType DataNode::datatype() const
{
  return datatype_;
}

ep::AccessType DataNode::accessMode() const
{
  return accessmode_;
}

void DataNode::setDatatype(ep::BaseType datatype)
{
  datatype_ = datatype;
}

std::size_t DataNode::size() const
{
  return size_;
}

int32_t DataNode::rank() const
{
  return rank_;
}

int DataNode::arrayelements() const
{
  return elements_;
}

std::vector<size_t> DataNode::arraydimensions() const
{
  return arraydimensions_;
}

void DataNode::setValue(void* invalue)
{
  value_ = invalue;
}

void DataNode::setSize(size_t sizein)
{
  size_ = sizein;
}

void DataNode::setAccessMode(ep::AccessType c)
{
  accessmode_ = c;
}
void DataNode::setRank(int rank)
{
  rank_ = rank;
}

void DataNode::print() const
{
  std::cout << this->name() << ": ";

  // Print node type
  std::cout << "{nodetype: " << ep::nodetype_to_string(this->nodetype())
            << "} ";

  // Print data type
  std::cout << "{datatype: " << ep::basetype_to_string(this->datatype())
            << "} ";

  // Print tooltip if it is not empty
  if (!this->tooltip().empty()) {
    std::cout << "{tooltip: " << this->tooltip() << "} ";
  }

  // Print if streamed
  if (streamed_) {
    std::cout << "{streamed} ";
  }

  // Print access mode
  if (this->accessmode_ != 0) {
    std::cout << "{accessmode: " << ep::accesstype_to_string(this->accessmode_)
              << "} ";
  }

  if (elements_ == 1)
    std::cout << "{scalar} ";
  else {
    std::cout << "{dim: [";
    for (size_t i = 0; i < arraydimensions_.size(); ++i) {
      std::cout << arraydimensions_[i];
      if (i < arraydimensions_.size() - 1) {
        std::cout << "][";
      }
    }
    std::cout << "]} ";
  }

  // Print value based on datatype
  if (this->value_) {
    std::cout << "{value: ";

    // Print array values
    if (this->rank() > 0) {
      std::cout << "[";

      size_t total_elements = this->arrayelements();

      // Determine how many elements to print
      size_t print_limit = std::min(
          total_elements,
          static_cast<size_t>(10));  // Print first 10 or fewer elements

      size_t printed = 0;
      for (size_t i = 0; i < total_elements; ++i) {
        if (i > 0) std::cout << ", ";
        if (printed >= print_limit) {
          std::cout << "...";  // Indicate that not all elements are printed
          break;
        }

        // Access and print value based on datatype
        switch (this->datatype()) {
          case EP_BOOL:
            std::cout << static_cast<bool*>(this->value_)[i];
            break;
          case EP_8C:
            std::cout << static_cast<char*>(this->value_)[i];
            break;
          case EP_8S:
            std::cout << static_cast<int8_t*>(this->value_)[i];
            break;
          case EP_8U:
            std::cout << static_cast<uint8_t*>(this->value_)[i];
            break;
          case EP_16S:
            std::cout << static_cast<int16_t*>(this->value_)[i];
            break;
          case EP_16U:
            std::cout << static_cast<uint16_t*>(this->value_)[i];
            break;
          case EP_32S:
            std::cout << static_cast<int32_t*>(this->value_)[i];
            break;
          case EP_32U:
            std::cout << static_cast<uint32_t*>(this->value_)[i];
            break;
          case EP_64S:
            std::cout << static_cast<int64_t*>(this->value_)[i];
            break;
          case EP_64U:
            std::cout << static_cast<uint64_t*>(this->value_)[i];
            break;
          case EP_32F:
            std::cout << static_cast<float*>(this->value_)[i];
            break;
          case EP_64F:
            std::cout << static_cast<double*>(this->value_)[i];
            break;
          case EP_STRING:
            std::cout << static_cast<std::string*>(this->value_)[i];
            break;
          default:
            std::cout << "NULL";
            break;
        }
        ++printed;
      }

      std::cout << "]";
    }
    else {
      // Print single value for non-array types
      switch (this->datatype()) {
        case EP_BOOL:
          std::cout << *static_cast<bool*>(this->value_);
          break;
        case EP_8C:
          std::cout << *static_cast<char*>(this->value_);
          break;
        case EP_8S:
          std::cout << *static_cast<int8_t*>(this->value_);
          break;
        case EP_8U:
          std::cout << *static_cast<uint8_t*>(this->value_);
          break;
        case EP_16S:
          std::cout << *static_cast<int16_t*>(this->value_);
          break;
        case EP_16U:
          std::cout << *static_cast<uint16_t*>(this->value_);
          break;
        case EP_32S:
          std::cout << *static_cast<int32_t*>(this->value_);
          break;
        case EP_32U:
          std::cout << *static_cast<uint32_t*>(this->value_);
          break;
        case EP_64S:
          std::cout << *static_cast<int64_t*>(this->value_);
          break;
        case EP_64U:
          std::cout << *static_cast<uint64_t*>(this->value_);
          break;
        case EP_32F:
          std::cout << *static_cast<float*>(this->value_);
          break;
        case EP_64F:
          std::cout << *static_cast<double*>(this->value_);
          break;
        case EP_STRING:
          std::cout << *static_cast<std::string*>(this->value_);
          break;
        default:
          std::cout << "NULL";
          break;
      }
    }
    std::cout << "} ";
  }
  else
    std::cout << "{value:} ";
  std::cout << std::endl;
}

CommandNode::CommandNode(const CommandNode& obj)
    : Node2(obj)
{
  *this = obj;
}

const ep::CommandNode& CommandNode::operator=(const ep::CommandNode& obj)
{
  Node2::operator=(obj);

  command_ = obj.command_;

  copyNodeChilds(obj);

  return *this;
}

int32_t CommandNode::run() const
{
  return command_();
}

AccessType CommandNode::accessMode() const
{
  return accessmode_;
}

}  // namespace ep
