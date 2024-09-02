// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef QUEUE_HANDLERS_H
#define QUEUE_HANDLERS_H

#include <yaml-cpp/yaml.h>

#include "queue.h"

namespace ep {

/**
 * @brief Class for writing messages to a queue.
 */
class QueueWriter {
  private:
    std::shared_ptr<Queue> q_; /**< Shared pointer to the queue object. */
    Message data_msg_; /**< Data message object to write to the queue. */
    Message hdr_msg_;  /**< Header message object to write to the queue. */
    int id_;           /**< Writer ID. */
    int lenA_, lenB_;  /**< Length of messages in parts A and B. */
    char *dataPtrA_, *dataPtrB_; /**< Data pointers for parts A and B. */
    char *hdrPtrA_, *hdrPtrB_;   /**< Header pointers for parts A and B. */
    int messages_;               /**< Number of messages. */
    int batch_size_;             /**< Number of messages to be read by call. */

  public:
    /**
     * @brief Constructor.
     * @param queue_ptr Shared pointer to the queue object.
     */
    QueueWriter(std::shared_ptr<Queue> queue_ptr);

    /**
     * @brief Destructor. Handles cleanup of resources.
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
     * @param msgs Number of messages to write.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startWrite(int msgs);

    /**
     * @brief End writing the message to the queue.
     * @return Status code indicating success or error.
     */
    int endWrite();

    /**
     * @brief Abort writing due to a failure.
     * @return Status code indicating success or error.
     */
    int endWriteAbort();

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
    Message* dataSchema();

    /**
     * @brief Get the header message pointer.
     * @return Pointer to the header message object.
     */
    Message* hdrSchema();

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
    Message& dataMsg(int i = 0);

    /**
     * @brief Get the header message reference for the ith message.
     * @param i Index of the message.
     * @return Header message reference.
     */
    Message& hdrMsg(int i = 0);

    /**
     * @brief Get the pointer to the start of the data block (Part A).
     * @return Pointer to the start of serialized data block Part A.
     */
    char* dataPtrA() const;

    /**
     * @brief Get the pointer to the start of the data block (Part B).
     * @return Pointer to the start of serialized data block Part B.
     */
    char* dataPtrB() const;

    /**
     * @brief Get the pointer to the start of the header block (Part A).
     * @return Pointer to the start of serialized header block Part A.
     */
    char* hdrPtrA() const;

    /**
     * @brief Get the pointer to the start of the header block (Part B).
     * @return Pointer to the start of serialized header block Part B.
     */
    char* hdrPtrB() const;

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

    /**
     * @brief Get the shared pointer to the queue.
     * @return Shared pointer to the queue object.
     */
    std::shared_ptr<Queue> queue() const;

  private:
    /**
     * @brief Subscribe the writer to the queue.
     * @return Status code indicating success or error.
     */
    int subscribe();

    /**
     * @brief Unsubscribe the writer from the queue.
     * @return Status code indicating success or error.
     */
    int unsubscribe();
};  // QueueWriter Class

/**
 * @brief Class for reading messages from a queue.
 */
class QueueReader {
  private:
    std::shared_ptr<Queue> q_; /**< Shared pointer to the queue object. */
    Message data_msg_; /**< Data message object to read from the queue. */
    Message hdr_msg_;  /**< Header message object to read from the queue. */
    int id_;           /**< Reader ID. */
    int lenA_, lenB_;  /**< Length of messages in parts A and B. */
    char *dataPtrA_, *dataPtrB_; /**< Data pointers for parts A and B. */
    char *hdrPtrA_, *hdrPtrB_;   /**< Header pointers for parts A and B. */
    int messages_;               /**< Number of messages. */
    int batch_size_;             /**< Number of messages to be read by call. */
    int new_per_batch_; /**< Number of new messages expected in the batch. */

  public:
    /**
     * @brief Constructor.
     * @param queue_ptr Shared pointer to the queue object.
     */
    QueueReader(std::shared_ptr<Queue> queue_ptr);

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
     * @brief Start reading a message from the queue.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startRead();

    /**
     * @brief Start reading a block of messages from the queue.
     * @param msgs Number of messages to read.
     * @param new_msgs Number of new messages expected in the block.
     * @return Status code indicating success (>=0) or error (<0).
     */
    int startRead(int msgs, int new_msgs);

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
    Message* dataSchema();

    /**
     * @brief Get the header message pointer.
     * @return Pointer to the header message object.
     */
    Message* hdrSchema();

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
    Message& dataMsg(int i = 0);

    /**
     * @brief Get the header message reference for the ith message.
     * @param i Index of the message.
     * @return Header message reference.
     */
    Message& hdrMsg(int i = 0);

    /**
     * @brief Get the pointer to the start of the data block (Part A).
     * @return Pointer to the start of serialized data block Part A.
     */
    char* dataPtrA() const;

    /**
     * @brief Get the pointer to the start of the data block (Part B).
     * @return Pointer to the start of serialized data block Part B.
     */
    char* dataPtrB() const;

    /**
     * @brief Get the pointer to the start of the header block (Part A).
     * @return Pointer to the start of serialized header block Part A.
     */
    char* hdrPtrA() const;

    /**
     * @brief Get the pointer to the start of the header block (Part B).
     * @return Pointer to the start of serialized header block Part B.
     */
    char* hdrPtrB() const;

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

    /**
     * @brief Set the number of new messages in the batch.
     * @param new_count Number of new messages expected.
     * @return Status code indicating success or error.
     */
    int setNewPerBatch(int new_count);

    /**
     * @brief Get the number of new messages in the batch.
     * @return Number of new messages expected in the batch.
     */
    int newPerBatch() const;

    /**
     * @brief Get the shared pointer to the queue.
     * @return Shared pointer to the queue object.
     */
    std::shared_ptr<Queue> queue() const;

  private:
    /**
     * @brief Subscribe the reader to the queue.
     * @return Status code indicating success or error.
     */
    int subscribe();

    /**
     * @brief Unsubscribe the reader from the queue.
     * @return Status code indicating success or error.
     */
    int unsubscribe();
};  // QueueReader Class

void queueReaderSettingsFromYaml(QueueReader* r, int index,
                                 const YAML::Node& queueSettings);
void queueWriterSettingsFromYaml(QueueWriter* w, int index,
                                 const YAML::Node& queueSettings);

}  // namespace ep

#endif  // QUEUE_HANDLERS_H
