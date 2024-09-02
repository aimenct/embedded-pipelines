// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "queue_handlers.h"

using namespace ep;

QueueWriter::QueueWriter(std::shared_ptr<Queue> q)
{
  if (q == nullptr) {
    std::cerr << "Error: QueueWriter - Queue pointer is null." << std::endl;
    exit(0);
  }

  q_ = q;
  id_ = -1;
  lenA_ = -1;
  lenA_ = -1;
  dataPtrA_ = nullptr;
  dataPtrB_ = nullptr;
  hdrPtrA_ = nullptr;
  hdrPtrB_ = nullptr;
  messages_ = 0;

  this->subscribe();
}

QueueWriter::~QueueWriter()
{
  this->unsubscribe();
  q_ = nullptr;
}

int QueueWriter::subscribe()
{
  if (q_ == nullptr) {
    std::cerr << "Writer subscribed q == nullptr " << std::endl;
    return -1;
  }

  id_ = q_->subscribeProducer();
  if (id_ >= 0) {
    //    std::cout << "Writer subscribed with ID: " << id_ << std::endl;

    if (q_->dataMessage() != nullptr)
      data_msg_ = *(q_->dataMessage());
    //    else
      // std::cout << "Writer subscribe: queue doesn't have data msg structure"
      //           << std::endl;

    if (q_->hdrMessage() != nullptr)
      hdr_msg_ = *(q_->hdrMessage());
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
  if (q_ == nullptr) {
    std::cerr << "Writer unsubscribed q == nullptr " << std::endl;
    return -1;
  }

  int err = 0;
  err = q_->unsubscribeProducer(id_);
  if (err < 0) {
    std::cerr << "Error: Writer unsubscribe operation failed " << err
              << std::endl;
    return -1;
  }
  //  std::cerr << "Writer unsubscribe with ID: " << id_ << std::endl;
  id_ = -1;
  return 0;
}

int QueueWriter::id() const
{
  return id_;
}

int QueueWriter::startWrite()
{
  messages_ = 1;
  int err = q_->startWrite(id_, messages_, &dataPtrA_, &hdrPtrA_, &lenA_,
                           &dataPtrB_, &hdrPtrB_, &lenB_);
  if (err < 0) {
    // std::cerr << "Error: Writer startWrite operation failed " << err
    //           << std::endl;
    return err;
  }
  data_msg_.updateMessage(dataPtrA_);
  hdr_msg_.updateMessage(hdrPtrA_);
  return err;
}

int QueueWriter::startWrite(int msgs)
{
  messages_ = msgs;
  int err = q_->startWrite(id_, messages_, &dataPtrA_, &hdrPtrA_, &lenA_,
                           &dataPtrB_, &hdrPtrB_, &lenB_);
  // if (err < 0) {
  //   // std::cerr << "Error: Reader startRead operation failed " << err
  //   //           << std::endl;
  //   return err;
  // }
  // //  msg = msg_->parse(data);
  // //  msg_.updateMessage(data);
  return err;
}

int QueueWriter::endWrite()
{
  int err = 0;
  err = q_->endWrite(id_);
  if (err < 0) {
    std::cerr << "Error: Writer endWrite operation failed " << err << std::endl;
    return -1;
  }
  return 0;
}

int QueueWriter::endWriteAbort()
{
  int err = 0;
  err = q_->endWriteAbort(id_);
  if (err < 0) {
    std::cerr << "Error: Writer endWrite Abort operation failed " << err
              << std::endl;
    return -1;
  }
  return 0;
}

int QueueWriter::setBlockingCalls(bool flag)
{
  int err = 0;
  err = q_->setWriteBlocking(id_, flag);
  if (err < 0) {
    std::cerr << "Error: Writer blockingCalls operation failed " << err
              << std::endl;
    return -1;
  }
  return 0;
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

Message &QueueWriter::dataMsg(int i)
{
  if (i < messages_)
    if (i < lenA_)
      data_msg_.updateMessage(dataPtrA_ + i * q_->dataSize());
    else
      data_msg_.updateMessage(dataPtrB_ + i * q_->dataSize());
  else
    std::cerr << "Error: dataMsg beyond limits i=" << i << std::endl;
  return data_msg_;
}

Message &QueueWriter::hdrMsg(int i)
{
  if (i < messages_)
    if (i < lenA_)
      hdr_msg_.updateMessage(hdrPtrA_ + i * q_->hdrSize());
    else
      hdr_msg_.updateMessage(hdrPtrB_ + i * q_->hdrSize());
  else
    std::cerr << "Error: hdrMsg beyond limits i=" << i << std::endl;
  return hdr_msg_;
}

char *QueueWriter::dataPtrA() const
{
  return dataPtrA_;
}

char *QueueWriter::dataPtrB() const
{
  return dataPtrB_;
}

char *QueueWriter::hdrPtrA() const
{
  return hdrPtrA_;
}

char *QueueWriter::hdrPtrB() const
{
  return hdrPtrB_;
}

int QueueWriter::lenA() const
{
  return lenA_;
}

int QueueWriter::lenB() const
{
  return lenB_;
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

std::shared_ptr<Queue> QueueWriter::queue() const
{
  return q_;
}

QueueReader::QueueReader(std::shared_ptr<Queue> q)
{
  if (q == nullptr) {
    std::cerr << "Error: QueueReader - Queue pointer is null." << std::endl;
    exit(0);
  }

  q_ = q;
  id_ = -1;
  lenA_ = -1;
  lenA_ = -1;
  dataPtrA_ = nullptr;
  dataPtrB_ = nullptr;
  hdrPtrA_ = nullptr;
  hdrPtrB_ = nullptr;
  messages_ = 0;

  this->subscribe();
}

QueueReader::~QueueReader()
{
  this->unsubscribe();
  q_ = nullptr;
}

int QueueReader::subscribe()
{
  if (q_ == nullptr) {
    std::cerr << "Reader subscribed q == nullptr " << std::endl;
    return -1;
  }

  id_ = q_->subscribeConsumer();

  if (id_ >= 0) {
    //    std::cout << "Reader subscribed with ID: " << id_ << std::endl;

    if (q_->dataMessage() != nullptr)
      data_msg_ = *(q_->dataMessage());
    // else
    //   std::cout << "Reader subscribe: queue doesn't have Msg structure"
    //             << std::endl;

    if (q_->hdrMessage() != nullptr)
      hdr_msg_ = *(q_->hdrMessage());
    // else
    //   std::cout << "Reader subscribe: queue doesn't have hdr msg structure"
    //             << std::endl;

    return 0;
  }
  std::cerr << "Error: Reader subscribe operation failed " << id_ << std::endl;
  return id_;
}

int QueueReader::unsubscribe()
{
  if (q_ == nullptr) {
    std::cerr << "Reader unsubscribed q == nullptr " << std::endl;
    return -1;
  }

  int err = 0;
  err = q_->unsubscribeConsumer(id_);
  if (err < 0) {
    std::cerr << "Error: Reader unsubscribe operation failed " << err
              << std::endl;
    return -1;
  }
  std::cerr << "Reader unsubscribe with ID: " << id_ << std::endl;
  id_ = -1;

  return 0;
}

int QueueReader::id() const
{
  return id_;
}

int QueueReader::startRead()
{
  int new_msgs = 1;
  messages_ = 1;
  int err = q_->startRead(id_, messages_, new_msgs, &dataPtrA_, &hdrPtrA_,
                          &lenA_, &dataPtrB_, &hdrPtrB_, &lenB_);
  if (err < 0) {
    // std::cerr << "Error: Reader startRead operation failed " << err
    //           << std::endl;
    return err;
  }
  data_msg_.updateMessage(dataPtrA_);
  hdr_msg_.updateMessage(hdrPtrA_);
  return 0;
}

int QueueReader::startRead(int msgs, int new_msgs)
{
  messages_ = msgs;
  int err = q_->startRead(id_, messages_, new_msgs, &dataPtrA_, &hdrPtrA_,
                          &lenA_, &dataPtrB_, &hdrPtrB_, &lenB_);
  // if (err < 0) {
  //   //   // std::cerr << "Error: Reader startRead operation failed " << err
  //   //   //           << std::endl;
  //   return err;
  // }

  return err;
}

int QueueReader::endRead()
{
  int err = 0;
  err = q_->endRead(id_);
  if (err < 0) {
    std::cerr << "Error: Reader endRead operation failed " << err << std::endl;
    return -1;
  }
  return 0;
}

int QueueReader::endReadAbort()
{
  int err = 0;
  err = q_->endReadAbort(id_);
  if (err < 0) {
    std::cerr << "Error: Reader endRead Abort operation failed " << err
              << std::endl;
    return -1;
  }
  return 0;
}

int QueueReader::setBlockingCalls(bool flag)
{
  int err = 0;
  err = q_->setReadBlocking(id_, flag);
  if (err < 0) {
    std::cerr << "Error: Reader blockingCalls operation failed " << err
              << std::endl;
    return -1;
  }
  return 0;
}

Message *QueueReader::dataSchema()
{
  return &data_msg_;
}

Message *QueueReader::hdrSchema()
{
  return &hdr_msg_;
}

int QueueReader::msgCount() const
{
  return messages_;
}

Message &QueueReader::dataMsg(int i)
{
  if (i < messages_)
    if (i < lenA_)
      data_msg_.updateMessage(dataPtrA_ + i * q_->dataSize());
    else
      data_msg_.updateMessage(dataPtrB_ + i * q_->dataSize());
  else
    std::cerr << "Error: dataMsg beyond limits i=" << i << std::endl;
  return data_msg_;
}

Message &QueueReader::hdrMsg(int i)
{
  if (i < messages_)
    if (i < lenA_)
      hdr_msg_.updateMessage(hdrPtrA_ + i * q_->hdrSize());
    else
      hdr_msg_.updateMessage(hdrPtrB_ + i * q_->hdrSize());
  else
    std::cerr << "Error: hdrMsg beyond limits i=" << i << std::endl;
  return hdr_msg_;
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

int QueueReader::setBatchSize(int size)
{
  batch_size_ = size;
  return 0;
}

int QueueReader::batchSize() const
{
  return batch_size_;
}

int QueueReader::setNewPerBatch(int size)
{
  new_per_batch_ = size;
  return 0;
}

int QueueReader::newPerBatch() const
{
  return new_per_batch_;
}

std::shared_ptr<Queue> QueueReader::queue() const
{
  return q_;
}

namespace ep {

void queueReaderSettingsFromYaml(QueueReader *r, int index,
                                 const YAML::Node &queueSettings)
{
  int batch = 1;
  int new_per_batch = 1;
  bool blocking = true;

  if (queueSettings["readers"]) {
    const YAML::Node &readers = queueSettings["readers"];
    std::cout << "Readers:" << std::endl;
    for (std::size_t i = 0; i < readers.size(); i++) {
      int id = readers[i]["id"] ? readers[i]["id"].as<int>() : -1;
      if (index == id) {
        batch = readers[i]["batch"] ? readers[i]["batch"].as<int>() : 1;
        new_per_batch = readers[i]["new per batch"]
                            ? readers[i]["new per batch"].as<int>()
                            : 1;
        blocking =
            readers[i]["blocking"] ? readers[i]["blocking"].as<bool>() : true;
        break;
      }
    }
  }
  r->setBatchSize(batch);
  r->setNewPerBatch(new_per_batch);
  r->setBlockingCalls(blocking);
  // std::cout << "  Reader ID: " << r->id() << std::endl;
  // std::cout << "    Batch: " << r->batchSize() << std::endl;
  // std::cout << "    New per batch: " << r->newPerBatch() << std::endl;
  // std::cout << "    Blocking: " << std::boolalpha << blocking << std::endl;
  return;
}

void queueWriterSettingsFromYaml(QueueWriter *w, int index,
                                 const YAML::Node &queueSettings)
{
  int batch = 1;
  bool blocking = true;
  if (queueSettings["writers"]) {
    const YAML::Node &writers = queueSettings["writers"];
    std::cout << "Writers:" << std::endl;
    for (std::size_t i = 0; i < writers.size(); i++) {
      int id = writers[i]["id"] ? writers[i]["id"].as<int>() : -1;
      if (index == id) {
        batch = writers[i]["batch"] ? writers[i]["batch"].as<int>() : 1;
        blocking =
            writers[i]["blocking"] ? writers[i]["blocking"].as<bool>() : true;
        break;
      }
    }
  }
  w->setBatchSize(batch);
  w->setBlockingCalls(blocking);
  // std::cout << "  Writer ID: " << w->id() << std::endl;
  // std::cout << "    Batch: " << w->batchSize() << std::endl;
  // std::cout << "    Blocking: " << std::boolalpha << blocking << std::endl;
  return;
}

}  // namespace ep
