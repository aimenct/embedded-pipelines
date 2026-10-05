#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "core.h"

using namespace epf;

class SettingsTest : public ::testing::Test {
  protected:
    Settings settings_;

    void SetUp() override
    {
      settings_ = Settings("test_filter");
    }
};

TEST_F(SettingsTest, constructor_initialization)
{
  EXPECT_NO_THROW(Settings local_settings("my_filter"));
}

TEST_F(SettingsTest, add_get_setting)
{
  int32_t some_value = 42;
  settings_.addSetting("some_setting", some_value, BASE_SETTING);

  const int *retrieved_value = settings_.value<int32_t>("base.some_setting");
  ASSERT_NE(retrieved_value, nullptr);
  EXPECT_EQ(*retrieved_value, 42);

  some_value = 21;
  EXPECT_EQ(*retrieved_value, some_value);
}

TEST_F(SettingsTest, rejected_parent_does_not_reserve_setting_name)
{
  int32_t value = 42;
  constexpr int32_t INVALID_PARENT_INDEX = 1000000;

  EXPECT_EQ(
      settings_.addSettingUnder("retry_setting", value, INVALID_PARENT_INDEX),
      -1);
  EXPECT_GE(settings_.addSetting("retry_setting", value, BASE_SETTING), 0);

  const int32_t *retrieved_value =
      settings_.value<int32_t>("base.retry_setting");
  ASSERT_NE(retrieved_value, nullptr);
  EXPECT_EQ(*retrieved_value, value);
}

TEST_F(SettingsTest, set_setting_value)
{
  int32_t some_value = 10;
  settings_.addSetting("modifiable_setting", some_value, BASE_SETTING);

  EXPECT_EQ(settings_.setValue("base.modifiable_setting", 99), 0);
  const int32_t *new_value =
      settings_.value<int32_t>("base.modifiable_setting");
  ASSERT_NE(new_value, nullptr);
  EXPECT_EQ(*new_value, 99);
}

TEST_F(SettingsTest, yaml_dump)
{
  int32_t test_val = 100;
  settings_.addSetting("yaml_test", test_val, DEVICE_SETTING);

  YAML::Node yaml_out;
  settings_.toYAML(yaml_out);

  Settings new_settings("test_filter");
  int32_t test_val1 = 200;
  new_settings.addSetting("yaml_test", test_val1, DEVICE_SETTING);
  new_settings.fromYAML(yaml_out);

  const int32_t *retrieved = new_settings.value<int32_t>("device.yaml_test");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(*retrieved, 100);
}

TEST_F(SettingsTest, add_command_and_run_it)
{
  bool executed = false;
  auto command = [&executed]() -> int32_t {
    executed = true;
    return 0;
  };

  settings_.addCommand("test_command", command);
  EXPECT_EQ(settings_.runCommand("control.test_command"), 0);
  EXPECT_TRUE(executed);
}

TEST_F(SettingsTest, string_conversion)
{
  int32_t value = 123;
  settings_.addSetting("int_setting", value);
  EXPECT_EQ(settings_.valueString("base.int_setting"), "123");
}

TEST_F(SettingsTest, list_key_names)
{
  int32_t a = 1, b = 2;
  settings_.addSetting("a", a, BASE_SETTING);
  settings_.addSetting("b", b, DEVICE_SETTING);

  auto all = settings_.listNames();
  EXPECT_GE(all.size(), 2);

  auto base = settings_.listNames(BASE_SETTING);
  auto device = settings_.listNames(DEVICE_SETTING);

  EXPECT_TRUE(std::find(base.begin(), base.end(), "base.a") != base.end());
  EXPECT_TRUE(std::find(device.begin(), device.end(), "device.b") !=
              device.end());
}

TEST_F(SettingsTest, is_device_setting)
{
  int32_t b = 42;
  settings_.addSetting("device_setting", b, DEVICE_SETTING);
  EXPECT_TRUE(settings_.isDeviceSetting("device.device_setting"));
}

TEST_F(SettingsTest, enum_setting_read_write_and_validation)
{
  int32_t queue_type = static_cast<int32_t>(epf::QueueType::fifo);
  settings_.addSetting("queue_type", queue_type, BASE_SETTING);
  settings_.addEnumOptions(
      "base.queue_type",
      {{static_cast<int32_t>(epf::QueueType::fifo), "fifo", "FIFO"},
       {static_cast<int32_t>(epf::QueueType::lifo), "lifo", "LIFO"}});

  EXPECT_TRUE(settings_.hasEnumOptions("base.queue_type"));
  EXPECT_EQ(settings_.valueStringRaw("base.queue_type"), "0");
  EXPECT_EQ(settings_.valueString("base.queue_type"), "FIFO");

  EXPECT_EQ(settings_.setValue("base.queue_type", std::string("lifo")), 0);
  ASSERT_NE(settings_.value<int32_t>("base.queue_type"), nullptr);
  EXPECT_EQ(*settings_.value<int32_t>("base.queue_type"),
            static_cast<int32_t>(epf::QueueType::lifo));
  EXPECT_EQ(settings_.valueString("base.queue_type"), "LIFO");

  EXPECT_EQ(settings_.setValue("base.queue_type", std::string("0")), 0);
  EXPECT_EQ(*settings_.value<int32_t>("base.queue_type"),
            static_cast<int32_t>(epf::QueueType::fifo));

  EXPECT_EQ(settings_.setValue("base.queue_type", std::string("invalid")), -1);
  EXPECT_EQ(*settings_.value<int32_t>("base.queue_type"),
            static_cast<int32_t>(epf::QueueType::fifo));
  EXPECT_EQ(settings_.setValue("base.queue_type", 42), -1);
}

TEST_F(SettingsTest, enum_value_nodes_are_scalar_int32)
{
  int32_t queue_type = static_cast<int32_t>(epf::QueueType::fifo);
  settings_.addSetting("queue_type", queue_type, BASE_SETTING);
  settings_.addEnumOptions(
      "base.queue_type",
      {{static_cast<int32_t>(epf::QueueType::fifo), "fifo", "FIFO"},
       {static_cast<int32_t>(epf::QueueType::lifo), "lifo", "LIFO"}});

  const Node *setting_node = settings_["base.queue_type"];
  ASSERT_NE(setting_node, nullptr);

  size_t enum_value_nodes = 0;
  for (const auto &enum_ref : setting_node->references()) {
    if (enum_ref.type() != epf::EP_HAS_ENUMVALUE) continue;

    const Node *enum_node = enum_ref.address();
    ASSERT_NE(enum_node, nullptr);

    bool has_value_child = false;
    for (const auto &child_ref : enum_node->references()) {
      if (child_ref.type() != epf::EP_HAS_CHILD) continue;
      const Node *child = child_ref.address();
      if (!child || !child->isDataNode() || child->name() != "value") continue;

      const auto *data_node = static_cast<const DataNode *>(child);
      EXPECT_EQ(data_node->datatype(), epf::EP_32S);
      EXPECT_EQ(data_node->rank(), 1);
      EXPECT_EQ(data_node->arrayelements(), 1U);
      EXPECT_EQ(data_node->size(), sizeof(int32_t));
      ASSERT_NE(data_node->value(), nullptr);

      has_value_child = true;
      ++enum_value_nodes;
    }

    EXPECT_TRUE(has_value_child);
  }

  EXPECT_EQ(enum_value_nodes, 2U);
}
