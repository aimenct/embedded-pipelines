// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _EP_ARV_CAM_H
#define _EP_ARV_CAM_H

#include <arv.h>
#include <math.h>

#include <cmath>
#include <iomanip>  // std::setw
#include <ios>      // std::left

#include "core.h"

namespace ep {
class ArvCam : public Filter {
  public:
    ArvCam(const YAML::Node &config); 
    ~ArvCam();                  

    int findDevices();

    ArvStream *stream();
    gboolean isHighPriority();
    gboolean isRealTime();
    GMainLoop *mainLoop();
    void setMainLoop(GMainLoop *main_loop);

    int sendCommand(char *msg, char **resp);
    int setGeniCamParameter(char *feature, char *token);
    int geniCamParameter(char *token);

    int32_t setDeviceSettingValue(const char *key, const void *value);
    int32_t setDeviceSettingValueStr(const char *key, const char *value);
    int32_t deviceSettingValue(const char *key, void *value);
  
    int addDeviceSetting(const char *setting_name);

    const char *geniCamXml(size_t *size);
    void parseCameraXml();

  protected:
    int32_t _job();
    int32_t _open(); 
    int32_t _set();  
    int32_t _start();
    int32_t _reset();
    int32_t _stop(); 
    int32_t _close();

    // callback for generating data when working in pull mode
    // int startReadCallback(void **data, void **hdr, void **usr_data);
    // int endReadCallback(void *usr_data);

    int32_t addDeviceSettingsFromYAML(const YAML::Node &config);

  private:
    YAML::Node yaml_config_;

    /* aravis camera & stream */
    ArvCamera *camera_;
    ArvStream *stream_;
    ArvGc *genicam_;
  
    GThread *thread_;

    int32_t arv_num_buffers_;
    std::string camera_name_;
    int32_t frame_rate_;
    int32_t stream_channel_;
    int32_t packet_delay_;
    int32_t packet_size_;
    int32_t socket_buffer_size_;
    uint32_t packet_timeout_;
    uint32_t frame_retention_;
    uint32_t bandwidth_limit_;
    double_t packet_request_ratio_;
    gboolean option_push_mode_;  // todo change to c++ bool
    gboolean auto_packet_size_;
    gboolean auto_socket_buffer_;
    gboolean no_packet_resend_;
    gboolean realtime_;       // realtime todo
    gboolean high_priority_;  // todo
    gboolean no_packet_socket_;
    char *debug_domains_;
    int32_t width_;
    int32_t height_;
    int32_t horizontal_binning_;
    int32_t vertical_binning_;
    int32_t gain_;
    double_t software_trigger_;
    double_t frequency_;
    double_t exposure_time_us_;
    gboolean snaphot_s;

    char args_[10][256];
    char response_[256];
    int32_t argc_;

    GMainLoop *main_loop_;  // todo

    ImageObject img_info_;
    int32_t data_size_;
    // settings for the output image description
    int32_t out_bands_;
    uint16_t out_interleave_;
    uint16_t out_color_;

    std::string nameArvFeatureType(ArvGcFeatureNode *feature_node);
};

}  // namespace ep

#endif  //_EP_ARV_CAM_H
