// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "queue_handlers.h"

#include <cstring>

#include "time_utils.h"

using namespace epf;

QueueWriter::QueueWriter(Queue *q)
{
  queue_ = q;
  subscribe(queue_);
}

QueueWriter::~QueueWriter()
{
  if (queue_) {
    this->unsubscribe();
    queue_ = nullptr;
    id_ = -1;
  }
}

int QueueWriter::subscribe(Queue *q)
{
  if (q == nullptr) {
    std::cerr << "QueueWriter subscribed q == nullptr " << std::endl;
    return -1;
  }

  if (id_ != -1) {
    std::cerr << "QueueWriter already subscribed id != -1 " << std::endl;
    return -1;
  }

  queue_ = q;

  id_ = queue_->subscribeProducer();
  if (id_ >= 0) {
    //    std::cout << "Writer subscribed with ID: " << id_ << std::endl;

    if (queue_->dataMessage() != nullptr) data_msg_ = *(queue_->dataMessage());
    //    else
    // std::cout << "Writer subscribe: queue doesn't have data msg structure"
    //           << std::endl;

    if (queue_->hdrMessage() != nullptr) hdr_msg_ = *(queue_->hdrMessage());
    //    else
    // std::cout << "Writer subscribe: queue doesn't have hdr msg structure"
    //           << std::endl;

    return 0;
  }
  std::cerr << "Error: Writer subscribe operation failed " << id_ << std::endl;
  return id_;
}

int QueueWriter::unsubscribe()
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter unsubscribed failed. Queue == nullptr "
              << std::endl;
    return -1;
  }

  if (id_ == -1) {
    std::cerr << "QueueWriter already unsubscribed id == -1 " << std::endl;
    return -1;
  }

  int err = 0;
  err = queue_->unsubscribeProducer(id_);
  if (err < 0) {
    std::cerr << "Error: Writer unsubscribe operation failed " << err
              << std::endl;
    return -1;
  }
  //  std::cerr << "Writer unsubscribe with ID: " << id_ << std::endl;
  id_ = -1;
  queue_ = nullptr;
  return 0;
}

int QueueWriter::id() const
{
  return id_;
}

int QueueWriter::startWrite()
{
  messages_ = 1;
  int32_t err = startWrite(messages_);
  if (err < 0) {
    return err;
  }

  data_msg_.updateMessage(data_ptr_a_);
  hdr_msg_.updateMessage(hdr_ptr_a_);
  return err;
}

int QueueWriter::startWrite(int messages)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter startWrite failed. Queue == nullptr "
              << std::endl;
    return -1;
  }

  messages_ = messages;
  int32_t err = queue_->startWrite(id_, messages_, &data_ptr_a_, &hdr_ptr_a_,
                                   &len_a_, &data_ptr_b_, &hdr_ptr_b_, &len_b_);
  if (err < 0) {
    // std::cerr << "Error: Writer startWrite operation failed " << err
    //           << std::endl;
    return err;
  }
  // data_msg_.updateMessage(data_ptr_a_);
  // hdr_msg_.updateMessage(hdr_ptr_a_);
  return err;
}

int QueueWriter::endWrite(int done)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter endWrite failed. Queue == nullptr " << std::endl;
    return -1;
  }

  if (timestamp_enabled_) {
    uint64_t ts = get_unix_timestamp_ms();
    for (int i = 0; i < messages_; ++i) {
      char *base_ptr = nullptr;
      if (i < len_a_) {
        base_ptr = hdr_ptr_a_ + static_cast<size_t>(i) * queue_->hdrSize();
      }
      else {
        base_ptr =
            hdr_ptr_b_ + static_cast<size_t>(i - len_a_) * queue_->hdrSize();
      }
      if (base_ptr != nullptr) {
        std::memcpy(base_ptr + timestamp_offset_, &ts, sizeof(uint64_t));
      }
    }
  }

  int32_t err = queue_->endWrite(id_, done);
  if (err < 0) {
    std::cerr << "Error: Writer endWrite operation failed " << err << std::endl;
  }

  return err;
}

int QueueWriter::endWriteAbort(int pending)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter endWriteAbort failed. Queue == nullptr "
              << std::endl;
    return -1;
  }

  int32_t err = queue_->endWriteAbort(id_, pending);
  if (err < 0) {
    std::cerr << "Error: Writer endWrite Abort operation failed " << err
              << std::endl;
  }
  return err;
}

int QueueWriter::setBlockingCalls(bool flag)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter setBlockingCalls failed. Queue == nullptr "
              << std::endl;
    return -1;
  }

  int32_t err = queue_->setWriteBlocking(id_, flag);
  if (err < 0) {
    std::cerr << "Error: Writer blockingCalls operation failed " << err
              << std::endl;
  }
  return err;
}

Message *QueueWriter::dataSchema()
{
  return &data_msg_;
}

Message *QueueWriter::hdrSchema()
{
  return &hdr_msg_;
}

int QueueWriter::msgCount() const
{
  return messages_;
}

Message *QueueWriter::dataMsg(int i)
{
  if ((i < messages_) && (i >= 0)) {
    if (i < len_a_) {
      char *address_to_update =
          data_ptr_a_ + static_cast<size_t>(i) * queue_->dataSize();
      data_msg_.updateMessage(address_to_update);
    }
    else {
      char *address_to_update =
          data_ptr_b_ + static_cast<size_t>(i) * queue_->dataSize();
      data_msg_.updateMessage(address_to_update);
    }
  }
  else {
    std::cerr << "QueueWriter: dataMsg beyond limits i=" << i << std::endl;
  }
  return &data_msg_;
}

Message *QueueWriter::hdrMsg(int i)
{
  if ((i < messages_) && (i >= 0)) {
    if (i < len_a_) {
      char *address_to_update =
          hdr_ptr_a_ + static_cast<size_t>(i) * queue_->hdrSize();
      hdr_msg_.updateMessage(address_to_update);
    }
    else {
      char *address_to_update =
          hdr_ptr_b_ + static_cast<size_t>(i) * queue_->hdrSize();
      hdr_msg_.updateMessage(address_to_update);
    }
  }
  else {
    std::cerr << "QueueWriter: hdrMsg beyond limits i=" << i << std::endl;
  }
  return &hdr_msg_;
}

char *QueueWriter::dataPtr(int idx) const
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter: dataPtr failed. Queue == nullptr" << std::endl;
    return nullptr;
  }
  int queue_slots = queue_->length();
  if (queue_->type() == fifo) {
    // FIFO queues keep one extra internal slot to disambiguate full vs empty.
    // startWrite() returns indices in that internal ring space.
    queue_slots += 1;
  }

  if ((idx < 0) || (idx >= queue_slots)) {
    std::cerr << "QueueWriter: dataPtr index out of range idx=" << idx
              << std::endl;
    return nullptr;
  }
  return queue_->dataBuffer() + static_cast<size_t>(idx) * queue_->dataSize();
}

Message *QueueWriter::hdrMsgIdx(int idx)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueWriter: hdrMsgIdx failed. Queue == nullptr" << std::endl;
    return &hdr_msg_;
  }
  int queue_slots = queue_->length();
  if (queue_->type() == fifo) {
    // FIFO queues keep one extra internal slot to disambiguate full vs empty.
    // startWrite() returns indices in that internal ring space.
    queue_slots += 1;
  }

  if ((idx < 0) || (idx >= queue_slots)) {
    std::cerr << "QueueWriter: hdrMsgIdx index out of range idx=" << idx
              << std::endl;
    return &hdr_msg_;
  }

  hdr_ptr_a_ =
      queue_->hdrBuffer() + static_cast<size_t>(idx) * queue_->hdrSize();
  hdr_ptr_b_ = nullptr;
  len_a_ = 1;
  len_b_ = 0;
  messages_ = 1;
  hdr_msg_.updateMessage(hdr_ptr_a_);
  return &hdr_msg_;
}

char *QueueWriter::dataPtrA() const
{
  return data_ptr_a_;
}

char *QueueWriter::dataPtrB() const
{
  return data_ptr_b_;
}

char *QueueWriter::hdrPtrA() const
{
  return hdr_ptr_a_;
}

char *QueueWriter::hdrPtrB() const
{
  return hdr_ptr_b_;
}

int QueueWriter::lenA() const
{
  return len_a_;
}

int QueueWriter::lenB() const
{
  return len_b_;
}

int QueueWriter::setBatchSize(int size)
{
  batch_size_ = size;
  return 0;
}

int QueueWriter::batchSize() const
{
  return batch_size_;
}

void QueueWriter::enableTimestamp(bool flag)
{
  timestamp_enabled_ = flag;
}

void QueueWriter::setTimestampOffset(size_t offset)
{
  timestamp_offset_ = offset;
}

bool QueueWriter::timestampEnabled() const
{
  return timestamp_enabled_;
}

Queue *QueueWriter::queue() const
{
  return queue_;
}

int QueueWriter::wakeUp()
{
  if (queue_ != nullptr) {
    queue_->wakeUpProducers();
    return 0;
  }
  else {
    std::cerr << "Error: Writer wake up - operation failed " << id_
              << std::endl;
    return -1;
  }
}

QueueReader::QueueReader(Queue *q)
{
  queue_ = q;
  subscribe(queue_);
}

QueueReader::~QueueReader()
{
  if (queue_) {
    this->unsubscribe();
    queue_ = nullptr;
    id_ = -1;
  }
}

int QueueReader::subscribe(Queue *q)
{
  if (q == nullptr) {
    std::cerr << "QueueReader: imposible to subscribe. q == nullptr "
              << std::endl;
    return -1;
  }

  if (id_ != -1) {
    std::cerr << "QueueReader: already subscribed id != -1 " << std::endl;
    return -1;
  }

  queue_ = q;

  id_ = queue_->subscribeConsumer();

  if (id_ >= 0) {
    //    std::cout << "Reader subscribed with ID: " << id_ << std::endl;

    if (queue_->dataMessage() != nullptr) data_msg_ = *(queue_->dataMessage());
    // else
    //   std::cout << "Reader subscribe: queue doesn't have Msg structure"
    //             << std::endl;

    if (queue_->hdrMessage() != nullptr) hdr_msg_ = *(queue_->hdrMessage());
    // else
    //   std::cout << "Reader subscribe: queue doesn't have hdr msg structure"
    //             << std::endl;

    return 0;
  }
  std::cerr << "Error: QueueReader: subscribe operation failed " << id_
            << std::endl;
  return id_;
}

int QueueReader::unsubscribe()
{
  if (queue_ == nullptr) {
    std::cerr << "Reader unsubscribed failed. Queue == nullptr " << std::endl;
    return -1;
  }

  if (id_ == -1) {
    std::cerr << "Reader already unsubscribed id == -1 " << std::endl;
    return -1;
  }

  int err = queue_->unsubscribeConsumer(id_);
  if (err < 0) {
    std::cerr << "Error: Reader with ID " << id_
              << " failed to unsubscribe. Error code: " << err << std::endl;
    return -1;
  }
  //  std::cerr << "Reader unsubscribe with ID: " << id_ << std::endl;
  id_ = -1;
  queue_ = nullptr;
  return 0;
}

int QueueReader::id() const
{
  return id_;
}

int QueueReader::startRead()
{
  int32_t ret = startRead(message_window_, message_stride_);
  if (ret < 0) {
    //   // std::cerr << "Error: Reader startRead operation failed " << err
    //   //           << std::endl;
    return ret;
  }

  data_msg_.updateMessage(dataPtrA_);
  hdr_msg_.updateMessage(hdrPtrA_);
  return ret;
}

int QueueReader::startRead(int message_window, int message_stride)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueReader startRead failed. Queue == nullptr " << std::endl;
    return -1;
  }
  int ret = queue_->startRead(id_, message_window, message_stride, &dataPtrA_,
                              &hdrPtrA_, &lenA_, &dataPtrB_, &hdrPtrB_, &lenB_);
  return ret;
}

int QueueReader::endRead()
{
  if (queue_ == nullptr) {
    std::cerr << "QueueReader endRead failed. Queue == nullptr " << std::endl;
    return -1;
  }

  int err = queue_->endRead(id_);
  if (err < 0) {
    std::cerr << "Error: Reader endRead operation failed " << err << std::endl;
  }
  return err;
}

int QueueReader::endReadAbort()
{
  int err = queue_->endReadAbort(id_);
  if (err < 0) {
    std::cerr << "Error: Reader endReadAbort operation failed " << err
              << std::endl;
  }
  return err;
}

int QueueReader::setBlockingCalls(bool flag)
{
  if (queue_ == nullptr) {
    std::cerr << "QueueReader setBlockingCalls failed. Queue == nullptr "
              << std::endl;
    return -1;
  }

  int err = queue_->setReadBlocking(id_, flag);
  if (err < 0) {
    std::cerr << "Error: Reader blockingCalls operation failed " << err
              << std::endl;
  }
  return err;
}

Message *QueueReader::dataSchema()
{
  return &data_msg_;
}

Message *QueueReader::hdrSchema()
{
  return &hdr_msg_;
}

Message *QueueReader::dataMsg(int i)
{
  if ((i < (lenA_ + lenB_)) && (i >= 0)) {
    if (i < lenA_) {
      char *address_to_update =
          dataPtrA_ + static_cast<size_t>(i) * queue_->dataSize();
      data_msg_.updateMessage(address_to_update);
    }
    else {
      char *address_to_update =
          dataPtrB_ + static_cast<size_t>(i) * queue_->dataSize();
      data_msg_.updateMessage(address_to_update);
    }
  }
  else {
    std::cerr << "QueueReader: dataMsg beyond limits i=" << i << std::endl;
    return nullptr;
  }
  return &data_msg_;
}

Message *QueueReader::hdrMsg(int i)
{
  if ((i < lenA_ + lenB_) && (i >= 0)) {
    if (i < lenA_) {
      char *address_to_update =
          hdrPtrA_ + static_cast<size_t>(i) * queue_->hdrSize();
      hdr_msg_.updateMessage(address_to_update);
    }
    else {
      char *address_to_update =
          hdrPtrB_ + static_cast<size_t>(i) * queue_->hdrSize();
      hdr_msg_.updateMessage(address_to_update);
    }
  }
  else {
    std::cerr << "QueueReader: hdrMsg beyond limits i=" << i << std::endl;
    return nullptr;
  }
  return &hdr_msg_;
}

char *QueueReader::dataPtrA() const
{
  return dataPtrA_;
}

char *QueueReader::dataPtrB() const
{
  return dataPtrB_;
}

char *QueueReader::hdrPtrA() const
{
  return hdrPtrA_;
}

char *QueueReader::hdrPtrB() const
{
  return hdrPtrB_;
}

int QueueReader::lenA() const
{
  return lenA_;
}

int QueueReader::lenB() const
{
  return lenB_;
}

int QueueReader::setMessageWindow(int size)
{
  message_window_ = size;
  return 0;
}

int QueueReader::messageWindow() const
{
  return message_window_;
}

int QueueReader::setMessageStride(int size)
{
  message_stride_ = size;
  return 0;
}

int QueueReader::messageStride() const
{
  return message_stride_;
}

Queue *QueueReader::queue() const
{
  return queue_;
}

int QueueReader::wakeUp()
{
  if (queue_ != nullptr) {
    queue_->wakeUpConsumers();
    return 0;
  }
  else {
    std::cerr << "Error: Reader wake up - operation failed " << id_
              << std::endl;
    return -1;
  }
}
