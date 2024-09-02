// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "queue.h"

using namespace ep;

ep::Queue::Queue()
{
  // status
  status_ = 'd';

  // mutex condition and variables initialization
  pthread_mutex_init(&buffer_mtx_, NULL);
  pthread_cond_init(&buffer_full_cond_, NULL);
  pthread_cond_init(&buffer_empty_cond_, NULL);
  pthread_cond_init(&exit_cond_, NULL);
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  // init data and header buffer Pointers
  data_buffer_ = NULL;
  hdr_buffer_ = NULL;

  // init data schema and header schema
  data_message_ = NULL;
  hdr_message_ = NULL;

  // data size and header size
  data_size_ = 0;
  hdr_size_ = 0;
  length_ = 0;

  // init consumers and producers
  max_producers_ = 1;
  max_consumers_ = 4;
  producers_ = NULL;
  consumers_ = NULL;

  // init queue type
  queue_type_ = lifo;
  schedule_mode_ = push;

  // init callbacks
  // start_read_callback_ = NULL;
  // end_read_callback_ = NULL;
}

ep::Queue::~Queue()
{
  //  std::cout<<"Queue destructor "<<std::endl;
  this->free();
  pthread_mutex_destroy(&buffer_mtx_);
  pthread_cond_destroy(&buffer_full_cond_);
  pthread_cond_destroy(&buffer_empty_cond_);
  pthread_cond_destroy(&exit_cond_);
}

int ep::Queue::init(int length, size_t data_size, size_t hdr_size,
                    int max_consumers, int max_producers)
{
  pthread_mutex_lock(&buffer_mtx_);  // TODO status_mtx_?
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }

  /* initialize circular queue pointers */
  workers_running_ = 0;
  counter_ = 0;
  counter_sat_ = 0;
  barrier_ = -1;
  next_input_ = 0;
  last_input_ = length - 1;

  /* initialize condition variable: queue full or queue empty */
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  max_producers_ = max_producers;
  max_consumers_ = max_consumers;

  /* create table and lists of consumers and producers */
  producers_ = new Qproducer[max_producers_];
  consumers_ = new Qconsumer[max_consumers_];
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
  length_ = length;

  // data size and header size
  data_size_ = data_size;
  hdr_size_ = hdr_size;

  if (data_buffer_ != NULL) std::free(data_buffer_);
  if (hdr_buffer_ != NULL) std::free(hdr_buffer_);
  hdr_buffer_ = (char *)malloc(hdr_size_ * length_);
  if (hdr_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(hdr_buffer_, 0, hdr_size_ * length_);
  data_buffer_ = (char *)malloc(data_size_ * length_);
  if (data_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(data_buffer_, 0, data_size_ * length_);

  /* set queue status to 'c'-connected */
  status_ = 'c';
  pthread_mutex_unlock(&buffer_mtx_);

  return 0;
}

int ep::Queue::init(int length, Message *data_s, Message *hdr_s,
                    int max_consumers, int max_producers)
{
  pthread_mutex_lock(&buffer_mtx_);  // TODO status_mtx_?
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }

  /* initialize circular queue pointers */
  workers_running_ = 0;
  counter_ = 0;
  counter_sat_ = 0;
  barrier_ = -1;
  next_input_ = 0;
  last_input_ = length - 1;

  /* initialize condition variable: queue full or queue empty */
  count_full_cond_ = 0;
  count_empty_cond_ = 0;

  max_producers_ = max_producers;
  max_consumers_ = max_consumers;

  /* create table and lists of consumers and producers */
  producers_ = new Qproducer[max_producers_];
  consumers_ = new Qconsumer[max_consumers_];
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
  if (data_s != NULL) {
    data_message_ = data_s;
    data_size_ = data_message_->size();
  }
  else
    data_size_ = 0;
  if (hdr_s != NULL) {
    hdr_message_ = hdr_s;
    hdr_size_ = hdr_message_->size();
  }
  else
    hdr_size_ = 0;

  /* set data sizes and length */
  length_ = length;

  if (data_buffer_ != NULL) std::free(data_buffer_);
  if (hdr_buffer_ != NULL) std::free(hdr_buffer_);

  hdr_buffer_ = (char *)malloc(hdr_size_ * length_);
  if (hdr_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(hdr_buffer_, 0, hdr_size_ * length_);

  data_buffer_ = (char *)malloc(data_size_ * length_);
  if (data_buffer_ == NULL) {
    pthread_mutex_unlock(&buffer_mtx_);
    printf("buffer_configure - err: Out of memory\n");
    return -1;
  }
  memset(data_buffer_, 0, data_size_ * length_);

  /* set queue status to 'c'-connected */
  status_ = 'c';
  pthread_mutex_unlock(&buffer_mtx_);

  return 0;
}

int ep::Queue::free()
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
  printf("free - %d workers_running\n", workers_running_);
  while ((count_empty_cond_) || (count_full_cond_) || (workers_running_ != 0)) {
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
  if (data_message_ != NULL) {
    delete data_message_;
    data_message_ = NULL;
  }
  if (hdr_message_ != NULL) {
    delete hdr_message_;
    hdr_message_ = NULL;
  }

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::setQueueType(QueueType flag)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'd') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -1;
  }
  if ((flag == fifo) || (flag = lifo)) {
    queue_type_ = flag;
    pthread_mutex_unlock(&buffer_mtx_);
    return 0;
  }
  else
    pthread_mutex_unlock(&buffer_mtx_);
  return -1;
}

QueueType ep::Queue::queueType()
{
  return queue_type_;
}

// int ep::Queue::setScheduleMode(ScheduleMode mode)
// {
//   pthread_mutex_lock(&buffer_mtx_);
//   if (status_ != 'd') {
//     pthread_mutex_unlock(&buffer_mtx_);
//     return -1;
//   }
//   /* set pull / push mode. pull mode only if callbacks are ready. */
//   // if ((mode == pull) && (start_read_callback_ != NULL)) {
//   //   schedule_mode_ = pull;
//   //   pthread_mutex_unlock(&buffer_mtx_);
//   //   return 0;
//   // }
//   if (mode == push) {
//     pthread_mutex_unlock(&buffer_mtx_);
//     schedule_mode_ = push;
//     return 0;
//   }
//   else {
//     pthread_mutex_unlock(&buffer_mtx_);
//     return -1;
//   }
// }

// ScheduleMode ep::Queue::scheduleMode()
// {
//   return schedule_mode_;
// }

int ep::Queue::subscribeProducer()
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

int ep::Queue::unsubscribeProducer(int id)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  for (std::list<int>::iterator it = producers_on_.begin();
       it != consumers_on_.end(); ++it) {
    if (*it == id) {
      producers_[id].reset();
      producers_on_.erase(it);
      producers_off_.push_back(id);
      pthread_mutex_unlock(&buffer_mtx_);
      return 0;
    }
  }
  pthread_mutex_unlock(&buffer_mtx_);
  return -1;
}

int ep::Queue::subscribeConsumer()
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  if (consumers_off_.empty()) return -1;
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

int ep::Queue::unsubscribeConsumer(int id)
{
  pthread_mutex_lock(&buffer_mtx_);
  if (status_ != 'c') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  printf("unsubscribe consumer ");

  for (std::list<int>::iterator it = consumers_on_.begin();
       it != consumers_on_.end(); ++it) {
    if (*it == id) {
      // check if working
      if (consumers_[id].status == 'w') {
        printf(" working ");

        // todo endRead
        Qconsumer *p = &consumers_[id];
        if ((status_ != 'c') || ((id < 0) || (id >= max_consumers_)) ||
            (p->id == -1)) {
          if ((consumers_[id].id != -1) && (id >= 0) && (id < max_consumers_)) {
            workers_running_--;
            printf("workers_running_-- %d-- cond broadcast ", workers_running_);
            pthread_cond_broadcast(&exit_cond_);
          }
          pthread_mutex_unlock(&buffer_mtx_);
          printf("return -2\n");
          return -2;
        }

        consumers_[id].reset();
        consumers_on_.erase(it);
        consumers_off_.push_back(id);

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
          p->last_out = -1;
          int imax = INT_MAX;  // TODO buffer max size
          int pid = -1;
          for (int x : consumers_on_) {
            if ((consumers_[x].last_out >= 0) &&
                (imax > consumers_[x].last_out)) {
              pid = x;
              imax = consumers_[x].last_out;
            }
          }
          //	  p->counter+=p->num_elements;
          barrier_ = pid == -1 ? -1 : imax;
        }

        workers_running_--;
        printf("workers_running -- %d status 's'\n", workers_running_);
        p->status = 's';
      }
      else {
        consumers_[id].reset();
        consumers_on_.erase(it);
        consumers_off_.push_back(id);

        if (consumers_on_.size() == 0) {
          barrier_ = -1;
        }

        printf(" not working \n");
      }
      pthread_mutex_unlock(&buffer_mtx_);
      return 0;
    }
  }

  pthread_mutex_unlock(&buffer_mtx_);
  return -1;
}

int ep::Queue::startWritePtr(int id, int num_elements)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_producers_) return -2;

  Qproducer &p = producers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  // check id correct and status = subscribed
  if ((status_ != 'c') || (p.status != 's')) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  // check if there are enough free buffers
  while ((barrier_ != -1) && (modulus(barrier_ - next_input_, length_) <
                              num_elements)) {  // TODO REV <=
    if (p.blocking == 'y') {                    // if blocking calls wait
      count_full_cond_++;
      pthread_cond_wait(&buffer_full_cond_, &buffer_mtx_);
      count_full_cond_--;
      pthread_cond_broadcast(&exit_cond_);
      if (status_ == 'd') {
        pthread_mutex_unlock(
            &buffer_mtx_);  // if queue disabled exit with error code
        return -2;
      }
    }
    else {
      pthread_mutex_unlock(
          &buffer_mtx_);  // if not blocking calls exit error code
      return -1;
    }
  }

  // if there are enough buffers:
  p.num_elements = num_elements;
  p.status = 'w';
  workers_running_++;

  int aux = next_input_;
  next_input_ = (next_input_ + num_elements) % length_;

  pthread_mutex_unlock(&buffer_mtx_);

  return aux;
}

char *ep::Queue::startWrite(int producer_id)
{
  int ptr = startWritePtr(producer_id, 1);
  if (ptr >= 0) {
    return &data_buffer_[ptr * data_size_];
  }
  else {
    return NULL;
  }
}

int ep::Queue::startWrite(int producer_id, char **data, char **hdr)
{
  int ptr = startWritePtr(producer_id, 1);
  if (ptr >= 0) {
    *data = &data_buffer_[ptr * data_size_];
    *hdr = &hdr_buffer_[ptr * hdr_size_];
    return ptr;
  }
  else {
    data = NULL;
    hdr = NULL;
    return ptr;
  }
}

int ep::Queue::startWrite(int producer_id, int num_elements, char **data,
                          char **hdr, int *len, char **data1, char **hdr1,
                          int *len1)
{
  int ptr = startWritePtr(producer_id, num_elements);
  if (ptr >= 0) {
    *data = &data_buffer_[ptr * data_size_];
    *hdr = &hdr_buffer_[ptr * hdr_size_];
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

int ep::Queue::endWrite(int id)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_producers_) return -2;

  Qproducer &p = producers_[id];

  pthread_mutex_lock(&buffer_mtx_);

  if (p.status != 'w') {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }

  int num_elements = p.num_elements;

  last_input_ = (last_input_ + num_elements) % length_;

  counter_ += num_elements;
  if (counter_ < 0) {
    printf("Queue Write - Counter of messages overflow! - \n");
    counter_ = modulus(counter_, INT_MIN);
    counter_sat_ = 1;
  }

  p.counter += num_elements;
  p.status = 's';
  workers_running_--;

  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_cond_broadcast(&exit_cond_);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::endWriteAbort(int id)
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

  // this would work only wihth one producer
  next_input_ = (next_input_ - p.num_elements) % length_;

  //  p.counter += num_elements;
  p.status = 's';
  workers_running_--;

  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_cond_broadcast(&exit_cond_);

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::startReadPtr(int id, int num_elements, int new_elements)
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
    new_buffers = modulus(counter_ - c.ptr_counter, INT_MAX);
  }
  while ((new_buffers < new_elements) ||
         ((counter_ < num_elements) && (!counter_sat_))) {
    if (c.blocking == 'y') {
      count_empty_cond_++;
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
      new_buffers = modulus(counter_ - c.ptr_counter, INT_MAX);
    }
  }

  // if there are enough buffers update state
  c.num_elements = num_elements;
  c.new_elements = new_elements;

  if (queue_type_ == fifo) {
    c.status = 'w';
    workers_running_++;

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

    pthread_mutex_unlock(&buffer_mtx_);
    return c.last_out;
  }
  return -1;
}

char *ep::Queue::startRead(int consumer_id)
{
  if (schedule_mode_ == push) {
    int ptr = startReadPtr(consumer_id, 1, 1);
    if (ptr >= 0) {
      return &data_buffer_[ptr * data_size_];
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

int ep::Queue::startRead(int consumer_id, char **data, char **hdr)
{
  if (schedule_mode_ == push) {
    int ptr = startReadPtr(consumer_id, 1, 1);
    if (ptr >= 0) {
      *data = &data_buffer_[ptr * data_size_];
      *hdr = &hdr_buffer_[ptr * hdr_size_];
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

int ep::Queue::startRead(int consumer_id, int num_elements, int new_elements,
                         char **data, char **hdr, int *len, char **data1,
                         char **hdr1, int *len1)
{
  if (schedule_mode_ == pull) return -1;
  int ptr = startReadPtr(consumer_id, num_elements, new_elements);
  if (ptr >= 0) {
    *data = &data_buffer_[ptr * data_size_];
    *hdr = &hdr_buffer_[ptr * hdr_size_];
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

int ep::Queue::endRead(int id)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_consumers_) return -2;

  // if (schedule_mode_ == pull)
  //   return end_read_callback_(pvt_, consumers_[id].ptr_aux);

  Qconsumer &c = consumers_[id];

  pthread_mutex_lock(&buffer_mtx_);

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

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::endReadAbort(int id)
{
  // Check if the id is within valid boundaries
  if (id < 0 || id >= max_consumers_) return -2;

  // if (schedule_mode_ == pull)
  //   return end_read_callback_(pvt_, consumers_[id].ptr_aux);

  Qconsumer &c = consumers_[id];

  pthread_mutex_lock(&buffer_mtx_);

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

  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

// int ep::Queue::readCopy(int consumer_id, char *data, char *hdr,
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

int ep::Queue::setWriteBlocking(int id, bool lock)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_producers_)) ||
      (producers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  producers_[id].blocking = lock ? 'y' : 'n';
  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::setReadBlocking(int id, bool lock)
{
  pthread_mutex_lock(&buffer_mtx_);
  if ((status_ != 'c') || ((id < 0) || (id >= max_consumers_)) ||
      (consumers_[id].id == -1)) {
    pthread_mutex_unlock(&buffer_mtx_);
    return -2;
  }
  consumers_[id].blocking = lock ? 'y' : 'n';
  pthread_mutex_unlock(&buffer_mtx_);
  return 0;
}

int ep::Queue::length() const
{
  return length_;
}

size_t ep::Queue::dataSize() const
{
  return data_size_;
}

size_t ep::Queue::hdrSize() const
{
  return hdr_size_;
}

char *ep::Queue::dataBuffer() const
{
  return data_buffer_;
}

char *ep::Queue::hdrBuffer() const
{
  return hdr_buffer_;
}

Message *ep::Queue::dataMessage() const
{
  return data_message_;
}

Message *ep::Queue::hdrMessage() const
{
  return hdr_message_;
}

void ep::Queue::printStats() const
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

void ep::Queue::wakeUpProducers()
{
  pthread_mutex_lock(&buffer_mtx_);
  pthread_cond_broadcast(&(buffer_full_cond_));
  pthread_mutex_unlock(&buffer_mtx_);
}

void ep::Queue::wakeUpConsumers()
{
  pthread_mutex_lock(&buffer_mtx_);
  pthread_cond_broadcast(&(buffer_empty_cond_));
  pthread_mutex_unlock(&buffer_mtx_);
}

// int ep::Queue::setCallbacks(start_callback_t cb1, end_callback_t cb2,
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
}

void Qproducer::print()
{
  std::cout << " Producer Id(" << id << "), "
            << "writes: " << counter << ", full calls: "
            << "X" << std::endl;
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
  std::cout << " Consumer Id(" << id << "), "
            << "reads: " << counter << ", losts: " << lost_counter
            << ", empty calls "
            << "X" << std::endl;
}
