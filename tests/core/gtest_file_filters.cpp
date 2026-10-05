#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "core.h"
#include "filters/src_filter.h"

using namespace epf;

namespace {

struct ScopedTempDir {
    std::filesystem::path path;

    ScopedTempDir()
    {
      const auto stamp =
          std::chrono::high_resolution_clock::now().time_since_epoch().count();
      path = std::filesystem::temp_directory_path() /
             ("epf_file_filters_test_" + std::to_string(stamp));
      std::filesystem::create_directories(path);
    }

    ~ScopedTempDir()
    {
      std::error_code ec;
      std::filesystem::remove_all(path, ec);
    }
};

class CountingSinkFilter : public Filter {
  public:
    CountingSinkFilter()
        : Filter(YAML::Node(), 1, 0)
    {
    }

    int32_t total_messages_read{0};
    int32_t successful_reads{0};

  protected:
    int32_t _job() override
    {
      QueueReader *reader = sourcePort(0)->reader();
      const int32_t err = reader->startRead();
      if (err < 0) return err;

      total_messages_read += reader->lenA() + reader->lenB();
      ++successful_reads;
      reader->endRead();
      return 0;
    }

    int32_t _open() override
    {
      return 0;
    }
    int32_t _close() override
    {
      return 0;
    }
    int32_t _set() override
    {
      return 0;
    }
    int32_t _reset() override
    {
      return 0;
    }
    int32_t _start() override
    {
      return 0;
    }
    int32_t _stop() override
    {
      return 0;
    }
};

YAML::Node make_file_sink_config(const std::filesystem::path &dir)
{
  YAML::Node cfg;
  cfg["name"] = "sink";
  cfg["type"] = "FileSink";
  cfg["settings"]["base"]["path"] = dir.string();
  cfg["settings"]["base"]["timeout"] = 0;
  cfg["settings"]["base"]["max file size"] = 1024 * 1024;
  return cfg;
}

YAML::Node make_file_src_config(const std::filesystem::path &data_schema,
                                const std::filesystem::path &hdr_schema)
{
  YAML::Node cfg;
  cfg["name"] = "src";
  cfg["type"] = "FileSrc";
  cfg["settings"]["base"]["data_file"] = data_schema.string();
  cfg["settings"]["base"]["hdr_file"] = hdr_schema.string();
  cfg["settings"]["base"]["timeout"] = 0;
  cfg["settings"]["base"]["loop"] = true;
  return cfg;
}

void write_single_int_message_schema(const std::filesystem::path &schema_path,
                                     const std::string &field_name)
{
  Message msg;
  msg.addItem(std::make_unique<DataNode>(field_name.c_str(), EP_32S,
                                         std::vector<size_t>{1}, nullptr));
  serialize_message(msg, schema_path.string());
}

void write_int32_stream(const std::filesystem::path &bin_path,
                        const std::vector<int32_t> &values)
{
  std::ofstream out(bin_path, std::ios::binary);
  ASSERT_TRUE(out.is_open());
  out.write(reinterpret_cast<const char *>(values.data()),
            static_cast<std::streamsize>(values.size() * sizeof(int32_t)));
  out.close();
}

}  // namespace

TEST(FileSinkFilterTest, RepeatedStartStopCyclesWriteDataWithoutStateErrors)
{
  ScopedTempDir tmp;
  SrcFilter producer;
  FileSink sink(make_file_sink_config(tmp.path));

  ASSERT_EQ(producer.open(), 0);
  ASSERT_EQ(producer.set(), 0);

  ASSERT_EQ(sink.open(), 0);
  ASSERT_EQ(sink.connect(0, producer.sinkPort(0)), 0);
  ASSERT_EQ(sink.set(), 0);

  for (int cycle = 0; cycle < 3; ++cycle) {
    ASSERT_EQ(producer.start(), 0);
    ASSERT_EQ(sink.start(), 0);
    ASSERT_EQ(sink.state(), RUNNING);

    for (int i = 0; i < 4; ++i) {
      ASSERT_EQ(producer.doJob(), 0);
      ASSERT_EQ(sink.doJob(), 0);
    }

    ASSERT_EQ(sink.stop(), 0);
    ASSERT_EQ(sink.doJob(), -1);
    EXPECT_EQ(sink.state(), SET);
    ASSERT_EQ(producer.stop(), 0);
    ASSERT_EQ(producer.doJob(), -1);
  }

  size_t data_bin_count = 0;
  for (const auto &entry : std::filesystem::directory_iterator(tmp.path)) {
    const auto name = entry.path().filename().string();
    if (name.find("_dat.bin") != std::string::npos) ++data_bin_count;
  }
  EXPECT_GT(data_bin_count, 0u);

  EXPECT_EQ(sink.close(), 0);
  EXPECT_EQ(producer.close(), 0);
}

TEST(FileSrcFilterTest, SetStartJobFeedsConnectedConsumer)
{
  ScopedTempDir tmp;
  const std::string timestamp = "20260101_000000";
  const auto data_schema = tmp.path / (timestamp + "_q0_dat.yml");
  const auto hdr_schema = tmp.path / (timestamp + "_q0_hdr.yml");
  const auto data_bin = tmp.path / (timestamp + "_0_q0_dat.bin");
  const auto hdr_bin = tmp.path / (timestamp + "_0_q0_hdr.bin");

  write_single_int_message_schema(data_schema, "value");
  write_single_int_message_schema(hdr_schema, "header_value");
  write_int32_stream(data_bin, {10, 20, 30});
  write_int32_stream(hdr_bin, {1, 2, 3});

  FileSrc source(make_file_src_config(data_schema, hdr_schema));
  CountingSinkFilter consumer;

  ASSERT_EQ(source.open(), 0);
  ASSERT_EQ(source.set(), 0);
  ASSERT_EQ(source.start(), 0);

  ASSERT_EQ(consumer.open(), 0);
  ASSERT_EQ(consumer.connect(0, source.sinkPort(0)), 0);
  ASSERT_EQ(consumer.set(), 0);
  ASSERT_EQ(consumer.start(), 0);

  ASSERT_EQ(source.doJob(), 0);
  ASSERT_EQ(consumer.doJob(), 0);
  EXPECT_GT(consumer.successful_reads, 0);
  EXPECT_GT(consumer.total_messages_read, 0);

  ASSERT_EQ(consumer.stop(), 0);
  ASSERT_EQ(consumer.doJob(), -1);
  ASSERT_EQ(consumer.close(), 0);

  ASSERT_EQ(source.stop(), 0);
  ASSERT_EQ(source.doJob(), -1);
  ASSERT_EQ(source.close(), 0);
}
