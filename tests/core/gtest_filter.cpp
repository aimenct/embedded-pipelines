#include <gtest/gtest.h>

#include <memory>

#include "config_vars.h"
#include "core.h"
#include "filters/basic_filter.h"
#include "filters/dummy_filter.h"
#include "filters/src_filter.h"

using namespace epf;

class BasicFilterTest : public ::testing::Test {
  protected:
    BasicFilter filter_;
};

TEST_F(BasicFilterTest, filter_states)
{
  EXPECT_EQ(filter_.state(), DISCONNECTED);

  EXPECT_EQ(filter_.open(), 0);
  EXPECT_EQ(filter_.state(), CONNECTED);

  EXPECT_EQ(filter_.set(), 0);
  EXPECT_EQ(filter_.state(), SET);

  EXPECT_EQ(filter_.start(), 0);
  EXPECT_EQ(filter_.state(), RUNNING);
  filter_.doJob();
  EXPECT_EQ(filter_.state(), RUNNING);

  EXPECT_EQ(filter_.stop(), 0);
  EXPECT_EQ(filter_.state(), STOP_REQUEST);

  filter_.doJob();
  EXPECT_EQ(filter_.state(), SET);

  EXPECT_EQ(filter_.reset(), 0);
  EXPECT_EQ(filter_.state(), CONNECTED);

  EXPECT_EQ(filter_.close(), 0);
  EXPECT_EQ(filter_.state(), DISCONNECTED);
}

class FilterTest : public ::testing::Test {
  protected:
    class TestableDummyFilter : public DummyFilter {
      public:
        using DummyFilter::deactivateSinkPorts;
        using DummyFilter::disconnectSourcePorts;
        using DummyFilter::DummyFilter;
    };

    std::string fname_ = "";
    YAML::Node yaml_config_ =
        YAML::LoadFile(std::string(test_paths::EPF_TEST_DATA_DIR) +
                       "/core/filters/dummy_config.yml");
    TestableDummyFilter filter_{yaml_config_};
};

TEST_F(FilterTest, constructor)
{
  filter_.open();
  EXPECT_EQ(&filter_.sourcePort(0)->messageWindow(),
            filter_.settingValue<int32_t>("source_ports.0.message_window"));
  EXPECT_EQ(&filter_.sourcePort(0)->blocking(),
            filter_.settingValue<bool>("source_ports.0.blocking"));
  EXPECT_EQ(&filter_.sourcePort(0)->messageStride(),
            filter_.settingValue<int32_t>("source_ports.0.message_stride"));

  EXPECT_EQ(&filter_.sinkPort(0)->batchSize(),
            filter_.settingValue<int32_t>("sink_ports.0.batch_size"));
  EXPECT_EQ(&filter_.sinkPort(0)->blocking(),
            filter_.settingValue<bool>("sink_ports.0.blocking"));
  EXPECT_EQ(&filter_.sinkPort(0)->length(),
            filter_.settingValue<int32_t>("sink_ports.0.length"));
  EXPECT_EQ(reinterpret_cast<int32_t *>(&filter_.sinkPort(0)->queueType()),
            filter_.settingValue<int32_t>("sink_ports.0.queue_type"));
  EXPECT_EQ(&filter_.sinkPort(0)->maxReaders(),
            filter_.settingValue<int32_t>("sink_ports.0.max_readers"));
  EXPECT_EQ(&filter_.sinkPort(0)->maxWriters(),
            filter_.settingValue<int32_t>("sink_ports.0.max_writers"));
  EXPECT_EQ(&filter_.sinkPort(0)->timestamp(),
            filter_.settingValue<bool>("sink_ports.0.timestamp"));
}

TEST_F(FilterTest, change_settings)
{
  int32_t new_counter_init = 23;
  filter_.setSettingValue<int32_t>("base.counter_init", new_counter_init);
  EXPECT_EQ(new_counter_init,
            *filter_.settingValue<int32_t>("base.counter_init"));

  EXPECT_EQ(nullptr, filter_.settingValue<int64_t>("base.counter_init"));
  EXPECT_EQ(nullptr, filter_.settingValue<int32_t>("base.inexistent"));
}

TEST_F(FilterTest, reopen_preserves_settings)
{
  ASSERT_EQ(filter_.open(), 0);
  filter_.setSettingValue<int32_t>("base.counter_init", 42);
  EXPECT_EQ(filter_.close(), 0);
  EXPECT_EQ(filter_.open(), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.counter_init"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.counter_init"), 42);
}

TEST_F(FilterTest, save_settings)
{
  filter_.open();
  YAML::Node yaml_node;
  filter_.writeSettings(yaml_node);

  std::cout << yaml_node << std::endl;
}

TEST_F(FilterTest, sink_port_length_loaded)
{
  ASSERT_EQ(filter_.sinkPort(0)->length(), 8);
}

TEST_F(FilterTest, state_setting_is_read_only_and_tracks_numeric_state)
{
  SrcFilter src;

  const auto state_value = filter_.settingValue<int32_t>("base.state_");
  ASSERT_NE(state_value, nullptr);
  EXPECT_EQ(*state_value, static_cast<int32_t>(DISCONNECTED));

  EXPECT_NE(filter_.setSettingValue<int32_t>("base.state_", 999), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(DISCONNECTED));

  ASSERT_EQ(filter_.open(), 0);
  ASSERT_EQ(src.open(), 0);
  ASSERT_EQ(src.set(), 0);
  ASSERT_EQ(filter_.connect(0, src.sinkPort(0)), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(CONNECTED));

  ASSERT_EQ(filter_.set(), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(SET));

  ASSERT_EQ(filter_.start(), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(RUNNING));

  ASSERT_EQ(filter_.stop(), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(STOP_REQUEST));

  filter_.doJob();
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(SET));

  ASSERT_EQ(filter_.close(), 0);
  ASSERT_EQ(src.close(), 0);
  ASSERT_NE(filter_.settingValue<int32_t>("base.state_"), nullptr);
  EXPECT_EQ(*filter_.settingValue<int32_t>("base.state_"),
            static_cast<int32_t>(DISCONNECTED));
}

TEST_F(FilterTest, connect_propagates_source_port_connect_failure)
{
  SrcFilter src;
  ASSERT_EQ(src.open(), 0);
  ASSERT_EQ(src.set(), 0);

  ASSERT_EQ(filter_.open(), 0);
  EXPECT_EQ(filter_.connect(0, src.sinkPort(0)), 0);

  // Reconnecting the same source port must report the underlying
  // QueueReader::subscribe() failure instead of returning success.
  EXPECT_LT(filter_.connect(0, src.sinkPort(0)), 0);

  EXPECT_EQ(filter_.close(), 0);
  EXPECT_EQ(src.close(), 0);
}
TEST_F(FilterTest, reset_is_safe_after_pre_teardown)
{
  SrcFilter src;
  ASSERT_EQ(src.open(), 0);
  ASSERT_EQ(src.set(), 0);

  ASSERT_EQ(filter_.open(), 0);
  ASSERT_EQ(filter_.connect(0, src.sinkPort(0)), 0);
  ASSERT_EQ(filter_.set(), 0);

  // Simulate pipeline-coordinated teardown before local filter reset.
  ASSERT_EQ(filter_.disconnectSourcePorts(), 0);
  ASSERT_EQ(filter_.deactivateSinkPorts(), 0);

  // Filter::reset() must remain self-contained and tolerate repeated teardown.
  EXPECT_EQ(filter_.reset(), 0);
  EXPECT_EQ(filter_.state(), CONNECTED);

  EXPECT_EQ(filter_.close(), 0);
  EXPECT_EQ(src.close(), 0);
}

// Helper filter to count device interactions
class CountingDummyFilter : public DummyFilter {
  public:
    using DummyFilter::DummyFilter;
    int read_calls{0};
    int write_calls{0};

    int32_t deviceSettingValue(const char *key, void *value) override
    {
      ++read_calls;
      return DummyFilter::deviceSettingValue(key, value);
    }
    int32_t setDeviceSettingValue(const char *key, const void *value) override
    {
      ++write_calls;
      return DummyFilter::setDeviceSettingValue(key, value);
    }
    int32_t setDeviceSettingValueStr(const char *key,
                                     const char *value) override
    {
      ++write_calls;
      return DummyFilter::setDeviceSettingValueStr(key, value);
    }
};

TEST(CountingFilterTest, device_setting_read_does_not_write)
{
  YAML::Node yaml_config =
      YAML::LoadFile(std::string(test_paths::EPF_TEST_DATA_DIR) +
                     "/core/filters/dummy_config.yml");
  CountingDummyFilter filter(yaml_config);

  // establish a known value via the write path
  filter.setSettingValue<int32_t>("device.dummy_setting", 1234);
  filter.read_calls = 0;
  filter.write_calls = 0;

  const int *val = filter.settingValue<int>("device.dummy_setting");
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(*val, 1234);
  EXPECT_EQ(filter.read_calls, 1);
  EXPECT_EQ(filter.write_calls, 0);
}
