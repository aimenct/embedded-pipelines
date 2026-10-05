#include "src_filter.h"

using namespace epf;

SrcFilter::SrcFilter()
    : Filter(YAML::Node(), 0, 5)
{
  addSetting("counter_init", counter_init);
  std::cout << "SrcFilter constructor " << std::endl;
}

SrcFilter::~SrcFilter()
{
  std::cout << "SrcFilter destructor" << std::endl;
}

int32_t SrcFilter::_job()
{
  int32_t err = writer_->startWrite();
  if (err >= 0) {
    // write unsigned int node
    counter_n_->write(static_cast<void *>(&my_counter_));
    counter_n2_->write(static_cast<void *>(&my_counter2_));

    array_->write(static_cast<void *>(my_array_));

    double_matrix_->write(static_cast<void *>(my_array_));
    my_counter_++;
    my_counter2_--;
    // std::cout << "Counter: " << my_counter_ << std::endl;
    for (size_t i = 0; i != 4; i++) {
      my_array_[i]++;
    }

    writer_->endWrite();
    return 0;
  }

  return err;
}

int32_t SrcFilter::_open()
{
  return 0;
}
int32_t SrcFilter::_close()
{
  return 0;
}

int32_t SrcFilter::_start()
{
  return 0;
}

int32_t SrcFilter::_stop()
{
  return 0;
}

int32_t SrcFilter::_set()
{
  my_counter_ = counter_init;

  // create message model for sink queue
  std::unique_ptr<Message> msg = std::make_unique<Message>();
  std::unique_ptr<DataNode> counter_n =
      std::make_unique<DataNode>("Counter", EP_32S, std::vector<size_t>{1},
                                 nullptr);  // value = nullptr (Queue Node)
  std::unique_ptr<DataNode> counter_n2 =
      std::make_unique<DataNode>("Counter2", EP_64F, std::vector<size_t>{1},
                                 nullptr);  // value = nullptr (Queue Node)
  msg->addItem(std::move(counter_n));
  msg->addItem(std::move(counter_n2));

  std::unique_ptr<DataNode> array = std::make_unique<DataNode>(
      "Array", EP_32F, std::vector<size_t>{4}, nullptr);

  msg->addItem(std::move(array));

  std::unique_ptr<DataNode> double_matrix = std::make_unique<DataNode>(
      "Double Matrix", EP_32F, std::vector<size_t>{2, 2}, nullptr);

  msg->addItem(std::move(double_matrix));

  // printf("Writer Message Model \n");
  // msg->print();

  //  addSinkQueue(0, std::move(msg));
  sinkPort(0)->activate(std::move(msg), nullptr);

  // get writer handler or queue 0
  writer_ = sinkPort(0)->writer();

  // get counter node - item 0
  Node *node = writer_->dataSchema()->item(0);
  assert(node->isDataNode());
  counter_n_ = static_cast<DataNode *>(node);  // static or dynamic cast

  Node *node2 = writer_->dataSchema()->item(1);
  assert(node2->isDataNode());  // ¿check if it is queued n->isQueued()?
  counter_n2_ = static_cast<DataNode *>(node2);  // static or dynamic cast

  Node *node3 = writer_->dataSchema()->item(2);
  assert(node3->isDataNode());
  array_ = static_cast<DataNode *>(node3);

  Node *node4 = writer_->dataSchema()->item(3);
  assert(node4->isDataNode());
  double_matrix_ = static_cast<DataNode *>(node4);

  return 0;
}

int32_t SrcFilter::_reset()
{
  return 0;
}
