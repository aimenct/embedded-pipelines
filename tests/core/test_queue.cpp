// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "core.h"

using namespace epf;
using namespace std;

// // Tests for evaluating DataNode
double c[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
double v[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

DataNode gato_f(int type)
{
  if (type == 0) {
    DataNode node("gato", EP_64F, {10});  // allocate mem
    return node;
  }
  if (type == 1) {
    DataNode node(
        "gato", EP_64F, {10},
        nullptr);  // do not allocate mem - queue node  ¿ TODO flag/ parametro
                   // en lugar de nullptr? por defecto a false
    return node;
  }
  if (type == 2) {
    DataNode node("gato", EP_64F, {10},
                  (void *)c);  // do not allocate mem - reference node
    return node;
  }
  DataNode node;
  return node;
}

int test_nodes()
{
  DataNode gato = gato_f(0);  // allocate mem
  printf("gato incializado \n");
  gato.write((void *)c);
  for (int i = 0; i < 10; i++) printf("i: %g\n", ((double *)gato.value())[i]);
  gato.print();

  printf("gato memory %d\n", gato.memMgmt());

  gato = gato_f(1);  // do not allocate mem - queue node
  gato.setValue(&c);
  gato.write((void *)v);
  for (int i = 0; i < 10; i++) printf("i: %g\n", ((double *)gato.value())[i]);
  gato.print();

  gato = gato_f(2);  // do not allocate mem - queue node
  gato.write((void *)v);
  for (int i = 0; i < 10; i++) printf("i: %g\n", ((double *)gato.value())[i]);
  gato.print();
  return 0;
}

int test_nodes_1()
{
  //  ObjectNode *n1 = new ObjectNode("obj1");
  ObjectNode n1("obj1");
  // ObjectNode *n2 = new ObjectNode("obj2");
  // ObjectNode *n3 = new ObjectNode("obj3");
  // DataNode *n4 = new DataNode("dn1", EP_64F, {10});
  auto n2 = std::make_unique<ObjectNode>("obj2");
  auto n3 = std::make_unique<ObjectNode>("obj3");
  //  auto n4 = std::make_unique<DataNode>("dn1", EP_64F, {10});
  auto n4 = std::make_unique<DataNode>("dn1", EP_64F, std::vector<size_t>{10});
  n3->addReference(epf::EP_HAS_CHILD, std::move(n4));
  n2->addReference(epf::EP_HAS_CHILD, std::move(n3));
  n1.addReference(epf::EP_HAS_CHILD, std::move(n2));

  n1.printTree();

  ObjectNode cpn("cpn");
  cpn.printTree();
  n1 = cpn;
  n1.printTree();

  return 0;
}

Message my_msg(int type)
{
  if (type == 0) {
    // DataNode *vaca = new DataNode("vaca", EP_64F, {10}, static_cast<void
    // *>(c)); DataNode *gallina = new DataNode("gallina", EP_32U, {10},
    // nullptr); DataNode *perro = new DataNode("perro", EP_64F, {10});
    // StringNode *can = new StringNode("can", "daisy");

    auto vaca = std::make_unique<DataNode>(
        "vaca", EP_64F, std::vector<size_t>{10}, static_cast<void *>(c));
    auto gallina = std::make_unique<DataNode>("gallina", EP_32U,
                                              std::vector<size_t>{10}, nullptr);
    auto perro =
        std::make_unique<DataNode>("perro", EP_64F, std::vector<size_t>{10});
    auto can = std::make_unique<StringNode>("can", "daisy");

    //      DataNode *polo = new DataNode("polo",EP_32U,{10},nullptr);
    std::string name = "granja";

    ObjectNode root(name);
    perro->addReference(epf::EP_HAS_CHILD, std::move(can));
    root.addReference(epf::EP_HAS_CHILD, std::move(vaca));
    root.addReference(epf::EP_HAS_CHILD, std::move(gallina));
    root.addReference(epf::EP_HAS_CHILD, std::move(perro));
    Message msg(root);

    printf("daisy\n");
    getchar();

    printf("\nmyMsg type 0\n");
    msg.print();
    printf("\n");

    printf("daisy\n");
    getchar();

    return msg;
  }
  if (type == 1) {
    //    DataNode *polo = new DataNode("polo", EP_32U, {10}, nullptr);
    auto polo = std::make_unique<DataNode>("polo", EP_32U,
                                           std::vector<size_t>{10}, nullptr);
    Message msg;
    msg.addItem(std::move(polo));
    printf("\nmyMsg() type 1\n");
    msg.print();
    printf("\n");
    return msg;
  }
  if (type == 2) {
    Message msg;
    //   //      DataNode raton("polo",EP_32U,{10},nullptr);
    //   ObjectNode raton("raton");
    //   msg.addItem(&raton);
    return msg;
  }
  Message msg;
  return msg;
}

// Tests for evaluating Message Class
int test_msg()
{
  Message msg = my_msg(0);
  printf("\nmyMsg type 0\n");
  msg.print();
  printf("size: %ld\n", msg.size());
  printf("\n");
  getchar();

  //  Message msg1 = msg;
  Message msg1 = my_msg(1);
  printf("\nmyMsg type 1\n");
  msg1.print();
  printf("size: %ld\n", msg1.size());
  printf("\n");

  getchar();

  msg1 = my_msg(0);
  printf("\nmyMsg type 0\n");
  msg1.print();
  printf("size: %ld\n", msg1.size());
  printf("\n");

  getchar();

  // This will not work (Nodes must always be allocated in the Heap)
  // Message msg2 = myMsg(2);
  // printf("\nmyMsg type 2\n");
  // msg2.print();
  // printf("size: %ld\n",msg2.size());
  // printf("\n");

  return 0;
}

std::unique_ptr<Message> create_message_model()
{
  // Create Message Model
  // Message *msg = new Message();
  auto msg = std::make_unique<Message>();

  // item 0 - Scalar
  //  int counter = 45;
  // DataNode *counter_n = new DataNode("Counter", EP_32U, {1},
  //                                    nullptr);  // value = nullptr (Queue
  //                                    Node)
  auto counter_n =
      std::make_unique<DataNode>("Counter", EP_32U, std::vector<size_t>{1},
                                 nullptr);  // value = nullptr (Queue Node)
  msg->addItem(std::move(counter_n));

  // item 1 - Image Object
  int width = 640;
  int height = 512;
  int channels = 4;
  PixelFormat pixel_format = epf::PixelFormat::RGBA8;
  ImageObject image("imagen 1", width, height, channels, pixel_format,
                    nullptr);  // data = nullptr (data_ queue node)
  //  msg->addItem( image.transferNodeTree() );
  msg->addItem(image.copyNode());

  printf("Message Model (size %ld bytes)\n", msg->size());
  msg->print();
  printf("size: %ld\n", msg->size());

  return msg;
}

void writer_function(Queue *q)
{
  printf("\n---- Writer 1 ----\n");
  unsigned int my_counter = 0;
  int err = 0;

  QueueWriter writer(q);

  printf("Writer Message Model\n");
  writer.dataSchema()->print();

  // write DataNode - item 0
  Node *n = writer.dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued() ?
  DataNode *counter_n = static_cast<DataNode *>(n);  // static or dynamic cast

  // write ImageObject - item 1
  n = writer.dataSchema()->item(1);
  auto on = dynamic_cast<ObjectNode *>(n);
  ImageObject img_obj(on);

  printf("loop \n");
  for (int i = 0; i < 5; i++) {
    // writer job() function
    err = writer.startWrite();
    if (err >= 0) {
      // write unsigned int node
      counter_n->write(static_cast<void *>(&my_counter));
      my_counter++;

      // write image data
      if (i % 2)
        memset(img_obj.data(), 5, img_obj.bufferSize());
      else
        memset(img_obj.data(), 0, img_obj.bufferSize());

      printf("writer print whole message %d\n", i);
      writer.dataSchema()->print();

      //      sleep(1);

      writer.endWrite();
    }
  }
}

void reader_function(Queue *q)
{
  printf("\n---- Reader 1 ----\n");
  int err = 0;

  QueueReader reader(q);

  // read DataNode - item 0
  Node *n = reader.dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued() ?
  DataNode *counter_node =
      static_cast<DataNode *>(n);  // static or dynamic cast

  // read ImageObject - item 1
  n = reader.dataSchema()->item(1);
  auto on = dynamic_cast<ObjectNode *>(n);
  ImageObject img_obj(on);

  // 2- reading loop
  for (int i = 0; i < 1; i++) {
    err = reader.startRead();
    if (err >= 0) {
      printf("reader print whole message %d\n", i);
      reader.dataSchema()->print();

      printf("DataQueueNode:\n");
      counter_node->print(); /* print node */

      /* cast value ptr */
      printf("Node value (cast): %d\n",
             *((unsigned int *)counter_node->value()));

      reader.endRead();
    }
  }
}

void reader_function1(Queue *q)
{
  printf("\n---- Reader 2 ----\n");
  int err = 0;

  QueueReader reader(q);

  // read DataNode - item 0
  Node *n = reader.dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued() ?
  DataNode *counter_node =
      static_cast<DataNode *>(n);  // static or dynamic cast

  // read ImageObject - item 1
  n = reader.dataSchema()->item(1);
  auto on = dynamic_cast<ObjectNode *>(n);
  ImageObject img_obj(on);

  // 2- reading loop
  for (int i = 0; i < 1; i++) {
    err = reader.startRead();
    if (err >= 0) {
      printf("reader print whole message %d\n", i);
      reader.dataSchema()->print();

      printf("DataQueueNode:\n");
      counter_node->print(); /* print node */

      /* cast value ptr */
      printf("Node value (cast): %d\n",
             *((unsigned int *)counter_node->value()));

      reader.endRead();
    }
  }
}

// Tests writing simple data nodes in the queue
int test_queue()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  // Create a shared_ptr to manage the Queue
  std::unique_ptr<Queue> q1 = std::make_unique<Queue>();

  int len = 10;
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)

  /******************************************************/
  /* Create Writer                                      */
  /******************************************************/
  writer_function(q1.get());

  // /******************************************************/
  // /* Create Reader 1                                    */
  // /******************************************************/
  reader_function(q1.get());
  // /******************************************************/
  // /* Create Reader 2                                    */
  // /******************************************************/
  reader_function1(q1.get());

  // /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

// Tests writing simple data nodes in the queue
int test_queue_v1()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  auto q1 = std::make_unique<Queue>();
  int len = 10;
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)

  printf("\n---- Writer 1 ----\n");
  unsigned int my_counter = 0;
  int err = 0;

  QueueWriter writer(q1.get());

  QueueReader reader(q1.get());

  // write DataNode - item 0
  Node *n = writer.dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued() ?
  DataNode *counter_n = static_cast<DataNode *>(n);  // static or dynamic cast
  // write ImageObject - item 1
  n = writer.dataSchema()->item(1);
  auto on = dynamic_cast<ObjectNode *>(n);
  ImageObject img_obj(on);

  // read DataNode - item 0
  n = reader.dataSchema()->item(0);
  assert(n->isDataNode());  // ¿check if it is queued n->isQueued() ?
  DataNode *counter_node_r =
      static_cast<DataNode *>(n);  // static or dynamic cast

  // read ImageObject - item 1
  n = reader.dataSchema()->item(1);
  on = dynamic_cast<ObjectNode *>(n);
  ImageObject img_obj_r(on);

  printf("loop \n");
  for (int i = 0; i < 20; i++) {
    q1->printStats();

    // writer job() function
    err = writer.startWrite();
    if (err >= 0) {
      // write unsigned int node
      counter_n->write(static_cast<void *>(&my_counter));
      my_counter++;
      // write image data
      if (i % 2)
        memset(img_obj.data(), 5, img_obj.bufferSize());
      else
        memset(img_obj.data(), 0, img_obj.bufferSize());

      printf("writer print whole message %d\n", i);
      writer.dataSchema()->print();
      writer.endWrite();
    }

    err = reader.startRead();
    if (err >= 0) {
      printf("reader print whole message %d\n", i);
      reader.dataSchema()->print();

      printf("DataQueueNode:\n");
      counter_node_r->print(); /* print node */

      /* cast value ptr */
      printf("Node value (cast): %d\n",
             *((unsigned int *)counter_node_r->value()));
      reader.endRead();
    }

    getchar();
  }

  /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

class TestReader {
  public:
    QueueReader reader;
    std::unique_ptr<char[]> data;
    std::unique_ptr<char[]> hdr;
    Message *dataMsg;
    Message *hdrMsg;
    int byte_count;
    int num_msgs;
    size_t msg_size;
    size_t hdr_size;
    int batch;
    int new_per_batch;

  public:
    //    TestReader(Queue *q, int msgs, bool blocking)
    TestReader(Queue *q, int msgs, bool blocking)
        : reader(q)
    {
      num_msgs = msgs;
      msg_size = static_cast<int>(reader.dataSchema()->size());
      hdr_size = static_cast<int>(reader.hdrSchema()->size());
      reader.setBlockingCalls(blocking);
      data = std::make_unique<char[]>(msg_size * num_msgs);
      hdr = std::make_unique<char[]>(hdr_size * num_msgs);
      byte_count = 0;
      batch = 1;
      new_per_batch = 1;
    }

    // ~TestReader()
    // {
    //   //      r->unsubscribe();
    //   // delete r;
    //   delete[] data;
    //   delete[] hdr;
    // }

    int read()
    {
      int err = reader.startRead();
      if (err >= 0) {
        memcpy(&data[byte_count], reader.dataPtrA(), msg_size);
        byte_count += static_cast<int>(msg_size);
        reader.endRead();
      }
      return err;
    }

    int read(int batchSize_, int newPerBatch_)
    {
      int err = reader.startRead(batchSize_, newPerBatch_);
      if (err >= 0) {
        memcpy(&data[byte_count], reader.dataPtrA(), msg_size * reader.lenA());
        byte_count += static_cast<int>(msg_size * reader.lenA());
        memcpy(&data[byte_count], reader.dataPtrB(), msg_size * reader.lenB());
        byte_count += static_cast<int>(msg_size * reader.lenB());
        // r->dataMsg(0).print();
        reader.endRead();
      }
      return err;
    }
    char *getData()
    {
      return data.get();
    }
    char *getHdr()
    {
      return hdr.get();
    }
    int getBatch()
    {
      return batch;
    }
    int getNewPerBatch()
    {
      return new_per_batch;
    }
};

class TestWriter {
  public:
    QueueWriter writer;
    std::unique_ptr<char[]> data;
    std::unique_ptr<char[]> hdr;
    Message *dataMsg;
    Message *hdrMsg;
    long int byte_count;
    int num_msgs;
    size_t msg_size;
    size_t hdr_size;
    int batch;

  public:
    TestWriter(Queue *q, int msgs, bool blocking)
        : writer(q)
    {
      num_msgs = msgs;
      msg_size = writer.dataSchema()->size();
      hdr_size = writer.hdrSchema()->size();
      writer.setBlockingCalls(blocking);
      data = std::make_unique<char[]>(msg_size * num_msgs);
      hdr = std::make_unique<char[]>(hdr_size * num_msgs);
      byte_count = 0;
      batch = 1;
      generateData();
    }
    // ~TestWriter()
    // {
    //   // w->unsubscribe();
    //   delete[] data;
    //   delete[] hdr;
    // }

    void generateData()
    {
      unsigned int counter = 0;
      for (int i = 0, ii = 0; i < num_msgs;
           i++, ii += static_cast<int>(msg_size)) {
        *((unsigned int *)&(data[ii])) = counter++;

        if (i % 2)
          memset(&(data[sizeof(counter) + ii]), i,
                 (msg_size - sizeof(unsigned int)));
        else
          memset(&(data[sizeof(counter) + ii]), 0,
                 (msg_size - sizeof(unsigned int)));
      }
    }

    int write()
    {
      // std::cout << "write \n";
      // getchar();

      int err = writer.startWrite();
      if (err >= 0) {
        memcpy(writer.dataPtrA(), &data[byte_count], msg_size);
        // w->dataMsg(0).print();
        // getchar();
        byte_count += msg_size;
        writer.endWrite();
      }
      return err;
    }

    int write(int batchSize_)
    {
      // std::cout << "write \n";
      // getchar();

      int err = writer.startWrite(batchSize_);
      if (err >= 0) {
        memcpy(writer.dataPtrA(), &data[byte_count], msg_size * writer.lenA());
        byte_count += msg_size * writer.lenA();
        memcpy(writer.dataPtrB(), &data[byte_count], msg_size * writer.lenB());
        byte_count += msg_size * writer.lenB();
        //        w->dataMsg(0).print();
        //        getchar();
        writer.endWrite();
      }
      return err;
    }
    char *getData()
    {
      return data.get();
    }
    char *getHdr()
    {
      return hdr.get();
    }
    int getBatch()
    {
      return batch;
    }
};

int test_queue_v2()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  // Create a shared_ptr to manage the Queue
  auto q1 = std::make_unique<Queue>();
  int len = 10;
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)
  q1->setType(lifo);
  // max consumers

  /******************************************************/
  /* Create Test Writers & Readers                      */
  /******************************************************/
  int num_msgs = 500;
  bool blocking = true;

  TestWriter tw(q1.get(), num_msgs, blocking);
  TestReader tr(q1.get(), num_msgs, blocking);
  TestReader tr1(q1.get(), num_msgs, blocking);

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_msgs; i++) {
    tw.write(1);
    tr.read(1, 1);
    tr1.read(1, 1);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  std::cout << "Execution time: " << duration << " us" << std::endl;

  size_t size = q1->dataSize() * num_msgs;

  int err = 0;
  // check memory
  if (memcmp(tw.getData(), tr.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader data" << std::endl;
    err = -1;
  }

  if (memcmp(tw.getData(), tr1.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader1 data" << std::endl;
    err = -1;
  }

  if (err == 0)
    std::cout << "Test - Passed " << std::endl;
  else
    std::cout << "Test - Not Passed " << std::endl;

  // /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

int test_queue_v3()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  // Create a shared_ptr to manage the Queue
  auto q1 = std::make_unique<Queue>();
  int len = 10;
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)
  q1->setType(lifo);
  // max consumers

  /******************************************************/
  /* Create Test Writers & Readers                      */
  /******************************************************/
  int num_msgs = 500;
  bool blocking = true;

  TestWriter tw(q1.get(), num_msgs, blocking);
  TestReader tr(q1.get(), num_msgs, blocking);
  TestReader tr1(q1.get(), num_msgs, blocking);

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_msgs / 2; i++) {
    tw.write(2);
    tr.read(2, 2);
    tr1.read(2, 2);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  std::cout << "Execution time: " << duration << " us" << std::endl;

  size_t size = q1->dataSize() * num_msgs;

  int err = 0;
  // check memory
  if (memcmp(tw.getData(), tr.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader data" << std::endl;
    err = -1;
  }

  if (memcmp(tw.getData(), tr1.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader1 data" << std::endl;
    err = -1;
  }

  if (err == 0)
    std::cout << "Test - Passed " << std::endl;
  else
    std::cout << "Test - Not Passed " << std::endl;

  /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

/////////////////////////  TEST QUEUE V4 ///////////////////////
bool flag = false;

void *test_writer_function_1(void *f)
{
  TestWriter *w = reinterpret_cast<TestWriter *>(f);
  for (int i = 0; i < w->num_msgs / w->getBatch(); i++) {
    w->write(w->getBatch());
    usleep(100);
    if (flag) break;
  }
  std::cout << "TWfun exit" << std::endl;
  pthread_exit(NULL);  // Correctly specify NULL as the exit status
}

void *test_reader_function_1(void *f)
{
  TestReader *r = reinterpret_cast<TestReader *>(f);
  for (int i = 0; i < r->num_msgs / r->getBatch(); i++) {
    r->read(r->getBatch(), r->getNewPerBatch());
    //    printf("----------------- count %d\n", i);
  }
  printf("TRfun exit\n");

  flag = true;
  r->reader.queue()->setWriteBlocking(0, 0);
  r->reader.queue()->wakeUpProducers();

  pthread_exit(NULL);
}

int test_queue_v4()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  // Create a shared_ptr to manage the Queue
  auto q1 = std::make_unique<Queue>();
  int len = 10;

  int er = q1->setType(fifo);
  if (er) {
    printf("setting fifo error\n");
  }
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)

  // max consumers

  /******************************************************/
  /* Create Test Writers & Readers                      */
  /******************************************************/
  int num_msgs = 50;
  bool blocking = true;

  TestWriter tw(q1.get(), 2 * num_msgs, blocking);
  TestReader tr(q1.get(), num_msgs, blocking);
  tr.batch = 5;
  tr.new_per_batch = 5;
  TestReader tr1(q1.get(), num_msgs, blocking);
  tr1.batch = 1;
  tr1.new_per_batch = 1;

  auto start = std::chrono::high_resolution_clock::now();

  // Create a pthread
  pthread_t thread[3];

  if (pthread_create(&thread[0], NULL, test_writer_function_1, &tw) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }

  if (pthread_create(&thread[1], NULL, test_reader_function_1, &tr) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }
  if (pthread_create(&thread[2], NULL, test_reader_function_1, &tr1) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }

  for (int i = 2; i >= 0; i--) {
    printf("joining thread %d\n", i);
    if (pthread_join(thread[i], NULL) != 0) {
      std::cerr << "Error joining thread" << std::endl;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  std::cout << "Execution time: " << duration << " us" << std::endl;

  size_t size = q1->dataSize() * num_msgs;

  int err = 0;
  if (memcmp(tw.getData(), tr.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader data" << std::endl;
    err = -1;
  }

  if (memcmp(tr.getData(), tr1.getData(), size) != 0) {
    std::cout << "Error: reader data differs from reader1 data" << std::endl;
    err = -1;
  }

  if (err == 0)
    std::cout << "Test - Passed " << std::endl;
  else
    std::cout << "Test - Not Passed " << std::endl;

  // char *d = tr1.getData();
  // std::cout << "t1: ";
  // for (int i = 0; i < num_msgs; i++) {
  //   std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  // }
  // std::cout << endl;

  // d = tr.getData();
  // std::cout << "t0: ";
  // for (int i = 0; i < num_msgs; i++) {
  //   std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  // }
  // std::cout << endl;

  // d = tw.getData();
  // std::cout << "w0: ";
  // for (int i = 0; i < num_msgs; i++) {
  //   std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  // }
  // std::cout << endl;

  /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

/////////////////////////  TEST QUEUE V5 ///////////////////////
bool flag1 = false;

void *test_writer_function_2(void *f)
{
  TestWriter *w = reinterpret_cast<TestWriter *>(f);
  for (int i = 0; i < w->num_msgs / w->getBatch(); i++) {
    w->write(w->getBatch());
    usleep(100);
  }

  flag1 = true;
  w->writer.queue()->setReadBlocking(0, 0);
  w->writer.queue()->setReadBlocking(1, 0);
  w->writer.queue()->wakeUpProducers();

  std::cout << "TWfun exit" << std::endl;
  pthread_exit(NULL);  // Correctly specify NULL as the exit status
}

void *test_reader_function_2(void *f)
{
  TestReader *r = reinterpret_cast<TestReader *>(f);
  for (int i = 0; i < r->num_msgs / r->getBatch(); i++) {
    r->read(r->getBatch(), r->getNewPerBatch());
    //    printf("----------------- count %d\n", i);
    if (flag1) break;
  }
  printf("TRfun exit\n");
  pthread_exit(NULL);
}

int test_queue_v5()
{
  /*----------------------------------*/
  /*  Create Queue                    */
  /*----------------------------------*/
  auto msg = create_message_model();

  // Create Queue with Message Structure
  // Create a shared_ptr to manage the Queue
  auto q1 = std::make_unique<Queue>();
  int len = 10;

  int er = q1->setType(lifo);
  if (er) {
    printf("setting fifo error\n");
  }
  q1->init(len, std::move(msg));  // (Queue gets the ownership of the Message)

  // max consumers

  /******************************************************/
  /* Create Test Writers & Readers                      */
  /******************************************************/
  int num_msgs = 50;
  bool blocking = true;

  TestWriter tw(q1.get(), 2 * num_msgs, blocking);
  tw.batch = 3;
  TestReader tr(q1.get(), 4 * num_msgs, blocking);
  tr.batch = 5;
  tr.new_per_batch = 5;
  TestReader tr1(q1.get(), 4 * num_msgs, blocking);
  tr1.batch = 2;
  tr1.new_per_batch = 2;

  auto start = std::chrono::high_resolution_clock::now();

  // Create a pthread
  pthread_t thread[3];

  if (pthread_create(&thread[0], NULL, test_writer_function_2, &tw) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }

  if (pthread_create(&thread[1], NULL, test_reader_function_2, &tr) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }
  if (pthread_create(&thread[2], NULL, test_reader_function_2, &tr1) != 0) {
    std::cerr << "Error creating thread" << std::endl;
  }

  for (int i = 2; i >= 0; i--) {
    printf("joining thread %d\n", i);
    if (pthread_join(thread[i], NULL) != 0) {
      std::cerr << "Error joining thread" << std::endl;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  std::cout << "Execution time: " << duration << " us" << std::endl;

  size_t size = q1->dataSize() * num_msgs;

  int err = 0;
  if (memcmp(tw.getData(), tr.getData(), size) != 0) {
    std::cout << "Error: writer data differs from reader data" << std::endl;
    err = -1;
  }

  if (memcmp(tr.getData(), tr1.getData(), size) != 0) {
    std::cout << "Error: reader data differs from reader1 data" << std::endl;
    err = -1;
  }

  if (err == 0)
    std::cout << "Test - Passed " << std::endl;
  else
    std::cout << "Test - Not Passed " << std::endl;

  char *d = tr1.getData();
  std::cout << "t1: ";
  for (int i = 0; i < num_msgs; i++) {
    std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  }
  std::cout << endl;

  d = tr.getData();
  std::cout << "t0: ";
  for (int i = 0; i < num_msgs; i++) {
    std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  }
  std::cout << endl;

  d = tw.getData();
  std::cout << "w0: ";
  for (int i = 0; i < num_msgs; i++) {
    std::cout << *(unsigned int *)&d[i * q1->dataSize()] << " ";
  }
  std::cout << endl;

  /* delete queue */
  // q1->free();
  // q1 = nullptr;
  return 0;
}

int main()
{
  test_nodes();
  test_nodes_1();
  test_msg();
  test_queue();
  test_queue_v1();
  test_queue_v2();
  test_queue_v3();
  test_queue_v4();
  return 0;
}
