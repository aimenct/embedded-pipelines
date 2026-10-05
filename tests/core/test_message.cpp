// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "core.h"
/*
Tests check Message NodeTree
*/

using namespace epf;
using namespace std;

// TEST case for DataNode (type double array)
double c[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
double v[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

DataNode create_node(int type, int interface)
{
  /* create different types of DataNodes:
     - type = 0,  memory managed
     - type = 1, not memory managed
     - type = 2, streamed
     with simple interface (=0) or full interface (=1)
  */

  if (type == 0) {
    if (interface == 0) {
      DataNode node("my node", EP_64F, {10});  // memory managed
      return node;
    }
    else {
      //  DataNode node("my node", EP_64F, {10});  // memory managed
      DataNode node("my_node", EP_64F, {10}, nullptr, true, "tooltip..");
      return node;
    }
  }
  else if (type == 1) {  // streamed
    if (interface == 0) {
      DataNode node("my_node", EP_64F, {10}, nullptr);
      return node;
    }
    else {
      DataNode node("my_node", EP_64F, {10}, nullptr, false, "tooltip..");
      return node;
    }
  }
  else if (type == 2) {  // not memory managed
    if (interface == 0) {
      DataNode node("my_node", EP_64F, {10}, (void *)c);
      return node;
    }
    else {
      DataNode node("my_node", EP_64F, {10}, (void *)c, false, "tooltip..");
      return node;
    }
  }
  else {
    DataNode node;
    return node;
  }
}

int test_data_node()
{
  /* test: create, write, print different types of data nodes */
  int err = 0;

  for (int interface = 0; interface < 2; interface++) {
    DataNode my_node = create_node(0, interface);  // memory managed
    my_node.write((void *)c);                      // copy c array to my_node
    if (memcmp((void *)c, my_node.value(), my_node.size()) != 0) {
      std::cout << " Test data node: not passed" << std::endl;
      std::cout << " memcpy(c,my_node.value(),my_node.size()) size"
                << my_node.size() << std::endl;
      err = -1;
    }

    my_node = create_node(1, interface);  // streamed
    my_node.setValue(&c);  // set my_node value pointing to c array
    if (my_node.value() != c) {
      std::cout << " Test data node: not passed" << std::endl;
      std::cout << " (my_node.value() == c)" << std::endl;
      err = -1;
    }

    my_node = create_node(2, interface);  // not memory managed
    my_node.write((void *)v);             // copy v array to my_node
    if (memcmp((void *)v, my_node.value(), my_node.size()) != 0) {
      std::cout << " Test data node: not passed" << std::endl;
      std::cout << " (my_node.value() == v)" << std::endl;
      err = -1;
    }
  }

  return err;
}

// // TEST case for DataNode (type string)
// std::string a = "hello!";
// std::string b = "bye bye!";
// DataNode create_string_datanode(int type, int interface)
// {
//   /* create different types of DataNodes:
//      - type = 0, memory managed
//      - type = 1, not memory managed
//      - type = 2, streamed --> this is not allowed
//      with simple interface (=0) or full interface (=1)
//   */

//   if (type == 0) {
//     if (interface == 0) {
//       DataNode node("my node", EP_STRING, {1});  // memory managed
//       *(std::string *)node.value() = a;
//       //      std::cout << *(std::string *)node.value() << std::endl;
//       //      DataNode n = node;
//       //      *(std::string *)node.value() = b;
//       // std::cout << *(std::string *)node.value() << std::endl;
//       // std::cout << *(std::string *)n.value() << std::endl;
//       // std::cout << "exit create ()"<< std::endl;
//       return node;
//     }
//     else {
//       //  DataNode node("my node", EP_64F, {10});  // memory managed
//       DataNode node("my_node", EP_STRING, {1}, nullptr, true, false,
//                     "tooltip..");
//       return node;
//     }
//   }
//   else if (type == 1) {  // streamed
//     if (interface == 0) {
//       DataNode node("my_node", EP_STRING, {1}, nullptr);
//       return node;
//     }
//     else {
//       DataNode node("my_node", EP_STRING, {1}, nullptr, false, true,
//                     "tooltip..");
//       return node;
//     }
//   }
//   else if (type == 2) {  // not memory managed
//     if (interface == 0) {
//       DataNode node("my_node", EP_STRING, {1}, (void *)&a);
//       return node;
//     }
//     else {
//       DataNode node("my_node", EP_STRING, {1}, (void *)&a, false, false,
//                     "tooltip..");
//       return node;
//     }
//   }
//   else {
//     DataNode node;
//     return node;
//   }
// }

// int test_string_data_node()
// {
//   /* test: create, write, print different types of string data nodes */
//   int err = 0;

//   for (int interface = 0; interface < 2; interface++) {
//     DataNode my_node = create_string_datanode(0, interface);  // memory
//     managed my_node.write((void *)&a);  // copy a string to my_node
//     //*(static_cast<std::string *>(my_node.value())) = a;
//     if (a != *(std::string *)my_node.value()) {
//       std::cout << " Test data node: not passed" << std::endl;
//       std::cout << " memcpy(c,my_node.value(),my_node.size()) size"
//                 << my_node.size() << std::endl;
//       std::cout << " " << *(std::string *)my_node.value() << std::endl;
//       std::cout << " " << a << std::endl;
//       err = -1;
//     }

//     // my_node = create_string_datanode(1, interface);  // streamed
//     // my_node.setValue(&c);  // set my_node value pointing to c array
//     // if (my_node.value() != c) {
//     //   std::cout << " Test data node: not passed" << std::endl;
//     //   std::cout << " (my_node.value() == c)" << std::endl;
//     //   err = -1;
//     // }

//     my_node = create_string_datanode(2, interface);  // not memory managed
//     my_node.write((void *)&b);  // copy string object a to my_node
//     if (memcmp((void *)&a, (void *)my_node.value(), my_node.size()) != 0) {
//       std::cout << " Test data node: not passed" << std::endl;
//       std::cout << " (my_node.value() == v)" << std::endl;
//       std::cout << " " << *(std::string *)my_node.value() << std::endl;
//       std::cout << " " << b << std::endl;
//       std::cout << " " << a << std::endl;
//       err = -1;
//     }
//   }

//   // DataNode string_array("my node", EP_STRING, {1, 2});  // memory managed
//   // std::string ss[] = {"cat", "dog"};
//   // std::cout << ss[0] << std::endl;
//   // std::cout << ss[1] << std::endl;
//   // string_array.write(&ss);

//   return err;
// }

// TEST case for ObjectNode
int my_fun()
{
  //  std::cout << "Running command: my_fun called\n";
  return 0;
}

// Class with a member function to be bound
class MyClass {
  public:
    int myMemberFunction()
    {
      //      std::cout << "Running command: MyClass member function called\n";
      return 0;
    }
};

int test_command_node()
{
  // Creating a CommandNode with a simple command (a lambda function)
  CommandNode my_comand_node("PrintCommand", []() -> int {
    // std::cout << "Running command: lambda function call" << std::endl;
    return 0;  // return 0 to indicate success
  });

  // Running the command
  int result = my_comand_node.run();
  if (result != 0) {
    return result;
    //    std::cout << "Command execution failed with code: " << result <<
    //    std::endl;
  }

  // Binding the my_fun function to the CommandNode
  CommandNode my_comand_node_1("MyFunctionCommand", std::bind(&my_fun));
  // Running the command
  result = my_comand_node_1.run();
  if (result != 0) {
    return result;
    //    std::cout << "Command execution failed with code: " << result <<
    //    std::endl;
  }

  // Binding the myMemberFunction of myClassInstance to the CommandNode
  MyClass my_class_instance;
  CommandNode my_command_node_2(
      "MyMemberFunctionCommand",
      std::bind(&MyClass::myMemberFunction, &my_class_instance));

  // Running the command
  result = my_command_node_2.run();
  if (result != 0) {
    //    std::cout << "Command execution failed with code: " << result <<
    //    std::endl;
    return result;
  }

  CommandNode my_command_node_3(my_command_node_2);
  result = my_command_node_3.run();
  if (result != 0) {
    //    std::cout << "Command execution failed with code: " << result <<
    //    std::endl;
    return result;
  }

  CommandNode my_command_node_4 = my_command_node_2;
  result = my_command_node_4.run();
  if (result != 0) {
    //    std::cout << "Command execution failed with code: " << result <<
    //    std::endl;
    return result;
  }

  return result;
}

// TEST case for ObjectNode
int test_object_node()
{
  /* test:
     - create a tree of object nodes with reference
     - print complete tree */

  int err = 0;

  // ObjectNode n1("Folder");
  // ObjectNode *n2 = new ObjectNode("Folder1");
  // ObjectNode *n3 = new ObjectNode("Folder2");
  // DataNode *n4 = new DataNode("file 1", EP_64F, {10});
  // DataNode *n5 = new DataNode("file 2", EP_64F, {10});
  // CommandNode *n6 = new CommandNode("PrintCommand", []() -> int {
  //   //    std::cout << "Running command: lambda function call" << std::endl;
  //   return 0;  // return 0 to indicate success
  // });

  ObjectNode n1("Folder");
  auto n2 = std::make_unique<ObjectNode>("Folder1");
  auto n3 = std::make_unique<ObjectNode>("Folder2");
  auto n4 =
      std::make_unique<DataNode>("file 1", EP_64F, std::vector<size_t>{10});
  auto n5 =
      std::make_unique<DataNode>("file 2", EP_64F, std::vector<size_t>{10});
  auto n6 = std::make_unique<CommandNode>("PrintCommand", []() -> int {
    //    std::cout << "Running command: lambda function call" << std::endl;
    return 0;  // return 0 to indicate success
  });

  n3->addReference(epf::EP_HAS_CHILD, std::move(n6));
  n3->addReference(epf::EP_HAS_CHILD, std::move(n5));
  n2->addReference(epf::EP_HAS_CHILD, std::move(n4));
  n1.addReference(epf::EP_HAS_CHILD, std::move(n2));
  n1.addReference(epf::EP_HAS_CHILD, std::move(n3));

  //  n1.printTree();

  ObjectNode cpn("cpn");
  //  cpn.printTree();
  n1 = cpn;
  //  n1.printTree();

  return err;
}

// TEST case for Message
Message create_msg(int type)
{
  if (type == 0) {
    // DataNode *cow = new DataNode("cow", EP_64F, {10},
    //                              static_cast<void *>(c));  // not memory
    //                              managed
    auto cow = std::make_unique<DataNode>(
        "cow", EP_64F, std::vector<size_t>{10},
        static_cast<void *>(c));  // not memory managed

    // DataNode *chicken =
    //     new DataNode("chicken", EP_32U, {10}, nullptr);  // streamed node
    // DataNode *dog = new DataNode("dog", EP_64F, {10});  // memory managed
    // StringNode *pig = new StringNode("pig", "daisy");

    auto chicken = std::make_unique<DataNode>(
        "chicken", EP_32U, std::vector<size_t>{10}, nullptr);  // streamed node
    auto dog = std::make_unique<DataNode>(
        "dog", EP_64F, std::vector<size_t>{10});  // memory managed
    auto pig = std::make_unique<StringNode>("pig", "daisy");
    std::string name = "farm";

    ObjectNode root(name);
    dog->addReference(epf::EP_HAS_CHILD, std::move(pig));
    root.addReference(epf::EP_HAS_CHILD, std::move(cow));
    root.addReference(epf::EP_HAS_CHILD, std::move(chicken));
    root.addReference(epf::EP_HAS_CHILD, std::move(dog));

    Message msg(root);
    // printf("\nmyMsg type 0\n");
    // msg.print();
    // printf("\n");
    return msg;
  }
  else if (type == 1) {
    // DataNode *chicken =
    //     new DataNode("chicken", EP_32U, {10}, nullptr);  // streamed node
    // Message msg;
    // msg.addItem(chicken);

    auto chicken = std::make_unique<DataNode>(
        "chicken", EP_32U, std::vector<size_t>{10}, nullptr);  // streamed node
    Message msg;
    msg.addItem(std::move(chicken));

    return msg;
  }
  else {
    Message msg;
    return msg;
  }
}

int test_message()
{
  Message msg = create_msg(0);
  // printf("\nmyMsg type 0\n");
  // msg.print();
  // printf("size: %ld\n", msg.size());
  // printf("\n");

  //  Message msg1 = msg;
  Message msg1 = create_msg(1);
  // printf("\nmyMsg type 1\n");
  // msg1.print();
  // printf("size: %ld\n", msg1.size());
  // printf("\n");

  msg1 = create_msg(0);
  // printf("\nmyMsg type 0\n");
  // msg1.print();
  // printf("size: %ld\n", msg1.size());
  // printf("\n");

  return 0;
}

// TEST case for Message Serialization
std::unique_ptr<Message> create_message_model()
{
  // Create Message Model
  //  Message *msg = new Message();
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

  // item 1 - Image Object (defined by the library)
  int width = 640;
  int height = 512;
  int channels = 4;
  PixelFormat pixel_format = epf::PixelFormat::RGBA8;
  ImageObject image("imagen 1", width, height, channels, pixel_format,
                    nullptr);  // stream node

  msg->addItem(image.copyNode());
  // msg->print();
  // getchar();

  // item 2 - Custom Object (manually defined with DataNodes and References)
  // ObjectNode *default_object = new ObjectNode("electrical signals");
  // DataNode *voltage = new DataNode("Current", EP_32U, {10},
  //                                  nullptr);  // value = nullptr (stream
  //                                  node)

  // StringNode *units_v = new StringNode("Eng. Units", "mV");
  // DataNode *current = new DataNode("Voltage", EP_32U, {1, 10},
  //                                  nullptr);  // value = nullptr (stream
  //                                  node)
  // StringNode *units_c = new StringNode("Eng. Units", "mA");

  auto default_object = std::make_unique<ObjectNode>("electrical signals");
  auto voltage =
      std::make_unique<DataNode>("Current", EP_32U, std::vector<size_t>{10},
                                 nullptr);  // value = nullptr (stream node)

  auto units_v = std::make_unique<StringNode>("Eng. Units", "mV");
  auto current =
      std::make_unique<DataNode>("Voltage", EP_32U, std::vector<size_t>{1, 10},
                                 nullptr);  // value = nullptr (stream node)
  auto units_c = std::make_unique<StringNode>("Eng. Units", "mA");

  voltage->addReference(epf::EP_HAS_PROPERTY, std::move(units_v));
  current->addReference(epf::EP_HAS_PROPERTY, std::move(units_c));
  default_object->addReference(epf::EP_HAS_CHILD, std::move(voltage));
  default_object->addReference(epf::EP_HAS_CHILD, std::move(current));

  // item 3 - Custom Object
  msg->addItem(std::move(default_object));

  // item 4 - Array
  float wavelengths[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  //  DataNode *wave_n = new DataNode("wavelengths", EP_32F, {1,10},
  //  static_cast<void*>(wavelengths), true); ¿?  errror initialization
  //  DataNode *wave_n = new DataNode("wavelengths", EP_32F, {1, 10});
  auto wave_n = std::make_unique<DataNode>("wavelengths", EP_32F,
                                           std::vector<size_t>{1, 10});
  wave_n->write(static_cast<void *>(wavelengths));
  msg->addItem(std::move(wave_n));

  // item 5 - Array of floats
  //  float mat[] = {0, 1, 2, 3};
  // DataNode *mat_n = new DataNode("mat", EP_32F, {2, 2});
  auto mat_n =
      std::make_unique<DataNode>("mat", EP_32F, std::vector<size_t>{2, 2});
  mat_n->write(static_cast<void *>(wavelengths));
  msg->addItem(std::move(mat_n));

  // // item 6 - Array of strings
  // // std::string color[] = {"red", "green", "blue", "pink"};
  // DataNode *color_n = new DataNode("color", EP_STRING, {2, 2});
  // msg->addItem(color_n);
  // // static_cast<std::string *>(color_n->value())[0] = "red";
  // // static_cast<std::string *>(color_n->value())[1] = "green";
  // // static_cast<std::string *>(color_n->value())[2] = "blue";
  // // static_cast<std::string *>(color_n->value())[3] = "pink";
  // std::string color[] = {"black", "orange", "green", "violet"};
  // color_n->write(static_cast<void *>(&color));

  // std::string *read_color_n = new std::string[color_n->arrayelements()];
  // color_n->read(read_color_n);
  // std::cout<<"color_n: ";
  // for (int i=0;i<color_n->arrayelements();i++)
  //   std::cout<<" "<<read_color_n[i];
  // std::cout<<std::endl;
  // delete[] read_color_n;

  // printf("Message model - end\n");
  // msg->print();
  // getchar();

  return msg;
}

int test_message_serialization()
{
  auto msg = create_message_model();

  // printf("messate_to_yaml?\n");
  // msg->print();
  // getchar();

  serialize_message(*msg, "file.yml");

  auto msg1 = deserialize_message(YAML::LoadFile("file.yml"));
  // printf("messate_from_yaml?\n");
  // msg1->print();
  // getchar();

  serialize_message(*msg1, "file1.yml");

  // comparision
  FILE *f1 = fopen("file.yml", "rb");
  FILE *f2 = fopen("file1.yml", "rb");

  if (f1 == nullptr || f2 == nullptr) {
    if (f1) fclose(f1);
    if (f2) fclose(f2);
    std::cout << "unable to open the files" << std::endl;
    return -1;  // Unable to open one of the files
  }

  // Seek to the end to get file sizes
  fseek(f1, 0, SEEK_END);
  fseek(f2, 0, SEEK_END);
  long size1 = ftell(f1);
  long size2 = ftell(f2);

  if (size1 != size2) {
    fclose(f1);
    fclose(f2);
    std::cout << "files have different sizes" << std::endl;
    return -1;  // Unable to open one of the files
  }

  // Return to the beginning of the files
  rewind(f1);
  rewind(f2);

  bool are_equal = true;
  char ch1, ch2;

  while (fread(&ch1, sizeof(char), 1, f1) && fread(&ch2, sizeof(char), 1, f2)) {
    if (ch1 != ch2) {
      are_equal = false;
      break;
    }
  }

  fclose(f1);
  fclose(f2);

  if (!are_equal) {
    std::cout << "files are not equal" << std::endl;
    return -1;
  }

  return 0;
}

int main()
{
  int err = 0;
  std::cout << "Test data node.. ";
  err = test_data_node();
  if (err == 0)
    std::cout << "passed" << std::endl;
  else
    std::cout << "failed" << std::endl;

  // std::cout << "Test string data node.. ";
  // err = test_string_data_node();
  // if (err == 0)
  //   std::cout << "passed" << std::endl;
  // else
  //   std::cout << "failed" << std::endl;

  std::cout << "Test command node.. ";
  err = test_command_node();
  if (err == 0)
    std::cout << "passed" << std::endl;
  else
    std::cout << "failed" << std::endl;

  std::cout << "Test object node.. ";
  err = test_object_node();
  if (err == 0)
    std::cout << "passed" << std::endl;
  else
    std::cout << "failed" << std::endl;

  std::cout << "Test message.. ";
  err = test_message();
  if (err == 0)
    std::cout << "passed" << std::endl;
  else
    std::cout << "failed" << std::endl;

  std::cout << "Test message serialization.. ";
  err = test_message_serialization();
  if (err == 0)
    std::cout << "passed" << std::endl;
  else
    std::cout << "failed" << std::endl;

  return 0;
}
