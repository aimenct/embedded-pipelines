// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include <opcua.h>

double c[] = {1, 2, 3, 4};

using namespace ep;

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

    int32_t increaseValueSetting();

  private:
    QueueWriter *writer_;  //  QueueWriter writer(q);
    float my_counter_ = 0;
    float my_counter2_ = 0;
    int fake_setting1;
    int fake_setting2;
    int fake_setting3;
    DataNode
        *counter_n_; 
    DataNode
        *counter_n2_; 
    ImageObject
        *img_obj_;  
    DataNode *img_encd_size_;  
                               
    DataNode *array_;
    DataNode *double_matrix_;
    int err_ = 0;
};

// - Implementation of F1 Filter -
F1::F1()
    : Filter()
{
  std::cout << "F1 constructor a " << std::endl;

  my_counter_ = 0;
  max_sinks_ = 1;
  max_sources_ = 2;

  writer_ = nullptr;
  my_counter_ = 0;
  counter_n_ = nullptr;
  counter_n2_ = nullptr;
  img_obj_ = nullptr;
  err_ = 0;
  fake_setting1 = 0;
  fake_setting2 = 2;
  fake_setting3 = 5;

  name_ = "Fake Filter";
  addSetting("max_sink_queues", max_sinks_);
  addSetting("fake_setting1", fake_setting1);
  addSetting("fake_setting2", fake_setting2);
  addSetting("fake_setting3", fake_setting3);

  addCommand("increase()", std::bind(&F1::increaseValueSetting, this));

  std::cout << "ll" << std::endl;
}

F1::~F1()
{
  std::cout << "F1 destructor" << std::endl;
}

int32_t F1::increaseValueSetting()
{
  fake_setting1 += 10;
  return 0;
}

int32_t F1::_job()
{
  int err = 0;
  err = writer_->startWrite();
  if (err >= 0) {
    // write unsigned int node
    counter_n_->write(static_cast<void *>(&my_counter_));
    counter_n2_->write(static_cast<void *>(&my_counter2_));

    array_->write(static_cast<void *>(&c));

    double_matrix_->write(static_cast<void *>(&c));

    my_counter_++;
    my_counter2_++;
    for (size_t i = 0; i != 4; i++) {
      c[i]++;
    }

    writer_->endWrite();

    usleep(100000);
    
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
  // float counter = 45;
  DataNode *counter_n = new DataNode("Counter", EP_32F, {1},
                                     nullptr);  // value = nullptr (Queue Node)
  DataNode *counter_n2 = new DataNode("Counter2", EP_32F, {1},
                                      nullptr);  // value = nullptr (Queue Node)
  msg->addItem(counter_n);
  msg->addItem(counter_n2);

  ImageObject image("imagen 1", 100, 100, 3, RGBa8, nullptr);

  msg->addItem(image.copyNodeTree());

  DataNode *img_encd_size =
      new DataNode("JPG size", EP_32U, {1},
                   nullptr);  // value = nullptr (Queue Node)

  msg->addItem(img_encd_size);

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

  n2 = writer_->dataSchema()->item(3);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  img_encd_size_ = static_cast<DataNode *>(n2);  // static or dynamic cast

  n2 = writer_->dataSchema()->item(4);
  assert(n->isDataNode());
  array_ = static_cast<DataNode *>(n2);

  n2 = writer_->dataSchema()->item(5);
  assert(n->isDataNode());
  double_matrix_ = static_cast<DataNode *>(n2);

  n = writer_->dataSchema()->item(2);
  img_obj_ = new ImageObject(n);

  return err;
}

int32_t F1::_reset()
{
  if (img_obj_ != nullptr) delete img_obj_;
  img_obj_ = nullptr;

  return 0;
}
using namespace ep;

int main(/*int argc, char *argv[]*/)
{
  using namespace ep;

  std::string fname("../../../tests/opcua/config_opcua_server.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  printf("press key to continue\n");
  getchar();

  YAML::Node node4 = config["filters"][3];
  OPCUAserver *server = new OPCUAserver(node4);
  F1 *w1 = new F1();
  F1 *w2 = new F1();

  int threads = 3;

  Pipeline pipe(threads);

  pipe.add(w1);
  pipe.add(w2);
  pipe.add(server);

  pipe.connect(w1, 0, server, 0);
  pipe.connect(w2, 0, server, 1);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.assignTask(0, w1);
  pipe.assignTask(1, server);
  pipe.assignTask(2, w2);

  printf("pres enter to start\n");
  getchar();

  // start pipeline
  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  // stop pipeline
  pipe.halt();
}
