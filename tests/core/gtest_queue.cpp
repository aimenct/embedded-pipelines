// Sequential/lock-step Queue correctness tests, single-threaded (guarded
// start/end calls driven directly by the test body). Concurrent, multi-thread
// stress testing lives in gtest_queue_stresstest.cpp instead.
#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

constexpr int K_IMAGE_WIDTH = 16;
constexpr int K_IMAGE_HEIGHT = 8;
constexpr int K_IMAGE_CHANNELS = 4;

double k_source_values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
double k_replacement_values[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

std::unique_ptr<Message> create_queue_message()
{
  auto msg = std::make_unique<Message>();

  auto counter = std::make_unique<DataNode>("Counter", EP_32U,
                                            std::vector<size_t>{1}, nullptr);
  msg->addItem(std::move(counter));

  ImageObject image("image", K_IMAGE_WIDTH, K_IMAGE_HEIGHT, K_IMAGE_CHANNELS,
                    PixelFormat::RGBA8, nullptr);
  msg->addItem(image.copyNode());

  return msg;
}

void fill_payload(std::vector<char> &payload, size_t msg_size)
{
  for (size_t msg = 0; msg < payload.size() / msg_size; ++msg) {
    auto *base = payload.data() + msg * msg_size;
    const uint32_t counter = static_cast<uint32_t>(msg);
    std::memcpy(base, &counter, sizeof(counter));

    for (size_t byte = sizeof(counter); byte < msg_size; ++byte) {
      base[byte] = static_cast<char>((msg + byte) & 0xff);
    }
  }
}

class QueuePayloadWriter {
  public:
    QueueWriter writer;
    std::vector<char> data;
    size_t msg_size{0};
    int num_msgs{0};
    int batch{1};
    size_t byte_count{0};

    QueuePayloadWriter(Queue *q, int messages, bool blocking)
        : writer(q),
          msg_size(writer.dataSchema()->size()),
          num_msgs(messages)
    {
      writer.setBlockingCalls(blocking);
      data.resize(msg_size * num_msgs);
      fill_payload(data, msg_size);
    }

    int write(int batch_size)
    {
      const int err = writer.startWrite(batch_size);
      if (err < 0) return err;

      const size_t bytes_a = msg_size * writer.lenA();
      const size_t bytes_b = msg_size * writer.lenB();
      std::memcpy(writer.dataPtrA(), data.data() + byte_count, bytes_a);
      byte_count += bytes_a;
      if (bytes_b > 0) {
        std::memcpy(writer.dataPtrB(), data.data() + byte_count, bytes_b);
        byte_count += bytes_b;
      }
      return writer.endWrite();
    }
};

class QueuePayloadReader {
  public:
    QueueReader reader;
    std::vector<char> data;
    size_t msg_size{0};
    int num_msgs{0};
    int message_window{1};
    int message_stride{1};
    size_t byte_count{0};

    QueuePayloadReader(Queue *q, int messages, bool blocking)
        : reader(q),
          msg_size(reader.dataSchema()->size()),
          num_msgs(messages)
    {
      reader.setBlockingCalls(blocking);
      data.resize(msg_size * num_msgs);
    }

    int read(int batch_size, int new_messages)
    {
      const int err = reader.startRead(batch_size, new_messages);
      if (err < 0) return err;

      const size_t bytes_a = msg_size * reader.lenA();
      const size_t bytes_b = msg_size * reader.lenB();
      std::memcpy(data.data() + byte_count, reader.dataPtrA(), bytes_a);
      byte_count += bytes_a;
      if (bytes_b > 0) {
        std::memcpy(data.data() + byte_count, reader.dataPtrB(), bytes_b);
        byte_count += bytes_b;
      }
      return reader.endRead();
    }
};

std::unique_ptr<Queue> make_queue(QueueType type, int length)
{
  auto queue = std::make_unique<Queue>();
  EXPECT_EQ(queue->setType(type), 0);
  EXPECT_EQ(queue->init(length, create_queue_message()), 0);
  return queue;
}

void expect_payload_eq(const std::vector<char> &expected,
                       const std::vector<char> &actual, size_t bytes)
{
  ASSERT_LE(bytes, expected.size());
  ASSERT_LE(bytes, actual.size());
  EXPECT_EQ(std::memcmp(expected.data(), actual.data(), bytes), 0);
}

Message create_nested_message()
{
  auto cow = std::make_unique<DataNode>("cow", EP_64F, std::vector<size_t>{10},
                                        k_source_values);
  auto chicken = std::make_unique<DataNode>("chicken", EP_32U,
                                            std::vector<size_t>{10}, nullptr);
  auto dog = std::make_unique<DataNode>("dog", EP_64F, std::vector<size_t>{10});
  auto pig = std::make_unique<StringNode>("pig", "daisy");

  ObjectNode root("farm");
  dog->addReference(epf::EP_HAS_CHILD, std::move(pig));
  root.addReference(epf::EP_HAS_CHILD, std::move(cow));
  root.addReference(epf::EP_HAS_CHILD, std::move(chicken));
  root.addReference(epf::EP_HAS_CHILD, std::move(dog));
  return Message(root);
}

}  // namespace

class QueueBasicTest : public ::testing::Test {
  protected:
    Queue queue_;
    int producer_{-1};
    int consumer_{-1};

    void SetUp() override
    {
      auto msg = std::make_unique<Message>();
      auto node = std::make_unique<DataNode>("counter", EP_32S,
                                             std::vector<size_t>{1}, nullptr);
      msg->addItem(std::move(node));
      queue_.init(4, std::move(msg));
      producer_ = queue_.subscribeProducer();
      consumer_ = queue_.subscribeConsumer();
      queue_.setWriteBlocking(producer_, false);
      queue_.setReadBlocking(consumer_, false);
    }

    void TearDown() override
    {
      if (producer_ >= 0) queue_.unsubscribeProducer(producer_);
      if (consumer_ >= 0) queue_.unsubscribeConsumer(consumer_);
      queue_.free();
    }
};

TEST_F(QueueBasicTest, WriteReadSingleValue)
{
  char *data = nullptr;
  char *hdr = nullptr;
  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value = reinterpret_cast<int32_t *>(data);
  *value = 77;
  ASSERT_EQ(queue_.endWrite(producer_), 0);

  ASSERT_GE(queue_.startRead(consumer_, &data, &hdr), 0);
  int32_t read_value = *reinterpret_cast<int32_t *>(data);
  EXPECT_EQ(read_value, 77);
  ASSERT_EQ(queue_.endRead(consumer_), 0);
}

TEST_F(QueueBasicTest, BlockingGettersReflectStateWithoutChangingIt)
{
  ASSERT_EQ(queue_.setWriteBlocking(producer_, false), 0);
  ASSERT_EQ(queue_.setReadBlocking(consumer_, false), 0);

  ASSERT_TRUE(queue_.writeBlocking(producer_).has_value());
  ASSERT_TRUE(queue_.readBlocking(consumer_).has_value());
  EXPECT_FALSE(queue_.writeBlocking(producer_).value());
  EXPECT_FALSE(queue_.readBlocking(consumer_).value());

  EXPECT_FALSE(queue_.writeBlocking(producer_).value());
  EXPECT_FALSE(queue_.readBlocking(consumer_).value());

  ASSERT_EQ(queue_.setWriteBlocking(producer_, true), 0);
  ASSERT_EQ(queue_.setReadBlocking(consumer_, true), 0);

  ASSERT_TRUE(queue_.writeBlocking(producer_).has_value());
  ASSERT_TRUE(queue_.readBlocking(consumer_).has_value());
  EXPECT_TRUE(queue_.writeBlocking(producer_).value());
  EXPECT_TRUE(queue_.readBlocking(consumer_).value());
}

// A failed subscription must release the queue mutex. Run the follow-up mutex
// operation in an alarm-guarded child so a regression reports a failure instead
// of hanging the entire GTest process.
TEST(QueueSubscriptionTest, ConsumerCapacityFailureReleasesMutex)
{
  EXPECT_EXIT(
      {
        Queue queue;
        if (queue.init(2, sizeof(uint32_t), 0, 1, 1) != 0) std::_Exit(1);

        const int consumer = queue.subscribeConsumer();
        if (consumer < 0) std::_Exit(2);

        ::alarm(2);
        if (queue.subscribeConsumer() != -1) std::_Exit(3);
        if (queue.setReadBlocking(consumer, false) != 0) std::_Exit(4);
        ::alarm(0);
        std::_Exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}

// Unsubscribing a consumer with an active read must release its contribution
// to workers_running_, otherwise Queue::free() waits forever.
TEST(QueueSubscriptionTest, WorkingConsumerUnsubscribeReleasesWorker)
{
  EXPECT_EXIT(
      {
        Queue queue;
        if (queue.init(2, sizeof(uint32_t), 0, 1, 1) != 0) std::_Exit(1);

        const int producer = queue.subscribeProducer();
        const int consumer = queue.subscribeConsumer();
        if (producer < 0 || consumer < 0) std::_Exit(2);

        char *data = nullptr;
        char *hdr = nullptr;
        if (queue.startWrite(producer, &data, &hdr) < 0) std::_Exit(3);
        if (queue.endWrite(producer) != 0) std::_Exit(4);
        if (queue.startRead(consumer, &data, &hdr) < 0) std::_Exit(5);

        ::alarm(2);
        if (queue.unsubscribeConsumer(consumer) != 0) std::_Exit(6);
        if (queue.unsubscribeProducer(producer) != 0) std::_Exit(7);
        if (queue.free() != 0) std::_Exit(8);
        ::alarm(0);
        std::_Exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}

TEST(QueueSubscriptionTest, SlowConsumerUnsubscribeUpdatesFifoBarrier)
{
  Queue queue;
  ASSERT_EQ(queue.setType(fifo), 0);
  ASSERT_EQ(queue.init(2, sizeof(uint32_t), 0, 2, 1), 0);

  const int producer = queue.subscribeProducer();
  const int slow_consumer = queue.subscribeConsumer();
  const int fast_consumer = queue.subscribeConsumer();
  ASSERT_GE(producer, 0);
  ASSERT_GE(slow_consumer, 0);
  ASSERT_GE(fast_consumer, 0);
  ASSERT_EQ(queue.setWriteBlocking(producer, false), 0);
  ASSERT_EQ(queue.setReadBlocking(slow_consumer, false), 0);
  ASSERT_EQ(queue.setReadBlocking(fast_consumer, false), 0);

  auto write_value = [&](uint32_t value) {
    char *data = nullptr;
    char *hdr = nullptr;
    const int result = queue.startWrite(producer, &data, &hdr);
    if (result < 0) return result;
    *reinterpret_cast<uint32_t *>(data) = value;
    return queue.endWrite(producer);
  };

  auto read_value = [&](int consumer, uint32_t expected) {
    char *data = nullptr;
    char *hdr = nullptr;
    const int result = queue.startRead(consumer, &data, &hdr);
    if (result < 0) return result;
    EXPECT_EQ(*reinterpret_cast<uint32_t *>(data), expected);
    return queue.endRead(consumer);
  };

  ASSERT_EQ(write_value(0), 0);
  ASSERT_EQ(read_value(slow_consumer, 0), 0);
  ASSERT_EQ(read_value(fast_consumer, 0), 0);

  ASSERT_EQ(write_value(1), 0);
  ASSERT_EQ(read_value(fast_consumer, 1), 0);
  ASSERT_EQ(write_value(2), 0);
  ASSERT_EQ(read_value(fast_consumer, 2), 0);

  EXPECT_LT(write_value(3), 0);

  ASSERT_EQ(queue.unsubscribeConsumer(slow_consumer), 0);
  ASSERT_EQ(write_value(3), 0);
  ASSERT_EQ(read_value(fast_consumer, 3), 0);

  const int resubscribed_consumer = queue.subscribeConsumer();
  ASSERT_GE(resubscribed_consumer, 0);
  ASSERT_EQ(queue.setReadBlocking(resubscribed_consumer, false), 0);

  char *data = nullptr;
  char *hdr = nullptr;
  EXPECT_LT(queue.startRead(resubscribed_consumer, &data, &hdr), 0);

  ASSERT_EQ(write_value(4), 0);
  ASSERT_EQ(read_value(fast_consumer, 4), 0);
  ASSERT_EQ(read_value(resubscribed_consumer, 4), 0);

  EXPECT_EQ(queue.unsubscribeConsumer(fast_consumer), 0);
  EXPECT_EQ(queue.unsubscribeConsumer(resubscribed_consumer), 0);
  EXPECT_EQ(queue.unsubscribeProducer(producer), 0);
  EXPECT_EQ(queue.free(), 0);
}

TEST_F(QueueBasicTest, MultipleStartPartialCommit)
{
  char *data = nullptr;
  char *hdr = nullptr;
  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value1 = reinterpret_cast<int32_t *>(data);
  *value1 = 10;

  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value2 = reinterpret_cast<int32_t *>(data);
  *value2 = 20;

  ASSERT_EQ(queue_.endWrite(producer_, 1), 0);

  ASSERT_GE(queue_.startRead(consumer_, &data, &hdr), 0);
  int32_t read1 = *reinterpret_cast<int32_t *>(data);
  EXPECT_EQ(read1, 10);
  ASSERT_EQ(queue_.endRead(consumer_), 0);

  ASSERT_EQ(queue_.endWrite(producer_), 0);

  ASSERT_GE(queue_.startRead(consumer_, &data, &hdr), 0);
  int32_t read2 = *reinterpret_cast<int32_t *>(data);
  EXPECT_EQ(read2, 20);
  ASSERT_EQ(queue_.endRead(consumer_), 0);
}

TEST_F(QueueBasicTest, AbortPendingBuffers)
{
  char *data = nullptr;
  char *hdr = nullptr;

  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value1 = reinterpret_cast<int32_t *>(data);
  *value1 = 10;

  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value2 = reinterpret_cast<int32_t *>(data);
  *value2 = 20;

  ASSERT_EQ(queue_.endWriteAbort(producer_), 0);

  ASSERT_GE(queue_.startWrite(producer_, &data, &hdr), 0);
  int32_t *value3 = reinterpret_cast<int32_t *>(data);
  *value3 = 30;
  ASSERT_EQ(queue_.endWrite(producer_), 0);

  ASSERT_GE(queue_.startRead(consumer_, &data, &hdr), 0);
  int32_t read = *reinterpret_cast<int32_t *>(data);
  EXPECT_EQ(read, 30);
  ASSERT_EQ(queue_.endRead(consumer_), 0);
}

TEST(QueueGuardTest, StartEndControlsPublicationAndReuse)
{
  Queue queue;
  ASSERT_EQ(queue.setType(fifo), 0);
  ASSERT_EQ(queue.init(1, create_queue_message()), 0);

  const int producer = queue.subscribeProducer();
  const int consumer = queue.subscribeConsumer();
  ASSERT_GE(producer, 0);
  ASSERT_GE(consumer, 0);
  ASSERT_EQ(queue.setWriteBlocking(producer, false), 0);
  ASSERT_EQ(queue.setReadBlocking(consumer, false), 0);

  char *write_data = nullptr;
  char *write_hdr = nullptr;
  ASSERT_GE(queue.startWrite(producer, &write_data, &write_hdr), 0);
  *reinterpret_cast<uint32_t *>(write_data) = 42;

  char *read_data = nullptr;
  char *read_hdr = nullptr;
  EXPECT_LT(queue.startRead(consumer, &read_data, &read_hdr), 0);

  ASSERT_EQ(queue.endWrite(producer), 0);
  ASSERT_GE(queue.startRead(consumer, &read_data, &read_hdr), 0);
  EXPECT_EQ(*reinterpret_cast<uint32_t *>(read_data), 42u);

  char *blocked_write_data = nullptr;
  char *blocked_write_hdr = nullptr;
  EXPECT_LT(queue.startWrite(producer, &blocked_write_data, &blocked_write_hdr),
            0);

  ASSERT_EQ(queue.endRead(consumer), 0);
  ASSERT_GE(queue.startWrite(producer, &write_data, &write_hdr), 0);
  *reinterpret_cast<uint32_t *>(write_data) = 84;
  ASSERT_EQ(queue.endWrite(producer), 0);

  ASSERT_GE(queue.startRead(consumer, &read_data, &read_hdr), 0);
  EXPECT_EQ(*reinterpret_cast<uint32_t *>(read_data), 84u);
  ASSERT_EQ(queue.endRead(consumer), 0);

  EXPECT_EQ(queue.unsubscribeProducer(producer), 0);
  EXPECT_EQ(queue.unsubscribeConsumer(consumer), 0);
  EXPECT_EQ(queue.free(), 0);
}

TEST(QueueNodeSchemaTest, DataNodeMemoryModesMatchLegacyHelpers)
{
  DataNode managed("managed", EP_64F, std::vector<size_t>{10});
  managed.write(k_source_values);
  ASSERT_NE(managed.value(), nullptr);
  EXPECT_TRUE(managed.memMgmt());
  EXPECT_EQ(std::memcmp(k_source_values, managed.value(), managed.size()), 0);

  DataNode streamed("streamed", EP_64F, std::vector<size_t>{10}, nullptr);
  streamed.setValue(k_source_values);
  EXPECT_FALSE(streamed.memMgmt());
  EXPECT_EQ(streamed.value(), k_source_values);

  DataNode external("external", EP_64F, std::vector<size_t>{10},
                    k_source_values);
  external.write(k_replacement_values);
  EXPECT_FALSE(external.memMgmt());
  EXPECT_EQ(
      std::memcmp(k_replacement_values, external.value(), external.size()), 0);
  EXPECT_EQ(std::memcmp(k_replacement_values, k_source_values,
                        sizeof(k_replacement_values)),
            0);
}

TEST(QueueNodeSchemaTest, ObjectNodeAssignmentReplacesNestedTree)
{
  ObjectNode root("obj1");
  auto obj2 = std::make_unique<ObjectNode>("obj2");
  auto obj3 = std::make_unique<ObjectNode>("obj3");
  auto data =
      std::make_unique<DataNode>("dn1", EP_64F, std::vector<size_t>{10});

  obj3->addReference(epf::EP_HAS_CHILD, std::move(data));
  obj2->addReference(epf::EP_HAS_CHILD, std::move(obj3));
  root.addReference(epf::EP_HAS_CHILD, std::move(obj2));

  ASSERT_EQ(root.references().size(), 1u);

  ObjectNode replacement("cpn");
  root = replacement;

  EXPECT_EQ(root.name(), "cpn");
  EXPECT_TRUE(root.references().empty());
}

TEST(QueueNodeSchemaTest, NestedMessageAssignmentRebuildsSchema)
{
  Message one_item;
  one_item.addItem(std::make_unique<DataNode>(
      "chicken", EP_32U, std::vector<size_t>{10}, nullptr));
  ASSERT_EQ(one_item.itemCount(), 1u);
  ASSERT_EQ(one_item.size(), sizeof(uint32_t) * 10u);

  Message nested = create_nested_message();
  ASSERT_EQ(nested.itemCount(), 3u);

  one_item = nested;

  EXPECT_EQ(one_item.itemCount(), nested.itemCount());
  EXPECT_EQ(one_item.size(), nested.size());
  EXPECT_NE(one_item.item("cow"), nullptr);
  EXPECT_NE(one_item.item("chicken"), nullptr);
  EXPECT_NE(one_item.item("dog"), nullptr);
}

TEST(QueueSchemaRoundTripTest, ScalarAndImageObjectRoundTrips)
{
  auto queue = make_queue(fifo, 10);
  QueuePayloadWriter writer(queue.get(), 8, false);
  QueuePayloadReader reader_a(queue.get(), 8, false);
  QueuePayloadReader reader_b(queue.get(), 8, false);

  ASSERT_EQ(writer.msg_size, reader_a.msg_size);
  ASSERT_EQ(writer.msg_size, reader_b.msg_size);

  auto *counter = writer.writer.dataSchema()->item(0);
  ASSERT_NE(counter, nullptr);
  ASSERT_TRUE(counter->isDataNode());

  auto *image_node =
      dynamic_cast<ObjectNode *>(writer.writer.dataSchema()->item(1));
  ASSERT_NE(image_node, nullptr);
  ImageObject writer_image(image_node);
  EXPECT_EQ(writer_image.width(), K_IMAGE_WIDTH);
  EXPECT_EQ(writer_image.height(), K_IMAGE_HEIGHT);
  EXPECT_EQ(writer_image.channels(), K_IMAGE_CHANNELS);
  EXPECT_EQ(writer_image.pixelFormat(), PixelFormat::RGBA8);

  for (int i = 0; i < writer.num_msgs; ++i) {
    ASSERT_EQ(writer.write(1), 0);
    ASSERT_EQ(reader_a.read(1, 1), 0);
    ASSERT_EQ(reader_b.read(1, 1), 0);
  }

  const size_t bytes = writer.msg_size * writer.num_msgs;
  expect_payload_eq(writer.data, reader_a.data, bytes);
  expect_payload_eq(writer.data, reader_b.data, bytes);
}

TEST(QueueBatchTest, LifoSingleMessageBatchesReachTwoReaders)
{
  auto queue = make_queue(lifo, 10);
  constexpr int K_MESSAGES = 500;
  QueuePayloadWriter writer(queue.get(), K_MESSAGES, true);
  QueuePayloadReader reader_a(queue.get(), K_MESSAGES, true);
  QueuePayloadReader reader_b(queue.get(), K_MESSAGES, true);

  for (int i = 0; i < K_MESSAGES; ++i) {
    ASSERT_EQ(writer.write(1), 0);
    ASSERT_EQ(reader_a.read(1, 1), 0);
    ASSERT_EQ(reader_b.read(1, 1), 0);
  }

  const size_t bytes = writer.msg_size * K_MESSAGES;
  expect_payload_eq(writer.data, reader_a.data, bytes);
  expect_payload_eq(writer.data, reader_b.data, bytes);
}

TEST(QueueBatchTest, LifoTwoMessageBatchesReachTwoReaders)
{
  auto queue = make_queue(lifo, 10);
  constexpr int K_MESSAGES = 500;
  QueuePayloadWriter writer(queue.get(), K_MESSAGES, true);
  QueuePayloadReader reader_a(queue.get(), K_MESSAGES, true);
  QueuePayloadReader reader_b(queue.get(), K_MESSAGES, true);

  for (int i = 0; i < K_MESSAGES / 2; ++i) {
    ASSERT_EQ(writer.write(2), 0);
    ASSERT_EQ(reader_a.read(2, 2), 0);
    ASSERT_EQ(reader_b.read(2, 2), 0);
  }

  const size_t bytes = writer.msg_size * K_MESSAGES;
  expect_payload_eq(writer.data, reader_a.data, bytes);
  expect_payload_eq(writer.data, reader_b.data, bytes);
}
