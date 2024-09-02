// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "arv_cam.h"

void *gs_thread_func(void *data)
{
  ep::ArvCam *ptr = (ep::ArvCam *)data;
  GMainContext *context = g_main_context_new();

  ptr->setMainLoop(g_main_loop_new(context, FALSE));
  g_main_loop_run(ptr->mainLoop());
  g_main_context_unref(context);
  return NULL;
}

static void control_lost_cb(ArvGvDevice * /*gv_device*/)
{
  /* Control of the device is lost. Display a message and force application exit
   */
  printf("Control lost\n");
}

static void stream_cb(void *user_data, ArvStreamCallbackType type,
                      ArvBuffer * /*buffer*/)
{
  ep::ArvCam *ptr = (ep::ArvCam *)user_data;
  if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
    // getRealtime() todo
    //    if (ptr->realtime) {
    if (ptr->isRealTime()) {
      if (!arv_make_thread_realtime(10))
        printf("Failed to make stream thread realtime\n");
      //    } else if (ptr->high_priority) {
    }
    else if (ptr->isHighPriority()) {
      // getHighPriority() todo
      if (!arv_make_thread_high_priority(-10))
        printf("Failed to make stream thread high priority\n");
    }
  }
}

static void new_buffer_cb(ArvStream *stream, ep::ArvCam *cam)
{
  ArvBufferPayloadType type;
  size_t size;
  const void *sptr;

  ArvBuffer *buffer;

  ep::QueueWriter *writer = cam->writer(0);
  ep::Node2 *item_node = writer->dataSchema()->item(0);
  ep::ImageObject image(item_node);
  std::size_t data_size = image.size();

  buffer = arv_stream_try_pop_buffer(stream);
  if (buffer != NULL) {
    if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
      type = arv_buffer_get_payload_type(buffer);

      if (type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE) {
        int32_t ret = writer->startWrite();
        if (ret >= 0) {
          sptr = arv_buffer_get_data(buffer, &size);

          assert(size == data_size);
          memcpy(image.data(), sptr, data_size);
          writer->endWrite();
        }
        else {
	  //          printf("new buffer cb - error\n");
        }
      }
      else {
	//        printf("new buffer cb - not image type\n");
        int32_t ret = writer->startWrite();
        if (ret >= 0) {
          sptr = (char *)arv_buffer_get_data(buffer, &size);
          assert(size == data_size);
          memcpy(image.data(), sptr, data_size);
          writer->endWrite();
        }
      }
    }
    arv_stream_push_buffer(stream, buffer);
  }
}

namespace ep {
ArvCam::ArvCam(const YAML::Node &config)
    : Filter()
{
  yaml_config_ = config;

  name_ = "myCam";
  if (config["name"]) name_ = config["name"].as<std::string>();
  type_ = "ArvCam";

  job_execution_model_ = OWN_THREAD;

  max_sinks_ = 1;
  max_sources_ = 0;

  camera_ = nullptr;
  stream_ = nullptr;

  out_bands_ = 1;
  arv_num_buffers_ = 20;
  frame_rate_ = -1;
  stream_channel_ = -1;
  packet_delay_ = -1;
  packet_size_ = 1500;
  socket_buffer_size_ = -1;
  packet_timeout_ = 20;
  frame_retention_ = 100;
  bandwidth_limit_ = -1;
  packet_request_ratio_ = -1.0;
  auto_packet_size_ = FALSE;
  auto_socket_buffer_ = TRUE;
  no_packet_resend_ = TRUE;
  realtime_ = TRUE;
  high_priority_ = TRUE;
  no_packet_socket_ = TRUE;
  debug_domains_ = NULL;
  camera_name_ = "";

  addSetting("output bands", out_bands_);
  addSetting("arv_num_buffers", arv_num_buffers_);
  addSetting("mtu", packet_size_);
  addSetting("mac", camera_name_);
  addSetting("packet_timeout", packet_timeout_);

  readSettings(yaml_config_);
}

int32_t ArvCam::addDeviceSettingsFromYAML(const YAML::Node &config)
{
  YAML::Node yaml_node = config["device_settings"];
  if (yaml_node) {
    for (YAML::const_iterator it = yaml_node.begin(); it != yaml_node.end();
         ++it) {
      std::string key = it->first.as<std::string>();
      std::string value;

      if (yaml_node[key].Type() == YAML::NodeType::Scalar) {
        value = it->second.as<std::string>();
        if (!addDeviceSetting(key.c_str())) {
          setSettingValue(key, value);
        };
      }
    }
  }

  return 0;
}

ArvCam::~ArvCam()
{
  std::cout << "ArvCam destructor " << std::endl;
}

int ArvCam::findDevices()
{
  arv_update_device_list();
  int n_devices = arv_get_n_devices();

  printf("Number of found Devices: %d\n", n_devices);
  for (int i = 0; i < n_devices; i++) {
    /* print device info */
    printf("-------------------------------\n");
    printf("DeviceID: %s\n", arv_get_device_id(i));
    printf("DevicePhyID: %s\n", arv_get_device_physical_id(i));
    printf("DeviceModel: %s\n", arv_get_device_model(i));
    printf("SerialNumber: %s\n", arv_get_device_serial_nbr(i));
    printf("DeviceVendor: %s\n", arv_get_device_vendor(i));
    printf("DeviceAddress: %s\n", arv_get_device_address(i));
    printf("DeviceProtocol: %s\n", arv_get_device_protocol(i));
    printf("-------------------------------\n");
    /* end print device info */
  }
  return 0;
}

int32_t ArvCam::_job()
{
  //   usleep(1000);
  return -1;
}

int32_t ArvCam::addDeviceSetting(const char *setting_name)
{
  GError *error = NULL;

  ArvGcNode *gc_node = arv_gc_get_node(genicam_, setting_name);

  // Check if the node is a GeniCam feature node
  if ((gc_node != NULL) && (ARV_IS_GC_FEATURE_NODE(gc_node))) {
    // Retrieve feature datatype
    ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE(gc_node);
    std::string tooltip =
        std::string(arv_gc_feature_node_get_tooltip(feature_node));

    if (ARV_IS_GC_INTEGER(feature_node)) {
      if (ARV_IS_GC_ENUMERATION(feature_node)) {
        std::string value = std::string(arv_gc_enumeration_get_string_value(
            ARV_GC_ENUMERATION(gc_node), &error));
        if (error) {
          printf("ERROR! Aravis response: %s\n", error->message);
        }
        else {
          // std::cout << setting_name << ", " << value << std::endl;
          // getchar();
          addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip);
          return 0;
        }
      }
      else {
        int64_t value =
            arv_gc_integer_get_value(ARV_GC_INTEGER(gc_node), &error);

        if (error) {
          printf("ERROR! Aravis response: %s\n", error->message);
        }
        else {
          // std::cout << setting_name << value << std::endl;
          // getchar();
          addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip);
          return 0;
        }
      }
    }
    else if (ARV_IS_GC_FLOAT(ARV_GC_FEATURE_NODE(gc_node))) {
      double value = arv_gc_float_get_value(ARV_GC_FLOAT(gc_node), &error);
      if (error) {
        printf("ERROR! Aravis response: %s\n", error->message);
      }
      else {
        // std::cout << setting_name << value << std::endl;
        // getchar();
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip);
        return 0;
      }
    }
    else if (ARV_IS_GC_STRING(ARV_GC_FEATURE_NODE(gc_node))) {
      std::string value =
          std::string(arv_gc_string_get_value(ARV_GC_STRING(gc_node), &error));

      if (error) {
        printf("ERROR! Aravis response: %s\n", error->message);
      }
      else {
        // std::cout << setting_name << value << std::endl;
        // getchar();
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip);
        return 0;
      }
    }
    else if (ARV_IS_GC_BOOLEAN(ARV_GC_FEATURE_NODE(gc_node))) {
      int64_t value = arv_gc_boolean_get_value(ARV_GC_BOOLEAN(gc_node), &error);
      if (error) {
        printf("ERROR! Aravis response: %s\n", error->message);
      }
      else {
        // std::cout << setting_name << value << std::endl;
        // getchar();
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip);
        return 0;
      }
    }
    else {
      printf("%s: GC TYPE not found\n", setting_name);
    }
  }

  printf("Warning! Node: \"%s\" not found!\n", setting_name);
  return -1;
}

int32_t ArvCam::_open()
{
  GError *gerror = NULL;

  //  if (debug_domains!=NULL) arv_debug_enable(debug_domains);

  if (!camera_name_.empty()) {
    /* find camera by serial number or mac address */
    int32_t device_found = -1;
    arv_update_device_list();

    int32_t n_devices = arv_get_n_devices();
    for (int i = 0; i < n_devices; i++) {
      if (strcmp(camera_name_.c_str(), arv_get_device_physical_id(i)) == 0 ||
          (strcmp(camera_name_.c_str(), arv_get_device_serial_nbr(i))) == 0) {
        printf("Camera id: %s\n", arv_get_device_id(i));
        device_found = i;
        break;
      }
    }
    if (device_found == -1) return -1;
    /* Instantiation of camera by id */
    camera_ = arv_camera_new(arv_get_device_id(device_found), NULL);
  }
  else {
    /* Instantiation of the first available camera */
    camera_ = arv_camera_new(NULL, &gerror);
    if (gerror) {
      printf("ERROR! Problem finding any camera.\n Aravis: %s",
             gerror->message);
      return -1;
    }
  }

  if (ARV_IS_CAMERA(camera_)) {
    std::cout << "Camera found!" << std::endl;

    this->genicam_ = arv_device_get_genicam(arv_camera_get_device(camera_));
    g_assert(ARV_IS_GC(genicam_));

    addDeviceSettingsFromYAML(yaml_config_);

    /* set camera parameters */
    if (arv_camera_is_gv_device(camera_)) {
      arv_camera_gv_select_stream_channel(camera_, stream_channel_, NULL);
      arv_camera_gv_set_packet_delay(camera_, packet_delay_, NULL);
      arv_camera_gv_set_packet_size(camera_, packet_size_, NULL);
      arv_camera_gv_set_stream_options(
          camera_, no_packet_socket_
                       ? ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED
                       : ARV_GV_STREAM_OPTION_NONE);
    }
    // if (src->arv_option_chunks[0]!='\0')
    // arv_camera_set_chunk_mode(src->camera, src->arv_option_chunks);
    if (auto_packet_size_) {
      arv_camera_gv_auto_packet_size(camera_, NULL);
    }
    if (frame_rate_) {
      arv_camera_set_frame_rate(camera_, frame_rate_, NULL);
    }

    /* Connect the control-lost signal */
    g_signal_connect(arv_camera_get_device(camera_), "control-lost",
                     G_CALLBACK(control_lost_cb), NULL);
    return 0;
  }
  else {
    printf("No camera found\n");
  }
  return -1;
}

int32_t ArvCam::_close()
{
  g_object_unref(camera_);
  return 0;
}

int32_t ArvCam::_set()
{
  GError *gerror = NULL;

  int32_t payload = 0;
  int32_t width;
  int32_t height;
  int32_t x, y;
  ArvPixelFormat arv_pixel_format;
  // const char *pixel_format_str;

  /* Create a new stream object */
  stream_ = arv_camera_create_stream(camera_, stream_cb, this, &gerror);
  if (stream_ == NULL) {
    printf(
        "Can't create stream thread (check if the device is not already "
        "used)\n");
    return -1;
  }

  /* set stream parameters */
  if (ARV_IS_GV_STREAM(stream_)) {
    if (auto_socket_buffer_) {
      g_object_set(stream_, "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
                   "socket-buffer-size", 0, NULL);
    }
    if (socket_buffer_size_ > 0) {
      g_object_set(stream_, "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
                   "socket-buffer-size", (unsigned)socket_buffer_size_, NULL);
    }
    if (no_packet_resend_) {
      g_object_set(stream_, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER,
                   NULL);
    }
    if (packet_request_ratio_ >= 0.0) {
      g_object_set(stream_, "packet-request-ratio", packet_request_ratio_,
                   NULL);
    }
    g_object_set(stream_, "packet-timeout", (unsigned)packet_timeout_ * 1000,
                 "frame-retention", (unsigned)frame_retention_ * 1000, NULL);

    // std::cout << "g_object_set(stream_, packet-timeout, "
    //              "(unsigned)packet_timeout_ * 1000,"
    //           << std::endl;
  }

  /* get stream info */
  arv_camera_get_region(camera_, &x, &y, &width, &height, NULL);
  // pixel_format_str = arv_camera_get_pixel_format_as_string(camera, NULL);
  arv_pixel_format = arv_camera_get_pixel_format(camera_, NULL);
  payload = width * height *
            static_cast<int32_t>(
                ceil(ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_pixel_format) / 8));

  /* Push num_buffers buffer in the aravis stream input buffer queue */
  for (int i = 0; i < arv_num_buffers_; i++) {
    arv_stream_push_buffer(stream_, arv_buffer_new(payload, NULL));
  }

  PixelFormat pixel_format;

  switch (arv_pixel_format) {
    case ARV_PIXEL_FORMAT_MONO_8:
      pixel_format = ep::Mono8;
      out_bands_ = 1;
      break;
    case ARV_PIXEL_FORMAT_MONO_10:
      pixel_format = ep::Mono10;
      out_bands_ = 1;
      break;
    case ARV_PIXEL_FORMAT_MONO_12:
      pixel_format = ep::Mono12;
      out_bands_ = 1;
      break;
    case ARV_PIXEL_FORMAT_MONO_16:
      pixel_format = ep::Mono16;
      out_bands_ = 1;
      break;
    case ARV_PIXEL_FORMAT_RGB_8_PACKED:
      pixel_format = ep::RGB8;
      out_bands_ = 3;
      break;
    case ARV_PIXEL_FORMAT_BGR_8_PACKED:
      pixel_format = ep::BGR8;
      out_bands_ = 1;
      break;
    default:
      printf("ArvCam not recognised pixel format\n");
      pixel_format = ep::RGB8;
      out_bands_ = 3;
      break;
  }

  ImageObject image =
      ImageObject("image", width, height, out_bands_, pixel_format, nullptr);

  /* Configure sink queues */
  // - set schemas
  Message *message = new Message();
  message->addItem(image.copyNodeTree());
  // printf("Writer Message Model \n");
  // message->print();

  addSinkQueue(0, message);

  /* 3- Get the pointer of the ImageObject in the Msg to be used in job */
  // get image node - item 1
  Node2 *item_node = writer(0)->dataSchema()->item(0);
  img_info_ = ImageObject(item_node);

  return 0;
}

int32_t ArvCam::_reset()
{
  g_object_unref(stream_);
  return 0;
}

int32_t ArvCam::_start()
{
  GError *gerror = NULL;
  /* custom */
  // if (push_mode_ == true) {
  //    std::cout << "push_mode --------------------------------" << std::endl;
  /* start thread */
  thread_ = g_thread_new("streaming-thread", gs_thread_func, this);
  /* Connect the new-buffer signal */
  g_signal_connect(stream_, "new-buffer", G_CALLBACK(new_buffer_cb), this);
  /* connect signals*/
  arv_stream_set_emit_signals(stream_, TRUE);
  // }

  //  printf("aravis_device - sending acquisition start command to the device\n");
  arv_camera_start_acquisition(camera_, &gerror);
  return 0;
}

int32_t ArvCam::_stop()
{
  GError *gerror = NULL;

  arv_camera_stop_acquisition(camera_, &gerror);

  // // only in push mode ??
  // if (push_mode_ == 'y') {
  //   for (std::size_t i = 0; i < sink_queues_.size(); i++) {
  //     sink_queues_[i]->setReadBlocking(sink_id_[i], 0);
  //   }
  //   for (std::size_t i = 0; i < src_queues_.size(); i++) {
  //     src_queues_[i]->setWriteBlocking(src_id_[i], 0);
  //   }

  arv_stream_set_emit_signals(stream_, FALSE);
  g_main_loop_quit(main_loop_);
  g_thread_join(thread_);
  //    g_thread_unref(thread);
  g_main_loop_unref(main_loop_);

  //   for (std::size_t i = 0; i < sink_queues_.size(); i++) {
  //     sink_queues_[i]->setReadBlocking(sink_id_[i], 1);
  //   }
  //   for (std::size_t i = 0; i < src_queues_.size(); i++) {
  //     src_queues_[i]->setWriteBlocking(src_id_[i], 1);
  //   }
  // }

  return 0;
}

// int32_t ArvCam::startReadCallback(void **data, void ** /*hdr*/, void
// **usr_data)
// {
//   if (status_ != RUNNING_STATE) {
//     std::cout << "DataSrcTemp: exit status!=runnnig" << std::endl;

//     pthread_mutex_lock(&status_mtx_);
//     status_ = SET_STATE;  // TODO: ----- _stop(), _close()  ¿?¿?
//     pthread_mutex_unlock(&status_mtx_);
//     return QE_DISABLED;
//   }

//   ArvBuffer *buffer;
//   ArvBufferPayloadType type;
//   size_t size;

//   buffer = arv_stream_try_pop_buffer(stream_);
//   if (buffer != NULL) {
//     if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
//       type = arv_buffer_get_payload_type(buffer);
//       if (type == ARV_BUFFER_PAYLOAD_TYPE_IMAGE) {
//         *data = (char *)arv_buffer_get_data(buffer, &size);
//         //	assert(size==data_size);
//         *usr_data = (void *)buffer;
//         return 0;
//       }
//     }
//     arv_stream_push_buffer(stream_, buffer);
//   }
//   *usr_data = NULL;
//   return -1;
// }

// int ArvCam::endReadCallback(void *usr_data)
// {
//   arv_stream_push_buffer(stream_, (ArvBuffer *)usr_data);
//   return 0;
// }

gboolean ArvCam::isHighPriority()
{
  return high_priority_;
}

gboolean ArvCam::isRealTime()
{
  return realtime_;
}

GMainLoop *ArvCam::mainLoop()
{
  return main_loop_;
}

void ArvCam::setMainLoop(GMainLoop *main_loop)
{
  main_loop_ = main_loop;
}

ArvStream *ArvCam::stream()
{
  return stream_;
}

int32_t ArvCam::setDeviceSettingValue(const char *key, const void *value)
{
  GError *error = NULL;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);

  int32_t align_name = 25;
  int32_t align_type = 15;
  int32_t align_value = 15;

  std::cout << std::setw(align_name) << std::left << key << " ";

  if ((node != NULL) && (ARV_IS_GC_FEATURE_NODE(node))) {
    ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE(node);
    // Check writability disablers before writting (missing by aravis)
    if (arv_gc_feature_node_is_available(feature_node, &error) &&
        !arv_gc_feature_node_is_locked(feature_node, &error)) {
      if (ARV_IS_GC_ENUMERATION(feature_node)) {
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value)
                  << static_cast<const std::string *>(value)->c_str();
        arv_gc_enumeration_set_string_value(
            ARV_GC_ENUMERATION(node),
            static_cast<const std::string *>(value)->c_str(), &error);
      }
      else if (ARV_IS_GC_INTEGER(feature_node)) {
        gint64 typed_value = *static_cast<const gint64 *>(value);
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << typed_value;
        arv_gc_integer_set_value(ARV_GC_INTEGER(node), typed_value, &error);
      }
      else if (ARV_IS_GC_FLOAT(feature_node)) {
        double typed_value = *static_cast<const double *>(value);
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << typed_value;
        arv_gc_float_set_value(ARV_GC_FLOAT(node), typed_value, &error);
      }
      else if (ARV_IS_GC_STRING(feature_node)) {
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << (char *)value;
        arv_gc_string_set_value(ARV_GC_STRING(node), (char *)value, &error);
      }
      else if (ARV_IS_GC_BOOLEAN(feature_node)) {
        int32_t typed_value = *static_cast<const int32_t *>(value);
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << typed_value;
        arv_gc_boolean_set_value(ARV_GC_BOOLEAN(node), *(int *)value, &error);
      }
      else {
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << (char *)value;
        arv_gc_feature_node_set_value_from_string(ARV_GC_FEATURE_NODE(node),
                                                  (char *)value, &error);
      }
    }
    else {
      std::cout << "[FAILED]" << std::endl;
      std::cout << "ERROR in ArvCam::setDeviceSettingValue." << std::endl
                << "Node not writable" << std::endl;
      return -1;
    }
  }
  else {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValue." << std::endl
              << "Node isn't a feature or doesn't exist" << std::endl;
    return -1;
  }

  if (error != NULL) {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValue." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }
  std::cout << "[SUCCESS]" << std::endl;
  return 0;
}

int32_t ArvCam::setDeviceSettingValueStr(const char *key, const char *value)
{
  GError *error = NULL;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);
  int32_t align_name = 25;
  int32_t align_type = 15;
  int32_t align_value = 15;

  std::cout << std::setw(align_name) << std::left << key << " ";

  if ((node != NULL) && (ARV_IS_GC_FEATURE_NODE(node))) {
    ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE(node);
    std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
              << std::setw(align_value) << value;

    // Check writability disablers before writting (missing by aravis)
    if (arv_gc_feature_node_is_available(ARV_GC_FEATURE_NODE(node), &error) &&
        !arv_gc_feature_node_is_locked(ARV_GC_FEATURE_NODE(node), &error)) {
      arv_gc_feature_node_set_value_from_string(ARV_GC_FEATURE_NODE(node),
                                                value, &error);
    }
    else {
      std::cout << "[FAILED]" << std::endl;
      std::cout << "ERROR in ArvCam::setDeviceSettingValueStr." << std::endl
                << "Node not writable" << std::endl;
      return -1;
    }
  }
  else {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValueStr." << std::endl
              << "Node isn't a feature or doesn't exist" << std::endl;
    return -1;
  }

  if (error != NULL) {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValueStr." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }

  std::cout << "[SUCCESS]" << std::endl;
  return 0;
}

int32_t ArvCam::deviceSettingValue(const char *key, void *value)
{
  GError *error = NULL;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);

  if ((node != NULL) && (ARV_IS_GC_FEATURE_NODE(node))) {
    ArvGcFeatureNode *feature_node = ARV_GC_FEATURE_NODE(node);
    if (ARV_IS_GC_ENUMERATION(feature_node)) {
      // strncpy(
      //     (char *)value,
      //     arv_gc_enumeration_get_string_value(ARV_GC_ENUMERATION(node),
      //     &error), 256);
      *static_cast<std::string *>(value) =
          arv_gc_enumeration_get_string_value(ARV_GC_ENUMERATION(node), &error);
    }
    else if (ARV_IS_GC_INTEGER(feature_node)) {
      *(int64_t *)value =
          arv_gc_integer_get_value(ARV_GC_INTEGER(node), &error);
    }
    else if (ARV_IS_GC_FLOAT(feature_node)) {
      *(double *)value = arv_gc_float_get_value(ARV_GC_FLOAT(node), &error);
    }
    else if (ARV_IS_GC_BOOLEAN(feature_node)) {
      *(int32_t *)value =
          arv_gc_boolean_get_value(ARV_GC_BOOLEAN(node), &error);
    }
    else if (ARV_IS_GC_STRING(feature_node)) {
      strncpy((char *)value,
              arv_gc_string_get_value(ARV_GC_STRING(node), &error), 256);
    }
    else {
      strncpy((char *)value,
              arv_gc_feature_node_get_value_as_string(ARV_GC_FEATURE_NODE(node),
                                                      &error),
              256);
    }
  }
  else {
    std::cout << "ERROR in ArvCam::deviceSettingValue." << std::endl
              << "Node isn't a feature or doesn't exist" << std::endl;
    return -1;
  }

  if (error != NULL) {
    std::cout << "ERROR in ArvCam::deviceSettingValue." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }
  return 0;
}

void process_node(ArvDomNode *a_node)
{
  ArvDomNode *cur_node = a_node;
  ArvDomNode *cur_node_child = NULL;
  //  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
  //  printf("parse_camera_xml 1\n");
  while (cur_node) {
    //    printf("parse_camera_xml 2\n");
    if ((ARV_IS_GC_FEATURE_NODE((ArvGcFeatureNode *)cur_node)) &&
        ((ARV_IS_GC_CATEGORY((ArvGcFeatureNode *)cur_node)))) {
      //	printf("FeatureName\n");
      // const char *szDomName = arv_dom_node_get_node_name(cur_node);
      // const char *name =
      //     arv_gc_feature_node_get_name((ArvGcFeatureNode *)cur_node);
      //      printf("%s\n", name);
      // const char *szFeatureValue =
      // arv_gc_feature_node_get_value_as_string((ArvGcFeatureNode *)cur_node,
      // NULL); if (szFeature && szFeatureValue && szFeatureValue[0])
      //   printf("FeatureName: %s%s, %s=%s",szDomName, szFeature,
      //   szFeatureValue);
    }

    cur_node_child = arv_dom_node_get_first_child(cur_node);
    process_node(cur_node_child);
    //    printf("parse_camera_xml 3\n");
    cur_node = arv_dom_node_get_next_sibling(cur_node);
  }
}

void ArvCam::parseCameraXml()
{
  //  ArvDomNode *node  = (ArvDomNode *)arv_gc_get_node(genicam,"Root");

  //  printf("parse_camera_xml\n");
  process_node((ArvDomNode *)genicam_);
}

std::string ArvCam::nameArvFeatureType(ArvGcFeatureNode *feature_node)
{
  GError *error = NULL;
  if (arv_gc_feature_node_is_available(feature_node, &error) &&
      !arv_gc_feature_node_is_locked(feature_node, &error)) {
    if (ARV_IS_GC_ENUMERATION(feature_node)) {
      return std::string("ENUM TYPE");
    }
    else if (ARV_IS_GC_INTEGER(feature_node)) {
      return std::string("INT TYPE");
    }
    else if (ARV_IS_GC_FLOAT(feature_node)) {
      return std::string("FLOAT TYPE");
    }
    else if (ARV_IS_GC_STRING(feature_node)) {
      return std::string("STRING TYPE");
    }
    else if (ARV_IS_GC_BOOLEAN(feature_node)) {
      return std::string("BOOL TYPE");
    }
    else {
      return std::string("OTHER TYPE");
    }
  }
  return std::string();
}

}  // namespace ep

