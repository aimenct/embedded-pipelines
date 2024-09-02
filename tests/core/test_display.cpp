// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include "../../examples/filters/filter_template.h"
// #include "../../examples/filters/image_src.h"
#include "core.h"

using namespace ep;
using namespace std;

#include <GL/glut.h>
//#include <tiffio.h>

#include <iostream>

// // utils
// #define EP_OPENCV

// /* OPENCV CONVERSION */
// #ifdef EP_OPENCV

// #include <iostream>
// #include <opencv2/opencv.hpp>

// // Function to convert PixelFormat to OpenCV format code;
// int pixelFormatToCVType(PixelFormat pixelFormat, unsigned int channels = 0);
// // Function to convert from OpenCV image type to PixelFormat
// PixelFormat CVTypeToPixelFormat(cv::Mat &image);
// // PixelFormat CVTypeToPixelFormat(int cv_type);

// /* OPENCV CONVERSION */
// // Function to convert PixelFormat to OpenCV format code;
// int pixelFormatToCVType(PixelFormat pixelFormat, unsigned int channels)
// {
//   switch (pixelFormat) {
//     case Mono8:
//       return CV_8U;
//     case Mono10:
//       return CV_16U;
//     case Mono12:
//       return CV_16U;
//     case Mono10p:
//       return CV_16U;
//     case Mono10g40:
//       return CV_16U;
//     case Mono12g24:
//       return CV_16U;
//       // raw bayer
//     case BayerRG8:
//       return CV_8U;
//     case BayerRG10:
//       return CV_16U;
//     case BayerRG10p:
//       return CV_16U;
//     case BayerRG12p:
//       return CV_16U;
//     case BayerRG10g40:
//       return CV_16U;
//     case BayerGR8:
//       return CV_8U;
//     case BayerGR10:
//       return CV_16U;
//     case BayerGR12:
//       return CV_16U;
//       // color
//     case RGB8:
//       return CV_8UC3;
//     case RGB10:
//       return CV_16UC3;
//     case RGB12:
//       return CV_16UC3;
//     case RGB10p32:
//       return CV_16UC3;
//     case BGR8:
//       return CV_8UC3;
//     case BGR10:
//       return CV_16UC3;
//     case BGR12:
//       return CV_16UC3;
//     case BGR10p32:
//       return CV_16UC3;
//     case RGBa8:
//       return CV_8UC4;
//     case RGBa12:
//       return CV_16UC4;
//     case BGRa8:
//       return CV_8UC4;
//     case BGRa10:
//       return CV_16UC4;
//     case BGRa12:
//       return CV_16UC4;
//     case Mosaic3u8:
//       return CV_8U;
//     case Mosaic4u8:
//       return CV_8U;
//     case Mosaic5u8:
//       return CV_8U;
//     case Mosaic3u10:
//       return CV_16U;
//     case Mosaic4u10:
//       return CV_16U;
//     case Mosaic5u10:
//       return CV_16U;
//     case Mosaic3u12:
//       return CV_16U;
//     case Mosaic4u12:
//       return CV_16U;
//     case Mosaic5u12:
//       return CV_16U;
//     case BIP8:
//       return CV_MAKETYPE(CV_8U, channels);
//     case BIP10:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BIP12:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BIP16:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BIP8s:
//       return CV_MAKETYPE(CV_8S, channels);
//     case BIP10s:
//       return CV_MAKETYPE(CV_16S, channels);
//     case BIP12s:
//       return CV_MAKETYPE(CV_16S, channels);
//     case BIP16s:
//       return CV_MAKETYPE(CV_16S, channels);
//     case BIP32f:
//       return CV_MAKETYPE(CV_32F, channels);
//     case BIP64f:
//       return CV_MAKETYPE(CV_64F, channels);
//     case BIL8:
//       return CV_MAKETYPE(CV_8U, channels);
//     case BIL10:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BIL12:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BIL16:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BSQ8:
//       return CV_MAKETYPE(CV_8U, channels);
//     case BSQ10:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BSQ12:
//       return CV_MAKETYPE(CV_16U, channels);
//     case BSQ16:
//       return CV_MAKETYPE(CV_16U, channels);
//     default:
//       std::cout << "Image Utils Error - conversion Pixelformat to OpenCV"
//                 << std::endl;
//       return -1;
//   };
// }

// PixelFormat CVTypeToPixelFormat(cv::Mat &image)
// {
//   // Extract number of channels and data type from cv_type
//   // int cv_type = image.type();
//   // //  int cv_type = image.type();
//   //  int channels = cv::Mat::channels(cv_type);
//   //  int data_type = cv::Mat::depth(cv_type);

//   PixelFormat pf = BIP8;

//   // nfo.bytesPerPixel = image.image_depth();
//   // nfo.numChannels = image.channels();
//   // nfo.size = image.size();

//   switch (image.depth()) {
//     case CV_8U:
//       // nfo.depth.push_back(8);
//       // nfo.baseType = EP_8U;
//       pf = BIP8;
//       break;
//     case CV_8S:
//       //      nfo.depth.push_back(8);
//       //      nfo.baseType = EP_8S;
//       pf = BIP8s;
//       break;
//     case CV_16U:
//       // nfo.depth.push_back(16);
//       // nfo.baseType = EP_16U;
//       pf = BIP16;
//       break;
//     case CV_16S:
//       // nfo.depth.push_back(16);
//       // nfo.baseType = EP_16S;
//       pf = BIP16s;
//       break;
//     case CV_32S:
//       // nfo.depth.push_back(32);
//       // nfo.baseType = EP_32S;
//       pf = BIP32s;
//       break;
//     case CV_32F:
//       // nfo.depth.push_back(32);
//       // nfo.baseType = EP_32F;
//       pf = BIP32f;
//       break;
//     case CV_64F:
//       // nfo.depth.push_back(64);
//       // nfo.baseType = EP_64F;
//       pf = BIP64f;
//       break;
//     default:
//       std::cout << "Image Utils Opencv Type Conversion Uknown" << std::endl;
//       break;
//   }

//   switch (image.type()) {
//     case CV_8UC1:
//       pf = Mono8;
//       break;
//     case CV_8UC3:
//       pf = RGB8;
//       break;
//     case CV_8UC4:
//       pf = RGBa8;
//       break;
//     case CV_16UC1:
//       pf = Mono16;
//       break;
//     case CV_16UC3:
//       pf = BIP16;
//       break;
//     case CV_16UC4:
//       pf = BIP16;
//       break;
//     default:
//       std::cout << "Uknown format " << std::endl;
//   }

//   return pf;
// }

// #endif  // EP_OPENCV

void test_src_display()
{
  // //  Define YAML data inline as a string
  // std::string config_src = R"(
  //         Filters:
  //         - name: Image1
  //           type: ImgSrc
  //           filter_settings:
  //             queue length: 10
  //             queue type: lifo
  //             timeout: 40000
  //             folder path: ../../../data/samples/tif1
  //   )";

  // std::string config_disp = R"(
  //         Filters:
  //         - name: Display
  //           type: GlDisplay
  //           filter_settings:
  //             timeout: 40000
  //             maximum images: 10
  //   )";

  // YAML::Node cfg_src = YAML::Load(config_src);
  // // YAML::Node config_node = YAML::LoadFile("config_imgsrc.yml");
  // YAML::Node cfg_disp = YAML::Load(config_disp);
  // // YAML::Node config_node = YAML::LoadFile("config_gldisplay.yml");

  // ImgSrc *src = new ImgSrc(cfg_src["Filters"][0]);
  // GlDisplay *disp = new GlDisplay();

  // int threads = 1;
  // Pipeline pipe(threads);

  // pipe.add(src);
  // pipe.add(disp);

  // pipe.connect(src, 0, disp, 0);

  // pipe.assignTask(0, src);
  // pipe.assignTask(0, disp);

  // pipe.run();

  // disp->mainLoop();

  // pipe.halt();

  return;
}

void test_postproc()
{
  // YAML::Node config;
  // ImgSrc *src = new ImgSrc(config);
  // GlDisplay *disp = new GlDisplay();
  // //  PostProc *post = new PostProc();

  // int threads = 1;
  // Pipeline pipe(threads);
  // // TODO  Pipeline pipe;

  // pipe.add(src);
  // //  pipe.add(post);
  // pipe.add(disp);

  // //  pipe.connect(src, 0, post, 0);
  // pipe.connect(src, 0, disp, 0);
  // // pipe.connect(post, 0, disp, 0);
  // //  pipe.connect(post,1,disp,2);

  // pipe.assignTask(0, src);
  // //  pipe.assignTask(0, post);

  // pipe.run();

  // // disp->loop.connect<&Filter2::job>(src);
  // // disp->loop.connect<&Filter2::job>(post);

  // disp->mainLoop();  // ¿in job?

  // pipe.halt();

  return;
}

void test_multiple_src()
{
  // //  YAML::Node config1, config2, config3;
  // std::string config1 = R"(
  //         Filters:
  //         - name: Image1
  //           type: ImgSrc
  //           filter_settings:
  //             queue length: 10
  //             queue type: lifo
  //             timeout: 40000
  //             folder path: ../../../data/samples/tif
  //   )";

  // std::string config2 = R"(
  //         Filters:
  //         - name: Image2
  //           type: ImgSrc
  //           filter_settings:
  //             queue length: 10
  //             queue type: lifo
  //             timeout: 40000
  //             folder path: ../../../data/samples/tif1
  //   )";

  // std::string config3 = R"(
  //         Filters:
  //         - name: Image3
  //           type: ImgSrc
  //           filter_settings:
  //             queue length: 10
  //             queue type: lifo
  //             timeout: 40000
  //             folder path: ../../../data/samples/tif1
  //   )";

  // YAML::Node cfg1 = YAML::Load(config1);
  // YAML::Node cfg2 = YAML::Load(config2);
  // YAML::Node cfg3 = YAML::Load(config3);

  // ImgSrc *src1 = new ImgSrc(cfg1["Filters"][0]);
  // ImgSrc *src2 = new ImgSrc(cfg2["Filters"][0]);
  // ImgSrc *src3 = new ImgSrc(cfg3["Filters"][0]);

  // const std::string *d = src1->settingValue<std::string>("folder path");
  // std::cout << "folder path read " << *d << endl;

  // d = src2->settingValue<std::string>("folder path");
  // std::cout << "folder path read " << *d << endl;

  // d = src3->settingValue<std::string>("folder path");
  // std::cout << "folder path read " << *d << endl;

  // GlDisplay *disp = new GlDisplay();
  // //  PostProc *post = new PostProc();

  // int threads = 1;
  // Pipeline pipe(threads);

  // pipe.add(disp);
  // pipe.add(src1);
  // pipe.add(src2);
  // pipe.add(src3);
  // //  pipe.add(post);

  // // pipe.connect(src, 0, post, 0);
  // //   pipe.connect(src,0,disp,0);
  // //   pipe.connect(post, 0, disp, 0);
  // //   pipe.connect(post,1,disp,2);
  // pipe.connect(src1, 0, disp, 0);
  // pipe.connect(src2, 0, disp, 1);
  // pipe.connect(src3, 0, disp, 2);

  // pipe.assignTask(0, src1);
  // pipe.assignTask(0, src2);
  // pipe.assignTask(0, src3);
  // //  pipe.assignTask(0, post);

  // pipe.run();

  // // disp->loop.connect<&Filter2::job>(src1);
  // // disp->loop.connect<&Filter2::job>(src2);
  // // disp->loop.connect<&Filter2::job>(src3);
  // // disp->loop.connect<&Filter2::job>(post);

  // disp->mainLoop();  // ¿in job?

  // pipe.halt();

  return;
}

void test_src_post_sink_display()
{
  // ImgSrc *src = new ImgSrc();
  // GlDisplay *disp = new GlDisplay();
  // PostProc *post = new PostProc();
  // Sink *sink = new Sink();

  // int threads = 2;
  // Pipeline pipe(threads);

  // pipe.add(src);
  // pipe.add(disp);
  // pipe.add(post);
  // pipe.add(sink);

  // pipe.connect(src,0,post,0);
  // pipe.connect(src,0,disp,0);
  // pipe.connect(post,0,disp,1);
  // pipe.connect(post,1,disp,2);
  // pipe.connect(src,0,sink,0);

  // pipe.assignTask(0,src);
  // pipe.assignTask(1,post);
  // pipe.assignTask(2,sink);
  // //  pipe.assignTask(0,disp);

  // // connect signals  : signature string "name [value]" -> slot string
  // "name [value]" pipe.connect(disp,"start_record",sink,"start");
  // pipe.connect(disp,"stop_record",sink,"stop");
  // pipe.connect(disp,"set_path",sink,"path");

  // pipe.run();

  // disp->mainLoop();

  // pipe.halt();

  return;
}

// #ifdef EP_OPENCV

int test_opencv()
{
  //   cv::Mat image1 = cv::imread("../../../data/samples/jpg/leon.jpg");
  //   cv::Mat image2 = cv::imread("../../../data/samples/jpg/paloma.jpg");
  //   //  cv::Mat image3 = cv::imread("../../../data/samples/tif/001.tiff");
  //   cv::Mat image3 =
  //       cv::imread("../../../data/samples/tif1/2022_11_10_13_08-0_12.tiff",
  //                  cv::IMREAD_ANYDEPTH);

  //   // Check if the image was loaded successfully
  //   if (image1.empty()) {
  //     std::cerr << "Failed to load image." << std::endl;
  //     return 1;
  //   }
  //   if (image2.empty()) {
  //     std::cerr << "Failed to load image." << std::endl;
  //     return 1;
  //   }
  //   if (image3.empty()) {
  //     std::cerr << "Failed to load image." << std::endl;
  //     return 1;
  //   }

  //   ImageObject img1("1", image1.cols, image1.rows, image1.channels(),
  //                    CVTypeToPixelFormat(image1),
  //                    image1.data);  // data = nullptr (data_ queue node)

  //   ImageObject img2("1", image2.cols, image2.rows, image2.channels(),
  //                    CVTypeToPixelFormat(image2),
  //                    image2.data);  // data = nullptr (data_ queue node)

  //   ImageObject img3("3", image3.cols, image3.rows, image3.channels(),
  //                    CVTypeToPixelFormat(image3),
  //                    image3.data);  // data = nullptr (data_ queue node)

  //   Message *msg1 = new Message();
  //   msg1->addItem(img1.copyNodeTree());

  //   //  Queue *q1 = new Queue();
  //   // Create a shared_ptr to manage the Queue
  //   std::shared_ptr<Queue> q1 = std::make_shared<Queue>();
  //   q1->init(10, msg1);

  //   QueueWriter writer1(q1);

  //   Message *msg2 = new Message();
  //   msg2->addItem(img2.copyNodeTree());

  //   //Queue *q2 = new Queue();
  //   std::shared_ptr<Queue> q2 = std::make_shared<Queue>();
  //   q2->init(10, msg2);

  //   QueueWriter writer2(q2);

  //   Message *msg3 = new Message();
  //   msg3->addItem(img3.copyNodeTree());

  //   //  Queue *q3 = new Queue();
  //   std::shared_ptr<Queue> q3 = std::make_shared<Queue>();
  //   q3->init(10, msg3);

  //   QueueWriter writer3(q3);

  //   GlDisplay disp;
  //   disp.open();
  //   disp.connectSourceQueue(0, q1);
  //   disp.connectSourceQueue(1, q2);
  //   disp.connectSourceQueue(2, q3);
  //   disp.set();
  //   disp.start();

  //   Node2 *n1 = writer1.dataSchema()->item(0);
  //   ImageObject img_obj1(n1);
  //   printf("Writer Message Model\n");
  //   writer1.dataSchema()->print();
  //   int err = writer1.startWrite();
  //   if (err >= 0) {
  //     //    img_obj->write(static_cast<void *>(&image1));
  //     printf("writer image\n");
  //     memcpy(img_obj1.data(), img1.data(), img_obj1.size());
  //     printf("writer image end\n");
  //   }
  //   writer1.endWrite();

  //   Node2 *n2 = writer2.dataSchema()->item(0);
  //   ImageObject img_obj2(n2);
  //   printf("Writer Message Model\n");
  //   writer2.dataSchema()->print();
  //   err = writer2.startWrite();
  //   if (err >= 0) {
  //     //    img_obj->write(static_cast<void *>(&image1));
  //     printf("writer image\n");
  //     memcpy(img_obj2.data(), img2.data(), img_obj2.size());
  //     printf("writer image end\n");
  //   }
  //   writer2.endWrite();

  //   Node2 *n3 = writer3.dataSchema()->item(0);
  //   ImageObject img_obj3(n3);
  //   printf("Writer Message Model\n");
  //   writer3.dataMsg(0).print();
  //   err = writer3.startWrite();
  //   if (err >= 0) {
  //     //    img_obj->write(static_cast<void *>(&image1));
  //     printf("writer image\n");
  //     memcpy(img_obj3.data(), img3.data(), img_obj3.size());
  //     printf("writer image end\n");
  //   }
  //   writer3.endWrite();

  //   disp.doJob(&disp);
  //   disp.mainLoop();

  //   // printf("leon: %d\n", pf);
  //   // getchar();

  //   // Display the image using OpenCV
  //   // cv::namedWindow("JPEG Image", cv::WINDOW_NORMAL);
  //   // cv::imshow("JPEG Image", image);
  //   // cv::waitKey(0);

  return 0;
}

//#endif

int main()
{
  //  test_src_display();
  //  test_postproc();
  // #ifdef EP_OPENCV
  //   test_opencv();
  // #endif
  test_multiple_src();

  return 0;
}
