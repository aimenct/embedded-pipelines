// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "opcua.h"

namespace ep {
class F1 : public Filter {
  public:
    F1();
    ~F1();

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();

  private:
    QueueWriter *writer_;  //  QueueWriter writer(q);
    double my_counter_ = 0;
    double my_counter2_ = 0;
    double my_array_[4] = {1, 2, 3, 4};

    DataNode *counter_n_;
    DataNode *counter_n2_;
    DataNode *img_encd_size_;

    DataNode *array_;
    DataNode *double_matrix_;
    int err_ = 0;
};

// - Implementation of F1 Filter -
F1::F1()
    : Filter()
{
  std::cout << "F1 constructor " << std::endl;

  my_counter_ = 0;
  max_sinks_ = 1;

  writer_ = nullptr;
  my_counter_ = 0;
  counter_n_ = nullptr;
  counter_n2_ = nullptr;
  err_ = 0;
}

F1::~F1()
{
  std::cout << "F1 destructor" << std::endl;
}

int32_t F1::_job()
{
  int err = 0;

  err = writer_->startWrite();
  if (err >= 0) {
    // write unsigned int node
    counter_n_->write(static_cast<void *>(&my_counter_));
    counter_n2_->write(static_cast<void *>(&my_counter2_));

    array_->write(static_cast<void *>(my_array_));

    double_matrix_->write(static_cast<void *>(my_array_));
    my_counter_++;
    my_counter2_++;
    for (size_t i = 0; i != 4; i++) {
      my_array_[i]++;
    }

    writer_->endWrite();
    return 0;
  }
  return err;
}

int32_t F1::_open()
{
  return 0;
}
int32_t F1::_close()
{
  return 0;
}

int32_t F1::_start()
{
  return 0;
}

int32_t F1::_stop()
{
  return 0;
}

int32_t F1::_set()
{
  int err = 0;
  std::cout << "F1: set" << std::endl;

  // create message model for sink queue
  Message *msg = new Message();
  DataNode *counter_n = new DataNode("Counter", EP_64F, {1},
                                     nullptr);  // value = nullptr (Queue Node)
  DataNode *counter_n2 = new DataNode("Counter2", EP_64F, {1},
                                      nullptr);  // value = nullptr (Queue Node)
  msg->addItem(counter_n);
  msg->addItem(counter_n2);

  DataNode *array = new DataNode("Array", EP_64F, {4}, nullptr);

  msg->addItem(array);

  DataNode *double_matrix =
      new DataNode("Double Matrix", EP_64F, {2, 2}, nullptr);

  msg->addItem(double_matrix);

  // printf("Writer Message Model \n");
  // msg->print();

  addSinkQueue(0,msg);

  // get writer handler or queue 0
  writer_ = writer(0);

  // get counter node - item 0
  Node2 *n = writer_->dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  counter_n_ = static_cast<DataNode *>(n);  // static or dynamic cast

  Node2 *n2 = writer_->dataSchema()->item(1);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  counter_n2_ = static_cast<DataNode *>(n2);  // static or dynamic cast

  n2 = writer_->dataSchema()->item(2);
  assert(n->isDataNode());
  array_ = static_cast<DataNode *>(n2);

  n2 = writer_->dataSchema()->item(3);
  assert(n->isDataNode());
  double_matrix_ = static_cast<DataNode *>(n2);

  return err;
}

int32_t F1::_reset()
{
  return 0;
}
}  // namespace ep

int main()
{
  using namespace ep;

  std::string fname("../tests/opcua/config_opcua_client.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  printf("press key to continue\n");
  getchar();

  YAML::Node node4 = config["filters"][0];
  OPCUAClient *client = new OPCUAClient(node4);
  F1 *fake_source = new F1();

  int threads = 3;

  Pipeline pipe(threads);

  pipe.add(fake_source);
  pipe.add(client);

  pipe.connect(fake_source, 0, client, 0);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.assignTask(0, fake_source);
  pipe.assignTask(0, client);

  printf("pres enter to start\n");
  getchar();

  // start pipeline
  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  // stop pipeline
  pipe.halt();
}
