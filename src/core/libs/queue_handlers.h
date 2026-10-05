// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef QUEUE_HANDLERS_H
#define QUEUE_HANDLERS_H

#include <yaml-cpp/yaml.h>

#include "queue.h"

namespace epf {

/**
 * @brief Class for writing messages to a queue.
 */
class QueueWriter {
  private:
    Queue *queue_{nullptr}; /**< Pointer to the queue object. */

    Message data_msg_{}; /**< Data message object to write to the queue. */
    Message hdr_msg_{};  /**< Header message object to write to the queue. */
    int32_t id_{-1};     /**< Writer ID. */

    /**< Length of messages in parts A and B. */
    int32_t len_a_{0};
    int32_t len_b_{0};

    /**< Data pointers for parts A and B. */
    char *data_ptr_a_{nullptr};
    char *data_ptr_b_{nullptr};

    /**< Header pointers for parts A and B. */
    char *hdr_ptr_a_{nullptr};
    char *hdr_ptr_b_{nullptr};

    int32_t messages_{1};   /**< Number of messages. */
    int32_t batch_size_{1}; /**< Number of messages to be read by call. */

    bool timestamp_enabled_{false}; /**< Automatic timestamp writing */
    size_t timestamp_offset_{0};    /**< Offset inside header */

  public:
    /**
     * @brief Default Constructor.
     */
    QueueWriter() = default;
    /**
     * @brief Constructor.
     * @param queue_ptr pointer to the queue object.
     */
    QueueWriter(Queue *queue_ptr);

    /**
     * @brief Default Destructor. Handles cleanup of resources.
     */
    ~QueueWriter();

    /**
     * @brief Get the ID of the writer.
     * @return ID of the writer (>=0) or error code (<0).
     */
    int id() const;

    /**
     * @brief Start writing a message to the queue.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startWrite();

    /**
     * @brief Start writing a block of messages to the queue.
     * @param messages Number of messages to write.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startWrite(int messages);

    /**
     * @brief End writing the message to the queue.
     * @param done Number of completed messages to commit (-1 all pending).
     * @return Status code indicating success or error.
     */
    int endWrite(int done = -1);

    /**
     * @brief Abort writing due to a failure.
     * @param pending Number of messages to roll back (-1 all pending).
     * @return Status code indicating success or error.
     */
    int endWriteAbort(int pending = -1);

    /**
     * @brief Set blocking behavior for queue operations.
     * @param flag Boolean flag to set blocking calls.
     * @return Status code indicating success or error.
     */
    int setBlockingCalls(bool flag);

    /**
     * @brief Get the data message pointer.
     * @return Pointer to the data message object.
     */
    Message *dataSchema();

    /**
     * @brief Get the header message pointer.
     * @return Pointer to the header message object.
     */
    Message *hdrSchema();

    /**
     * @brief Get the number of available messages.
     * @return Number of available messages.
     */
    int msgCount() const;

    /**
     * @brief Get the data message reference for the ith message.
     * @param i Index of the message.
     * @return Data message reference.
     */
    Message *dataMsg(int i = 0);

    /**
     * @brief Get the header message reference for the ith message.
     * @param i Index of the message.
     * @return Header message reference.
     */
    Message *hdrMsg(int i = 0);

    /**
     * @brief Get the data pointer for a given queue index.
     * @param idx Queue index.
     * @return Pointer to the start of the data block for that index.
     */
    char *dataPtr(int idx) const;

    /**
     * @brief Prepare header message for a given queue index.
     * @param idx Queue index.
     * @return Header message reference.
     */
    Message *hdrMsgIdx(int idx);

    /**
     * @brief Get the pointer to the start of the data block (Part A).
     * @return Pointer to the start of serialized data block Part A.
     */
    char *dataPtrA() const;

    /**
     * @brief Get the pointer to the start of the data block (Part B).
     * @return Pointer to the start of serialized data block Part B.
     */
    char *dataPtrB() const;

    /**
     * @brief Get the pointer to the start of the header block (Part A).
     * @return Pointer to the start of serialized header block Part A.
     */
    char *hdrPtrA() const;

    /**
     * @brief Get the pointer to the start of the header block (Part B).
     * @return Pointer to the start of serialized header block Part B.
     */
    char *hdrPtrB() const;

    /**
     * @brief Get the number of messages in Part A.
     * @return Number of messages in Part A.
     */
    int lenA() const;

    /**
     * @brief Get the number of messages in Part B.
     * @return Number of messages in Part B.
     */
    int lenB() const;

    /**
     * @brief Set the number of messages to read per batch.
     * @param size Number of messages per batch.
     * @return Status code indicating success or error.
     */
    int setBatchSize(int size);

    /**
     * @brief Get the number of messages to read per batch.
     * @return Number of messages per batch.
     */
    int batchSize() const;

    /** Enable or disable automatic timestamping */
    void enableTimestamp(bool flag);

    /** Set offset for timestamp inside header */
    void setTimestampOffset(size_t offset);

    /** Check if timestamp is enabled */
    bool timestampEnabled() const;

    /**
     * @brief Get the pointer to the queue.
     * @return pointer to the queue object.
     */
    Queue *queue() const;

    /**
     * @brief Subscribe the writer to the queue.
     * @param Queue pointer.
     * @return Status code indicating success or error.
     */
    int subscribe(Queue *q);

    /**
     * @brief Unsubscribe the writer from the queue.
     * @return Status code indicating success or error.
     */
    int unsubscribe();

    /**
     * @brief Signal to wake up writers waiting in the queue.
     * @return Status code indicating success or error.
     */
    int wakeUp();

};  // QueueWriter Class

/**
 * @brief Class for reading messages from a queue.
 */
class QueueReader {
  private:
    Queue *queue_{nullptr}; /**< pointer to the queue object. */
    Message data_msg_{};    /**< Data message object to read from the queue. */
    Message hdr_msg_{}; /**< Header message object to read from the queue. */
    int32_t id_{-1};    /**< Reader ID. */

    /**< Length of messages in parts A and B. */
    int32_t lenA_{0};
    int32_t lenB_{0};

    /**< Data pointers for parts A and B. */
    char *dataPtrA_{nullptr};
    char *dataPtrB_{nullptr};

    /**< Header pointers for parts A and B. */
    char *hdrPtrA_{nullptr};
    char *hdrPtrB_{nullptr};

    int32_t message_window_{1};  // Number of messages to be read.
    int32_t message_stride_{1};  // Messages to advance after each read.

  public:
    /**
     * @brief Default Constructor.
     */
    QueueReader() = default;
    /**
     * @brief Constructor.
     * @param queue_ptr pointer to the queue object.
     */
    QueueReader(Queue *queue_ptr);

    /**
     * @brief Destructor. Handles cleanup of resources.
     */
    ~QueueReader();

    /**
     * @brief Get the ID of the reader.
     * @return ID of the reader (>=0) or error code (<0).
     */
    int id() const;

    /**
     * @brief Start reading using the configured message window and stride.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startRead();

    /**
     * @brief Start reading a block of messages from the queue.
     * @param message_window Number of messages exposed in the read window.
     * @param message_stride Number of messages by which the read window
     * advances.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startRead(int message_window, int message_stride);

    /**
     * @brief End reading the message from the queue.
     * @return Status code indicating success or error.
     */
    int endRead();

    /**
     * @brief Abort reading due to a failure.
     * @return Status code indicating success or error.
     */
    int endReadAbort();

    /**
     * @brief Set blocking behavior for queue operations.
     * @param flag Boolean flag to set blocking calls.
     * @return Status code indicating success or error.
     */
    int setBlockingCalls(bool flag);

    /**
     * @brief Get the data message pointer.
     * @return Pointer to the data message object.
     */
    Message *dataSchema();

    /**
     * @brief Get the header message pointer.
     * @return Pointer to the header message object.
     */
    Message *hdrSchema();

    /**
     * @brief Get the data message pointer for the ith message.
     * @param i Index of the message.
     * @return Data message pointer, nullptr if failed.
     */
    Message *dataMsg(int i = 0);

    /**
     * @brief Get the header message pointer for the ith message.
     * @param i Index of the message.
     * @return Header message pointer, nullptr if failed.
     */
    Message *hdrMsg(int i = 0);

    /**
     * @brief Get the pointer to the start of the data block (Part A).
     * @return Pointer to the start of serialized data block Part A.
     */
    char *dataPtrA() const;

    /**
     * @brief Get the pointer to the start of the data block (Part B).
     * @return Pointer to the start of serialized data block Part B.
     */
    char *dataPtrB() const;

    /**
     * @brief Get the pointer to the start of the header block (Part A).
     * @return Pointer to the start of serialized header block Part A.
     */
    char *hdrPtrA() const;

    /**
     * @brief Get the pointer to the start of the header block (Part B).
     * @return Pointer to the start of serialized header block Part B.
     */
    char *hdrPtrB() const;

    /**
     * @brief Get the number of messages in Part A.
     * @return Number of messages in Part A.
     */
    int lenA() const;

    /**
     * @brief Get the number of messages in Part B.
     * @return Number of messages in Part B.
     */
    int lenB() const;

    /**
     * @brief Set the number of messages to read per batch.
     * @param size Number of messages per batch.
     * @return Status code indicating success or error.
     */
    int setMessageWindow(int size);

    /**
     * @brief Get the number of messages to read per batch.
     * @return Number of messages per batch.
     */
    int messageWindow() const;

    /**
     * @brief Set the number of new messages in the batch.
     * @param new_count Number of new messages expected.
     * @return Status code indicating success or error.
     */
    int setMessageStride(int new_count);

    /**
     * @brief Get the number of new messages in the batch.
     * @return Number of new messages expected in the batch.
     */
    int messageStride() const;

    /**
     * @brief Get a pointer to the queue.
     * @return Raw pointer to the queue object (unsafe, can be dangling).
     */
    Queue *queue() const;

    /**
     * @brief Subscribe the reader to the queue.
     * @param Queue pointer.
     * @return Status code indicating success or error.
     */
    int subscribe(Queue *q);

    /**
     * @brief Unsubscribe the reader from the queue.
     * @return Status code indicating success or error.
     */
    int unsubscribe();

    /**
     * @brief Signal Wake up reader waiting in the queue.
     * @return Status code indicating success or error.
     */
    int wakeUp();

};  // QueueReader Class

}  // namespace epf

#endif  // QUEUE_HANDLERS_H
