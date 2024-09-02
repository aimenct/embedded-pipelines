// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef QUEUE_H
#define QUEUE_H

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cstdlib>
#include <list>
#include <map>

#include "ep_types.h"
#include "message.h"

inline int modulus(int x, int N)
{
  return (((x < 0) ? ((x % N) + N) : x) % N);
}

namespace ep {

enum QueueType { fifo, lifo };
enum ScheduleMode { push, pull };

/* Queue - return error definition */
constexpr int QE_OK = 0;              // success
constexpr int QE_NOT_PERMITTED = -1;  // operation not permitted
constexpr int QE_DISABLED = -2;       // queue disabled

/* Forward declaration of Producer and Consumer classes for internal use */
class Qproducer;
class Qconsumer;

/**
 * @class Queue
 * @brief A versatile circular buffer with support for multiple producers and
 * consumers.
 *
 * The Queue class provides a flexible implementation of a circular buffer
 * designed to manage a queue of N messages, each with the same size and
 * structure. It supports both LIFO (Last In First Out) and FIFO (First In First
 * Out) queuing strategies, allowing multiple producers and consumers. Various
 * configuration options for buffer management and blocking behavior are
 * available.
 */
class Queue {
  private:
    char status_;  // Queue status: 'c'-connected, 'd'-disconnected

    /* Synchronization primitives */
    pthread_mutex_t buffer_mtx_;
    pthread_cond_t buffer_full_cond_;   // condition variable for producers
    pthread_cond_t buffer_empty_cond_;  // condition variable for consumers
    pthread_cond_t exit_cond_;
    int count_full_cond_;
    int count_empty_cond_;

    size_t data_size_;  // Size in bytes of a data element
    size_t hdr_size_;   // Size in bytes of a header element
    int length_;        // Number of elements in the queue

    char *data_buffer_;  // Circular buffer for data
    char *hdr_buffer_;   // Circular buffer for headers

    Message *data_message_;
    Message *hdr_message_;

    int max_producers_;  // Maximum number of producers allowed
    int max_consumers_;  // Maximum number of consumers allowed

    /* Lists to track the status of producers and consumers */
    Qproducer *producers_;
    Qconsumer *consumers_;
    std::list<int> consumers_on_;
    std::list<int> consumers_off_;
    std::list<int> producers_on_;
    std::list<int> producers_off_;

    QueueType queue_type_;        // fifo, lifo
    ScheduleMode schedule_mode_;  // push, pull

    //  Callbacks for reading in pull mode
    //    void *pvt_;
    //  start_callback_t start_read_callback_;
    //  end_callback_t end_read_callback_;

    int barrier_;  // Pointer to the first buffer blocked. If -1, barrier is not
                   // active
    int workers_running_;  // Number of producer/consumers with buffers in use
    int counter_;          // Number of elements inserted in the queue
    int counter_sat_;      // Counter with saturation
    int next_input_;  // Pointer to the first empty buffer (next producer write
                      // location)
    int last_input_;  // Pointer to the last inserted buffer

  public:
    /**
     * @brief Default constructor.
     * Initializes an empty Queue.
     */
    Queue();

    /**
     * @brief Destructor.
     * Cleans up resources used by the Queue.
     */
    ~Queue();

    /**
     * @brief Initialize the circular buffer with given dimensions, allocating
     * memory.
     * @param length Number of messages in the circular buffer.
     * @param data_size Size of each data element in bytes.
     * @param hdr_size Size of each header element in bytes (optional).
     * @param max_consumers Maximum number of concurrent consumers allowed
     * (optional).
     * @param max_producers Maximum number of concurrent producers allowed
     * (optional).
     * @return Error code: 0 for success, non-zero for failure.
     */
    int init(int length, size_t data_size, size_t hdr_size = 0,
             int max_consumers = 4, int max_producers = 1);

    /**
     * @brief Initialize the circular buffer with given dimensions, using
     * message schema.
     * @param length Number of messages in the circular buffer.
     * @param data_s Pointer to data schema.
     * @param hdr_s Pointer to header schema (optional).
     * @param max_consumers Maximum number of concurrent consumers allowed
     * (optional).
     * @param max_producers Maximum number of concurrent producers allowed
     * (optional).
     * @return Error code: 0 for success, non-zero for failure.
     */
    int init(int length, Message *data_s, Message *hdr_s = nullptr,
             int max_consumers = 4, int max_producers = 1);

    /**
     * @brief Release the queue, freeing allocated memory.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int free();

    /**
     * @brief Set type of Queue: LIFO or FIFO.
     * @param type Queue type: LIFO or FIFO.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int setQueueType(QueueType type);

    /**
     * @brief Get type of Queue: LIFO or FIFO.
     * @return Queue type: LIFO or FIFO.
     */
    QueueType queueType();

    //  /**
    //  * @brief Set scheduling mode: PUSH or PULL.
    //  * @param mode Scheduling mode: PUSH or PULL.
    //  * @return Error code: 0 for success, non-zero for failure.
    //  */
    // int setScheduleMode(ScheduleMode mode);

    //  /**
    //  * @brief Get schedule mode: PUSH or PULL.
    //  * @return Scheduling mode: PUSH or PULL.
    //  */
    // ScheduleMode scheduleMode();

    /**
     * @brief Subscribe a producer to the queue.
     * @return Producer ID >0, which is a unique identifier for the subscribed
     * producer, or Error Code <0.
     */
    int subscribeProducer();

    /**
     * @brief Unsubscribe a producer from the queue.
     * @param producer_id Producer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int unsubscribeProducer(int producer_id);

    /**
     * @brief Subscribe a consumer to the queue.
     * @return Consumer ID >0, which is a unique identifier for the subscribed
     * consumer, or Error Code <0.
     */
    int subscribeConsumer();

    /**
     * @brief Unsubscribe a consumer from the queue.
     * @param consumer_id Consumer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int unsubscribeConsumer(int consumer_id);

    /**
     * @brief Get the position in the circular buffer where to write a block of
     * n messages.
     * @param producer_id Producer ID.
     * @param num_messages Number of data messages to write.
     * @return Position in the circular buffer if (>=0), error code if (<0).
     */
    int startWritePtr(int producer_id, int num_messages);

    /**
     * @brief Get a pointer to the messages in the circular buffer for
     * writing.
     * @param producer_id Producer ID.
     * @return Pointer to the message in the queue, or nullptr if error.
     */
    char *startWrite(int producer_id);

    /**
     * @brief Get data and header pointers where to write new data.
     * @param producer_id Producer ID.
     * @param data Pointer to data circular buffer.
     * @param hdr Pointer to header circular buffer.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int startWrite(int producer_id, char **data, char **hdr);

    /**
     * @brief Get data and header pointers for writing new data with multiple
     * buffers.
     * @param producer_id Producer ID.
     * @param num_messages Number of data messages to write.
     * @param data Pointer to data circular buffer.
     * @param hdr Pointer to header circular buffer.
     * @param len Number of messages to write from data and hdr pointers.
     * @param data1 Pointer to second data circular buffer.
     * @param hdr1 Pointer to second header circular buffer.
     * @param len1 Number of messages to write from data1 and hdr1 pointers.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int startWrite(int producer_id, int num_messages, char **data, char **hdr,
                   int *len, char **data1, char **hdr1, int *len1);

    /**
     * @brief Release the buffer pointer after writing.
     * @param producer_id Producer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int endWrite(int producer_id);

    /**
     * @brief End write operation without writing buffers (for one producer
     * only).
     * @param producer_id Producer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int endWriteAbort(int producer_id);

    /**
     * @brief Get the position in the circular buffer from where to read a block
     * of n messages.
     * @param consumer_id Consumer ID.
     * @param num_messages Number of messages to read.
     * @param new_messages Read at least this amount of new messages.
     * @return Position in the circular buffer if (>=0), error code if (<0).
     */
    int startReadPtr(int consumer_id, int num_messages, int new_messages);

    /**
     * @brief Get a pointer to the message in the circular buffer for reading.
     * @param consumer_id Consumer ID.
     * @return Pointer to the message in the queue, or nullptr if error.
     */
    char *startRead(int consumer_id);

    /**
     * @brief Get a buffer pointer from where to read a data block.
     * @param consumer_id Consumer ID.
     * @param data Pointer to the data circular buffer.
     * @param hdr Pointer to the header circular buffer.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int startRead(int consumer_id, char **data, char **hdr);

    /**
     * @brief Get data and header pointers from where to read a data block with
     * multiple buffers.
     * @param consumer_id Consumer ID.
     * @param num_messages Number of data messages to read.
     * @param new_messages Read at least this amount of new messages.
     * @param data Pointer to the data circular buffer.
     * @param hdr Pointer to the header circular buffer.
     * @param len Number of messages to read from data and hdr pointers.
     * @param data1 Pointer to the second data circular buffer.
     * @param hdr1 Pointer to the second header circular buffer.
     * @param len1 Number of messages to read from data1 and hdr1 pointers.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int startRead(int consumer_id, int num_messages, int new_messages,
                  char **data, char **hdr, int *len, char **data1, char **hdr1,
                  int *len1);

    /**
     * @brief Release the buffer pointer after reading.
     * @param consumer_id Consumer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int endRead(int consumer_id);

    /**
     * @brief End read operation without reading buffers (for one consumer
     * only).
     * @param consumer_id Consumer ID.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int endReadAbort(int consumer_id);

    //  /**
    //  * @brief Read data and header messages from the circular buffer with
    //  * memory copy.
    //  * @param consumer_id Consumer ID.
    //  * @param data Data pointer where to copy data from the queue.
    //  * @param hdr Header pointer where to copy header data from the queue.
    //  * @param num_messages Number of data messages to read (copy).
    //  * @param new_messages Read at least this amount of new messages.
    //  * @return Error code: 0 for success, non-zero for failure.
    //  */
    // int readCopy(int consumer_id, char *data, char *hdr, int num_messages =
    // 1,
    //              int new_messages = 1);

    /**
     * @brief Set the blocking behavior for write operations.
     * @param producer_id Producer ID.
     * @param blocking True if write operations should block, false otherwise.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int setWriteBlocking(int id, bool blocking);

    /**
     * @brief Set the blocking behavior for read operations.
     * @param consumer_id Consumer ID.
     * @param blocking True if read operations should block, false otherwise.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int setReadBlocking(int id, bool blocking);

    /**
     * @brief Get the number of messages in the circular buffer.
     * @return Number of messages.
     */
    int length() const;

    /**
     * @brief Get the size of the data element in the circular buffer.
     * @return Size of data element in bytes.
     */
    size_t dataSize() const;

    /**
     * @brief Get the size of the header element in the circular buffer.
     * @return Size of header element in bytes.
     */
    size_t hdrSize() const;

    /**
     * @brief Get the pointer to the data buffer in the circular buffer.
     * @return Pointer to the data buffer.
     */
    char *dataBuffer() const;

    /**
     * @brief Get the pointer to the header buffer in the circular buffer.
     * @return Pointer to the header buffer.
     */
    char *hdrBuffer() const;

    /**
     * @brief Get the pointer to the data message schema.
     * @return Pointer to the data message schema.
     */
    Message *dataMessage() const;

    /**
     * @brief Get the pointer to the header message schema.
     * @return Pointer to the header message schema.
     */
    Message *hdrMessage() const;

    /**
     * @brief Print the statistics of the circular buffer.
     */
    void printStats() const;

    /**
     * @brief Wake up all the producers.
     */
    void wakeUpProducers();

    /**
     * @brief Wake up all the consumers.
     */
    void wakeUpConsumers();
};

class Qproducer {
  public:
    int id;            // thead identifier
    char status;       // 's'-subscribed, 'u'-unsubscribed, 'w'-working
    int counter;       // counter of produced elements
    char blocking;     // blocking calls: 'n'-no, 'y'-yes
    int next_input;    // pointer to next empty buffer for writting
    int num_elements;  // number of elements to write
    Qproducer();
    void reset();
    void print();
};

class Qconsumer {
  public:
    int id;            // thread identifier
    char status;       // 's'-subscribed, 'u'-unsubscribed, 'w'-working
    int last_out;      // TODO last_output pointer to the last read buffer
    int ptr_counter;   // last count number
    int counter;       // counter of consumed elements
    int lost_counter;  // buffers lost - elements in the queue
    void *ptr_aux;     // pointer for callback use
    char blocking;     // blocking calls: 'n'-no, 'y'-yes
    int num_elements;  // number of elements to read
    int new_elements;  // number of new elements to read
    int free_elements;
    Qconsumer();
    void reset();
    void print();
};

}  // namespace ep

#endif  // QUEUE_H
