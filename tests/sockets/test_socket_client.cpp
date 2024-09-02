// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.



#include <time.h>

#include "sockets.h"

#define MAX_STR 256

using namespace ep;

double c[] = {1, 2, 3, 4};

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
    int fake_setting1_;
    int fake_setting2_;
    int fake_setting3_;
    DataNode
        *counter_n_;  // = static_cast<DataNode*>(n); // static or dynamic cast
    DataNode
        *counter_n2_;  // = static_cast<DataNode*>(n); // static or dynamic cast
    ImageObject
        *img_obj_;  //  n = writer.msg()->item(1); ImageObject1 img_obj(n);
    DataNode *img_encd_size_;  // = static_cast<DataNode*>(n); // static or
                               // dynamic cast
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
  fake_setting1_ = 0;
  fake_setting2_ = 2;
  fake_setting3_ = 5;

  name_ = "Fake Filter";
  addSetting("fake_setting1", fake_setting1_);
  addSetting("fake_setting2", fake_setting2_);
  addSetting("fake_setting3", fake_setting3_);

  addCommand("increase()", std::bind(&F1::increaseValueSetting, this));

  std::cout << "ll" << std::endl;
}

F1::~F1()
{
  std::cout << "F1 destructor" << std::endl;
}

int32_t F1::increaseValueSetting()
{
  fake_setting1_ += 10;
  return 0;
}

int32_t F1::_job()
{
  // std::cout << "F1 job" << std::endl;
  int err = 0;

  // writer_ job() function
  err = writer_->startWrite();
  // std::string imagePath = "/embedded-pipelines/tests/opcua/paisaje.jpg";
  // cv::Mat cvimage = cv::imread(imagePath);
  // std::vector<unsigned char> buffer;
  // cv::imencode(".jpg", cvimage, buffer);
  // cvimage.release();
  if (err >= 0) {
    // write unsigned int node
    counter_n_->write(static_cast<void *>(&my_counter_));
    counter_n2_->write(static_cast<void *>(&my_counter2_));

    // unsigned int bufferSize = static_cast<unsigned int>(buffer.size());
    // img_encd_size_->write(static_cast<void *>(&bufferSize));

    // array_->write(static_cast<void *>(&c));

    // double_matrix_->write(static_cast<void *>(&c));

    // memcpy(img_obj_->data(), buffer.data(), buffer.size());

    // std::cout << img_obj_->size() << "<---- This is the size" <<std::endl;
    // std::cout << (unsigned char*)img_obj_->data() << "<---- This is the
    // image data" <<std::endl;

    // writer_->msg()->print();
    my_counter_++;
    my_counter2_++;
    // for (size_t i = 0; i != 4; i++) {
    //   c[i]++;
    // }

    writer_->endWrite();
  }
  return 0;
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

  printf("Writer Message Model \n");
  msg->print();

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

  return err;
}

int32_t F1::_reset()
{
  if (img_obj_ != nullptr) delete img_obj_;
  img_obj_ = nullptr;

  return 0;
}

int main(int /*argc*/, char** /*argv[]*/)
{
  using namespace ep;


  std::string fname("../../../tests/sockets/test_socket_client.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  std::cout << "Here's the input YAML:" << std::endl << config << std::endl;

  printf("press key to continue\n");
  getchar();

  /* create FilterSocketClient object */
  YAML::Node node_src = config["filters"][0];
  // FakeSrc *src;
  YAML::Node node_eth_client = config["filters"][1];
  SocketClient *eth_client;

  // src = new FakeSrc(&node_src);
  eth_client = new SocketClient(node_eth_client);
  F1 *w1 = new F1();
  F1 *w2 = new F1();

  int threads = 3;
  Pipeline pipe(threads);

  pipe.add(eth_client);
  pipe.add(w1);
  pipe.add(w2);

  pipe.connect(w1, 0, eth_client, 0);
  pipe.connect(w2, 0, eth_client, 1);

  pipe.assignTask(0, eth_client);
  pipe.assignTask(1, w1);
  pipe.assignTask(2, w2);
  printf("pres enter to start\n");
  getchar();

  pipe.run();
  printf("pres enter to stop\n");
  getchar();

  pipe.halt();
}
