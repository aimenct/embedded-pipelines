// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "arv_cam.h"

static void *gs_thread_func(void *data)
{
  auto *ptr = static_cast<epf::ArvCam *>(data);
  GMainContext *context = g_main_context_new();

  ptr->setMainLoop(g_main_loop_new(context, false));
  g_main_loop_run(ptr->mainLoop());
  g_main_context_unref(context);
  return nullptr;
}

static void control_lost_cb(ArvGvDevice * /*gv_device*/)
{
  /* Control of the device is lost. Display a message and force application exit
   */
  std::cerr << "[ArvCam] Control channel lost from camera device." << std::endl;
}

static void stream_cb(void *user_data, ArvStreamCallbackType type,
                      ArvBuffer * /*buffer*/)
{
  auto *ptr = static_cast<epf::ArvCam *>(user_data);
  if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
    if (ptr->isRealTime()) {
      if (!arv_make_thread_realtime(10))
        std::cerr << "[ArvCam] Warning: failed to set stream thread realtime "
                     "priority."
                  << std::endl;
    }
    else if (ptr->isHighPriority()) {
      if (!arv_make_thread_high_priority(-10))
        std::cerr
            << "[ArvCam] Warning: failed to set stream thread high priority."
            << std::endl;
    }
  }
}

static void new_buffer_cb(ArvStream *stream, epf::ArvCam *cam)
{
  if (cam->state() != epf::FilterState::RUNNING) {
    return;
  }

  size_t size;
  const void *sptr;

  epf::QueueWriter *writer = cam->sinkPort(0)->writer();
  epf::Node *item_node = writer->dataSchema()->item(0);
  auto on = dynamic_cast<epf::ObjectNode *>(item_node);
  epf::ImageObject image(on);
  std::size_t data_size = image.bufferSize();

  ArvBuffer *buffer = arv_stream_try_pop_buffer(stream);
  if (buffer == nullptr) {
    return;
  }

  if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS &&
      arv_buffer_get_payload_type(buffer) == ARV_BUFFER_PAYLOAD_TYPE_IMAGE) {
    int32_t ret = writer->startWrite();
    if (ret >= 0) {
      cam->updateHeader(writer->hdrSchema());
      sptr = arv_buffer_get_data(buffer, &size);
      int32_t write_ret{-1};
      if (size == data_size) {
        write_ret = image.dataNode()->write(sptr);
      }
      if (write_ret < 0) {
        std::cerr << "[ArvCam] Unable to write Aravis frame in queue. "
                  << std::endl;
      }
      writer->endWrite();
    }
    else {
      std::cerr << "[ArvCam] Coudln't lock SinkPort message.";
    }
  }
  arv_stream_push_buffer(stream, buffer);
}

void process_node(ArvDomNode *a_node)
{
  ArvDomNode *cur_node = a_node;
  ArvDomNode *cur_node_child = nullptr;
  while (cur_node) {
    cur_node_child = arv_dom_node_get_first_child(cur_node);
    process_node(cur_node_child);
    cur_node = arv_dom_node_get_next_sibling(cur_node);
  }
}

namespace epf {
ArvCam::ArvCam(const YAML::Node &config)
    : Filter(config, 0, 1)
{
  camera_ = nullptr;
  stream_ = nullptr;
  thread_ = nullptr;
  main_loop_ = nullptr;
  new_buffer_handler_id_ = 0;
  stream_signals_connected_ = false;

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
  auto_packet_size_ = false;
  auto_socket_buffer_ = true;
  no_packet_resend_ = true;
  realtime_ = true;
  high_priority_ = true;
  no_packet_socket_ = true;
  debug_domains_ = nullptr;
  camera_name_ = "";

  job_execution_model_ = JobExecutionModel::OWN_THREAD;

  addSetting("arv_num_buffers", arv_num_buffers_);
  addSetting("trigger_mode", trigger_mode_);

  addCommand("trigger", std::bind(&ArvCam::trigger, this));
  addSetting("mtu", packet_size_);
  addSetting("mac", camera_name_);
  addSetting("packet_timeout", packet_timeout_);

  readSettings(config);
}

int32_t ArvCam::addDeviceSettingsFromYAML(const YAML::Node &config)
{
  YAML::Node yaml_node = config["settings"]["device"];
  if (yaml_node) {
    for (YAML::const_iterator it = yaml_node.begin(); it != yaml_node.end();
         ++it) {
      std::string key = it->first.as<std::string>();
      std::string value;

      if (yaml_node[key].Type() == YAML::NodeType::Scalar) {
        value = it->second.as<std::string>();
        if (addDeviceSetting(key.c_str()) == 0) {
          setSettingValue("device." + key, value);
        }
        else {
          std::cout << name()
                    << "::addDeviceSettingsFromYAML failed for setting: " << key
                    << std::endl;
        }
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
  int n_devices = static_cast<int>(arv_get_n_devices());

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
  GError *error = nullptr;
  const AccessType writable_when_attached =
      epf::W_D | epf::W_C | epf::W_S | epf::W_R;

  ArvGcNode *gc_node = arv_gc_get_node(genicam_, setting_name);

  // Check if the node is a GeniCam feature node
  if ((gc_node != nullptr) && (ARV_IS_GC_FEATURE_NODE(gc_node))) {
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
          addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip,
                     writable_when_attached);
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
          addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip,
                     writable_when_attached);
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
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip,
                   writable_when_attached);
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
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip,
                   writable_when_attached);
        return 0;
      }
    }
    else if (ARV_IS_GC_BOOLEAN(ARV_GC_FEATURE_NODE(gc_node))) {
      int32_t value = arv_gc_boolean_get_value(ARV_GC_BOOLEAN(gc_node), &error);
      if (error) {
        printf("ERROR! Aravis response: %s\n", error->message);
      }
      else {
        // std::cout << setting_name << value << std::endl;
        // getchar();
        addSetting(std::string(setting_name), value, DEVICE_SETTING, tooltip,
                   writable_when_attached);
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
  GError *gerror = nullptr;

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
    if (device_found == -1) {
      std::cerr << "[ArvCam] Device: " << camera_name_ << " not found."
                << std::endl;
      std::cerr << "[ArvCam] Available devices: " << n_devices << std::endl;
      for (int i = 0; i < n_devices; i++) {
        std::cerr << "[ArvCam] Device " << i << ": "
                  << std::string(arv_get_device_physical_id(i)) << std::endl;
      }
      return -1;
    }
    /* Instantiation of camera by id */
    camera_ = arv_camera_new(arv_get_device_id(device_found), nullptr);
  }
  else {
    /* Instantiation of the first available camera */
    camera_ = arv_camera_new(nullptr, &gerror);
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

    std::cout << yaml_config_ << std::endl;
    addDeviceSettingsFromYAML(yaml_config_);

    /* set camera parameters */
    if (arv_camera_is_gv_device(camera_)) {
      arv_camera_gv_select_stream_channel(camera_, stream_channel_, nullptr);
      arv_camera_gv_set_packet_delay(camera_, packet_delay_, nullptr);
      arv_camera_gv_set_packet_size(camera_, packet_size_, nullptr);
      arv_camera_gv_set_stream_options(
          camera_, no_packet_socket_
                       ? ARV_GV_STREAM_OPTION_PACKET_SOCKET_DISABLED
                       : ARV_GV_STREAM_OPTION_NONE);
    }
    // if (src->arv_option_chunks[0]!='\0')
    // arv_camera_set_chunk_mode(src->camera, src->arv_option_chunks);
    if (auto_packet_size_) {
      arv_camera_gv_auto_packet_size(camera_, nullptr);
    }
    if (frame_rate_) {
      arv_camera_set_frame_rate(camera_, frame_rate_, nullptr);
    }

    if (trigger_mode_ == "software") {
      // std::string on = "On";
      // std::string frame_start = "FrameStart";
      // std::string software = "Software";
      // setDeviceSettingValue("TriggerMode", &on);
      // setDeviceSettingValue("TriggerSelector", &frame_start);
      // setDeviceSettingValue("TriggerSource", &software);
      if (arv_camera_is_software_trigger_supported(camera_, nullptr)) {
        arv_camera_set_acquisition_mode(
            camera_, ARV_ACQUISITION_MODE_CONTINUOUS, nullptr);
        // arv_camera_clear_triggers(camera_, nullptr);
        std::string trigger = "Software";
        arv_camera_set_trigger_source(camera_, trigger.c_str(), nullptr);
      }
      else {
        std::cout << "[ArvCam] Camera " << camera_name_
                  << " does not support software trigger." << std::endl;
        return -1;
      }
    }

    /* Connect the control-lost signal */
    g_signal_connect(arv_camera_get_device(camera_), "control-lost",
                     G_CALLBACK(control_lost_cb), nullptr);
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
  camera_ = nullptr;
  return 0;
}

int32_t ArvCam::_set()
{
  GError *gerror = nullptr;

  /* Create a new stream object */
  stream_ = arv_camera_create_stream(camera_, stream_cb, this, &gerror);
  if (stream_ == nullptr) {
    printf(
        "Can't create stream thread (check if the device is not already "
        "used)\n");
    return -1;
  }

  /* set stream parameters */
  if (ARV_IS_GV_STREAM(stream_)) {
    if (auto_socket_buffer_) {
      g_object_set(stream_, "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
                   "socket-buffer-size", 0, nullptr);
    }
    if (socket_buffer_size_ > 0) {
      g_object_set(stream_, "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_FIXED,
                   "socket-buffer-size", (unsigned)socket_buffer_size_,
                   nullptr);
    }
    if (no_packet_resend_) {
      g_object_set(stream_, "packet-resend", ARV_GV_STREAM_PACKET_RESEND_NEVER,
                   nullptr);
    }
    if (packet_request_ratio_ >= 0.0) {
      g_object_set(stream_, "packet-request-ratio", packet_request_ratio_,
                   nullptr);
    }
    g_object_set(stream_, "packet-timeout", (unsigned)packet_timeout_ * 1000,
                 "frame-retention", (unsigned)frame_retention_ * 1000, nullptr);

    // std::cout << "g_object_set(stream_, packet-timeout, "
    //              "(unsigned)packet_timeout_ * 1000,"
    //           << std::endl;
  }

  /* get stream info */
  int32_t width;
  int32_t height;
  int32_t x, y;
  arv_camera_get_region(camera_, &x, &y, &width, &height, nullptr);
  // pixel_format_str = arv_camera_get_pixel_format_as_string(camera, NULL);
  ArvPixelFormat arv_pixel_format =
      arv_camera_get_pixel_format(camera_, nullptr);
  size_t payload = width * height *
                   static_cast<int32_t>(ceil(
                       ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_pixel_format) / 8));

  /* Push num_buffers buffer in the aravis stream input buffer queue */
  for (int i = 0; i < arv_num_buffers_; i++) {
    arv_stream_push_buffer(stream_, arv_buffer_new(payload, nullptr));
  }

  PixelFormat pixel_format = epf::PixelFormat::RGB8;
  bool known_format = false;

#ifdef ARV_PIXEL_FORMAT_YUV_422_PACKED
  if (arv_pixel_format == ARV_PIXEL_FORMAT_YUV_422_PACKED) {
    pixel_format = epf::PixelFormat::YUV422_8_UYVY;
    out_bands_ = 3;
    known_format = true;
  }
#endif

  if (!known_format) {
    switch (arv_pixel_format) {
      case ARV_PIXEL_FORMAT_MONO_8:
        pixel_format = epf::PixelFormat::Mono8;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_MONO_10:
        pixel_format = epf::PixelFormat::Mono10;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_MONO_12:
        pixel_format = epf::PixelFormat::Mono12;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_MONO_16:
        pixel_format = epf::PixelFormat::Mono16;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_RGB_8_PACKED:
        pixel_format = epf::PixelFormat::RGB8;
        out_bands_ = 3;
        break;
      case ARV_PIXEL_FORMAT_BGR_8_PACKED:
        pixel_format = epf::PixelFormat::BGR8;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_BAYER_RG_8:
        pixel_format = epf::PixelFormat::BayerRG8;
        out_bands_ = 1;
        break;
      case ARV_PIXEL_FORMAT_BAYER_GR_8:
        pixel_format = epf::PixelFormat::BayerGR8;
        out_bands_ = 1;
        break;
      default:
        std::cerr << "[ArvCam] Warning: unrecognized Aravis pixel format, "
                     "falling back to RGB8."
                  << std::endl;
        pixel_format = epf::PixelFormat::RGB8;
        out_bands_ = 3;
        break;
    }
  }

  ImageObject image =
      ImageObject("image", width, height, out_bands_, pixel_format, nullptr);

  /* Configure sink queues */
  // - set schemas
  std::unique_ptr<Message> message = std::make_unique<Message>();
  message->addItem(image.copyNode());

  std::unique_ptr<Message> header = std::make_unique<Message>();
  header_nodes_.clear();
  header_string_encoded_.clear();
  header_dirty_.clear();

  std::cout << "[ArvCam] Building header schema from configured device nodes."
            << std::endl;
  YAML::Node hdr_cfg = yaml_config_["settings"]["header"]["device"];
  if (hdr_cfg) {
    for (YAML::const_iterator it = hdr_cfg.begin(); it != hdr_cfg.end(); ++it) {
      std::string name = it->as<std::string>();

      // Skip duplicates already added in header
      if (header_nodes_.find(name) != header_nodes_.end()) {
        std::cerr << "[ArvCam] Header setting '" << name
                  << "' is duplicated in configuration; ignoring duplicate."
                  << std::endl;
        // LOG info: header node already exists for 'name'; skipping.
        continue;
      }

      // if device settings not in settings try to add it
      if (!settings_.settingExists("device." + name)) {
        if (addDeviceSetting(name.c_str()) == 0) {
          std::cout << "[ArvCam] Added dynamic device setting 'device." << name
                    << "' for header streaming." << std::endl;
        }
        else {
          std::cerr << "[ArvCam] Failed to add missing device setting 'device."
                    << name << "' requested by header config." << std::endl;
          continue;
        }
      }

      // add device setting to header
      const Node *snode = settings_["device." + name];
      if (snode && snode->isDataNode()) {
        const DataNode *sd = static_cast<const DataNode *>(snode);
        auto hdr_node = std::make_unique<DataNode>(
            name, sd->datatype(), sd->arraydimensions(), nullptr);  // streamed
        //        DataNode *hdr_ptr = hdr_node.get();
        header->addItem(std::move(hdr_node));
        header_nodes_[name] = hdr_node.get();
        header_string_encoded_[name] = false;
        header_dirty_[name] = false;
      }
      else if (snode && snode->isStringNode()) {
        // String/enum settings are encoded into a fixed-size streamed char
        // array so downstream filters can consume them from queue headers.
        auto hdr_node = std::make_unique<DataNode>(
            name, epf::EP_8C, std::vector<size_t>{256}, nullptr);
        header->addItem(std::move(hdr_node));
        header_nodes_[name] = hdr_node.get();
        header_string_encoded_[name] = true;
        header_dirty_[name] = false;
      }
      else
        std::cerr << "[ArvCam] Setting 'device." << name
                  << "' is unavailable or not a DataNode/StringNode; skipping "
                     "header item."
                  << std::endl;
    }
  }

  sinkPort(0)->activate(std::move(message), std::move(header));

  // get node pointers
  for (auto &kv : header_nodes_) {
    Node *n = sinkPort(0)->writer()->hdrSchema()->item(kv.first);
    if ((n) && (n->isDataNode())) kv.second = static_cast<DataNode *>(n);
  }

  // sinkPort(0)->writer()->dataSchema()->print();
  // sinkPort(0)->writer()->hdrSchema()->print();
  // getchar();

  /* 3- Get the pointer of the ImageObject in the Msg to be used in job */
  // get image node - item 1
  Node *item_node = sinkPort(0)->writer()->dataSchema()->item(0);
  auto on = dynamic_cast<ObjectNode *>(item_node);
  img_info_ = ImageObject(on);
  std::cout << "[ArvCam] set() completed. Queue len="
            << sinkPort(0)->writer()->queue()->length()
            << ", image bytes/frame=" << img_info_.bufferSize()
            << ", header items=" << header_nodes_.size() << "." << std::endl;

  return 0;
}

int32_t ArvCam::_reset()
{
  if (stream_ != nullptr) {
    if (stream_signals_connected_ && new_buffer_handler_id_ != 0) {
      g_signal_handler_disconnect(stream_, new_buffer_handler_id_);
      new_buffer_handler_id_ = 0;
      stream_signals_connected_ = false;
    }
    g_object_unref(stream_);
    stream_ = nullptr;
  }
  return 0;
}

int32_t ArvCam::_start()
{
  GError *gerror = nullptr;

  if (stream_ == nullptr || camera_ == nullptr) {
    std::cerr << "[ArvCam] Cannot start: camera or stream is not initialized."
              << std::endl;
    return -1;
  }

  while (true) {
    ArvBuffer *stale = arv_stream_try_pop_buffer(stream_);
    if (stale == nullptr) break;
    arv_stream_push_buffer(stream_, stale);
  }

  if (thread_ == nullptr) {
    thread_ = g_thread_new("streaming-thread", gs_thread_func, this);
    for (int i = 0; i < 500 && main_loop_ == nullptr; ++i) {
      g_usleep(1000);
    }
  }

  if (!stream_signals_connected_) {
    new_buffer_handler_id_ = g_signal_connect(stream_, "new-buffer",
                                              G_CALLBACK(new_buffer_cb), this);
    stream_signals_connected_ = (new_buffer_handler_id_ != 0);
  }

  arv_stream_set_emit_signals(stream_, true);
  arv_camera_start_acquisition(camera_, &gerror);
  if (gerror) {
    std::cerr << "[ArvCam] Failed to start acquisition: " << gerror->message
              << std::endl;
    g_error_free(gerror);
    return -1;
  }
  return 0;
}

int32_t ArvCam::_stop()
{
  GError *gerror = nullptr;

  if (camera_ != nullptr) {
    arv_camera_stop_acquisition(camera_, &gerror);
    if (gerror) {
      std::cerr << "[ArvCam] Failed to stop acquisition: " << gerror->message
                << std::endl;
      g_error_free(gerror);
      gerror = nullptr;
    }
  }

  if (stream_ != nullptr) {
    arv_stream_set_emit_signals(stream_, false);
    if (stream_signals_connected_ && new_buffer_handler_id_ != 0) {
      g_signal_handler_disconnect(stream_, new_buffer_handler_id_);
      new_buffer_handler_id_ = 0;
      stream_signals_connected_ = false;
    }
  }

  if (main_loop_ != nullptr) {
    g_main_loop_quit(main_loop_);
  }
  if (thread_ != nullptr) {
    g_thread_join(thread_);
    thread_ = nullptr;
  }
  if (main_loop_ != nullptr) {
    g_main_loop_unref(main_loop_);
    main_loop_ = nullptr;
  }

  return 0;
}

bool ArvCam::isHighPriority() const
{
  return high_priority_;
}

bool ArvCam::isRealTime() const
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
  if (!camera_) {
    std::cout << std::setw(25) << std::left << key << " " << "[FAILED]"
              << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValue." << std::endl
              << "Camera is disconnected" << std::endl;
    return -1;
  }
  GError *error = nullptr;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);

  int32_t align_name = 25;
  int32_t align_type = 15;
  int32_t align_value = 15;

  std::cout << std::setw(align_name) << std::left << key << " ";

  if ((node != nullptr) && (ARV_IS_GC_FEATURE_NODE(node))) {
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
                  << std::setw(align_value) << static_cast<const char *>(value);
        arv_gc_string_set_value(
            ARV_GC_STRING(node),
            const_cast<char *>(static_cast<const char *>(value)), &error);
      }
      else if (ARV_IS_GC_BOOLEAN(feature_node)) {
        int32_t typed_value = *static_cast<const int32_t *>(value);
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << typed_value;
        arv_gc_boolean_set_value(ARV_GC_BOOLEAN(node),
                                 *static_cast<const int32_t *>(value), &error);
      }
      else {
        std::cout << std::setw(align_type) << nameArvFeatureType(feature_node)
                  << std::setw(align_value) << static_cast<const char *>(value);
        arv_gc_feature_node_set_value_from_string(
            ARV_GC_FEATURE_NODE(node),
            const_cast<char *>(static_cast<const char *>(value)), &error);
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

  if (error != nullptr) {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValue." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }
  std::cout << "[SUCCESS]" << std::endl;
  std::string key_str(key);
  if (header_nodes_.find(key_str) != header_nodes_.end()) {
    header_dirty_[key_str] = true;
  }
  return 0;
}

int32_t ArvCam::setDeviceSettingValueStr(const char *key, const char *value)
{
  if (!camera_) {
    std::cout << std::setw(25) << std::left << key << " " << "[FAILED]"
              << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValueStr." << std::endl
              << "Camera is disconnected" << std::endl;
    return -1;
  }
  GError *error = nullptr;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);
  int32_t align_name = 25;
  int32_t align_type = 15;
  int32_t align_value = 15;

  std::cout << std::setw(align_name) << std::left << key << " ";

  if ((node != nullptr) && (ARV_IS_GC_FEATURE_NODE(node))) {
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

  if (error != nullptr) {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "ERROR in ArvCam::setDeviceSettingValueStr." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }

  std::cout << "[SUCCESS]" << std::endl;
  std::string key_str(key);
  if (header_nodes_.find(key_str) != header_nodes_.end()) {
    header_dirty_[key_str] = true;
  }
  return 0;
}

int32_t ArvCam::deviceSettingValue(const char *key, void *value)
{
  GError *error = nullptr;
  ArvGcNode *node = arv_device_get_feature(arv_camera_get_device(camera_), key);

  if ((node != nullptr) && (ARV_IS_GC_FEATURE_NODE(node))) {
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
      *static_cast<int64_t *>(value) =
          arv_gc_integer_get_value(ARV_GC_INTEGER(node), &error);
    }
    else if (ARV_IS_GC_FLOAT(feature_node)) {
      *static_cast<double *>(value) =
          arv_gc_float_get_value(ARV_GC_FLOAT(node), &error);
    }
    else if (ARV_IS_GC_BOOLEAN(feature_node)) {
      *static_cast<int32_t *>(value) =
          arv_gc_boolean_get_value(ARV_GC_BOOLEAN(node), &error);
    }
    else if (ARV_IS_GC_STRING(feature_node)) {
      strncpy(static_cast<char *>(value),
              arv_gc_string_get_value(ARV_GC_STRING(node), &error), 256);
    }
    else {
      strncpy(static_cast<char *>(value),
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

  if (error != nullptr) {
    std::cout << "ERROR in ArvCam::deviceSettingValue." << std::endl
              << "Errcode: " << error->code << ". " << error->message
              << std::endl;
    return -1;
  }
  return 0;
}

int32_t ArvCam::trigger()
{
  if (trigger_mode_ != "software") return -1;
  GError *gerror = nullptr;
  arv_camera_software_trigger(camera_, &gerror);
  if (gerror) {
    std::cout << "ERROR in ArvCam::trigger. " << gerror->message << std::endl;
    return -1;
  }
  return 0;
}

void ArvCam::updateHeader(Message *hdr)
{
  (void)hdr;
  for (auto &kv : header_nodes_) {
    if (header_string_encoded_[kv.first]) {
      const Node *n = settings_["device." + kv.first];
      std::string value;
      if (n && n->isStringNode()) {
        const auto *s = static_cast<const StringNode *>(n);
        if (s->value()) value = *(s->value());
      }

      char *dst = static_cast<char *>(kv.second->value());
      if (dst != nullptr && kv.second->size() > 0) {
        std::memset(dst, 0, kv.second->size());
        if (!value.empty()) {
          std::strncpy(dst, value.c_str(), kv.second->size() - 1);
        }
      }
      header_dirty_[kv.first] = false;
      continue;
    }

    if (header_dirty_[kv.first]) {  // if dirty read from device
      deviceSettingValue(kv.first.c_str(), kv.second->value());
      header_dirty_[kv.first] = false;
    }
    else {  // else update without reading from device - using cache settings
      const Node *n = settings_["device." + kv.first];
      if (n && n->isDataNode()) {
        const DataNode *d = static_cast<const DataNode *>(n);
        int32_t ret = d->read(kv.second->value());
        if (ret < 0) {
          std::cerr
              << "[ArvCam] Unable to update header from cache for setting: "
              << d->name() << std::endl;
        }
      }
    }

    // std::cout << kv.first << std::endl;
    // if (kv.second != nullptr) kv.second->print();
  }
}

void ArvCam::parseCameraXml()
{
  process_node((ArvDomNode *)genicam_);
}

std::string ArvCam::nameArvFeatureType(ArvGcFeatureNode *feature_node)
{
  GError *error = nullptr;
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

void ArvCam::printStatistics()
{
  // Get statistics
  guint64 n_completed_buffers, n_failures, n_underruns;
  gint n_input_buffers, n_output_buffers;

  arv_stream_get_statistics(stream_, &n_completed_buffers, &n_failures,
                            &n_underruns);
  arv_stream_get_n_buffers(stream_, &n_input_buffers, &n_output_buffers);

  // Compact printing using std::cout
  std::cout << "\n----- Aravis Statistics -----\n"
            << "Completed: " << n_completed_buffers << "  "
            << "Failures: " << n_failures << "  "
            << "Underruns: " << n_underruns << "\n"
            << "Input Buffers: " << n_input_buffers << "  "
            << "Output Buffers: " << n_output_buffers << "\n"
            << "----------------------------\n";
}

}  // namespace epf
