#include <gtest/gtest.h>

#include <memory>

#include "core.h"

using namespace epf;

class SinkPortTest : public ::testing::Test {
  protected:
    SinkPort sink_port_default_;
    SinkPort sink_port_param_{20, 3, 2, QueueType::fifo, 5, false};
};

TEST_F(SinkPortTest, constructor)
{
  EXPECT_EQ(sink_port_default_.length(), 10);
  EXPECT_EQ(sink_port_default_.maxReaders(), 5);
  EXPECT_EQ(sink_port_default_.maxWriters(), 1);
  EXPECT_EQ(sink_port_default_.queueType(), QueueType::lifo);
  EXPECT_EQ(sink_port_default_.batchSize(), 1);
  EXPECT_TRUE(sink_port_default_.blocking());
}

TEST_F(SinkPortTest, param_constructor)
{
  EXPECT_EQ(sink_port_param_.length(), 20);
  EXPECT_EQ(sink_port_param_.maxReaders(), 3);
  EXPECT_EQ(sink_port_param_.maxWriters(), 2);
  EXPECT_EQ(sink_port_param_.queueType(), QueueType::fifo);
  EXPECT_EQ(sink_port_param_.batchSize(), 5);
  EXPECT_FALSE(sink_port_param_.blocking());
}

TEST_F(SinkPortTest, activate)
{
  std::unique_ptr<Message> data_schema = std::make_unique<Message>();
  std::unique_ptr<Message> hdr_schema = std::make_unique<Message>();

  int32_t ret = sink_port_default_.activate(std::move(data_schema),
                                            std::move(hdr_schema));
  // Assuming activate returns 0 on success
  EXPECT_EQ(ret, 0);
  EXPECT_TRUE(sink_port_default_.isActivated());
}

TEST_F(SinkPortTest, deactivate)
{
  std::unique_ptr<Message> data_schema = std::make_unique<Message>();
  std::unique_ptr<Message> hdr_schema = std::make_unique<Message>();

  sink_port_default_.activate(std::move(data_schema), std::move(hdr_schema));
  int32_t ret = sink_port_default_.deactivate();
  EXPECT_EQ(ret, 0);
  EXPECT_FALSE(sink_port_default_.isActivated());
}

TEST_F(SinkPortTest, blocking_calls)
{
  EXPECT_TRUE(sink_port_default_.blocking());
  sink_port_default_.setBlocking(false);
  EXPECT_FALSE(sink_port_default_.blocking());
  sink_port_default_.setBlocking(true);
  EXPECT_TRUE(sink_port_default_.blocking());
}

// --- SourcePort Tests ---

class SourcePortTest : public ::testing::Test {
  protected:
    SinkPort sink_port_;
    SourcePort source_port_;
};

TEST_F(SourcePortTest, DefaultValues)
{
  EXPECT_EQ(source_port_.messageWindow(), 1);
  EXPECT_EQ(source_port_.messageStride(), 1);
  EXPECT_TRUE(source_port_.blocking());
  EXPECT_FALSE(source_port_.isConnected());
  EXPECT_EQ(source_port_.queue(), nullptr);
}

TEST_F(SourcePortTest, connect_disconnect)
{
  std::unique_ptr<Message> data_schema = std::make_unique<Message>();
  std::unique_ptr<Message> hdr_schema = std::make_unique<Message>();

  sink_port_.activate(std::move(data_schema), std::move(hdr_schema));
  int32_t ret = source_port_.connect(sink_port_.queue());

  EXPECT_EQ(ret, 0);  // Assuming 0 means success
  EXPECT_TRUE(source_port_.isConnected());
  EXPECT_EQ(sink_port_.queue(), sink_port_.queue());

  ret = source_port_.disconnect();
  EXPECT_EQ(ret, 0);
  EXPECT_FALSE(source_port_.isConnected());
  EXPECT_EQ(source_port_.queue(), nullptr);
}

class HandlersTest : public ::testing::Test {
  protected:
    SinkPort sink_port_{2, 1, 1, epf::fifo, 1, false};
    // SinkPort sink_port_{2, 1, 1, epf::lifo, 1, false};
    SourcePort source_port_{1, 1, false};
    int32_t counter_{543};
};

TEST_F(HandlersTest, handlers_queue_synch)
{
  std::unique_ptr<Message> data_schema = std::make_unique<Message>();
  std::unique_ptr<Message> hdr_schema = std::make_unique<Message>();

  sink_port_.activate(std::move(data_schema), std::move(hdr_schema));
  source_port_.connect(sink_port_.queue());

  sink_port_.setBlocking(true);
  EXPECT_EQ(sink_port_.queue()->writeBlocking(sink_port_.writer()->id()), true);
  source_port_.setBlocking(true);
  EXPECT_EQ(sink_port_.queue()->readBlocking(source_port_.reader()->id()),
            true);
}

TEST_F(HandlersTest, writing_reading)
{
  std::unique_ptr<Message> data_schema = std::make_unique<Message>();
  std::unique_ptr<Message> hdr_schema = std::make_unique<Message>();

  std::unique_ptr<DataNode> node = std::make_unique<DataNode>(
      "counter", EP_32S, std::vector<size_t>{1}, nullptr);

  data_schema->addItem(std::move(node));

  sink_port_.activate(std::move(data_schema), std::move(hdr_schema));
  source_port_.connect(sink_port_.queue());
  EXPECT_GE(sink_port_.writer()->startWrite(), 0);
  DataNode *writing_node =
      static_cast<DataNode *>(sink_port_.writer()->dataMsg()->item(0));
  int32_t *writing_value = static_cast<int32_t *>(writing_node->value());
  *writing_value = counter_;

  EXPECT_EQ(*writing_value, counter_);
  EXPECT_EQ(source_port_.reader()->startRead(), -1);
  EXPECT_EQ(sink_port_.writer()->endWrite(), 0);

  EXPECT_EQ(source_port_.reader()->startRead(), 0);

  DataNode *reading_node =
      static_cast<DataNode *>(source_port_.reader()->dataMsg()->item(0));
  int32_t *reading_value = static_cast<int32_t *>(reading_node->value());
  EXPECT_EQ(*reading_value, counter_);
  EXPECT_EQ(sink_port_.writer()->startWrite(), 1);

  EXPECT_EQ(sink_port_.writer()->endWrite(), 0);
  EXPECT_EQ(sink_port_.writer()->startWrite(), -1);
  EXPECT_EQ(source_port_.reader()->endRead(), 0);
}

TEST(WriterReaderIntegration, OneOffReadUsesReturnedWindowForDataAndHeader)
{
  SinkPort sink_port{8, 1, 1, epf::fifo, 1, false};
  SourcePort source_port{1, 1, false};

  auto data_schema = std::make_unique<Message>();
  data_schema->addItem(std::make_unique<DataNode>(
      "data", EP_32S, std::vector<size_t>{1}, nullptr));

  auto hdr_schema = std::make_unique<Message>();
  hdr_schema->addItem(std::make_unique<DataNode>(
      "header", EP_32S, std::vector<size_t>{1}, nullptr));

  ASSERT_EQ(sink_port.activate(std::move(data_schema), std::move(hdr_schema)),
            0);
  ASSERT_EQ(source_port.connect(sink_port.queue()), 0);
  ASSERT_EQ(source_port.reader()->messageWindow(), 1);
  ASSERT_EQ(source_port.reader()->messageStride(), 1);

  ASSERT_GE(sink_port.writer()->startWrite(3), 0);
  for (int index = 0; index < 3; ++index) {
    auto *data_node =
        static_cast<DataNode *>(sink_port.writer()->dataMsg(index)->item(0));
    auto *hdr_node =
        static_cast<DataNode *>(sink_port.writer()->hdrMsg(index)->item(0));
    *static_cast<int32_t *>(data_node->value()) = 100 + index;
    *static_cast<int32_t *>(hdr_node->value()) = 200 + index;
  }
  ASSERT_EQ(sink_port.writer()->endWrite(), 0);

  ASSERT_GE(source_port.reader()->startRead(3, 3), 0);
  EXPECT_EQ(source_port.reader()->messageWindow(), 1);
  EXPECT_EQ(source_port.reader()->messageStride(), 1);

  for (int index = 0; index < 3; ++index) {
    Message *data_message = source_port.reader()->dataMsg(index);
    Message *hdr_message = source_port.reader()->hdrMsg(index);
    ASSERT_NE(data_message, nullptr);
    EXPECT_NE(hdr_message, nullptr);

    auto *data_node = static_cast<DataNode *>(data_message->item(0));
    EXPECT_EQ(*static_cast<int32_t *>(data_node->value()), 100 + index);
    if (hdr_message != nullptr) {
      auto *hdr_node = static_cast<DataNode *>(hdr_message->item(0));
      EXPECT_EQ(*static_cast<int32_t *>(hdr_node->value()), 200 + index);
    }
  }

  EXPECT_EQ(source_port.reader()->dataMsg(3), nullptr);
  EXPECT_EQ(source_port.reader()->hdrMsg(3), nullptr);
  EXPECT_EQ(source_port.reader()->endRead(), 0);
  EXPECT_EQ(source_port.disconnect(), 0);
  EXPECT_EQ(sink_port.deactivate(), 0);
}

TEST(WriterReaderIntegration, BufferIndices)
{
  SinkPort sink_port{10, 1, 1, epf::fifo, 1, false};
  SourcePort source_port{1, 1, false};

  auto data_schema = std::make_unique<Message>();
  auto hdr_schema = std::make_unique<Message>();
  auto node = std::make_unique<DataNode>("counter", EP_32S,
                                         std::vector<size_t>{1}, nullptr);
  data_schema->addItem(std::move(node));

  ASSERT_EQ(sink_port.activate(std::move(data_schema), std::move(hdr_schema)),
            0);
  ASSERT_EQ(source_port.connect(sink_port.queue()), 0);

  for (int i = 0; i < 100; ++i) {
    int write_idx = sink_port.writer()->startWrite();
    ASSERT_GE(write_idx, 0);
    auto *w_node =
        static_cast<DataNode *>(sink_port.writer()->dataMsg()->item(0));
    auto *w_val = static_cast<int32_t *>(w_node->value());
    *w_val = i;

    ASSERT_EQ(sink_port.writer()->endWrite(), 0);

    int read_idx = source_port.reader()->startRead();
    ASSERT_GE(read_idx, 0);
    EXPECT_EQ(write_idx, read_idx);

    auto *r_node =
        static_cast<DataNode *>(source_port.reader()->dataMsg()->item(0));
    auto *r_val = static_cast<int32_t *>(r_node->value());

    EXPECT_EQ(*r_val, i);

    ASSERT_EQ(source_port.reader()->endRead(), 0);
  }

  EXPECT_EQ(source_port.disconnect(), 0);
  EXPECT_EQ(sink_port.deactivate(), 0);
}

TEST(WriterReaderIntegration, ConsecutiveWrites)
{
  SinkPort sink_port{10, 1, 1, epf::lifo, 1, false};
  SourcePort source_port{1, 1, false};

  auto data_schema = std::make_unique<Message>();
  auto hdr_schema = std::make_unique<Message>();
  auto node = std::make_unique<DataNode>("counter", EP_32S,
                                         std::vector<size_t>{1}, nullptr);
  data_schema->addItem(std::move(node));

  ASSERT_EQ(sink_port.activate(std::move(data_schema), std::move(hdr_schema)),
            0);
  ASSERT_EQ(source_port.connect(sink_port.queue()), 0);

  int write_idx = -1;
  for (int i = 0; i < 9; ++i) {
    write_idx = sink_port.writer()->startWrite(1);
    ASSERT_GE(write_idx, 0);

    auto *w_node =
        static_cast<DataNode *>(sink_port.writer()->dataMsg()->item(0));
    auto *w_val = static_cast<int32_t *>(w_node->value());
    *w_val = i;
  }

  ASSERT_EQ(sink_port.writer()->endWrite(1), 0);
  //  ASSERT_EQ(sink_port.writer()->endWrite(), 0);

  int read_idx = source_port.reader()->startRead();
  ASSERT_GE(read_idx, 0);

  auto *r_node =
      static_cast<DataNode *>(source_port.reader()->dataMsg()->item(0));
  auto *r_val = static_cast<int32_t *>(r_node->value());

  EXPECT_EQ(*r_val, 0);

  ASSERT_EQ(source_port.reader()->endRead(), 0);

  std::cout << "read index : " << read_idx << std::endl;

  EXPECT_EQ(sink_port.writer()->endWriteAbort(), 0);
  EXPECT_EQ(source_port.disconnect(), 0);
  EXPECT_EQ(sink_port.deactivate(), 0);
}
