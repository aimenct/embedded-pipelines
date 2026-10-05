// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "node.h"

namespace epf {

Reference::Reference(epf::RefType type, std::unique_ptr<epf::Node> address)
    : type_(type),
      address_(std::move(address))
{
}

epf::RefType Reference::type() const
{
  return type_;
}

Node *Reference::address() const
{
  return address_.get();
}

Node::Node(const Node &obj)
    : name_(obj.name_),
      nodetype_(obj.nodetype_),
      tooltip_(obj.tooltip_),
      access_mode_(obj.access_mode_),
      visibility_(obj.visibility_)

{
  // copyNodeChilds(obj);
}

epf::Node &Node::operator=(const epf::Node &obj)
{
  if (this == &obj) {
    return *this;
  }

  references_.clear();

  name_ = obj.name_;
  nodetype_ = obj.nodetype_;
  tooltip_ = obj.tooltip_;
  visibility_ = obj.visibility_;
  access_mode_ = obj.access_mode_;

  // copyNodeChilds(obj);

  return *this;
}

Node::~Node()
{
  references_.clear();
}

std::string Node::name() const
{
  return name_;
}

epf::NodeType Node::nodetype() const
{
  return nodetype_;
}

std::string Node::tooltip() const
{
  return tooltip_;
}

epf::VisibilityType Node::visibility() const
{
  return visibility_;
}

epf::AccessType Node::accessMode() const
{
  return access_mode_;
}

void Node::addReference(epf::RefType referencetype,
                        std::unique_ptr<epf::Node> address)
{
  Reference new_ref(referencetype, std::move(address));
  references_.push_back(std::move(new_ref));
}

void Node::removeReference(std::size_t index)
{
  auto it = references_.begin();
  std::advance(it, index);
  references_.erase(it);
}

void Node::removeReference(epf::Node *node)
{
  for (std::size_t i = 0; i < references_.size(); i++) {
    if (references_[i].address() == node) {
      removeReference(i);
      return;
    }
  }
  throw;
}

const std::vector<epf::Reference> &Node::references() const
{
  return references_;
}

void Node::setTooltip(const std::string &b)
{
  tooltip_ = b;
}

void Node::setVisibility(const epf::VisibilityType &visibility)
{
  visibility_ = visibility;
}

void Node::setAccessMode(epf::AccessType access_mode)
{
  access_mode_ = access_mode;
}

void Node::setName(const std::string name)
{
  name_ = name;
}

void Node::print() const
{
  std::cout << this->name_ << ": ";
  std::cout << "{nodetype: " << "(" << epf::nodetype_to_string(this->nodetype_)
            << ")}" << " ";
  std::cout << "{tooltip:" << this->tooltip_ << "}";
  std::cout << std::endl;
}

void visit_node(const epf::Node *node, int &level)
{
  for (int i = 0; i < level; i++) std::cout << "\t";

  if (node->isDataNode()) {
    static_cast<const DataNode *>(node)->print();
  }
  else {
    node->print();
  }

  for (const Reference &ref : node->references()) {
    level = level + 1;
    //    visit_node(ref.address(), level, online, full);
    visit_node(ref.address(), level);
  }
  level = level - 1;
  return;
}

void Node::printTree() const
{
  int level = 0;
  visit_node(this, level);
}

void Node::copyNodeChilds(const epf::Node &node)
{
  for (const Reference &ref : node.references()) {
    switch (ref.address()->nodetype()) {
      case epf::EP_OBJECTNODE: {
        std::unique_ptr<ObjectNode> new_node = std::make_unique<ObjectNode>();
        *new_node = *dynamic_cast<epf::ObjectNode *>(ref.address());
        addReference(ref.type(), std::move(new_node));
        break;
      }
      case epf::EP_STRINGNODE: {
        std::unique_ptr<StringNode> new_node = std::make_unique<StringNode>();
        *new_node = *dynamic_cast<epf::StringNode *>(ref.address());
        addReference(ref.type(), std::move(new_node));
        break;
      }
      case epf::EP_DATANODE: {
        std::unique_ptr<DataNode> new_node = std::make_unique<DataNode>();
        *new_node = *dynamic_cast<epf::DataNode *>(ref.address());
        addReference(ref.type(), std::move(new_node));
        break;
      }
      case epf::EP_COMMANDNODE: {
        std::unique_ptr<CommandNode> new_node = std::make_unique<CommandNode>();
        *new_node = *dynamic_cast<epf::CommandNode *>(ref.address());
        addReference(ref.type(), std::move(new_node));
        break;
      }
    }
  }
}

ObjectType ObjectNode::objecttype() const
{
  return objecttype_;
}

ObjectNode::ObjectNode(const ObjectNode &obj)
    : Node(obj),
      objecttype_(obj.objecttype_)
{
  copyNodeChilds(obj);
}

epf::ObjectNode &ObjectNode::operator=(const epf::ObjectNode &obj)
{
  if (this == &obj) {
    return *this;
  }

  Node::operator=(obj);
  objecttype_ = obj.objecttype_;
  copyNodeChilds(obj);

  return *this;
}

std::string *StringNode::value() const
{
  return value_;
}

StringNode::StringNode(const StringNode &obj)
    : Node(obj),  // Base copy constructor
      managed_data_(),
      value_(nullptr)
{
  copyFrom(obj);
}

StringNode &StringNode::operator=(const StringNode &obj)
{
  if (this == &obj) {
    return *this;
  }

  Node::operator=(obj);  // Base assignment operator
  copyFrom(obj);

  return *this;
}

void StringNode::copyFrom(const StringNode &obj)
{
  if (obj.value_ == &obj.managed_data_) {
    managed_data_ = obj.managed_data_;
    value_ = &managed_data_;
  }
  else {
    managed_data_.clear();
    value_ = obj.value_;  // Intentional shallow copy
  }

  copyNodeChilds(obj);
}

void StringNode::setValue(std::string *value)
{
  //  if (!memMgmt()) {
  if (value_ != &managed_data_) {
    value_ = value;
  }
  else {
    std::cerr << "WARNING: In StringNode::setValue()" << name()
              << "invalid when the memory is managed by the node." << std::endl;
  }
  //  value_ = value;
}

void StringNode::print() const
{
  std::cout << this->name() << ": ";
  std::cout << "{nodetype: " << epf::nodetype_to_string(this->nodetype())
            << "} ";

  if (this->tooltip() != "") {
    std::cout << "{tooltip: " << this->tooltip() << "} ";
  }
  std::cout << "{value: " << *(this->value()) << "} ";
  std::cout << "\n";
}

DataNode::DataNode(std::string name, epf::BaseType datatype,
                   std::vector<size_t> arraydim, void *value,
                   bool memory_managed, std::string tooltip,
                   epf::AccessType access_mode)
    : Node(name, EP_DATANODE, tooltip, access_mode),
      datatype_(datatype),
      arraydimensions_(arraydim)
// streamed_(streamed)
{
  rank_ = 0;
  size_ = type_size(datatype);
  elements_ = 1;
  for (size_t dim : arraydimensions_) {
    if (dim > 0) {
      rank_++;
      size_ *= dim;
      elements_ *= dim;
    }
  }

  if (rank_ == 0) {
    size_ = 0;
    elements_ = 0;
  }

  if (memory_managed) {
    // std::cout << "DataNode: " << name << " memory managed" << std::endl;
    managed_data_ = std::make_unique<char[]>(size_);
    if (value) {
      memcpy(managed_data_.get(), value, size_);
    }
    else {
      memset(managed_data_.get(), 0, size_);
    }
  }
  else {
    value_ = value;
  }
}

DataNode::DataNode(const DataNode &obj)
    : Node(obj),
      datatype_(obj.datatype_),
      arraydimensions_(obj.arraydimensions_),
      size_(obj.size_),
      rank_(obj.rank_),
      elements_(obj.elements_)
{
  copyFrom(obj);
}

epf::DataNode &DataNode::operator=(const epf::DataNode &obj)
{
  if (this == &obj) {
    return *this;
  }

  Node::operator=(obj);

  datatype_ = obj.datatype_;
  arraydimensions_ = obj.arraydimensions_;
  rank_ = obj.rank_;
  size_ = obj.size_;
  elements_ = obj.elements_;

  copyFrom(obj);

  return *this;
}

void DataNode::copyFrom(const DataNode &obj)
{
  if (obj.memMgmt()) {
    managed_data_ = std::make_unique<char[]>(size_);
    memcpy(this->value(), obj.value(), this->size());
    value_ = nullptr;
  }
  else {
    managed_data_.reset();
    value_ = obj.value();
  }

  copyNodeChilds(obj);
}

void *DataNode::value() const
{
  if (memMgmt()) {
    return managed_data_.get();
  }
  else {
    return value_;
  }
}

epf::BaseType DataNode::datatype() const
{
  return datatype_;
}

void DataNode::setDatatype(epf::BaseType datatype)
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

size_t DataNode::arrayelements() const
{
  return elements_;
}

std::vector<size_t> DataNode::arraydimensions() const
{
  return arraydimensions_;
}

void DataNode::setValue(void *invalue)
{
  if (!memMgmt()) {
    value_ = invalue;
  }
  else {
    if (invalue == nullptr) {
      std::cerr << "WARNING: In DataNode::setValue() " << name()
                << " received nullptr for memory-managed node." << std::endl;
      return;
    }
    if (!managed_data_) {
      managed_data_ = std::make_unique<char[]>(size_);
    }
    memcpy(managed_data_.get(), invalue, size_);
  }
}

void DataNode::setSize(size_t sizein)
{
  if (!memMgmt()) {
    size_ = sizein;
  }
  else {
    std::cerr << "WARNING: In DataNode::setSize()" << name()
              << "invalid when the memory is managed by the node." << std::endl;
  }
}

void DataNode::setRank(int rank)
{
  rank_ = rank;
}

int32_t DataNode::write(const void *value)
{
  if ((this->value() == nullptr) || (value == nullptr)) {
    std::cerr << "DataNode write error value == nullptr \n" << std::endl;
    return -1;
  }
  memcpy(this->value(), value, this->size());
  return 0;
}

int32_t DataNode::read(void *value) const
{
  if ((this->value() == nullptr) || (value == nullptr)) {
    std::cerr << "DataNode read error value == nullptr \n" << std::endl;
    return -1;
  }
  memcpy(value, this->value(), this->size());
  return 0;
}

void DataNode::print() const
{
  std::cout << this->name() << ": ";

  // Print node type
  std::cout << "{nodetype: " << epf::nodetype_to_string(this->nodetype())
            << "} ";

  // Print data type
  std::cout << "{datatype: " << epf::basetype_to_string(this->datatype())
            << "} ";

  // Print tooltip if it is not empty
  if (!this->tooltip().empty()) {
    std::cout << "{tooltip: " << this->tooltip() << "} ";
  }

  // Print access mode
  if (this->accessMode() != 0) {
    std::cout << "{accessmode: "
              << epf::accesstype_to_string(this->accessMode()) << "} ";
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
  if (this->value()) {
    std::cout << "{value: ";

    // Print array values
    if (this->rank() > 0) {
      std::cout << "[";

      size_t total_elements = this->arrayelements();

      // Determine how many elements to print
      // Print first 10 or fewer elements
      size_t print_limit = std::min(total_elements, static_cast<size_t>(10));

      size_t printed = 0;
      for (size_t i = 0; i < total_elements; ++i) {
        if (i > 0) {
          std::cout << ", ";
        }

        if (printed >= print_limit) {
          // Indicate that not all elements are printed
          std::cout << "...";
          break;
        }

        // Access and print value based on datatype
        switch (this->datatype()) {
          case EP_BOOL:
            std::cout << static_cast<bool *>(this->value())[i];
            break;
          case EP_8C:
            std::cout << static_cast<char *>(this->value())[i];
            break;
          case EP_8S:
            std::cout << static_cast<int8_t *>(this->value())[i];
            break;
          case EP_8U:
            std::cout << static_cast<uint8_t *>(this->value())[i];
            break;
          case EP_16S:
            std::cout << static_cast<int16_t *>(this->value())[i];
            break;
          case EP_16U:
            std::cout << static_cast<uint16_t *>(this->value())[i];
            break;
          case EP_32S:
            std::cout << static_cast<int32_t *>(this->value())[i];
            break;
          case EP_32U:
            std::cout << static_cast<uint32_t *>(this->value())[i];
            break;
          case EP_64S:
            std::cout << static_cast<int64_t *>(this->value())[i];
            break;
          case EP_64U:
            std::cout << static_cast<uint64_t *>(this->value())[i];
            break;
          case EP_32F:
            std::cout << static_cast<float *>(this->value())[i];
            break;
          case EP_64F:
            std::cout << static_cast<double *>(this->value())[i];
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
          std::cout << *static_cast<bool *>(this->value());
          break;
        case EP_8C:
          std::cout << *static_cast<char *>(this->value());
          break;
        case EP_8S:
          std::cout << *static_cast<int8_t *>(this->value());
          break;
        case EP_8U:
          std::cout << *static_cast<uint8_t *>(this->value());
          break;
        case EP_16S:
          std::cout << *static_cast<int16_t *>(this->value());
          break;
        case EP_16U:
          std::cout << *static_cast<uint16_t *>(this->value());
          break;
        case EP_32S:
          std::cout << *static_cast<int32_t *>(this->value());
          break;
        case EP_32U:
          std::cout << *static_cast<uint32_t *>(this->value());
          break;
        case EP_64S:
          std::cout << *static_cast<int64_t *>(this->value());
          break;
        case EP_64U:
          std::cout << *static_cast<uint64_t *>(this->value());
          break;
        case EP_32F:
          std::cout << *static_cast<float *>(this->value());
          break;
        case EP_64F:
          std::cout << *static_cast<double *>(this->value());
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

bool DataNode::memMgmt() const
{
  if (managed_data_.get()) {
    return true;
  }
  else {
    return false;
  }
}

CommandNode::CommandNode(const CommandNode &obj)
    : Node(obj),
      command_(obj.command_)
{
}

const epf::CommandNode &CommandNode::operator=(const epf::CommandNode &obj)
{
  if (this != &obj) {
    Node::operator=(obj);
    command_ = obj.command_;
    copyNodeChilds(obj);
  }
  return *this;
}

int32_t CommandNode::run() const
{
  return command_();
}

}  // namespace epf
