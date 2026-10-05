// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <yaml-cpp/yaml.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "epf_types.h"
#include "image_object.h"
#include "message.h"
#include "node.h"
#include "node_tree.h"
#include "settings.h"

namespace epf {

// Message serialization
int serialize_message(const Message &msg, const std::string &filename = "");
std::unique_ptr<Message> deserialize_message(const YAML::Node &node);

// Node serialization
void serialize_node(const Node &node, YAML::Emitter &out_yaml);
std::unique_ptr<Node> deserialize_node(const YAML::Node &node);
std::string data_node_value_to_string(const DataNode &node);

// Array serialization helpers
std::vector<size_t> parse_dimensions(const YAML::Node &dimensions_node);

template <typename T>
void stream_values(std::ostringstream &oss, T *data,
                   const std::vector<size_t> &dims, size_t dim_index = 0,
                   size_t offset = 0)
{
  if (dim_index == dims.size() - 1) {
    oss << "[";
    for (int i = 0; i < static_cast<int>(dims[dim_index]); ++i) {
      oss << data[offset + i];
      if (i < static_cast<int>(dims[dim_index]) - 1) {
        oss << ", ";
      }
    }
    oss << "]";
  }
  else {
    oss << "[";
    for (int i = 0; i < static_cast<int>(dims[dim_index]); ++i) {
      if (i > 0) {
        oss << ", ";
      }
      stream_values(oss, data, dims, dim_index + 1,
                    offset + i * dims[dim_index + 1]);
    }
    oss << "]";
  }
}

// Array deserialization helpers

std::string trim(const std::string &str);

template <typename T>
void parse_array(std::stringstream &ss, std::vector<std::vector<T>> &result)
{
  char c;
  while (ss >> c) {
    if (c == '[') {
      std::vector<T> inner_array;
      T value;
      while (ss >> value) {
        inner_array.push_back(value);
        if (ss.peek() == ',')
          ss.ignore();
        else if (ss.peek() == ']')
          break;
      }
      result.push_back(inner_array);
      if (ss.peek() == ']') ss.ignore();
    }
    if (ss.peek() == ',')
      ss.ignore();
    else if (ss.peek() == ']')
      break;
  }
}

template <typename T>
std::vector<std::vector<T>> parse_nested_array(const std::string &str)
{
  std::vector<std::vector<T>> result;
  std::stringstream ss(trim(str));
  char c;

  if (ss.get() != '[') throw std::invalid_argument("Invalid format");

  while (ss >> c && c != ']') {
    if (c == '[') {
      std::vector<T> inner_array;
      std::string value;
      std::getline(ss, value, ']');  // Read until the closing bracket

      // Remove the leading and trailing brackets and split by comma
      value = trim(value);
      size_t start = value.find_first_not_of('[');
      size_t end = value.find_last_not_of(']');
      value = (start == std::string::npos)
                  ? ""
                  : value.substr(start, end - start + 1);

      std::stringstream inner_stream(value);
      std::string token;
      while (std::getline(inner_stream, token, ',')) {
        T item;
        std::stringstream token_stream(trim(token));
        token_stream >> item;  // Extract the value of type T
        inner_array.push_back(item);
      }

      result.push_back(inner_array);
      if (ss.peek() == ']') ss.ignore();
      if (ss.peek() == ',') ss.ignore();
    }
    else if (std::isalpha(c) || c == '"' || std::isdigit(c) || c == '-' ||
             c == '.') {
      // Handling simple array case
      std::vector<T> simple_array;
      ss.putback(c);  // Put back the character for reading
      std::string value;
      std::getline(ss, value, ']');

      std::stringstream simple_stream(value);
      std::string token;
      while (std::getline(simple_stream, token, ',')) {
        T item;
        std::stringstream token_stream(trim(token));
        token_stream >> item;  // Extract the value of type T
        simple_array.push_back(item);
      }

      result.push_back(simple_array);
      break;  // Since it's a single-level array, we can break the loop
    }

    if (ss.peek() == ',') ss.ignore();
  }

  return result;
}

template <typename T>
std::vector<size_t> determine_dimensions(
    const std::vector<std::vector<T>> &nested_array)
{
  std::vector<size_t> dims;
  dims.push_back(nested_array.size());
  if (!nested_array.empty()) {
    dims.push_back(nested_array[0].size());
  }
  return dims;
}

// Reference deserialization helpers

template <typename T>
void add_references(std::unique_ptr<T> &node, const YAML::Node &yaml_node)
{
  for (const auto &ref : yaml_node["references"]) {
    auto nr = deserialize_node(ref);
    if (nr) {
      auto ref_type = ref["ref_type"]
                          ? string_to_reftype(ref["ref_type"].as<std::string>())
                          : EP_HAS_CHILD;
      node->addReference(ref_type, std::move(nr));
    }
  }
}

// Array buffer helpers

template <typename T>
std::vector<T> flatten_nested_array(
    const std::vector<std::vector<T>> &nested_array)
{
  std::vector<T> flat_array;
  for (const auto &inner_array : nested_array) {
    flat_array.insert(flat_array.end(), inner_array.begin(), inner_array.end());
  }
  return flat_array;
}

template <typename T>
void *parse_flat_buffer_from_nested_array(const std::string &str)
{
  std::vector<std::vector<T>> nested_array = parse_nested_array<T>(str);
  //  std::vector<size_t> dimensions = determine_dimensions(nested_array);
  std::vector<T> flat_array = flatten_nested_array(nested_array);
  // Allocate memory for the flattened array

  T *value = new T[flat_array.size()];
  std::copy(flat_array.begin(), flat_array.end(), value);

  return static_cast<void *>(value);
}

}  // namespace epf

#endif  // SERIALIZATION_H
