// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "image_src.h"

using namespace ep;
using namespace std;

ImgSrc::ImgSrc(const YAML::Node &config)
    : Filter()
{
  std::cout << "ImgSrc constructor " << std::endl;
  yaml_config_ = config;

  max_sinks_ = 1;
  image_index_ = 0;
  //  file_ = nullptr;
  img_obj_ = nullptr;
  folder_path_ = "my_path";
  name_ = "my_name";
  timeout_ = 30000;
  queue_length_ = 10;
  queue_type_ = lifo;
  loop_ = 'y';
  name_ = "my_name_";

  if (config["name"]) name_ = config["name"].as<std::string>();

  // settings to be replaced !!
  //  name_ =  name;
  addSetting("timeout", timeout_);
  addSetting("queue length", queue_length_);
  //  addSetting("queue type", queue_type_);
  addSetting("folder path", folder_path_);
  addSetting("loop", loop_);

  YAML::Node yaml_settings = config;
  settings_.fromYAML(yaml_settings);

  settings_.print();
}

ImgSrc::~ImgSrc()
{
  cout << "ImgSrc destructor" << endl;
}

int ImgSrc::_set()
{
  std::cout << "ImgSrc: set()" << std::endl;

  struct dirent **name_list;
  int nn, i;
  nn = scandir(folder_path_.c_str(), &name_list, 0, versionsort);

  if (nn >= 0) {
    printf("ImgSrc: set() number of files: %d\n", nn);
    std::cout << "ImgSrc set(): some of the files in the directory: "
              << std::endl;
    for (i = 0; (i < 10 && i < nn); ++i) {
      printf("%s\n", name_list[i]->d_name);
    }
    //    std::cout << "Number of files in dir: " << nn << std::endl;
  }
  else {
    std::cout << "ImgSrc: set() folder [" << folder_path_
              << "] not found or empty" << std::endl;
    return -1;
  }

  files_.resize(nn);
  int j = 0;
  for (i = 0; i < nn; ++i) {
    const char *dot = strrchr(name_list[i]->d_name, '.');
    if (((dot != NULL) && (strncmp(dot + 1, "tiff", 5) == 0)) ||
        ((dot != NULL) && (strncmp(dot + 1, "tif", 5) == 0))) {
      files_[j] = name_list[i]->d_name;
      files_[j] = folder_path_ + '/' + files_[j];
      j++;
    }
  }
  files_.resize(j);

  /* 1- Create Message Model */
  uint32_t w, h;
  size_t npixels;
  //  uint32_t *raster;

  // Print the names of files
  // std::cout << "Files in folder:" << std::endl;
  // for (const auto& file : files_) {
  //   std::cout << file << std::endl;
  // }

  // Open the first TIFF image file
  if (!files_.empty()) {
    TIFF *tiff = TIFFOpen(files_[0].c_str(), "r");
    if (tiff != nullptr) {
      // Display the image (you may need to process the image data)
      std::cout << "ImgSrc: set() Successfully opened image: " << files_[0]
                << std::endl;
      TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
      TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
      npixels = w * h;
      std::cout << "w: " << w << " h: " << h << " npixels " << npixels
                << std::endl;
      TIFFClose(tiff);
    }
    else {
      std::cerr << "ImgSrc: set() Failed to open image: " << files_[0]
                << std::endl;
    }
  }
  else {
    std::cerr << "ImgSrc: set() No TIFF image files found in the folder."
              << std::endl;
  }

  // item 1 - Image Object
  ImageObject image("imagen 1", w, h, 4, RGBa8,
                    nullptr);  // data = nullptr (data_ queue node)

  // create message model for sink queue
  Message *msg = new Message();
  msg->addItem(image.copyNodeTree());
  printf("ImgSrc: set() Writer Message Model \n");
  msg->print();

  /* 2-  instantiate sink queue */
  int err = addSinkQueue(0,msg);

  /* 3- Get the pointer of the ImageObject in the Msg to be used in job */
  // get image node - item 1
  Node2 *n = writer(0)->dataSchema()->item(0);
  img_obj_ = new ImageObject(n);

  return err;
}

int32_t ImgSrc::_job()
{
  // Attempt to start writing using the first writer
  int err = writer(0)->startWrite();
  if (err < 0) {
    usleep(timeout_);
    return err;
  }

  // Open the next file
  file_ = files_[image_index_++];
  if (image_index_ >= files_.size()) {
    image_index_ = 0;
  }

  TIFF *tif = TIFFOpen(file_.c_str(), "r");
  if (!tif) {
    std::cerr << "ImgSrc: Failed to open TIFF file: " << file_ << std::endl;
    writer(0)->endWriteAbort();
    usleep(timeout_);
    return -1;  // Indicate failure to process this job
  }

  uint32_t width = 0, height = 0;
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  size_t npixels = static_cast<size_t>(width) * height;

  if (npixels * sizeof(uint32_t) != img_obj_->size()) {
    std::cerr << "ImgSrc: job open file size error" << std::endl;
    std::cerr << "ImgSrc: img size " << npixels * sizeof(uint32_t) << std::endl;
    std::cerr << "ImgSrc: img size " << img_obj_->size() << std::endl;
    TIFFClose(tif);
    writer(0)->endWriteAbort();
    usleep(timeout_);
    return -1;  // Indicate failure due to size mismatch
  }

  // Read image data into the buffer
  if (!TIFFReadRGBAImage(tif, width, height,
                         reinterpret_cast<unsigned int *>(img_obj_->data()),
                         0)) {
    std::cerr << "ImgSrc: Failed to read RGBA image data from TIFF file: "
              << file_ << std::endl;
    TIFFClose(tif);
    writer(0)->endWriteAbort();
    usleep(timeout_);
    return -1;  // Indicate failure to read image data
  }

  TIFFClose(tif);
  writer(0)->endWrite();
  usleep(timeout_);
  return 0;  // Success
}

// int ImgSrc::job()
// {
//   // writer_ job() function
//   int err = writers_[0]->startWrite();
//   if (err >= 0) {
//     // open next file
//     file_ = files_[image_index_++];
//     if (image_index_ >= files_.size()) image_index_ = 0;

//     TIFF *tif = TIFFOpen(file_.c_str(), "r");
//     if (tif) {
//       uint32_t w, h;
//       size_t npixels;
//       //        uint32_t *raster;

//       TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
//       TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
//       npixels = w * h;
//       if (npixels * sizeof(uint32) != img_obj_->size()) {
//         cout << "ImgSrc: job open file size error" << endl;
//         cout << "ImgSrc: img size " << npixels * sizeof(uint32) << endl;
//         cout << "ImgSrc: img size " << img_obj_->size() << endl;
//       }
//       else
//         // write image data in the queue
//         if (TIFFReadRGBAImage(tif, w, h, (unsigned int *)img_obj_->data(),
//         0)) {
//         }
//     }
//     TIFFClose(tif);

//     writers_[0]->endWrite();
//   }
//   usleep(timeout_);
//   return err;
// }

int ImgSrc::_open()
{
  return 0;
}
int ImgSrc::_close()
{
  return 0;
}

int ImgSrc::_start()
{
  return 0;
}

int ImgSrc::_stop()
{
  return 0;
}

int ImgSrc::_reset()
{
  if (img_obj_ != nullptr) delete img_obj_;
  img_obj_ = nullptr;

  return 0;
}
