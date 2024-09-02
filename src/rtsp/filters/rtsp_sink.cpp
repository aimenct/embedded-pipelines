// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "rtsp_sink.h"

/* called when we need to give data to appsrc */
static void need_data_cb(GstElement *appsrc, guint /*unused*/,
                         ep::RtspSink *rtsp_server_filter)
{
  GstBuffer *buffer;
  GstMapInfo map;
  GstFlowReturn ret;

  // struct timeval t1, t2;
  // double elapsedTime;
  //  gettimeofday(&t1, NULL);

  ep::QueueReader *reader = rtsp_server_filter->reader(0);
  ep::ImageObject *image = rtsp_server_filter->image();

  // GstClockTime *timestamp;

  buffer = gst_buffer_new_allocate(NULL, image->size(), NULL);
  if (buffer != NULL) {
    int32_t err = reader->startRead();
    if (err < 0) {
      gst_buffer_unref(buffer);
      printf("buffer_out_unblock %d\n", err);
      fflush(stdout);
      if (err == -2) {  // buffer stopped or free
        printf("unsubscribe_consumer\n");
        fflush(stdout);
        exit(0);
        return;
      }
    }
    // copy data from buffer
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
      memcpy(map.data, image->data(), map.size);
      // std::cout << "memcpy!" << std::endl;
      // printf("map size %d\n",map.size); fflush(stdout);
      gst_buffer_unmap(buffer, &map);
    }
    else {
      gst_buffer_unref(buffer);
      printf("gst_buffer_map error\n");
      exit(0);
    }
    reader->endRead();

    GST_BUFFER_PTS(buffer) = *(rtsp_server_filter->timestamp());
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(
        1, GST_SECOND, rtsp_server_filter->framerate());
    GstClockTime *timestamp = rtsp_server_filter->timestamp();
    *timestamp = (*timestamp) + GST_BUFFER_DURATION(buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    // printf("buffer_duration %d, timestamp:
    // %d\n",GST_BUFFER_DURATION(buffer),GST_TIME_AS_MSECONDS(ptr->timestamp));
    gst_buffer_unref(buffer);
  }
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory * /*factory*/,
                            GstRTSPMedia *media, void *usr)
{
  ep::RtspSink *rtsp_server_filter = (ep::RtspSink *)usr;

  GstElement *element, *appsrc;
  // GstClockTime *timestamp;

  /* get the element used for providing the streams of the media */
  element = gst_rtsp_media_get_element(media);
  std::cout << "element: " << element << std::endl;

  /* get our appsrc, we named it 'mysrc' with the name property */
  appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");
  std::cout << "appsrc: " << appsrc << std::endl;
  if (appsrc) {
    std::cout << "appsrc created" << std::endl;
    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
    /* setup application source from input buffer */
    // TODO
    ep::ImageObject *image = rtsp_server_filter->image();
    std::string gst_format;

    if (image->pixelFormat() == ep::Mono8) {
      gst_format = "GRAY8";
    }
    else if (image->pixelFormat() == ep::Mono16) {
      gst_format = "GRAY16_LE";
    }
    else if (image->pixelFormat() == ep::RGB8) {
      gst_format = "RGB";
    }
    else if (image->pixelFormat() == ep::RGB12) {
      gst_format = "RGB16";
    }

    g_object_set(G_OBJECT(appsrc), "caps",
                 gst_caps_new_simple(
                     "video/x-raw", "format", G_TYPE_STRING, gst_format.c_str(),
                     "width", G_TYPE_INT, image->width(), "height", G_TYPE_INT,
                     image->height(), "framerate", GST_TYPE_FRACTION,
                     rtsp_server_filter->framerate(), 1, NULL),
                 NULL);
    //		      "framerate", GST_TYPE_FRACTION, 0, ptr->getFramerate(),
    // NULL), NULL);
    g_object_set(G_OBJECT(appsrc), "is-live", TRUE, "do-timestamp", TRUE, NULL);
    g_object_set(G_OBJECT(appsrc), "stream-type", 0, "format", GST_FORMAT_TIME,
                 "max-latency", 9223372036854775807, NULL);
    // printf("format:%s width:%d height:%d framerate:
    // %d\n",format,img_nfo->width,img_nfo->height,ptr->getFramerate());
    fflush(stdout);
    g_assert(appsrc);

    /* initialize timestamp */
    *(rtsp_server_filter->timestamp()) = 0;

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback)need_data_cb,
                     rtsp_server_filter);
    gst_object_unref(appsrc);
  }
  if (element) gst_object_unref(element);
  std::cout << "!configured!!" << std::endl;
}

void *rtsp_thread(void *user)
{
  ep::RtspSink *rtsp_server_filter = (ep::RtspSink *)user;

  GstRTSPMountPoints *mounts;

  gst_init(NULL, NULL);

  //  https://stackoverflow.com/questions/62396219/running-every-gstremer-pipeline-into-a-separate-glib-thread
  // ptr->setContext(g_main_context_new());
  // ptr->setGMainLoop(g_main_loop_new(ptr->getContext(), FALSE));
  rtsp_server_filter->setMainLoop(g_main_loop_new(NULL, FALSE));

  /* create a server instance */
  //  server = gst_rtsp_server_new ();
  rtsp_server_filter->setServer(gst_rtsp_server_new());

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points(rtsp_server_filter->server());

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines.
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  rtsp_server_filter->setFactory(gst_rtsp_media_factory_new());

  gst_rtsp_media_factory_set_launch(
      rtsp_server_filter->factory(),
      rtsp_server_filter->factoryConfig().c_str());

  /* notify when our media is ready, This is called whenever someone asks for
   * the media and a new pipeline with our appsrc is created */
  g_signal_connect(rtsp_server_filter->factory(), "media-configure",
                   (GCallback)media_configure, rtsp_server_filter);

  /* attach the test factory to the /test url */
  gst_rtsp_mount_points_add_factory(mounts, "/test",
                                    rtsp_server_filter->factory());

  /* don't need the ref to the mounts anymore */
  g_object_unref(mounts);

  /* attach the server to the default maincontext */
  gst_rtsp_server_attach(rtsp_server_filter->server(), NULL);

  /* start serving */
  g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
  g_main_loop_run(rtsp_server_filter->mainLoop());

  printf("exit rtsp stream thread\n");
  return NULL;
}

namespace ep {
/* Filter constructor - create settings */
RtspSink::RtspSink(const YAML::Node &config)
    : Filter()
{
  std::cout << "RtspSink constructor " << std::endl;

  name_ = "Rtsp";
  type_ = "RtspSink";

  job_execution_model_ = OWN_THREAD;

  max_sources_ = 1;
  framerate_ = 15;
  factory_cfg_ =
      "( appsrc name=mysrc "
      " ! videoconvert ! videoscale ! video/x-raw,width=1920,height=1080 !"
      "x264enc ! rtph264pay name=pay0 pt=96 )";
  // factory_cfg_ =
  //     "( appsrc name=mysrc ! videoconvert ! x264enc ! video/x-h264, "
  //     "profile=(string)main ! rtph264pay config-interval=1 name=pay0 pt=96
  //     )";

  addSetting("framerate", framerate_, FILTER_SETTING, "framerate [Hz]", W);
  addSetting("factory_cfg", factory_cfg_, FILTER_SETTING, "factory", W);

  readSettings(config);
  std::cout << factory_cfg_ << std::endl;
}

/* Filter destructor*/
RtspSink::~RtspSink()
{
  std::cout << "RtspSink destructor " << std::endl;
}

int32_t RtspSink::_open()
{
  return 0;
}

int32_t RtspSink::_close()
{
  return 0;
}

int32_t RtspSink::_set()
{
  loop_ = nullptr;
  appsrc_ = nullptr;
  ctx_ = nullptr;
  server_ = nullptr;
  factory_ = nullptr;

  /* Configure source queues */
  // check at least 1 queue connected
  for (int i = 0; i < sourcePorts(); i++) {
    if (sourceQueue(i) != nullptr)

      for (size_t idx = 0; idx < reader(0)->dataSchema()->itemCount(); idx++) {
        Node2 *item_node = reader(0)->dataSchema()->item(idx);
        if (item_node->nodetype() == EP_OBJECTNODE) {
          ObjectNode *object_node = static_cast<ObjectNode *>(item_node);
          if (object_node->objecttype() == EP_IMAGE_OBJ) {
            image_ = ImageObject(object_node);
            return 0;
          }
        }
      }
  }

  std::cout << "RtspSink::_set() error: no image found in src queue"
            << std::endl;

  return -1;
}

int32_t RtspSink::_reset()
{
  return 0;
}

int32_t RtspSink::_job()
{
  return -1;
}

int32_t RtspSink::_start()
{
  gst_thread_ = g_thread_new("rtsp-thread", rtsp_thread, this);
  return 0;
}

int32_t RtspSink::_stop()
{
  g_main_loop_quit(this->loop_);
  if (this->loop_) {
    g_main_loop_unref(this->loop_);
    this->loop_ = NULL;
    g_print("Exit g_main_loop\n");
  }

  g_thread_join(this->gst_thread_);
  //    g_thread_unref(thread);

  // TODO - do we need to free these:?
  //  https://github.com/freedesktop/gstreamer-gst-rtsp-server/blob/master/examples/test-appsrc2.c
  if (this->factory()) {
    // gst_rtsp_factory_unref(this->getFactory());
    g_object_unref(this->factory());
    this->factory_ = NULL;
  }
  if (this->server()) {
    //    g_main_rtsp_server_unref (ptr->getServer());
    g_object_unref(this->server());
    this->server_ = NULL;
  }

  return 0;
}

GMainContext *RtspSink::context()
{
  return ctx_;
}
void RtspSink::setContext(GMainContext *context)
{
  ctx_ = context;
}
GMainLoop *RtspSink::mainLoop()
{
  return loop_;
}
void RtspSink::setMainLoop(GMainLoop *mainLoop)
{
  loop_ = mainLoop;
}
GstRTSPServer *RtspSink::server()
{
  return server_;
}
void RtspSink::setServer(GstRTSPServer *server)
{
  server_ = server;
}

GstRTSPMediaFactory *RtspSink::factory()
{
  return factory_;
}
void RtspSink::setFactory(GstRTSPMediaFactory *factory)
{
  factory_ = factory;
}

std::string RtspSink::factoryConfig()
{
  return factory_cfg_;
}

GstClockTime *RtspSink::timestamp()
{
  return &timestamp_;
}

int RtspSink::framerate()
{
  return framerate_;
}

ImageObject *RtspSink::image()
{
  return &image_;
}

}  // namespace ep
