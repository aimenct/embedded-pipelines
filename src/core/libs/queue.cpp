// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "queue.h"

#ifdef EPF_QUEUE_DEBUG
#include <sstream>
#endif

using namespace epf;

#ifdef EPF_QUEUE_DEBUG
#define QDBG_STREAM(expr)                      \
  do {                                         \
    std::ostringstream _qdbg_oss;              \
    _qdbg_oss << "[QDBG] " << expr;            \
    std::cerr << _qdbg_oss.str() << std::endl; \
  } while (0)
#else
#define QDBG_STREAM(expr) \
  do {                    \
  } while (0)
#endif

Queue::Queue()
    : Queue(QueueType::lifo)
{
}

epf::Queue::Queue(QueueType type)
    : queue_type_{type}
{
  // mutex condition and variables initialization
  pthread_mutex_init(&buffer_mtx_, NULL);
  pthread_cond_init(&buffer_full_cond_, NULL);
  pthread_cond_init(&buffer_empty_cond_, NULL);
  pthread_cond_init(&exit_cond_, NULL);
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  // init data and header buffer Pointers
  data_buffer_ = nullptr;
  hdr_buffer_ = nullptr;

  // init data schema and header schema
  data_message_ = nullptr;
  hdr_message_ = nullptr;

  // data size and header size
  data_size_ = 0;
  hdr_size_ = 0;
  length_ = 0;

  // init consumers and producers
  max_producers_ = 1;
  max_consumers_ = 4;
  producers_ = nullptr;
  consumers_ = nullptr;

  // init callbacks
  // start_read_callback_ = NULL;
  // end_read_callback_ = NULL;
}

epf::Queue::~Queue()
{
  //  std::cout<<"Queue destructor "<<std::endl;
  this->free();
  pthread_mutex_destroy(&buffer_mtx_);
  pthread_cond_destroy(&buffer_full_cond_);
  pthread_cond_destroy(&buffer_empty_cond_);
  pthread_cond_destroy(&exit_cond_);
}

void Queue::updateBarrierLocked()
{
  // TODO REVIEW - update barrier
  if (queue_type_ == fifo) {
    // update_barrier();
    //	  printf("length: %d, last_out %d, last_input
    //%d\n",length,consumers[id].last_out,last_input);

    // TODO - poner a -1 p->last_output = -1 ¿?
    int imax = -1;
    int pid = 0;
    for (int x : consumers_on_) {
      //	    printf("imax %d, MOD %d
      //\n",imax,MOD(last_input-consumers[x].last_out,length));
      int a = modulus(last_input_ - consumers_[x].last_out, length_);
      if (imax < a) {
        imax = a;
        pid = x;
      }
    }
    //	  printf("--length: %d, imax %d, last_out %d, last_input
    //%d\n",length,imax,consumers[id].last_out,last_input);
    barrier_ = imax == -1 ? -1 : consumers_[pid].last_out;
    //	  printf("BARRIER(id: %d): %d, last_out %d, next_input %d, in
    //%d\n",id,barrier, consumers[id].last_out,next_input,last_input);
  }
  else {
    // update_barrier() lifo ;
    // lifo: (hay un error!! p por consumers[x] - no recorre lista dentro
    // bucle)
    int imax = std::numeric_limits<int32_t>::max();  // TODO buffer max size
    int pid = -1;
    for (int x : consumers_on_) {
      if ((consumers_[x].last_out >= 0) && (imax > consumers_[x].last_out)) {
        pid = x;
        imax = consumers_[x].last_out;
      }
    }
    //	  p->counter+=p->num_elements;
    barrier_ = pid == -1 ? -1 : imax;
  }
  return;
}

int epf::Queue::init(int length, size_t data_size, size_t hdr_size,
                     int max_consumers, int max_producers)
{
  pthread_mutex_lock(&buffer_mtx_);  // TODO status_mtx_?
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }
  assert(max_consumers >= 0);
  assert(max_producers >= 0);
  assert(length >= 0);

  /* initialize circular queue pointers */
  workers_running_ = 0;
  counter_ = 0;
  counter_sat_ = 0;
  barrier_ = -1;
  next_input_ = 0;
  last_input_ = length - 1;
  if (queue_type_ == fifo) last_input_ = length;

  /* initialize condition variable: queue full or queue empty */
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  max_producers_ = max_producers;
  max_consumers_ = max_consumers;

  /* create table and lists of consumers and producers */
  producers_ = new Qproducer[static_cast<size_t>(max_producers_)];
  consumers_ = new Qconsumer[static_cast<size_t>(max_consumers_)];
  consumers_off_.clear();
  consumers_on_.clear();
  producers_off_.clear();
  producers_on_.clear();
  for (int x = 0; x < max_consumers_; x++) consumers_off_.push_back(x);
  for (int x = 0; x < max_producers_; x++) producers_off_.push_back(x);

  /* initialize callbacks for PULL mode */
  //   startReadCallback = NULL;
  //   endReadCallback = NULL;

  /* set data sizes and length */
  if (queue_type_ == fifo)
    length_ = length + 1;
  else
    length_ = length;

  size_t u_length = static_cast<size_t>(length_);

  // data size and header size
  data_size_ = data_size;
  hdr_size_ = hdr_size;

  if (data_buffer_ != NULL) std::free(data_buffer_);
  if (hdr_buffer_ != NULL) std::free(hdr_buffer_);
  hdr_buffer_ = (char *)malloc(hdr_size_ * u_length);
  if (hdr_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(hdr_buffer_, 0, hdr_size_ * u_length);
  data_buffer_ = (char *)malloc(data_size_ * u_length);
  if (data_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(data_buffer_, 0, data_size_ * u_length);

  /* set queue status to 'c'-connected */
  status_ = 'c';
  pthread_mutex_unlock(&buffer_mtx_);

  return 0;
}

// int epf::Queue::init(int length, Message *data_s, Message *hdr_s,
//                     int max_consumers, int max_producers)
int epf::Queue::init(int length, std::unique_ptr<Message> data_s,
                     std::unique_ptr<Message> hdr_s, int max_consumers,
                     int max_producers)
{
  pthread_mutex_lock(&buffer_mtx_);  // TODO status_mtx_?
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }

  assert(max_consumers >= 0);
  assert(max_producers >= 0);
  assert(length >= 0);

  /* initialize circular queue pointers */
  workers_running_ = 0;
  counter_ = 0;
  counter_sat_ = 0;
  barrier_ = -1;
  next_input_ = 0;
  last_input_ = length - 1;
  if (queue_type_ == fifo) last_input_ = length;

  /* initialize condition variable: queue full or queue empty */
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  max_producers_ = max_producers;
  max_consumers_ = max_consumers;

  /* create table and lists of consumers and producers */
  producers_ = new Qproducer[static_cast<size_t>(max_producers_)];
  consumers_ = new Qconsumer[static_cast<size_t>(max_consumers_)];
  consumers_off_.clear();
  consumers_on_.clear();
  producers_off_.clear();
  producers_on_.clear();
  for (int x = 0; x < max_consumers_; x++) consumers_off_.push_back(x);
  for (int x = 0; x < max_producers_; x++) producers_off_.push_back(x);

  /* initialize callbacks for PULL mode */
  //   startReadCallback = NULL;
  //   endReadCallback = NULL;

  // init data schema and header schema
  // data size and header size
  if (data_s != nullptr) {
    data_message_ = std::move(data_s);
    data_size_ = data_message_->size();
  }
  else
    data_size_ = 0;
  if (hdr_s != nullptr) {
    hdr_message_ = std::move(hdr_s);
    hdr_size_ = hdr_message_->size();
  }
  else
    hdr_size_ = 0;

  /* set data sizes and length */
  if (queue_type_ == fifo)
    length_ = length + 1;
  else
    length_ = length;
  size_t u_length = static_cast<size_t>(length_);

  if (data_buffer_ != NULL) std::free(data_buffer_);
  if (hdr_buffer_ != NULL) std::free(hdr_buffer_);

  hdr_buffer_ = (char *)malloc(hdr_size_ * u_length);
  if (hdr_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(hdr_buffer_, 0, hdr_size_ * u_length);

  data_buffer_ = (char *)malloc(data_size_ * u_length);
  if (data_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(data_buffer_, 0, data_size_ * u_length);

  /* set queue status to 'c'-connected */
  status_ = 'c';
  pthread_mutex_unlock(&buffer_mtx_);

  return 0;
}

int epf::Queue::free()
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  status_ = 'd';
  pthread_cond_broadcast(&(buffer_full_cond_));
  pthread_cond_broadcast(&(buffer_empty_cond_));

  /* don't free the queue until all producers/consumers release their buffers.
  - any producer/consumer waiting in the queue & any producer/consumers have
  locked buffers */
  // printf("free - %d workers_running\n", workers_running_);
  QDBG_STREAM("q=" << this << " evt=free-enter status=" << status_
                   << " wr=" << workers_running_ << " ce=" << count_empty_cond_
                   << " cf=" << count_full_cond_);
  while ((count_empty_cond_) || (count_full_cond_) || (workers_running_ != 0)) {
#ifdef EPF_QUEUE_DEBUG
    std::ostringstream producer_states;
    producer_states << "[";
    bool first = true;
    for (int pid : producers_on_) {
      if (!first) producer_states << ",";
      first = false;
      producer_states << "{id=" << pid << ",st=" << producers_[pid].status
                      << ",pend=" << producers_[pid].pending_elements << "}";
    }
    producer_states << "]";

    std::ostringstream consumer_states;
    consumer_states << "[";
    first = true;
    for (int cid : consumers_on_) {
      if (!first) consumer_states << ",";
      first = false;
      consumer_states << "{id=" << cid << ",st=" << consumers_[cid].status
                      << ",num=" << consumers_[cid].num_elements
                      << ",new=" << consumers_[cid].new_elements << "}";
    }
    consumer_states << "]";

    QDBG_STREAM("q=" << this << " evt=free-wait status=" << status_ << " wr="
                     << workers_running_ << " ce=" << count_empty_cond_
                     << " cf=" << count_full_cond_
                     << " producers=" << producer_states.str()
                     << " consumers=" << consumer_states.str());
#endif
    printf("free - %d workers_running\n", workers_running_);
    pthread_cond_wait(&exit_cond_, &buffer_mtx_);
  }

  /* delete table and lists of consumers and producers */
  consumers_off_.clear();
  consumers_on_.clear();
  producers_off_.clear();
  producers_on_.clear();
  delete[] producers_;
  delete[] consumers_;
  consumers_ = NULL;
  producers_ = NULL;

  std::free(hdr_buffer_);
  hdr_buffer_ = NULL;
  std::free(data_buffer_);
  data_buffer_ = NULL;

  /* reset schemas */
  // if (data_message_ != NULL) {
  //   delete data_message_;
  //   data_message_ = NULL;
  // }
  // if (hdr_message_ != NULL) {
  //   delete hdr_message_;
  //   hdr_message_ = NULL;
  // }
  if (data_message_ != nullptr) {
    data_message_.reset();
    data_message_ = nullptr;
  }
  if (hdr_message_ != nullptr) {
    hdr_message_.reset();
    hdr_message_ = nullptr;
  }

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

char epf::Queue::status()
{
  return status_;
}

int epf::Queue::setType(QueueType flag)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }
  if ((flag == fifo) || (flag == lifo)) {
    queue_type_ = flag;
    pthread_mutex_unlock(&buffer_mtx_);
    return 0;
  }
  else {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }
}

QueueType epf::Queue::type()
{
  return queue_type_;
}

int epf::Queue::subscribeProducer()
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (producers_off_.empty()) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }
  int id = producers_off_.front();
  producers_off_.remove(id);
  producers_[id].id = id;
  producers_[id].status = 's';
  producers_on_.push_back(id);

  pthread_mutex_unlock(&buffer_mtx_);
  return id;
}

int epf::Queue::unsubscribeProducer(int id)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  QDBG_STREAM("q=" << this << " evt=unsubscribeProducer-enter id=" << id
                   << " wr=" << workers_running_);
  for (std::list<int>::iterator it = producers_on_.begin();
       it != producers_on_.end(); ++it) {
    if (*it == id) {
      QDBG_STREAM("q=" << this << " evt=unsubscribeProducer-found id=" << id
                       << " st=" << producers_[id].status
                       << " pend=" << producers_[id].pending_elements
                       << " wr=" << workers_running_);
      if (producers_[id].status == 'w') {
        workers_running_--;
        pthread_cond_broadcast(&exit_cond_);
        QDBG_STREAM("q=" << this << " evt=unsubscribeProducer-working-fix id="
                         << id << " wr=" << workers_running_);
      }
      producers_[id].reset();
      producers_on_.erase(it);
      producers_off_.push_back(id);
      QDBG_STREAM("q=" << this << " evt=unsubscribeProducer-exit id=" << id
                       << " wr=" << workers_running_);
      pthread_mutex_unlock(&buffer_mtx_);
      return 0;
    }
  }
  QDBG_STREAM("q=" << this << " evt=unsubscribeProducer-miss id=" << id
                   << " wr=" << workers_running_);
  pthread_mutex_unlock(&buffer_mtx_);
  return -1;
}

int epf::Queue::subscribeConsumer()
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (consumers_off_.empty()) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }

  int id = consumers_off_.front();
  consumers_off_.remove(id);
  consumers_[id].id = id;
  consumers_[id].status = 's';
  if (queue_type_ == fifo) consumers_[id].last_out = last_input_;
  consumers_on_.push_back(id);

  if (queue_type_ == fifo) {
    int imax = -1;
    int pid = 0;
    for (int x : consumers_on_) {
      int a = modulus(last_input_ - consumers_[x].last_out, length_);
      if (imax < a) {
        imax = a;
        pid = x;
      }
    }
    barrier_ = imax == -1 ? -1 : consumers_[pid].last_out;
  }

  pthread_mutex_unlock(&buffer_mtx_);
  return id;
}

int epf::Queue::unsubscribeConsumer(int consumer_index)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (consumer_index < 0 || consumer_index >= max_consumers_) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  Qconsumer &consumer = consumers_[consumer_index];

  auto position =
      std::find(consumers_on_.begin(), consumers_on_.end(), consumer_index);

  if (position == consumers_on_.end()) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }

  if (consumer.id != consumer_index) {
    // Internal invariant violation.
    std::cerr << "[Queue] Internal state violation: consumer.id != "
                 "consumer_index."
              << std::endl;
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  const bool was_working = consumer.status == 'w';

  consumer.reset();
  consumers_on_.erase(position);
  consumers_off_.push_back(consumer_index);

  if (was_working) {
    workers_running_--;
    pthread_cond_broadcast(&exit_cond_);
  }

  updateBarrierLocked();
  pthread_cond_broadcast(&buffer_full_cond_);
  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int epf::Queue::startWritePtr(int id, int num_elements)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_producers_) return -2;

  Qproducer &p = producers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  // check id correct and status = subscribed or already working
  if ((status_ != 'c') || ((p.status != 's') && (p.status != 'w'))) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  // check if there are enough free buffers
  // printf("barrier %d next_input_ %d num_elements %d\n", barrier_,
  // next_input_,
  //        num_elements);
  while ((barrier_ != -1) && (modulus(barrier_ - next_input_, length_) <
                              num_elements)) {  // TODO REV <=
    if (p.blocking) {                           // if blocking calls wait
      count_full_cond_++;
      pthread_cond_wait(&buffer_full_cond_, &buffer_mtx_);
      count_full_cond_--;
      pthread_cond_broadcast(&exit_cond_);
      if (status_ == 'd') {
        pthread_mutex_unlock(
            &buffer_mtx_);  // if queue disabled exit with error code
                            // printf("queue disable\n");
        return -2;
      }
    }
    else {
      pthread_mutex_unlock(
          &buffer_mtx_);  // if not blocking calls exit error code
      // printf("no enough\n");
      return -1;
    }
  }

  // if there are enough buffers:
  bool first_allocation = (p.status == 's');
  //  const int workers_before = workers_running_;
  //  const int pending_before = p.pending_elements;
  //  const char status_before = p.status;
  p.num_elements = num_elements;
  if (first_allocation) {
    p.status = 'w';
    workers_running_++;
  }
  p.pending_elements += num_elements;

  int aux = next_input_;
  next_input_ = (next_input_ + num_elements) % length_;
  QDBG_STREAM("q=" << this << " evt=startWritePtr id=" << id
                   << " role=prod st=" << status_before << "->" << p.status
                   << " wr=" << workers_before << "->" << workers_running_
                   << " pend=" << pending_before << "->" << p.pending_elements
                   << " num=" << num_elements << " ptr=" << aux);

  pthread_mutex_unlock(&buffer_mtx_);

  return aux;
}

char *epf::Queue::startWrite(int producer_id)
{
  int ptr = startWritePtr(producer_id, 1);
  if (ptr >= 0) {
    return &data_buffer_[static_cast<size_t>(ptr) * data_size_];
  }
  else {
    return NULL;
  }
}

int epf::Queue::startWrite(int producer_id, char **data, char **hdr)
{
  int ptr = startWritePtr(producer_id, 1);
  if (ptr >= 0) {
    *data = &data_buffer_[static_cast<size_t>(ptr) * data_size_];
    *hdr = &hdr_buffer_[static_cast<size_t>(ptr) * hdr_size_];
    return ptr;
  }
  else {
    data = NULL;
    hdr = NULL;
    return ptr;
  }
}

int epf::Queue::startWrite(int producer_id, int num_elements, char **data,
                           char **hdr, int *len, char **data1, char **hdr1,
                           int *len1)
{
  int ptr = startWritePtr(producer_id, num_elements);
  if (ptr >= 0) {
    *data = &data_buffer_[static_cast<size_t>(ptr) * data_size_];
    *hdr = &hdr_buffer_[static_cast<size_t>(ptr) * hdr_size_];
    *data1 = data_buffer_;
    *hdr1 = hdr_buffer_;
    *len = length_ - ptr;
    *len1 = num_elements - *len;
    if ((*len1) < 0) {
      *len = num_elements;
      *len1 = 0;
    }
    return ptr;
  }
  else {
    *data = NULL;
    *hdr = NULL;
    *data1 = NULL;
    *hdr1 = NULL;
    *len = 0;
    *len1 = 0;
    return ptr;
  }
}

int epf::Queue::endWrite(int id, int done)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_producers_) return -2;

  Qproducer &p = producers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  if (p.status != 'w') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (done < 0) done = p.pending_elements;
  if (done > p.pending_elements) done = p.pending_elements;
  // const int workers_before = workers_running_;
  // const int pending_before = p.pending_elements;
  // const char status_before = p.status;

  last_input_ = (last_input_ + done) % length_;

  counter_ += done;
  if (counter_ < 0) {
    printf("Queue Write - Counter of messages overflow! - \n");
    counter_ = modulus(counter_, std::numeric_limits<int32_t>::min());
    counter_sat_ = 1;
  }

  p.counter += done;
  p.pending_elements -= done;
  if (p.pending_elements == 0) {
    p.status = 's';
    workers_running_--;
  }

  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_cond_broadcast(&exit_cond_);
  QDBG_STREAM("q=" << this << " evt=endWrite id=" << id
                   << " role=prod st=" << status_before << "->" << p.status
                   << " wr=" << workers_before << "->" << workers_running_
                   << " pend=" << pending_before << "->" << p.pending_elements
                   << " done=" << done);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int epf::Queue::endWriteAbort(int id, int pending)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_producers_) return -2;

  Qproducer &p = producers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  if (p.status != 'w') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (max_producers_ > 1) {
    printf("Queue endWriteAbort not implemented for more than 1 producer\n");
    exit(0);
  }

  if (pending < 0) pending = p.pending_elements;
  if (pending > p.pending_elements) pending = p.pending_elements;
  // const int workers_before = workers_running_;
  // const int pending_before = p.pending_elements;
  // const char status_before = p.status;

  // this works only with one producer
  next_input_ = modulus(next_input_ - pending, length_);

  p.pending_elements -= pending;
  if (p.pending_elements == 0) {
    p.status = 's';
    workers_running_--;
  }

  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_cond_broadcast(&exit_cond_);
  QDBG_STREAM("q=" << this << " evt=endWriteAbort id=" << id
                   << " role=prod st=" << status_before << "->" << p.status
                   << " wr=" << workers_before << "->" << workers_running_
                   << " pend=" << pending_before << "->" << p.pending_elements
                   << " abort=" << pending);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int epf::Queue::startReadPtr(int id, int num_elements, int new_elements)
{
  int new_buffers;

  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_consumers_) return -2;

  Qconsumer &c = consumers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  // check id correct and status = subscribed
  if ((status_ != 'c') || (c.status != 's')) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  // check if there are enough buffers
  if (queue_type_ == fifo) {
    new_buffers = modulus(last_input_ - c.last_out, length_);
  }
  else {
    new_buffers =
        modulus(counter_ - c.ptr_counter, std::numeric_limits<int32_t>::max());
  }
  while ((new_buffers < new_elements) ||
         ((counter_ < num_elements) && (!counter_sat_))) {
    // std::cout << "blocking-> " << c.blocking << std::endl;
    if (c.blocking) {
      count_empty_cond_++;
      //      std::cout << "waiting..." << std::endl;
      pthread_cond_wait(&buffer_empty_cond_, &buffer_mtx_);
      count_empty_cond_--;
      pthread_cond_broadcast(&exit_cond_);
      if (status_ == 'd') {
        pthread_mutex_unlock(&buffer_mtx_);
        return -2;
      }
    }
    else {
      pthread_mutex_unlock(&buffer_mtx_);
      return -1;
    }
    if (queue_type_ == fifo) {
      new_buffers = modulus(last_input_ - c.last_out, length_);
    }
    else {
      new_buffers = modulus(counter_ - c.ptr_counter,
                            std::numeric_limits<int32_t>::max());
    }
  }

  // if there are enough buffers update state
  c.num_elements = num_elements;
  c.new_elements = new_elements;
  // const int workers_before = workers_running_;
  // const char status_before = c.status;

  if (queue_type_ == fifo) {
    c.status = 'w';
    workers_running_++;
    QDBG_STREAM("q=" << this << " evt=startReadPtr id=" << id
                     << " role=cons st=" << status_before << "->" << c.status
                     << " wr=" << workers_before << "->" << workers_running_
                     << " num=" << num_elements << " new=" << new_elements);

    pthread_mutex_unlock(&buffer_mtx_);
    return modulus(c.last_out + 1, length_);
  }
  else {
    if (new_buffers > num_elements) {
      c.lost_counter += new_buffers - num_elements;
    }
    c.last_out = modulus(last_input_ + 1 - num_elements, length_);
    barrier_ = barrier_ == -1 ? c.last_out : barrier_;

    c.ptr_counter = counter_;
    c.status = 'w';
    workers_running_++;
    QDBG_STREAM("q=" << this << " evt=startReadPtr id=" << id
                     << " role=cons st=" << status_before << "->" << c.status
                     << " wr=" << workers_before << "->" << workers_running_
                     << " num=" << num_elements << " new=" << new_elements);

    pthread_mutex_unlock(&buffer_mtx_);
    return c.last_out;
  }
  return -1;
}

char *epf::Queue::startRead(int consumer_id)
{
  if (schedule_mode_ == push) {
    int ptr = startReadPtr(consumer_id, 1, 1);
    if (ptr >= 0) {
      return &data_buffer_[static_cast<size_t>(ptr) * data_size_];
    }
    else {
      return NULL;
    }
  }
  else {
    pthread_mutex_lock(&buffer_mtx_);
    if (status_ != 'c') {
      pthread_mutex_unlock(&buffer_mtx_);
      return NULL;
    }
    else {  // TODO ??
      pthread_mutex_unlock(&buffer_mtx_);
      return NULL;
      // char **data;
      // char **hdr;
      // int err = start_read_callback_(pvt_, data, hdr,
      //                             &(consumers_[consumer_id].ptr_aux));
      // return *data;
    }
  }
}

int epf::Queue::startRead(int consumer_id, char **data, char **hdr)
{
  if (schedule_mode_ == push) {
    int ptr = startReadPtr(consumer_id, 1, 1);
    if (ptr >= 0) {
      *data = &data_buffer_[static_cast<size_t>(ptr) * data_size_];
      *hdr = &hdr_buffer_[static_cast<size_t>(ptr) * hdr_size_];
      return ptr;
    }
    else {
      data = NULL;
      hdr = NULL;
      return ptr;
    }
  }
  else {
    pthread_mutex_lock(&buffer_mtx_);
    if (status_ != 'c') {
      pthread_mutex_unlock(&buffer_mtx_);
      return -2;
    }
    else {
      pthread_mutex_unlock(&buffer_mtx_);
      // TODO
      return 0;
      // return start_read_callback_(pvt_, data, hdr,
      //                             &(consumers_[consumer_id].ptr_aux));
    }
  }
}

int epf::Queue::startRead(int consumer_id, int num_elements, int new_elements,
                          char **data, char **hdr, int *len, char **data1,
                          char **hdr1, int *len1)
{
  if (schedule_mode_ == pull) return -1;
  int ptr = startReadPtr(consumer_id, num_elements, new_elements);
  if (ptr >= 0) {
    *data = &data_buffer_[static_cast<size_t>(ptr) * data_size_];
    *hdr = &hdr_buffer_[static_cast<size_t>(ptr) * hdr_size_];
    *data1 = data_buffer_;
    *hdr1 = hdr_buffer_;
    *len = length_ - ptr;
    *len1 = num_elements - *len;
    if ((*len1) < 0) {
      *len = num_elements;
      *len1 = 0;
    }
    return ptr;
  }
  else {
    *data = NULL;
    *hdr = NULL;
    *data1 = NULL;
    *hdr1 = NULL;
    *len = 0;
    *len1 = 0;
    return ptr;
  }
}

int epf::Queue::endRead(int id)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_consumers_) return -2;

  // if (schedule_mode_ == pull)
  //   return end_read_callback_(pvt_, consumers_[id].ptr_aux);

  Qconsumer &c = consumers_[id];

  pthread_mutex_lock(&buffer_mtx_);
  // const int workers_before = workers_running_;
  // const char status_before = c.status;

  if (c.status != 'w') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  // update barrier()
  int imax = -1;
  int pid = 0;
  int a = 0;
  if (queue_type_ == fifo) {  // fifo
    c.last_out = modulus(c.last_out + c.new_elements, length_);
    for (int x : consumers_on_) {
      a = modulus(last_input_ - consumers_[x].last_out, length_);
      if (imax < a) {
        imax = a;
        pid = x;
      }
    }
  }
  else {  // lifo
    c.last_out = -1;
    for (int x : consumers_on_) {
      if (consumers_[x].last_out != -1) {
        a = modulus(last_input_ - consumers_[x].last_out, length_);
        if (imax < a) {
          imax = a;
          pid = x;
        }
      }
    }
  }
  barrier_ = imax == -1 ? -1 : consumers_[pid].last_out;

  c.counter += c.num_elements;
  c.status = 's';
  workers_running_--;

  pthread_cond_broadcast(&(buffer_full_cond_));
  pthread_cond_broadcast(&exit_cond_);
  QDBG_STREAM("q=" << this << " evt=endRead id=" << id
                   << " role=cons st=" << status_before << "->" << c.status
                   << " wr=" << workers_before << "->" << workers_running_
                   << " num=" << c.num_elements << " new=" << c.new_elements);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int epf::Queue::endReadAbort(int id)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_consumers_) return -2;

  // if (schedule_mode_ == pull)
  //   return end_read_callback_(pvt_, consumers_[id].ptr_aux);

  Qconsumer &c = consumers_[id];

  pthread_mutex_lock(&buffer_mtx_);
  // const int workers_before = workers_running_;
  // const char status_before = c.status;

  if (c.status != 'w') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  // ¿ update barrier() ?
  int imax = -1;
  int pid = 0;
  int a = 0;
  if (queue_type_ == fifo) {  // fifo
    //    c.last_out = modulus(c.last_out + c.new_elements, length_);
    for (int x : consumers_on_) {
      a = modulus(last_input_ - consumers_[x].last_out, length_);
      if (imax < a) {
        imax = a;
        pid = x;
      }
    }
  }
  else {  // lifo
    c.last_out = -1;
    for (int x : consumers_on_) {
      if (consumers_[x].last_out != -1) {
        a = modulus(last_input_ - consumers_[x].last_out, length_);
        if (imax < a) {
          imax = a;
          pid = x;
        }
      }
    }
  }
  barrier_ = imax == -1 ? -1 : consumers_[pid].last_out;

  //  c.counter += c.num_elements;
  c.status = 's';
  workers_running_--;

  pthread_cond_broadcast(&(buffer_full_cond_));
  pthread_cond_broadcast(&exit_cond_);
  QDBG_STREAM("q=" << this << " evt=endReadAbort id=" << id
                   << " role=cons st=" << status_before << "->" << c.status
                   << " wr=" << workers_before << "->" << workers_running_
                   << " num=" << c.num_elements << " new=" << c.new_elements);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

// int epf::Queue::readCopy(int consumer_id, char *data, char *hdr,
//                           int num_elements, int new_elements)
// {
//   char *data1, *data2, *hdr1, *hdr2;
//   int len1, len2, err;
//   // push mode
//   if (schedule_mode_ == push) {
//     err = startRead(consumer_id, num_elements, new_elements, &data1, &hdr1,
//                     &len1, &data2, &hdr2, &len2);
//     if (err >= 0) {
//       memcpy(data, data1, len1 * this->dataSize());
//       memcpy(data, data2, len2 * this->dataSize());
//       memcpy(hdr, hdr1, len1 * this->hdrSize());
//       memcpy(hdr, hdr2, len2 * this->hdrSize());
//       return endRead(consumer_id);
//     }
//     return err;
//   }
//   else {
//     std::cout << " readCopy not implemented in PULL mode: \n" << std::endl;
//     return -1;
//   }
// }

int epf::Queue::setWriteBlocking(int id, bool lock)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_producers_)) ||
      (producers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  producers_[id].blocking = lock;
  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int epf::Queue::setReadBlocking(int id, bool lock)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_consumers_)) ||
      (consumers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  consumers_[id].blocking = lock;
  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

std::optional<bool> epf::Queue::readBlocking(int32_t id)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_consumers_)) ||
      (consumers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  bool blocking = consumers_[id].blocking;
  pthread_mutex_unlock(&buffer_mtx_);
  return blocking;
}

std::optional<bool> epf::Queue::writeBlocking(int32_t id)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_producers_)) ||
      (producers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  bool blocking = producers_[id].blocking;
  pthread_mutex_unlock(&buffer_mtx_);
  return blocking;
}

int epf::Queue::length() const
{
  if (queue_type_ == fifo)
    return length_ - 1;
  else
    return length_;
}

size_t epf::Queue::dataSize() const
{
  return data_size_;
}

size_t epf::Queue::hdrSize() const
{
  return hdr_size_;
}

char *epf::Queue::dataBuffer() const
{
  return data_buffer_;
}

char *epf::Queue::hdrBuffer() const
{
  return hdr_buffer_;
}

const Message *epf::Queue::dataMessage() const
{
  return data_message_.get();
}

const Message *epf::Queue::hdrMessage() const
{
  return hdr_message_.get();
}

void epf::Queue::printStats() const
{
  using namespace std;

  cout << endl;
  cout << "--------------------------------" << endl;
  cout << " Status:         " << status_ << endl;

  if (schedule_mode_ == push) {
    cout << " Schedule mode:      push" << endl;
  }
  else {
    cout << " Schedule mode:      pull" << endl;
  }

  if (queue_type_ == fifo) {
    cout << " Queue type:          fifo" << endl;
  }
  else {
    cout << " Queue type:         lifo" << endl;
  }

  cout << " Inserted elements:     " << counter_ << endl;
  cout << " Workers running:       " << workers_running_ << endl;
  //  std::cout<<"--------------------------------"<<std::endl;
  cout << endl << endl;

  for (int x : producers_on_) {
    producers_[x].print();
  }
  for (int x : consumers_on_) {
    consumers_[x].print();
  }
  cout << "--------------------------------" << endl;

  // std::cout<<"--------------------------------"<<std::endl;
  // std::cout<<" Status: "<<status;
  // std::cout<<" Schedule_mode: "<<schedule_mode;
  // std::cout<<" Counter: "<<counter;
  // std::cout<<" Workers: "<<workers_running;
  // std::cout<<" barrier: "<<barrier;
  // std::cout<<" next_input: "<<next_input;
  // std::cout<<" last_input: "<<last_input<<std::endl;
  // std::cout<<"--------------------------------"<<std::endl;

  // for (int x : producers_on) {
  //   producers[x].print();
  // }
  // for (int x : consumers_on) {
  //   consumers[x].print();
  // }

  return;
}

void epf::Queue::wakeUpProducers()
{
  pthread_mutex_lock(&buffer_mtx_);
  pthread_cond_broadcast(&(buffer_full_cond_));
  pthread_mutex_unlock(&buffer_mtx_);
}

void epf::Queue::wakeUpConsumers()
{
  pthread_mutex_lock(&buffer_mtx_);
  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_mutex_unlock(&buffer_mtx_);
}

// int epf::Queue::setCallbacks(start_callback_t cb1, end_callback_t cb2,
//                               void *usr_ptr)
// {
//   pthread_mutex_lock(&buffer_mtx_);
//   if (status_ != 'd') {  // TODO: only if disconnected
//     pthread_mutex_unlock(&buffer_mtx_);
//     return -2;
//   }
//   pvt_ = usr_ptr;
//   start_read_callback_ = cb1;
//   end_read_callback_ = cb2;

//   pthread_mutex_unlock(&buffer_mtx_);
//   return 0;
// }

/* Queue Class */
Qproducer::Qproducer()
{
  reset();
}
void Qproducer::reset()
{
  id = -1;
  status = 'u';
  counter = 0;
  //  ptr_aux = NULL;
  blocking = 'y';
  //  next_input = 0;
  num_elements = 0;
  pending_elements = 0;
}

void Qproducer::print()
{
  std::cout << " Producer Id(" << id << "), writes: " << counter
            << ", full calls: X" << std::endl;
}

Qconsumer::Qconsumer()
{
  reset();
}
void Qconsumer::reset()
{
  id = -1;
  status = 'u';
  last_out = -1;
  ptr_counter = 0;
  lost_counter = 0;
  counter = 0;
  ptr_aux = NULL;
  blocking = 'y';
  num_elements = 0;
  new_elements = 0;
}

void Qconsumer::print()
{
  std::cout << "Consumer Id(" << id << "), " << "reads: " << counter
            << ", losts: " << lost_counter << ", empty calls X" << std::endl;
}
