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

#include "ep_types.h"
#include "image_object.h"
#include "message.h"
#include "node.h"
#include "node_tree.h"
#include "settings.h"

namespace ep {

int message_to_yaml(const Message &msg, const std::string &filename = "");
void message_node_to_yaml(const Node2 &my_node, YAML::Emitter &out,
                          bool flow = false);

Message *message_from_yaml(const YAML::Node &node);
Node2 *message_node_from_yaml(const YAML::Node &node);

std::string dataNodeValueToString(const DataNode &node);

void nodeTree2Yaml(const NodeTree &my_tree);

int parse_nodetree_from_yaml(const YAML::Node &node, const NodeTree &tree);

std::vector<int> parseDimensions(const YAML::Node node);

template <typename T>
void streamValues(std::ostringstream &oss, T *data,
                  const std::vector<size_t> &dims, size_t dimIndex = 0,
                  size_t offset = 0)
{
  if (dimIndex == dims.size() - 1) {
    oss << "[";
    for (int i = 0; i < static_cast<int>(dims[dimIndex]); ++i) {
      oss << data[offset + i];
      if (i < static_cast<int>(dims[dimIndex]) - 1) {
        oss << ", ";
      }
    }
    oss << "]";
  }
  else {
    oss << "[";
    for (int i = 0; i < static_cast<int>(dims[dimIndex]); ++i) {
      if (i > 0) {
        oss << ", ";
      }
      streamValues(oss, data, dims, dimIndex + 1,
                   offset + i * dims[dimIndex + 1]);
    }
    oss << "]";
  }
}

// Helper function to trim whitespace from both ends of a string
std::string trim(const std::string &str);

// Recursive function to parse a string into a nested vector of arbitrary type
template <typename T>
void parseArray(std::stringstream &ss, std::vector<std::vector<T>> &result)
{
  char c;
  while (ss >> c) {
    if (c == '[') {
      std::vector<T> innerArray;
      T value;
      while (ss >> value) {
        innerArray.push_back(value);
        if (ss.peek() == ',')
          ss.ignore();
        else if (ss.peek() == ']')
          break;
      }
      result.push_back(innerArray);
      if (ss.peek() == ']') ss.ignore();
    }
    if (ss.peek() == ',')
      ss.ignore();
    else if (ss.peek() == ']')
      break;
  }
}

// Templated function to parse nested arrays from a YAML value field
template <typename T>
std::vector<std::vector<T>> parseNestedArray(const std::string &str)
{
  std::vector<std::vector<T>> result;
  std::stringstream ss(trim(str));
  char c;

  if (ss.get() != '[') throw std::invalid_argument("Invalid format");

  while (ss >> c && c != ']') {
    if (c == '[') {
      std::vector<T> innerArray;
      std::string value;
      std::getline(ss, value, ']');  // Read until the closing bracket

      // Remove the leading and trailing brackets and split by comma
      value = trim(value);
      size_t start = value.find_first_not_of('[');
      size_t end = value.find_last_not_of(']');
      value = (start == std::string::npos)
                  ? ""
                  : value.substr(start, end - start + 1);

      std::stringstream innerStream(value);
      std::string token;
      while (std::getline(innerStream, token, ',')) {
        T item;
        std::stringstream tokenStream(trim(token));
        tokenStream >> item;  // Extract the value of type T
        innerArray.push_back(item);
      }

      result.push_back(innerArray);
      if (ss.peek() == ']') ss.ignore();
      if (ss.peek() == ',') ss.ignore();
    }
    else if (std::isalpha(c) || c == '"' || std::isdigit(c) || c == '-' ||
             c == '.') {
      // Handling simple array case
      std::vector<T> simpleArray;
      ss.putback(c);  // Put back the character for reading
      std::string value;
      std::getline(ss, value, ']');

      std::stringstream simpleStream(value);
      std::string token;
      while (std::getline(simpleStream, token, ',')) {
        T item;
        std::stringstream tokenStream(trim(token));
        tokenStream >> item;  // Extract the value of type T
        simpleArray.push_back(item);
      }

      result.push_back(simpleArray);
      break;  // Since it's a single-level array, we can break the loop
    }

    if (ss.peek() == ',') ss.ignore();
  }

  return result;
}

// Function to determine dimensions from a nested vector
template <typename T>
std::vector<size_t> determineDimensions(
    const std::vector<std::vector<T>> &nestedArray)
{
  std::vector<size_t> dims;
  dims.push_back(nestedArray.size());
  if (!nestedArray.empty()) {
    dims.push_back(nestedArray[0].size());
  }
  return dims;
}

// Function to flatten nested vector into a flat vector
template <typename T>
std::vector<T> flattenNestedArray(
    const std::vector<std::vector<T>> &nestedArray)
{
  std::vector<T> flatArray;
  for (const auto &innerArray : nestedArray) {
    flatArray.insert(flatArray.end(), innerArray.begin(), innerArray.end());
  }
  return flatArray;
}

// Function to create a flat buffer from a string representation of a
// nested array
template <typename T>
void *parseFlatBufferFromNestedArray(const std::string &str)
{
  std::vector<std::vector<T>> nestedArray = parseNestedArray<T>(str);
  //  std::vector<size_t> dimensions = determineDimensions(nestedArray);
  std::vector<T> flatArray = flattenNestedArray(nestedArray);
  // Allocate memory for the flattened array

  T *value = new T[flatArray.size()];
  std::copy(flatArray.begin(), flatArray.end(), value);

  return (void *)value;
}

}  // namespace ep

#endif  // SERIALIZATION_H
