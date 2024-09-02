// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include <opencv2/opencv.hpp>

#include "di_nodeids.h"
#include "i4aas_nodeids.h"
#include "namespace_aas_generated.h"
#include "namespace_di_generated.h"
#include "namespace_i4aas_generated.h"
#include "opcua.h"
#include "types_di_generated_handling.h"
#include "types_i4aas_generated_handling.h"

double c[] = {1, 2, 3, 4};
namespace ep {
class F1 : public Filter2 {
  public:
    F1(YAML::Node *config = nullptr);  // create settings
    ~F1();                             // delete settings

  protected:
    void job();
    int _open();   // establish connection with device
    int _close();  // close connection with device
    int _set();    // create queues and allocate memory
    int _reset();  // delete queues and release memory
    int _start();  // close connection with device
    int _stop();   // create queues and allocate memory

    // TODO: saveConfig();  // ¿save current settings in a yaml?

    // Vector of Signals:
    // Signal: error (int error code)
    // Signal: emit open, close, set, reset, start, stop
    // Where? to Whom?

    // callback for generating data when working in pull mode
    // It can be implemented only by Filter Sources to avoid mem copy-
    // int startReadCallback(void **data, void **hdr, void **usr_data);
    // int endReadCallback(void *usr_data);

  private:
    QueueWriter *writer_;  //  QueueWriter writer(q);
    double my_counter_ = 0;
    double my_counter2_ = 0;
    int fake_setting1;
    int fake_setting2;
    int fake_setting3;
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
F1::F1(YAML::Node *config)
    : Filter2()
{
  std::cout << "F1 constructor " << std::endl;

  my_counter_ = 0;
  max_sink_queues_ = 1;
  max_src_queues_ = 2;

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
  addSetting("max_sink_queues", max_sink_queues_);
  addSetting("fake_setting1", fake_setting1);
  addSetting("fake_setting2", fake_setting2);
  addSetting("fake_setting3", fake_setting3);

  // if ((*config)["name"]!=NULL) {
  //   this->name_ = (*config)["name"].as<std::string>();
  // }

  // YAML::Node yamlSettings = (*config)["settings"];

  // TODO    Settings settings;
  //    setings.add( new Setting("start","startmethod",CMD,W_S,F1::start() )  );
  //    setings.add( new Setting("stop","stopmethod",CMD,W_S,F1::stop())  );

  // settings->pushBack("start", "start() method", EP_CMD,
  //                    ep::W_S, (void *)(ep::cmd_t)(&F1::start), NULL);
  // settings->pushBack("stop", "stop() method", EP_CMD,
  //                    ep::W_R, (void *)(ep::cmd_t)(&F1::stop), NULL);
  // settings->pushBack("open", "open() method", EP_CMD,
  //                    ep::W_D, (void *)(ep::cmd_t)(&F1::open), NULL);
  // settings->pushBack("close", "close() method", EP_CMD,
  //                    ep::W_C | ep::W_S | ep::W_R,
  //                    (void *)(ep::cmd_t)(&F1::close), NULL);
  // settings->pushBack("set", "set() method", EP_CMD,
  //                    ep::W_C, (void *)(ep::cmd_t)(&F1::set), NULL);
  // settings->pushBack("reset", "reset() method", EP_CMD,
  //                    ep::W_S, (void *)(ep::cmd_t)(&F1::reset), NULL);

  //    YAMLParser(&yamlSettings);
}

F1::~F1()
{
  std::cout << "F1 destructor" << std::endl;
}

void F1::job()
{
  // std::cout << "F1 job" << std::endl;
  int err = 0;

  if (status_ == ep::RUNNING_STATE) {
    // writer_ job() function
    err = writer_->startWrite();
    std::string imagePath = "/embedded-pipelines/tests/opcua/paisaje.jpg";
    cv::Mat cvimage = cv::imread(imagePath);
    std::vector<unsigned char> buffer;
    cv::imencode(".jpg", cvimage, buffer);
    if (err >= 0) {
      // write unsigned int node
      counter_n_->write(static_cast<void *>(&my_counter_));
      counter_n2_->write(static_cast<void *>(&my_counter2_));

      unsigned int bufferSize = static_cast<unsigned int>(buffer.size());
      img_encd_size_->write(static_cast<void *>(&bufferSize));

      array_->write(static_cast<void *>(&c));

      double_matrix_->write(static_cast<void *>(&c));

      memcpy(img_obj_->data(), buffer.data(), buffer.size());

      // std::cout << img_obj_->size() << "<---- This is the size" <<std::endl;
      // std::cout << (unsigned char*)img_obj_->data() << "<---- This is the
      // image data" <<std::endl;

      // writer_->msg()->print();
      my_counter_++;
      my_counter2_++;
      for (size_t i = 0; i != 4; i++) {
        c[i]++;
      }

      writer_->endWrite();
    }
    else {
      // signal["error"].emit(err);
    }
  }
}

int F1::_open()
{
  return 0;
}
int F1::_close()
{
  return 0;
}

int F1::_start()
{
  return 0;
}

int F1::_stop()
{
  return 0;
}

int F1::_set()
{
  int err = 0;
  std::cout << "F1: set" << std::endl;

  // create message model for sink queue
  Message *msg = new Message();
  double counter = 45;
  DataNode *counter_n = new DataNode("Counter", EP_64F, {1},
                                     nullptr);  // value = nullptr (Queue Node)
  DataNode *counter_n2 = new DataNode("Counter2", EP_64F, {1},
                                      nullptr);  // value = nullptr (Queue Node)
  msg->addItem(counter_n);
  msg->addItem(counter_n2);

  std::string imagePath = "/embedded-pipelines/tests/opcua/paisaje.jpg";
  cv::Mat cvimage = cv::imread(imagePath);

  int height = cvimage.rows;
  int width = cvimage.cols;

  // Obtener el número de canales
  int channels = cvimage.channels();
  PixelFormat pixel_format = RGBa8;

  std::vector<unsigned char> buffer;
  cv::imencode(".jpg", cvimage, buffer);

  ImageObject image("imagen 1", width, height, channels, pixel_format, nullptr);

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

  printf("Writer Message Model \n");
  msg->print();

  // instantiate sink queue
  Queue *q = new Queue();
  int len = 10;
  q->init(len, msg);  // (Queue gets the ownership of the Message)
  if (err < 0) {
    std::cout << "F1: error init queue" << std::endl;
    exit(0);
  }

  // connect sink queue ptr to source port 0
  connectSinkQueue(0, q);

  // get writer handler or queue 0
  writer_ = writers_[0];

  // get counter node - item 0
  Node2 *n = writer_->msg()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  counter_n_ = static_cast<DataNode *>(n);  // static or dynamic cast

  Node2 *n2 = writer_->msg()->item(1);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  counter_n2_ = static_cast<DataNode *>(n2);  // static or dynamic cast

  n2 = writer_->msg()->item(3);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued()?
  img_encd_size_ = static_cast<DataNode *>(n2);  // static or dynamic cast

  n2 = writer_->msg()->item(4);
  assert(n->isDataNode());
  array_ = static_cast<DataNode *>(n2);

  n2 = writer_->msg()->item(5);
  assert(n->isDataNode());
  double_matrix_ = static_cast<DataNode *>(n2);

  n = writer_->msg()->item(2);
  img_obj_ = new ImageObject(n);

  return err;
}

int F1::_reset()
{
  if (img_obj_ != nullptr) delete img_obj_;
  img_obj_ = nullptr;

  return 0;
}
}  // namespace ep

UA_Boolean running = true;

static void stopHandler(int sign)
{
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
  running = false;
}

int main(int argc, char *argv[])
{
  using namespace ep;

  // /* check command line arguments */
  // if (argc!=2) {
  //     std::cout << "Usage: " << argv[0] << std::endl;
  //     std::cout << "[../tests/basic_io/config_opcua_server.yml]" <<
  //     std::endl; exit(0);
  // }

  std::string fname("/embedded-pipelines/tests/opcua/xml2server/config_opcua_xml2server.yml");

  if (access(fname.c_str(), F_OK) == -1) {
    std::cout << "file not found" << std::endl;
    exit(0);
  }

  /* check input YAML */
  YAML::Node config = YAML::LoadFile(fname);
  printf("press key to continue\n");
  getchar();

  //   ///* create FakeSrc object */
  //   YAML::Node node1 = config["filters"][0];
  //   FakeSrc *src1 = new FakeSrc(&node1);
  //   //
  //   ///* create FakeSrc object */
  //   YAML::Node node2 = config["filters"][1];
  //   FakeSrc *src2 = new FakeSrc(&node2);
  //
  UA_Server *serveropc = UA_Server_new();
  UA_ServerConfig_setDefault(UA_Server_getConfig(serveropc));

  UA_StatusCode retval;
  if (namespace_aas_generated(serveropc) != UA_STATUSCODE_GOOD) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                 "Could not add the example nodeset. "
                 "Check previous output for any error.");
    retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
  }
  YAML::Node node4 = config["filters"][3];
  OPCUAXmlserver *server =
      new OPCUAXmlserver(&node4, serveropc, (char *)fname.c_str());
  F1 *w1 = new F1();
  F1 *w2 = new F1();

  int threads = 3;

  Pipeline pipe(threads);

  Settings2 settings = w1->copysettings();
  settings.print();
  w1->copysettings().print();

  pipe.add(w1);
  pipe.add(w2);
  pipe.add(server);

  // server->connectFilter(w1);
  // server->connectFilter(w2);

  // server->setSrcFilter2Port(0,w1);
  // server->setSrcFilter2Port(1,w2);

  pipe.connect(w1, 0, server, 0);
  pipe.connect(w2, 0, server, 1);

  pipe.printFilters();
  pipe.printGraph();
  getchar();

  pipe.addTask(0, w1);
  pipe.addTask(1, server);
  pipe.addTask(2, w2);

  printf("pres enter to start\n");
  getchar();

  // 6 - start pipeline
  pipe.run();

  printf("pres enter to stop\n");
  getchar();

  settings = w1->copysettings();
  settings.print();
  w2->copysettings().print();

  // 7 - stop pipeline
  pipe.halt();
}
