// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_RTSP_SERVER_H
#define _EP_RTSP_SERVER_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <math.h>

#include "core.h"

namespace ep {
class RtspSink : public Filter {
  public:
    gboolean white;

    RtspSink(const YAML::Node &config);  // create settings
    ~RtspSink();                   // delete settings

    GMainContext *context();
    void setContext(GMainContext *context);
    GMainLoop *mainLoop();
    void setMainLoop(GMainLoop *mainLoop);

    GstRTSPServer *server();

    void setServer(GstRTSPServer *server);

    GstRTSPMediaFactory *factory();

    void setFactory(GstRTSPMediaFactory *factory);

    std::string factoryConfig();
    GstClockTime *timestamp();
    int32_t framerate();

    ImageObject *image();

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _start();  // create queues and allocate memory
    int32_t _stop();   // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _close();  // close connection with device

  private:
    GThread *gst_thread_;

    int32_t framerate_;        // appsrc expected frame rate
    std::string factory_cfg_;  // gstreamer pipeline

    /* server variables */
    GstElement *appsrc_;
    GMainLoop *loop_;
    GMainContext *ctx_;
    GstRTSPServer *server_;
    //  GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory_;
    GstClockTime timestamp_;
    ep::ImageObject image_;
};

}  // namespace ep

#endif
