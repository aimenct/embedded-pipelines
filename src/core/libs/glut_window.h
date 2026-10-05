// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef WINDOW_H
#define WINDOW_H

#include <GL/freeglut.h>

#include <algorithm>  // For std::max_element
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "image_object.h"
#include "image_processing.h"
#include "queue_handlers.h"

// Window Base Class
class Window {
  public:
    enum class WindowType { Image, Hist, Plot2, Plot3 };
    enum class BufferType { Data, Header };

    Window(WindowType type, const std::string &name, int width, int height,
           int pos_x, int pos_y);

    virtual ~Window();

    static void registerWindowInstance(int windwosID, Window *instance);
    static Window *getCurrentInstance(int value);

    // Glut callbacks acting as dispatchers
    static void display();  // render function
    static void reshape(int width, int height);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void motionOver(int x, int y);
    static void keyboard(unsigned char key, int x = 0, int y = 0);
    static void timerCallback(int value);

    int32_t start();  // enable rendering updates tied to timer callback
    int32_t stop();   // enable rendering updates tied to timer callback

    WindowType getType() const;
    int id()
    {
      return id_;
    }
    const std::string &getName() const;

    // Method to set the keyboard callback
    static void setKeyboardCallback(void (*keyboard_cb)(unsigned char, int,
                                                        int));
    void setTimeout(int timeout);

  protected:
    // Each window type implements its own rendering logic
    virtual void _update() = 0;
    virtual void _display() = 0;
    virtual void _reshape(int width, int height) = 0;
    virtual void _mouse(int button, int state, int x, int y) = 0;
    virtual void _motion(int x, int y) = 0;
    virtual void _motionOver(int x, int y) = 0;
    virtual void _keyboard(unsigned char key, int x = 0, int y = 0) = 0;
    virtual void _idle() = 0;

    WindowType type_;
    std::string name_;
    int width_;
    int height_;
    int pos_x_;
    int pos_y_;
    int timeout_;  // milliseconds
    int id_;       // windows id
    bool is_active_;
    bool programatic_resize_;
    bool read_success_;

    // Static function pointer to hold the keyboard callback
    static void (*keyboard_callback_)(unsigned char, int, int);

    static std::unordered_map<int, Window *>
        window_instances_;  // Maps GLUT window IDs to Window instances
};

struct DataSource {
    int item = 0;
    Window::BufferType buffer_type = Window::BufferType::Data;
    epf::QueueReader *reader = nullptr;
    epf::DataNode *dn = nullptr;
    size_t dn_offset = 0;
    size_t msg_size = 0;
    epf::BaseType base_type = epf::EP_8U;

    DataSource() = default;
};

// Plot2DWindow
class Plot2DWindow : public Window {
  public:
    Plot2DWindow(const std::string &name, int width, int height, int pos_x,
                 int pos_y, int buffer_length = 10000);
    ~Plot2DWindow();

    // int32_t setXSource(std::shared_ptr<Queue> x, int item,
    //                    BufferType buf_type = BufferType::Data);
    int32_t setXSource(epf::Queue *x, int item,
                       BufferType buf_type = BufferType::Data);

    // int32_t setYSource(std::shared_ptr<Queue> y, int item,
    //                      BufferType buf_type = BufferType::Data);
    int32_t setYSource(epf::Queue *y, int item,
                       BufferType buf_type = BufferType::Data);

    void setYRange(float min_y, float max_y);
    void setBatchSize(int batch_size, int step);
    void setBlocking(bool blocking);

  protected:
    void _update();
    void _display();
    void _reshape(int width, int height);
    void _mouse(int button, int state, int x, int y);
    void _motion(int x, int y);
    void _motionOver(int x, int y);
    void _keyboard(unsigned char key, int x = 0, int y = 0);
    void _idle();

  private:
    int buffer_length_;

    // zoom, pan variables
    float zoom_factor_;
    float raster_x_;
    float raster_y_;
    float prev_x_;
    float prev_y_;
    float range_x_;
    float range_y_;

    bool autoscale_;
    float x0_, y0_;
    float x1_, y1_;
    float pan_x_;
    float pan_y_;

    float gamma_x_;
    float gamma_y_;

    // internal circular buffer

    int ptr_count_;
    bool count_sat_;
    int step_;

    // Data sources for x, y, z, and value
    DataSource xSource_;
    DataSource ySource_;
    int batch_size_;

    // Ticks params
    const float tick_length_{0.1f};
    const int major_tick_count_{5};
    const int minor_tick_count_{4};

    // data samples
    float *x_data_;
    float *y_data_;

    void zoom(float factor, int x, int y);
    void drawAxis();

    // Map to store unique readers_ per queue
    // std::unordered_map<std::shared_ptr<Queue>, std::unique_ptr<QueueReader>>
    //     readers_;
    std::unordered_map<epf::Queue *, std::unique_ptr<epf::QueueReader>>
        readers_;
};

// Plot3DWindow
// TODO - refactor using modern OpenGL Vertex Buffer Objects (VBO)
class Plot3DWindow : public Window {
  public:
    Plot3DWindow(const std::string &name, int width, int height, int pos_x,
                 int pos_y, int buffer_length = 10000);
    ~Plot3DWindow();

    int32_t setXSource(epf::Queue *q, int item,
                       BufferType buf_type = BufferType::Data);
    int32_t setYSource(epf::Queue *q, int item,
                       BufferType buf_type = BufferType::Data);
    int32_t setZSource(epf::Queue *q, int item,
                       BufferType buf_type = BufferType::Data);
    int32_t setVSource(epf::Queue *q, int item,
                       BufferType buf_type = BufferType::Data);

    //  void setColormap(Colormap::jet);  // monochrome /
    void setVRange(float min_y, float max_y);
    void setBatchSize(int batch_size, int step);
    void setBlocking(bool blocking);
    //    int32_t saveFrameBuffer();

  protected:
    void _update();
    void _display();
    void _reshape(int width, int height);
    void _mouse(int button, int state, int x, int y);
    void _motion(int x, int y);
    void _motionOver(int x, int y);
    void _keyboard(unsigned char key, int x = 0, int y = 0);
    void _idle();

  private:
    int buffer_length_;
    bool autoscale_;

    float camera_angle_x_;
    float camera_angle_y_;
    float camera_distance_;
    float pan_x_, pan_y_;

    bool is_rotating_;
    bool is_panning_;
    bool is_zooming_;
    int last_mouse_x_, last_mouse_y_;

    int ptr_count_;
    bool count_sat_;
    int step_;

    DataSource source_[4];
    int batch_size_;

    float *data_[4];

    float min_x_, min_y_, min_z_, max_x_, max_y_, max_z_, min_v_, max_v_;

    std::unordered_map<epf::Queue *, std::unique_ptr<epf::QueueReader>>
        readers_;

    int32_t setSource(int source_index, epf::Queue *q, int item,
                      BufferType buf_type);

    bool sourceConfigured(int source_index) const;
    bool requiredSourcesConfigured() const;
    void resetBounds();
    void recomputeBounds(int limit);

    float getBoundingBoxSize();
    void resetCamera();
};

// HistogramWindow
class HistWindow : public Window {
  public:
    HistWindow(const std::string &name, int width, int height, int pos_x,
               int pos_y);
    ~HistWindow();

    double minVal() const
    {
      return markerToValue(min_val_);
    }

    double maxVal() const
    {
      return markerToValue(max_val_);
    }

    void computeHistogram(epf::ImageObject &data);
    void draw();
    void setData(epf::ImageObject &data);

  protected:
    void _update() override
    {
    }  // No real-time updates, data driven by the parent window
    void _display() override;  // Redraw histogram plot and min/max markers
    void _reshape(int width, int height) override;  // nothing
    void _mouse(int button, int state, int x, int y) override;
    void _motion(int /*x*/, int /*y*/) override {};      // nothing
    void _motionOver(int /*x*/, int /*y*/) override {};  // print hist value
    void _keyboard(unsigned char /*key*/, int /*x = 0*/,
                   int /*y = 0*/) override {};  // nothing
    void _idle() override {};                   // nothing

  private:
    void setupViewport();  // Helper for OpenGL setup
    void drawHistogram();  // Encapsulated histogram drawing
    void drawMarkers();    // Encapsulated min/max marker drawing
    double markerToValue(float marker) const;

    //    std::vector<float> data_;  // Use std::vector for safety and
    //    flexibility
    std::vector<float> histogram_;
    float min_val_;  // Selected min value
    float max_val_;  // Selected max value
    float range_x_;  // Histogram X-axis range
    float range_y_;  // Histogram Y-axis range

    double hist_min_value_;
    double hist_max_value_;
    bool hist_valid_range_;
    double basetype_min_value_;
    double basetype_max_value_;

    int bins_;
};

// ImageWindow
class ImageWindow : public Window {
  public:
    ImageWindow(const std::string &name, int width, int height, int pos_x,
                int pos_y);

    ~ImageWindow();
    int32_t setDataSource(epf::Queue *y, int item,
                          BufferType buf_type = BufferType::Data);
    void setFloatAutoscale(bool enabled);
    void setBlocking(bool blocking);
    int32_t setProbeSource(epf::Queue *queue, int item,
                           BufferType buf_type = BufferType::Data);

    void setProbeOptions(bool enabled, bool hover, bool click,
                         const std::string &units = "", int precision = 2,
                         int throttle_ms = 100);

  protected:
    void _update();  // override;
    void _display();
    void _reshape(int width, int height);
    void _mouse(int button, int state, int x, int y);
    void _motion(int x, int y);
    void _motionOver(int x, int y);
    void _keyboard(unsigned char key, int x = 0, int y = 0);
    void _idle();

  private:
    epf::Queue *display_queue_;
    DataSource probeSource_;
    epf::ImageObject *probe_image_;

    bool probe_enabled_;
    bool probe_hover_;
    bool probe_click_;
    std::string probe_units_;
    int probe_precision_;
    int probe_throttle_ms_;
    int last_probe_x_;
    int last_probe_y_;
    std::chrono::steady_clock::time_point last_probe_print_;

    bool windowToImagePixel(int x, int y, int &px, int &py) const;
    bool readPixelChannels(const epf::ImageObject &img, int px, int py,
                           std::vector<double> &values) const;
    void printProbe(int x, int y, const char *event_name, bool throttle);

    double zoom_f_;
    int window_w_;
    int window_h_;
    float aspect_;
    int raster_x_;
    int raster_y_;
    int x_prev_;
    int y_prev_;

    int img_w_;
    int img_h_;

    GLuint texture_;
    GLenum data_type_;
    GLenum internal_data_format_;
    GLenum data_format_;

    epf::GLPixelFormatInfo gl_info_;
    bool upload_supported_;

    epf::QueueReader *reader_;
    epf::ImageObject *image_;
    bool float_autoscale_;

    static void menuCallback(int option);
    void zoom(float factor, int x, int y);

    bool compute_histogram_;
    bool normalize_;
    epf::ImageObject *norm_image_;
    HistWindow *hist_window_;

    // Helpers for the new upload flow
    bool configureUploadFormat(const epf::ImageObject &img);
    void configureTextureState();
    void configureTextureSwizzle(bool is_single_channel);
    bool uploadImagePixels(const epf::ImageObject &img, const void *pixels);
};

#endif  // WINDOW
