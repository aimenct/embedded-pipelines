#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

class TemporaryYamlFile {
  public:
    TemporaryYamlFile()
        : path_(std::filesystem::temp_directory_path() /
                ("epf_serialization_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()) +
                 ".yml"))
    {
    }

    ~TemporaryYamlFile()
    {
      std::error_code error;
      std::filesystem::remove(path_, error);
    }

    const std::filesystem::path &path() const
    {
      return path_;
    }

  private:
    std::filesystem::path path_;
};

YAML::Node serialize_node_to_yaml(const Node &node)
{
  YAML::Emitter out_yaml;
  out_yaml << YAML::BeginMap;
  serialize_node(node, out_yaml);
  out_yaml << YAML::EndMap;
  return YAML::Load(out_yaml.c_str());
}

}  // namespace

TEST(SerializationTest, DataNodeRoundTripPreservesTypeSpecificState)
{
  uint32_t expected_value = 45;
  DataNode managed_node("counter", EP_32U, std::vector<size_t>{1});
  ASSERT_EQ(managed_node.write(&expected_value), 0);

  const YAML::Node managed_yaml = serialize_node_to_yaml(managed_node);
  EXPECT_EQ(managed_yaml["node_type"].as<std::string>(),
            nodetype_to_string(EP_DATANODE));
  EXPECT_TRUE(managed_yaml["managed"].as<bool>());
  EXPECT_TRUE(managed_yaml["value"]);

  std::unique_ptr<Node> managed_result = deserialize_node(managed_yaml);
  const auto *managed_copy =
      dynamic_cast<const DataNode *>(managed_result.get());
  ASSERT_NE(managed_copy, nullptr);
  EXPECT_EQ(managed_copy->name(), managed_node.name());
  EXPECT_EQ(managed_copy->datatype(), managed_node.datatype());
  EXPECT_EQ(managed_copy->arraydimensions(), managed_node.arraydimensions());
  EXPECT_EQ(managed_copy->accessMode(), managed_node.accessMode());
  EXPECT_TRUE(managed_copy->memMgmt());
  uint32_t actual_value = 0;
  ASSERT_EQ(managed_copy->read(&actual_value), 0);
  EXPECT_EQ(actual_value, expected_value);

  DataNode unmanaged_node("samples", EP_32F, std::vector<size_t>{2, 2},
                          nullptr);
  const YAML::Node unmanaged_yaml = serialize_node_to_yaml(unmanaged_node);
  EXPECT_EQ(unmanaged_yaml["node_type"].as<std::string>(),
            nodetype_to_string(EP_DATANODE));
  EXPECT_FALSE(unmanaged_yaml["managed"].as<bool>());
  EXPECT_FALSE(unmanaged_yaml["value"]);

  std::unique_ptr<Node> unmanaged_result = deserialize_node(unmanaged_yaml);
  const auto *unmanaged_copy =
      dynamic_cast<const DataNode *>(unmanaged_result.get());
  ASSERT_NE(unmanaged_copy, nullptr);
  EXPECT_EQ(unmanaged_copy->name(), unmanaged_node.name());
  EXPECT_EQ(unmanaged_copy->datatype(), unmanaged_node.datatype());
  EXPECT_EQ(unmanaged_copy->arraydimensions(),
            unmanaged_node.arraydimensions());
  EXPECT_EQ(unmanaged_copy->accessMode(), unmanaged_node.accessMode());
  EXPECT_FALSE(unmanaged_copy->memMgmt());
  EXPECT_EQ(unmanaged_copy->value(), nullptr);
}

TEST(SerializationTest, StringNodeRoundTripPreservesTypeSpecificState)
{
  StringNode original("units", "mV", W, "measurement units");

  const YAML::Node yaml = serialize_node_to_yaml(original);
  EXPECT_EQ(yaml["node_type"].as<std::string>(),
            nodetype_to_string(EP_STRINGNODE));

  std::unique_ptr<Node> result = deserialize_node(yaml);
  const auto *copy = dynamic_cast<const StringNode *>(result.get());
  ASSERT_NE(copy, nullptr);
  EXPECT_EQ(copy->name(), original.name());
  EXPECT_EQ(static_cast<const Node &>(*copy).accessMode(),
            static_cast<const Node &>(original).accessMode());
  EXPECT_EQ(copy->tooltip(), original.tooltip());
  ASSERT_NE(copy->value(), nullptr);
  EXPECT_EQ(*copy->value(), *original.value());
}

TEST(SerializationTest, ObjectNodeRoundTripPreservesTypeSpecificState)
{
  ObjectNode original("metadata", EP_OBJ, "message metadata");
  original.addReference(EP_HAS_PROPERTY,
                        std::make_unique<StringNode>("units", "mV"));

  const YAML::Node yaml = serialize_node_to_yaml(original);
  EXPECT_EQ(yaml["node_type"].as<std::string>(),
            nodetype_to_string(EP_OBJECTNODE));

  std::unique_ptr<Node> result = deserialize_node(yaml);
  const auto *copy = dynamic_cast<const ObjectNode *>(result.get());
  ASSERT_NE(copy, nullptr);
  EXPECT_EQ(copy->name(), original.name());
  EXPECT_EQ(copy->objecttype(), original.objecttype());
  EXPECT_EQ(copy->tooltip(), original.tooltip());
  ASSERT_EQ(copy->references().size(), 1u);
  EXPECT_EQ(copy->references().front().type(), EP_HAS_PROPERTY);

  const auto *property =
      dynamic_cast<const StringNode *>(copy->references().front().address());
  ASSERT_NE(property, nullptr);
  EXPECT_EQ(property->name(), "units");
  ASSERT_NE(property->value(), nullptr);
  EXPECT_EQ(*property->value(), "mV");
}

TEST(SerializationTest, YamlRoundTripPreservesNestedStringProperty)
{
  Message original_message;
  auto metadata_node = std::make_unique<ObjectNode>("metadata");
  metadata_node->addReference(EP_HAS_PROPERTY,
                              std::make_unique<StringNode>("units", "mV"));
  original_message.addItem(std::move(metadata_node));

  TemporaryYamlFile serialized_message_file;
  ASSERT_EQ(serialize_message(original_message,
                              serialized_message_file.path().string()),
            0);

  const YAML::Node serialized_yaml =
      YAML::LoadFile(serialized_message_file.path().string());
  ASSERT_TRUE(serialized_yaml["items"].IsSequence());
  ASSERT_EQ(serialized_yaml["items"].size(), 1u);
  ASSERT_TRUE(serialized_yaml["items"][0]["references"].IsSequence());
  EXPECT_EQ(serialized_yaml["items"][0]["references"][0]["ref_type"]
                .as<std::string>(),
            reftype_to_string(EP_HAS_PROPERTY));

  std::unique_ptr<Message> deserialized_message =
      deserialize_message(serialized_yaml);
  ASSERT_NE(deserialized_message, nullptr);
  ASSERT_EQ(deserialized_message->itemCount(), original_message.itemCount());

  const auto *deserialized_metadata =
      dynamic_cast<const ObjectNode *>(deserialized_message->item("metadata"));
  ASSERT_NE(deserialized_metadata, nullptr);
  ASSERT_EQ(deserialized_metadata->references().size(), 1u);
  EXPECT_EQ(deserialized_metadata->references().front().type(),
            EP_HAS_PROPERTY);

  const Node *deserialized_units =
      deserialized_metadata->references().front().address();
  ASSERT_NE(deserialized_units, nullptr);
  ASSERT_TRUE(deserialized_units->isStringNode());
  EXPECT_EQ(deserialized_units->name(), "units");
  const auto *units_value =
      static_cast<const StringNode *>(deserialized_units)->value();
  ASSERT_NE(units_value, nullptr);
  EXPECT_EQ(*units_value, "mV");
}

TEST(SerializationTest, DataNodeYamlRoundTripPreservesManagedAndStreamedData)
{
  Message original_message;

  uint32_t expected_counter = 45;
  auto managed_counter = std::make_unique<DataNode>("managed_counter", EP_32U,
                                                    std::vector<size_t>{1});
  ASSERT_EQ(managed_counter->write(&expected_counter), 0);
  original_message.addItem(std::move(managed_counter));

  auto streamed_samples = std::make_unique<DataNode>(
      "streamed_samples", EP_32F, std::vector<size_t>{2, 2}, nullptr);
  original_message.addItem(std::move(streamed_samples));
  ASSERT_EQ(original_message.size(), sizeof(float) * 4);

  TemporaryYamlFile serialized_message_file;
  ASSERT_EQ(serialize_message(original_message,
                              serialized_message_file.path().string()),
            0);

  const YAML::Node serialized_yaml =
      YAML::LoadFile(serialized_message_file.path().string());
  ASSERT_TRUE(serialized_yaml["items"].IsSequence());
  ASSERT_EQ(serialized_yaml["items"].size(), 2u);
  EXPECT_TRUE(serialized_yaml["items"][0]["managed"].as<bool>());
  EXPECT_FALSE(serialized_yaml["items"][1]["managed"].as<bool>());
  EXPECT_EQ(serialized_yaml["message_size"].as<size_t>(),
            original_message.size());

  std::unique_ptr<Message> deserialized_message =
      deserialize_message(serialized_yaml);
  ASSERT_NE(deserialized_message, nullptr);
  ASSERT_EQ(deserialized_message->itemCount(), original_message.itemCount());
  EXPECT_EQ(deserialized_message->size(), original_message.size());

  const auto *deserialized_counter = dynamic_cast<const DataNode *>(
      deserialized_message->item("managed_counter"));
  ASSERT_NE(deserialized_counter, nullptr);
  EXPECT_TRUE(deserialized_counter->memMgmt());
  EXPECT_EQ(deserialized_counter->datatype(), EP_32U);
  EXPECT_EQ(deserialized_counter->arraydimensions(), (std::vector<size_t>{1}));
  uint32_t actual_counter = 0;
  ASSERT_EQ(deserialized_counter->read(&actual_counter), 0);
  EXPECT_EQ(actual_counter, expected_counter);

  const auto *deserialized_samples = dynamic_cast<const DataNode *>(
      deserialized_message->item("streamed_samples"));
  ASSERT_NE(deserialized_samples, nullptr);
  EXPECT_FALSE(deserialized_samples->memMgmt());
  EXPECT_EQ(deserialized_samples->datatype(), EP_32F);
  EXPECT_EQ(deserialized_samples->arraydimensions(),
            (std::vector<size_t>{2, 2}));
  EXPECT_EQ(deserialized_samples->size(), sizeof(float) * 4);
  EXPECT_EQ(deserialized_message->streamedNodeOffset(deserialized_samples), 0u);
}
