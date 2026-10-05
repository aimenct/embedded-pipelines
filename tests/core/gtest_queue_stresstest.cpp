// Concurrency stress tests for epf::Queue.
//
// These are deliberately separate from gtest_queue.cpp (which covers
// single-threaded/guarded correctness): this file hammers the queue with
// many consumer threads with one producer to exercise the pthread mutex/condvar
// bookkeeping (barrier recompute, subscribe/unsubscribe churn, teardown
// while blocked) under real contention. Intended to also be built and run
// under ThreadSanitizer (-fsanitize=thread) as a separate CI lane.
#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

constexpr int K_STRESS_TAG_WORDS = 32;  // 256-byte payload

// A payload whose every word carries the same (producer, sequence) tag. Any
// torn/interleaved write by a racing producer shows up as mismatched words.
struct StressPayload {
    std::array<uint64_t, K_STRESS_TAG_WORDS> words;
};

uint64_t make_tag(uint32_t producer_id, uint32_t sequence)
{
  return (static_cast<uint64_t>(producer_id) << 32) | sequence;
}

void fill_stress_payload(StressPayload &payload, uint32_t producer_id,
                         uint32_t sequence)
{
  payload.words.fill(make_tag(producer_id, sequence));
}

bool tag_is_consistent(const StressPayload &payload, uint32_t &producer_id,
                       uint32_t &sequence)
{
  const uint64_t tag = payload.words[0];
  for (uint64_t word : payload.words) {
    if (word != tag) return false;
  }
  producer_id = static_cast<uint32_t>(tag >> 32);
  sequence = static_cast<uint32_t>(tag & 0xffffffffu);
  return true;
}

std::unique_ptr<Queue> make_stress_queue(QueueType type, int length,
                                         int max_consumers)
{
  auto queue = std::make_unique<Queue>();
  EXPECT_EQ(queue->setType(type), 0);
  EXPECT_EQ(queue->init(length, sizeof(StressPayload), 0, max_consumers,
                        /*max_producers=*/1),
            0);
  return queue;
}

// Batched writer/reader pair built on QueueWriter/QueueReader (rather than
// the raw single-message Queue:: calls used elsewhere in this file), so
// batch sizes can differ per consumer and the wraparound A/B split path gets
// exercised under real contention. Each written message is self-tagged
// (producer_id=0, sequential index), so a reader can detect corruption
// without keeping a separate expected-payload buffer around.
class StressBatchWriter {
  public:
    QueueWriter writer;
    int batch_size;
    uint32_t next_sequence{0};

    StressBatchWriter(Queue *q, int batch, bool blocking)
        : writer(q),
          batch_size(batch)
    {
      writer.setBlockingCalls(blocking);
    }

    int writeBatch()
    {
      const int err = writer.startWrite(batch_size);
      if (err < 0) return err;

      const int len_a = writer.lenA();
      const int len_b = writer.lenB();
      char *ptr_a = writer.dataPtrA();
      char *ptr_b = writer.dataPtrB();
      for (int i = 0; i < len_a; ++i) {
        fill_stress_payload(
            *reinterpret_cast<StressPayload *>(
                ptr_a + static_cast<size_t>(i) * sizeof(StressPayload)),
            /*producer_id=*/0, next_sequence++);
      }
      for (int i = 0; i < len_b; ++i) {
        fill_stress_payload(
            *reinterpret_cast<StressPayload *>(
                ptr_b + static_cast<size_t>(i) * sizeof(StressPayload)),
            /*producer_id=*/0, next_sequence++);
      }
      return writer.endWrite();
    }
};

class StressBatchReader {
  public:
    QueueReader reader;
    int message_window;
    int message_stride;
    uint32_t expected_sequence{0};

    StressBatchReader(Queue *q, int batch, int new_msgs, bool blocking)
        : reader(q),
          message_window(batch),
          message_stride(new_msgs)
    {
      reader.setBlockingCalls(blocking);
    }

    // Reads one batch; every message must be internally consistent and the
    // next expected sequence number in strict order (single producer, FIFO
    // delivery order is well-defined per consumer).
    int readBatch(std::atomic<int> &corruption_count)
    {
      const int err = reader.startRead(message_window, message_stride);
      if (err < 0) return err;

      const int len_a = reader.lenA();
      const int len_b = reader.lenB();
      char *ptr_a = reader.dataPtrA();
      char *ptr_b = reader.dataPtrB();

      auto check = [&](char *base, int count) {
        for (int i = 0; i < count; ++i) {
          uint32_t producer_id = 0;
          uint32_t sequence = 0;
          const auto &payload = *reinterpret_cast<StressPayload *>(
              base + static_cast<size_t>(i) * sizeof(StressPayload));
          const bool consistent =
              tag_is_consistent(payload, producer_id, sequence);
          if (!consistent || producer_id != 0 ||
              sequence != expected_sequence) {
            ++corruption_count;
          }
          ++expected_sequence;
        }
      };
      check(ptr_a, len_a);
      check(ptr_b, len_b);
      return reader.endRead();
    }
};

// GoogleTest has no built-in per-test timeout. Without this, a regression
// that reintroduces a lost-wakeup/deadlock in Queue would hang the test
// binary (and CI) forever instead of failing loudly. Construct at the top of
// a stress test body; if the scope hasn't exited within `timeout`, the
// process is aborted with a diagnostic instead of hanging.
class DeadlineWatchdog {
  public:
    DeadlineWatchdog(std::string label, std::chrono::milliseconds timeout)
        : label_(std::move(label)),
          timeout_(timeout),
          thread_([this] { run(); })
    {
    }

    ~DeadlineWatchdog()
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        done_ = true;
      }
      cv_.notify_all();
      thread_.join();
    }

  private:
    void run()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!cv_.wait_for(lock, timeout_, [this] { return done_; })) {
        std::cerr << "[DeadlineWatchdog] \"" << label_
                  << "\" did not finish within " << timeout_.count()
                  << "ms - assuming Queue deadlock, aborting process."
                  << std::endl;
        std::abort();
      }
    }

    std::string label_;
    std::chrono::milliseconds timeout_;
    bool done_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread thread_;
};

}  // namespace

TEST(QueueStressTest,
     FifoSingleProducerConcurrentConsumersDeliverEveryMessageIntact)
{
  DeadlineWatchdog watchdog(
      "FifoSingleProducerConcurrentConsumersDeliverEveryMessageIntact",
      std::chrono::seconds(30));

  constexpr int K_CONSUMERS = 4;
  constexpr uint32_t K_MESSAGES = 20000;

  auto queue = make_stress_queue(fifo, /*length=*/8, K_CONSUMERS);

  const int producer_id = queue->subscribeProducer();
  ASSERT_GE(producer_id, 0);
  ASSERT_EQ(queue->setWriteBlocking(producer_id, true), 0);

  std::vector<int> consumer_ids;
  for (int i = 0; i < K_CONSUMERS; ++i) {
    const int id = queue->subscribeConsumer();
    ASSERT_GE(id, 0);
    ASSERT_EQ(queue->setReadBlocking(id, true), 0);
    consumer_ids.push_back(id);
  }

  std::atomic<int> corrupted_messages{0};

  std::thread producer_thread([queue = queue.get(), producer_id] {
    for (uint32_t seq = 0; seq < K_MESSAGES; ++seq) {
      char *data = nullptr;
      char *hdr = nullptr;
      if (queue->startWrite(producer_id, &data, &hdr) < 0) return;
      fill_stress_payload(*reinterpret_cast<StressPayload *>(data),
                          /*producer_id=*/0, seq);
      queue->endWrite(producer_id);
    }
  });

  // Single producer means FIFO delivery order is well-defined: every
  // consumer must see sequence 0..K_MESSAGES-1 exactly once, in order.
  std::vector<std::thread> consumer_threads;
  for (int c = 0; c < K_CONSUMERS; ++c) {
    consumer_threads.emplace_back([queue = queue.get(),
                                   consumer_id = consumer_ids[c],
                                   &corrupted_messages] {
      uint32_t expected_sequence = 0;
      for (uint32_t i = 0; i < K_MESSAGES; ++i) {
        char *data = nullptr;
        char *hdr = nullptr;
        if (queue->startRead(consumer_id, &data, &hdr) < 0) return;

        uint32_t producer_id_out = 0;
        uint32_t sequence = 0;
        const bool consistent =
            tag_is_consistent(*reinterpret_cast<StressPayload *>(data),
                              producer_id_out, sequence);
        queue->endRead(consumer_id);

        if (!consistent || producer_id_out != 0 ||
            sequence != expected_sequence) {
          ++corrupted_messages;
        }
        ++expected_sequence;
      }
    });
  }

  producer_thread.join();
  for (auto &t : consumer_threads) t.join();

  EXPECT_EQ(corrupted_messages.load(), 0);

  for (int id : consumer_ids) queue->unsubscribeConsumer(id);
  queue->unsubscribeProducer(producer_id);
  queue->free();
}

TEST(QueueStressTest, FifoProducerUnblocksAfterMixedBatchConsumersFinish)
{
  DeadlineWatchdog watchdog(
      "FifoProducerUnblocksAfterMixedBatchConsumersFinish",
      std::chrono::seconds(30));

  constexpr uint32_t K_MESSAGES = 20000;
  // Headroom so the writer is never the bottleneck for any reader's batch
  // rounding; unconsumed tail is drained defensively below.
  constexpr uint32_t K_WRITER_MESSAGES = K_MESSAGES + 64;

  auto queue = make_stress_queue(fifo, /*length=*/32, /*max_consumers=*/3);

  StressBatchWriter writer(queue.get(), /*batch=*/4, /*blocking=*/true);
  StressBatchReader reader_a(queue.get(), /*batch=*/7, /*new_msgs=*/7,
                             /*blocking=*/true);
  StressBatchReader reader_b(queue.get(), /*batch=*/1, /*new_msgs=*/1,
                             /*blocking=*/true);
  StressBatchReader reader_c(queue.get(), /*batch=*/3, /*new_msgs=*/3,
                             /*blocking=*/true);

  std::atomic<int> corrupted_messages{0};

  std::thread writer_thread([&] {
    while (writer.next_sequence < K_WRITER_MESSAGES) {
      if (writer.writeBatch() < 0) return;
    }
  });

  auto reader_job = [&](StressBatchReader *reader) {
    while (reader->expected_sequence < K_MESSAGES) {
      if (reader->readBatch(corrupted_messages) < 0) return;
    }
  };

  std::thread reader_thread_a(reader_job, &reader_a);
  std::thread reader_thread_b(reader_job, &reader_b);
  std::thread reader_thread_c(reader_job, &reader_c);

  reader_thread_a.join();
  reader_thread_b.join();
  reader_thread_c.join();

  // All readers are done; nobody will consume the writer's remaining
  // headroom, so unblock it defensively rather than let it hang.
  writer.writer.setBlockingCalls(false);
  queue->wakeUpProducers();
  writer_thread.join();

  EXPECT_EQ(corrupted_messages.load(), 0);
  EXPECT_GE(reader_a.expected_sequence, K_MESSAGES);
  EXPECT_GE(reader_b.expected_sequence, K_MESSAGES);
  EXPECT_GE(reader_c.expected_sequence, K_MESSAGES);
}

TEST(QueueStressTest, SubscribeUnsubscribeChurnUnderSteadyLoad)
{
  DeadlineWatchdog watchdog("SubscribeUnsubscribeChurnUnderSteadyLoad",
                            std::chrono::seconds(30));

  constexpr int K_MAX_CONSUMERS = 4;
  auto queue = make_stress_queue(fifo, /*length=*/8, K_MAX_CONSUMERS);

  const int producer_id = queue->subscribeProducer();
  ASSERT_GE(producer_id, 0);
  ASSERT_EQ(queue->setWriteBlocking(producer_id, true), 0);

  std::atomic<bool> stop_producer{false};
  std::atomic<uint32_t> next_sequence{0};

  std::thread producer_thread(
      [queue = queue.get(), producer_id, &stop_producer, &next_sequence] {
        while (!stop_producer.load(std::memory_order_relaxed)) {
          char *data = nullptr;
          char *hdr = nullptr;
          if (queue->startWrite(producer_id, &data, &hdr) < 0) return;
          const uint32_t seq =
              next_sequence.fetch_add(1, std::memory_order_relaxed);
          fill_stress_payload(*reinterpret_cast<StressPayload *>(data),
                              /*producer_id=*/0, seq);
          queue->endWrite(producer_id);
        }
      });

  std::atomic<int> corrupted_messages{0};

  // Repeatedly subscribe as a consumer, read a handful of messages, then
  // unsubscribe - this is the subscribe/unsubscribe + barrier-recompute path
  // flagged as the most complex/least-reviewed part of Queue.
  auto churn_job = [queue = queue.get(), &corrupted_messages] {
    constexpr int K_ITERATIONS = 200;
    constexpr int K_READS_PER_SUBSCRIPTION = 5;
    for (int iter = 0; iter < K_ITERATIONS; ++iter) {
      const int consumer_id = queue->subscribeConsumer();
      if (consumer_id < 0) {
        std::this_thread::yield();
        continue;
      }
      queue->setReadBlocking(consumer_id, true);

      for (int i = 0; i < K_READS_PER_SUBSCRIPTION; ++i) {
        char *data = nullptr;
        char *hdr = nullptr;
        if (queue->startRead(consumer_id, &data, &hdr) < 0) break;

        uint32_t producer_id_out = 0;
        uint32_t sequence_out = 0;
        if (!tag_is_consistent(*reinterpret_cast<StressPayload *>(data),
                               producer_id_out, sequence_out)) {
          ++corrupted_messages;
        }
        queue->endRead(consumer_id);
      }
      queue->unsubscribeConsumer(consumer_id);
    }
  };

  constexpr int K_CHURN_THREADS = 3;
  std::vector<std::thread> churn_threads;
  for (int i = 0; i < K_CHURN_THREADS; ++i)
    churn_threads.emplace_back(churn_job);

  for (auto &t : churn_threads) t.join();

  stop_producer.store(true);
  queue->setWriteBlocking(producer_id, false);
  queue->wakeUpProducers();
  producer_thread.join();

  EXPECT_EQ(corrupted_messages.load(), 0);

  // The queue must still be fully usable after the churn: a fresh
  // subscription should round-trip cleanly.
  const int final_consumer = queue->subscribeConsumer();
  ASSERT_GE(final_consumer, 0);
  ASSERT_EQ(queue->setReadBlocking(final_consumer, true), 0);
  ASSERT_EQ(queue->setWriteBlocking(producer_id, true), 0);

  char *data = nullptr;
  char *hdr = nullptr;
  ASSERT_GE(queue->startWrite(producer_id, &data, &hdr), 0);
  fill_stress_payload(*reinterpret_cast<StressPayload *>(data), 0, 999999);
  ASSERT_EQ(queue->endWrite(producer_id), 0);

  ASSERT_GE(queue->startRead(final_consumer, &data, &hdr), 0);
  uint32_t producer_id_out = 0;
  uint32_t sequence_out = 0;
  EXPECT_TRUE(tag_is_consistent(*reinterpret_cast<StressPayload *>(data),
                                producer_id_out, sequence_out));
  EXPECT_EQ(sequence_out, 999999u);
  ASSERT_EQ(queue->endRead(final_consumer), 0);

  queue->unsubscribeProducer(producer_id);
  queue->unsubscribeConsumer(final_consumer);
  queue->free();
}

TEST(QueueStressTest, FreeWakesBlockedProducerInsteadOfHanging)
{
  DeadlineWatchdog watchdog("FreeWakesBlockedProducerInsteadOfHanging",
                            std::chrono::seconds(15));

  auto queue = make_stress_queue(fifo, /*length=*/1, /*max_consumers=*/1);

  const int producer_id = queue->subscribeProducer();
  const int consumer_id = queue->subscribeConsumer();
  ASSERT_GE(producer_id, 0);
  ASSERT_GE(consumer_id, 0);
  ASSERT_EQ(queue->setWriteBlocking(producer_id, true), 0);

  // Fill the only slot. With nobody having read it yet, a second write must
  // block (queue full).
  char *data = nullptr;
  char *hdr = nullptr;
  ASSERT_GE(queue->startWrite(producer_id, &data, &hdr), 0);
  fill_stress_payload(*reinterpret_cast<StressPayload *>(data), 0, 0);
  ASSERT_EQ(queue->endWrite(producer_id), 0);

  std::atomic<int> blocked_write_result{1};  // sentinel: not yet returned
  std::thread blocked_writer(
      [queue = queue.get(), producer_id, &blocked_write_result] {
        char *inner_data = nullptr;
        char *inner_hdr = nullptr;
        blocked_write_result.store(
            queue->startWrite(producer_id, &inner_data, &inner_hdr));
      });

  // Best-effort: give the writer a chance to actually enter its blocking
  // wait before tearing the queue down under it.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ASSERT_EQ(queue->free(), 0);
  blocked_writer.join();

  EXPECT_LT(blocked_write_result.load(), 0);
  (void)consumer_id;
}

TEST(QueueStressTest, FreeWakesBlockedConsumerInsteadOfHanging)
{
  DeadlineWatchdog watchdog("FreeWakesBlockedConsumerInsteadOfHanging",
                            std::chrono::seconds(15));

  auto queue = make_stress_queue(fifo, /*length=*/4, /*max_consumers=*/1);

  const int producer_id = queue->subscribeProducer();
  const int consumer_id = queue->subscribeConsumer();
  ASSERT_GE(producer_id, 0);
  ASSERT_GE(consumer_id, 0);
  ASSERT_EQ(queue->setReadBlocking(consumer_id, true), 0);

  // Nothing has been written yet, so a blocking read has to wait.
  std::atomic<int> blocked_read_result{1};  // sentinel: not yet returned
  std::thread blocked_reader(
      [queue = queue.get(), consumer_id, &blocked_read_result] {
        char *data = nullptr;
        char *hdr = nullptr;
        blocked_read_result.store(queue->startRead(consumer_id, &data, &hdr));
      });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ASSERT_EQ(queue->free(), 0);
  blocked_reader.join();

  EXPECT_LT(blocked_read_result.load(), 0);
  (void)producer_id;
}
