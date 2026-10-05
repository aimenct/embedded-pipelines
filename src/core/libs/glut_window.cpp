#include "glut_window.h"

// Linking Timer to Window:

// The value passed to timerCallback via glutTimerFunc is crucial for
// identifying which window the callback is for. Use a std::map or similar data
// structure to map the value (e.g., windowIndex) to the corresponding Window
// instance. Periodic Execution:

// Within timerCallback, reschedule itself with glutTimerFunc to create a
// periodic update cycle. Decouple Logic:

// Keep rendering and update logic separate. Use update() for data logic and
// render() for drawing.

// ##############  ################//

// TODOS:
// PLOT2D:
// - improve axis
// - add possibility to add y and x (e.g timestamp).

// ############## WINDOW ################//

using namespace std;
using namespace epf;

void (*Window::keyboard_callback_)(unsigned char, int, int) = nullptr;

std::unordered_map<int, Window *> Window::window_instances_;

///////////////////////////////////////////////////////////
// Window
///////////////////////////////////////////////////////////

Window::Window(WindowType type, const std::string &name, int width, int height,
               int pos_x, int pos_y)
    : type_(type),
      name_(name),
      width_(width),
      height_(height),
      pos_x_(pos_x),
      pos_y_(pos_y)
{
  // Register the window instance to the global map
  if (type == WindowType::Plot3)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  programatic_resize_ = true;
  glutInitWindowSize(width_, height_);
  glutInitWindowPosition(pos_x_, pos_y);

  id_ = glutCreateWindow(name.c_str());
  registerWindowInstance(id_, this);

  if (type == WindowType::Plot3) {
    // Initialize OpenGL settings
    glEnable(GL_DEPTH_TEST);
    // glPointSize(5.0f);
  }

  timeout_ = 1000;  // 10 default Hz
  is_active_ = false;
  read_success_ = false;

  glutSetWindow(id_);
  glutDisplayFunc(Window::display);
  glutReshapeFunc(Window::reshape);
  glutMouseFunc(Window::mouse);
  glutMotionFunc(Window::motion);
  glutPassiveMotionFunc(Window::motionOver);
  glutKeyboardFunc(Window::keyboard);
}

void Window::setKeyboardCallback(void (*keyboard_callback)(unsigned char, int,
                                                           int))
{
  keyboard_callback_ = keyboard_callback;
}

void Window::registerWindowInstance(int windowID, Window *instance)
{
  window_instances_[windowID] = instance;
}

Window *Window::getCurrentInstance(int value)
{
  auto it = window_instances_.find(value);
  return (it != window_instances_.end()) ? it->second : nullptr;
}

void Window::display()
{
  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    it->second->_display();  // Call the specific instance's rendering logic
  }
}

void Window::reshape(int width, int height)
{
  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    it->second->_reshape(width, height);
  }
}

void Window::mouse(int button, int state, int x, int y)
{
  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    it->second->_mouse(button, state, x, y);
  }
}

void Window::motion(int x, int y)
{
  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    it->second->_motion(x, y);
  }
}

void Window::motionOver(int x, int y)
{
  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    it->second->_motionOver(x, y);
  }
}

void Window::keyboard(unsigned char key, int x, int y)
{
  if (keyboard_callback_ != nullptr) keyboard_callback_(key, x, y);

  // Find the current window instance using the GLUT window ID
  int id = glutGetWindow();
  auto it = window_instances_.find(id);
  if (it != window_instances_.end() && it->second) {
    if (key == 'p') {
      if (it->second->is_active_) {
        printf("p - pause\n");
        it->second->stop();
      }
      else {
        printf("p - play\n");
        it->second->start();
      }
    }
    it->second->_keyboard(key, x, y);
  }
}

void Window::timerCallback(int value)
{
  Window *instance = Window::getCurrentInstance(value);
  if (!instance || !instance->is_active_) return;

  const int current = glutGetWindow();

  if (current == 0) {
    instance->is_active_ = false;
    Window::window_instances_.erase(value);
    return;
  }

  glutSetWindow(value);

  if (glutGetWindow() != value) {
    instance->is_active_ = false;
    Window::window_instances_.erase(value);
    return;
  }

  instance->_update();

  if (instance->read_success_) {
    glutPostRedisplay();
  }

  if (instance->is_active_) {
    glutTimerFunc(instance->timeout_, timerCallback, value);
  }
}

int32_t Window::start()
{
  if (!is_active_) {
    is_active_ = true;
    glutTimerFunc(timeout_, Window::timerCallback, id_);
    std::cout << "Window " << id_ << " started." << std::endl;
  }
  return 0;
}

int32_t Window::stop()
{
  if (is_active_) {
    is_active_ = false;
    std::cout << "Window " << id_ << " stopped." << std::endl;
  }
  return 0;
}

Window::WindowType Window::getType() const
{
  return type_;
}

const std::string &Window::getName() const
{
  return name_;
}

void Window::setTimeout(int timeout)
{
  timeout_ = timeout;
}

Window::~Window()
{
  is_active_ = false;
  window_instances_.erase(id_);
  id_ = 0;
}

///////////////////////////////////////////////////////////
// Plot2DWindow : public Window
///////////////////////////////////////////////////////////

void draw_text(float x, float y, const std::string &text)
{
  glRasterPos2f(x, y);
  for (char c : text) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
  }
}

float safe_range(float min_v, float max_v, float fallback = 1.0f)
{
  const float r = max_v - min_v;
  if (std::isfinite(r) && std::abs(r) > 1e-12f) return r;
  return fallback;
}

void expand_degenerate_range(float &min_v, float &max_v)
{
  if (!std::isfinite(min_v) || !std::isfinite(max_v)) {
    min_v = 0.0f;
    max_v = 1.0f;
    return;
  }

  if (max_v > min_v) return;

  const float center = min_v;
  const float eps = std::max(std::abs(center) * 1e-6f, 1e-6f);
  min_v = center - eps;
  max_v = center + eps;
}

Plot2DWindow::Plot2DWindow(const std::string &name, int width, int height,
                           int pos_x, int pos_y, int buffer_length)
    : Window(WindowType::Plot2, name, width, height, pos_x, pos_y),
      buffer_length_(std::max(buffer_length, 1)),
      autoscale_(true),
      ptr_count_(0),
      count_sat_(false),
      step_(1),
      batch_size_(1),
      x_data_(nullptr),
      y_data_(nullptr)
{
  xSource_ = DataSource{0, BufferType::Data, nullptr, nullptr, 0, 0, EP_8U};
  ySource_ = DataSource{0, BufferType::Data, nullptr, nullptr, 0, 0, EP_8U};

  // Allocate memory
  x_data_ = new float[buffer_length_];
  y_data_ = new float[buffer_length_];

  // Check if memory allocation was successful (mostly for older compilers)
  if (!x_data_ || !y_data_) {
    std::cerr << "Memory allocation failed in Plot2DWindow constructor!"
              << std::endl;
    delete[] x_data_;
    delete[] y_data_;
    x_data_ = nullptr;
    y_data_ = nullptr;
    return;  // Stop further execution
  }

  // Initialize x_data_ and y_data_
  for (int i = 0; i < buffer_length_; i++) {
    x_data_[i] = static_cast<float>(i);
    y_data_[i] = 0.0f;
  }

  x0_ = x_data_[0];
  x1_ = x_data_[buffer_length_ - 1];
  y0_ = 0;
  y1_ = 500;

  pan_x_ = 0.0;
  pan_y_ = 0.0;
  raster_x_ = x0_ + pan_x_;
  raster_y_ = y0_ + pan_y_;
  range_x_ = safe_range(x0_, x1_);
  range_y_ = safe_range(y0_, y1_);
  gamma_x_ = static_cast<float>(width_) / range_x_;
  gamma_y_ = static_cast<float>(height_) / range_y_;
  zoom_factor_ = 1.0;

  // OpenGL Window Initialization (only if id_ is valid)
  if (id_ > 0) {
    glutSetWindow(id_);
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glViewport(0, 0, width_, height_);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(raster_x_, raster_x_ + range_x_, raster_y_,
               raster_y_ + range_y_);
  }
}

Plot2DWindow::~Plot2DWindow()
{
  delete[] y_data_;
  delete[] x_data_;
}

int32_t Plot2DWindow::setXSource(Queue *queue, int item,
                                 Window::BufferType bufferType)
{
  // Check for null queue
  if (!queue) {
    std::cerr << "setXSource: Queue is null" << std::endl;
    return -1;
  }

  // Ensure a reader exists for the queue
  if (readers_.find(queue) == readers_.end()) {
    readers_[queue] = std::make_unique<QueueReader>(queue);
  }

  QueueReader &reader = *readers_[queue];

  // Get the schema based on the buffer type
  Node *nn = (bufferType == BufferType::Data) ? reader.dataSchema()->item(item)
                                              : reader.hdrSchema()->item(item);

  if (!nn) {
    std::cerr << "setXSource: Schema item is null" << std::endl;
    return -1;
  }

  // Verify the schema node is a DataNode
  DataNode *dn = nullptr;
  if (nn->isDataNode()) {
    dn = static_cast<DataNode *>(nn);
  }
  else {
    std::cerr << "setXSource: Item is not a DataNode" << std::endl;
    return -1;
  }

  size_t dn_offset = (bufferType == BufferType::Data)
                         ? reader.dataSchema()->streamedNodeOffset(dn)
                         : reader.hdrSchema()->streamedNodeOffset(dn);

  // Calculate message size based on buffer type
  size_t msg_size = (bufferType == BufferType::Data)
                        ? reader.dataSchema()->size()
                        : reader.hdrSchema()->size();

  // Assign the data source
  xSource_ = {item,      bufferType, readers_[queue].get(), dn,
              dn_offset, msg_size,   dn->datatype()};

  return 0;  // Success
}

int32_t Plot2DWindow::setYSource(Queue *queue, int item,
                                 Window::BufferType bufferType)
{
  // Check for null queue
  if (!queue) {
    std::cerr << "setYSource: Queue is null" << std::endl;
    return -1;
  }

  // Ensure a reader exists for the queue
  if (readers_.find(queue) == readers_.end()) {
    readers_[queue] = std::make_unique<QueueReader>(queue);
  }

  QueueReader &reader =
      *readers_[queue];  // Dereference unique_ptr for readability

  // Get the schema based on the buffer type
  Node *nn = (bufferType == BufferType::Data) ? reader.dataSchema()->item(item)
                                              : reader.hdrSchema()->item(item);

  if (!nn) {
    std::cerr << "setYSource: Schema item is null" << std::endl;
    return -1;
  }

  // Verify the schema node is a DataNode
  DataNode *dn = nullptr;
  if (nn->isDataNode()) {
    dn = static_cast<DataNode *>(nn);
  }
  else {
    std::cerr << "setYSource: Item is not a DataNode" << std::endl;
    return -1;
  }

  size_t dn_offset = (bufferType == BufferType::Data)
                         ? reader.dataSchema()->streamedNodeOffset(dn)
                         : reader.hdrSchema()->streamedNodeOffset(dn);

  // Calculate message size based on buffer type
  size_t msg_size = (bufferType == BufferType::Data)
                        ? reader.dataSchema()->size()
                        : reader.hdrSchema()->size();

  // Assign the data source
  ySource_ = {item,      bufferType, readers_[queue].get(), dn,
              dn_offset, msg_size,   dn->datatype()};

  return 0;  // Success
}

void Plot2DWindow::setBlocking(bool blocking)
{
  if (ySource_.reader != nullptr) ySource_.reader->setBlockingCalls(blocking);
  if (xSource_.reader != nullptr) xSource_.reader->setBlockingCalls(blocking);
}

void Plot2DWindow::_update()
{
  if (glutGetWindow() == 0) {
    read_success_ = false;
    is_active_ = false;
    return;
  }

  glutSetWindow(id_);

  read_success_ = false;

  bool read_started = false;

  auto abort_started_reads = [&]() {
    if (!read_started) return;
    for (auto &[queue, reader] : readers_) {
      (void)queue;
      reader->endReadAbort();
    }
  };

  if (!ySource_.reader || !ySource_.dn) {
    std::cerr << "ERROR: Y source is not configured." << std::endl;
    return;
  }

  if (step_ <= 0 || batch_size_ <= 0) {
    std::cerr << "ERROR: Invalid batch_size or step." << std::endl;
    return;
  }

  for (auto &[queue, reader] : readers_) {
    (void)queue;
    const int err = reader->startRead(batch_size_, batch_size_);
    if (err < 0) {
      abort_started_reads();
      return;
    }
  }

  read_started = true;

  QueueReader *reader_y = ySource_.reader;
  DataNode *dn_y = ySource_.dn;
  const size_t dn_y_offset = ySource_.dn_offset;
  const BaseType item_y_basetype = ySource_.base_type;

  if (!reader_y || !dn_y) {
    std::cerr << "ERROR: Null reader or DataNode detected for Y data!"
              << std::endl;
    abort_started_reads();
    return;
  }

  const int len_a_y = reader_y->lenA() / step_;
  const int len_b_y = reader_y->lenB() / step_;
  const int total_y = len_a_y + len_b_y;

  if (len_a_y < 0 || len_b_y < 0 || ptr_count_ + total_y > buffer_length_) {
    std::cerr << "ERROR: Y message length exceeds buffer limits!" << std::endl;
    abort_started_reads();
    return;
  }

  char *base_ptr = (ySource_.buffer_type == BufferType::Data)
                       ? reader_y->dataPtrA()
                       : reader_y->hdrPtrA();

  if (!base_ptr) {
    std::cerr << "ERROR: Invalid buffer pointer A for Y data!" << std::endl;
    abort_started_reads();
    return;
  }

  convertToFloat(&y_data_[ptr_count_], base_ptr + dn_y_offset, item_y_basetype,
                 len_a_y, step_ * ySource_.msg_size);

  base_ptr = (ySource_.buffer_type == BufferType::Data) ? reader_y->dataPtrB()
                                                        : reader_y->hdrPtrB();

  if (!base_ptr) {
    std::cerr << "ERROR: Invalid buffer pointer B for Y data!" << std::endl;
    abort_started_reads();
    return;
  }

  convertToFloat(&y_data_[ptr_count_ + len_a_y], base_ptr + dn_y_offset,
                 item_y_basetype, len_b_y, step_ * ySource_.msg_size);

  // --- X Data Processing ---
  if (xSource_.reader != nullptr) {
    QueueReader *reader_x = xSource_.reader;
    DataNode *dn_x = xSource_.dn;
    const size_t dn_x_offset = xSource_.dn_offset;
    const BaseType item_x_basetype = xSource_.base_type;

    if (!reader_x || !dn_x) {
      std::cerr << "ERROR: Null reader or DataNode detected for X data!"
                << std::endl;
      abort_started_reads();
      return;
    }

    const int len_a_x = reader_x->lenA() / step_;
    const int len_b_x = reader_x->lenB() / step_;
    const int total_x = len_a_x + len_b_x;

    if (len_a_x < 0 || len_b_x < 0 || ptr_count_ + total_x > buffer_length_) {
      std::cerr << "ERROR: X message length exceeds buffer limits!"
                << std::endl;
      abort_started_reads();
      return;
    }

    base_ptr = (xSource_.buffer_type == BufferType::Data) ? reader_x->dataPtrA()
                                                          : reader_x->hdrPtrA();

    if (!base_ptr) {
      std::cerr << "ERROR: Invalid buffer pointer A for X data!" << std::endl;
      abort_started_reads();
      return;
    }

    convertToFloat(&x_data_[ptr_count_], base_ptr + dn_x_offset,
                   item_x_basetype, len_a_x, step_ * xSource_.msg_size);

    base_ptr = (xSource_.buffer_type == BufferType::Data) ? reader_x->dataPtrB()
                                                          : reader_x->hdrPtrB();

    if (!base_ptr) {
      std::cerr << "ERROR: Invalid buffer pointer B for X data!" << std::endl;
      abort_started_reads();
      return;
    }

    // Important fix: append B after A instead of overwriting A.
    convertToFloat(&x_data_[ptr_count_ + len_a_x], base_ptr + dn_x_offset,
                   item_x_basetype, len_b_x, step_ * xSource_.msg_size);
  }
  else {
    // No explicit X source: keep X as sample index.
    for (int i = 0; i < total_y; ++i) {
      x_data_[ptr_count_ + i] = static_cast<float>(ptr_count_ + i);
    }
  }

  const int new_ptr_count = ptr_count_ + total_y;

  if (new_ptr_count >= buffer_length_) {
    ptr_count_ = 0;
    count_sat_ = true;
  }
  else {
    ptr_count_ = new_ptr_count;
  }

  for (auto &[queue, reader] : readers_) {
    (void)queue;
    reader->endRead();
  }

  read_started = false;
  read_success_ = true;

  if (autoscale_) {
    const int limit = count_sat_ ? buffer_length_ : ptr_count_;

    if (limit > 0) {
      float min_y = y_data_[0];
      float max_y = y_data_[0];
      float min_x = x_data_[0];
      float max_x = x_data_[0];

      for (int i = 0; i < limit; i++) {
        min_y = std::min(min_y, y_data_[i]);
        max_y = std::max(max_y, y_data_[i]);
        min_x = std::min(min_x, x_data_[i]);
        max_x = std::max(max_x, x_data_[i]);
      }

      expand_degenerate_range(min_x, max_x);
      expand_degenerate_range(min_y, max_y);

      x0_ = min_x;
      x1_ = max_x;
      y0_ = min_y;
      y1_ = max_y;

      zoom_factor_ = 1.0f;
      pan_x_ = 0.0f;
      pan_y_ = 0.0f;

      range_x_ = safe_range(x0_, x1_);
      range_y_ = safe_range(y0_, y1_);

      gamma_x_ = static_cast<float>(width_) / range_x_;
      gamma_y_ = static_cast<float>(height_) / range_y_;

      raster_x_ = x0_ + pan_x_;
      raster_y_ = y0_ + pan_y_;
    }
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(raster_x_, raster_x_ + range_x_, raster_y_, raster_y_ + range_y_);
}

void Plot2DWindow::_display()
{
  //  printf("display plot2 %d\n", id_);
  glutSetWindow(id_);

  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw axes with ticks
  drawAxis();

  // Draw data points
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_LINE_STRIP);

  if (xSource_.reader) {
    int loop_count = count_sat_ ? buffer_length_ : ptr_count_;
    for (int i = 0; i < loop_count; i++) {
      glVertex2f(x_data_[i], y_data_[i]);
    }
  }
  else {
    int ii = 0;
    if (count_sat_ == false) {
      for (int i = 0; i < ptr_count_; i++) {
        glVertex2f(x_data_[i], y_data_[i]);
      }
    }
    else {
      for (int i = ptr_count_; i < buffer_length_; i++) {
        glVertex2f(x_data_[ii++], y_data_[i]);
      }
      for (int i = 0; i < ptr_count_; i++) {
        glVertex2f(x_data_[ii++], y_data_[i]);
      }
    }
  }

  glEnd();

  glutSwapBuffers();
}

void Plot2DWindow::_reshape(int width, int height)
{
  glutSetWindow(id_);

  width_ = width;
  height_ = height;

  if (programatic_resize_)
    programatic_resize_ = false;
  else
    autoscale_ = false;

  if (std::abs(gamma_x_) > 1e-12f)
    range_x_ = static_cast<float>(width_) / gamma_x_;
  else
    range_x_ = safe_range(x0_, x1_);

  if (std::abs(gamma_y_) > 1e-12f)
    range_y_ = static_cast<float>(height_) / gamma_y_;
  else
    range_y_ = safe_range(y0_, y1_);

  glViewport(0, 0, width_, height_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(raster_x_, raster_x_ + range_x_, raster_y_, raster_y_ + range_y_);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glutPostRedisplay();
}

void Plot2DWindow::_mouse(int button, int state, int x, int y)
{
  glutSetWindow(id_);
  y = height_ - y - 1;

  if (state == GLUT_DOWN) {
    prev_x_ = static_cast<float>(x);
    prev_y_ = static_cast<float>(y);
  }
  else if (button == GLUT_MIDDLE_BUTTON) {
    autoscale_ = true;
  }
  else if (button == 3) {
    zoom(1.3f, x, y);
  }
  else if (button == 4) {
    zoom(1 / 1.3f, x, y);
  }
}

void Plot2DWindow::_motion(int x, int y)
{
  glutSetWindow(id_);
  y = height_ - y - 1;

  raster_x_ -= pan_x_;
  raster_y_ -= pan_y_;

  pan_x_ -= ((static_cast<float>(x) - prev_x_) / gamma_x_);
  pan_y_ -= ((static_cast<float>(y) - prev_y_) / gamma_y_);

  prev_x_ = static_cast<float>(x);
  prev_y_ = static_cast<float>(y);

  raster_x_ += pan_x_;
  raster_y_ += pan_y_;

  autoscale_ = false;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(raster_x_, raster_x_ + range_x_, raster_y_, raster_y_ + range_y_);
  glutPostRedisplay();
}

void Plot2DWindow::_motionOver(int /*x*/, int /*y*/)
{
}

void Plot2DWindow::_keyboard(unsigned char /*key*/, int /*x*/, int /*y*/)
{
}

void Plot2DWindow::_idle()
{
}

void Plot2DWindow::zoom(float factor, int /*x*/, int /*y*/)
{
  glutSetWindow(id_);

  if (!(factor > 0.0f) || !std::isfinite(factor)) return;

  zoom_factor_ *= factor;
  zoom_factor_ = std::max(zoom_factor_, 1e-6f);

  autoscale_ = false;

  const float p_range_x = range_x_;
  const float p_range_y = range_y_;

  range_x_ = safe_range(x0_, x1_) / zoom_factor_;
  range_y_ = safe_range(y0_, y1_) / zoom_factor_;

  range_x_ = std::max(range_x_, 1e-6f);
  range_y_ = std::max(range_y_, 1e-6f);

  gamma_x_ = static_cast<float>(width_) / range_x_;
  gamma_y_ = static_cast<float>(height_) / range_y_;

  const float center_x = raster_x_ + p_range_x / 2.0f;
  const float center_y = raster_y_ + p_range_y / 2.0f;

  raster_x_ = center_x - range_x_ / 2.0f;
  raster_y_ = center_y - range_y_ / 2.0f;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(raster_x_, raster_x_ + range_x_, raster_y_, raster_y_ + range_y_);

  glutPostRedisplay();
}

void Plot2DWindow::drawAxis()
{
  glColor3f(0.0f, 0.0f, 0.0f);  // Black color for axes
  glLineWidth(1.0f);

  // Draw X-axis
  float y_center = static_cast<float>(0.5 * (y0_ + y1_));
  float x_center = static_cast<float>(0.5 * (x0_ + x1_));

  glBegin(GL_LINES);
  glVertex2f(x0_, y_center);
  glVertex2f(x1_, y_center);
  glEnd();

  // Draw Y-axis
  glBegin(GL_LINES);
  glVertex2f(x_center, y0_);
  glVertex2f(x_center, y1_);
  glEnd();

  // Draw ticks and labels
  float major_tick_step_x =
      static_cast<float>((x1_ - x0_) / static_cast<float>(major_tick_count_));
  float major_tick_step_y =
      static_cast<float>((y1_ - y0_) / static_cast<float>(major_tick_count_));
  float minor_tick_step_x = static_cast<float>(
      major_tick_step_x / static_cast<float>(minor_tick_count_));
  float minor_tick_step_y = static_cast<float>(
      major_tick_step_y / static_cast<float>(minor_tick_count_));

  // Draw X-axis ticks
  for (int i = 0; i <= major_tick_count_; ++i) {
    float x =
        static_cast<float>(x0_) + static_cast<float>(i) * major_tick_step_x;

    // Major tick
    glBegin(GL_LINES);
    glVertex2f(x, y_center - tick_length_);
    glVertex2f(x, y_center + tick_length_);
    glEnd();

    // Label for major tick
    std::ostringstream label;
    label << std::fixed << std::setprecision(2) << x;
    draw_text(x, y_center - 2 * tick_length_, label.str());

    // Minor ticks
    for (int j = 1; j < minor_tick_count_; ++j) {
      float x_minor = x + static_cast<float>(j) * minor_tick_step_x;
      glBegin(GL_LINES);
      glVertex2f(x_minor, y_center - tick_length_ / 2);
      glVertex2f(x_minor, y_center + tick_length_ / 2);
      glEnd();
    }
  }

  // Draw Y-axis ticks
  for (int i = 0; i <= major_tick_count_; ++i) {
    float y = y0_ + static_cast<float>(i) * major_tick_step_y;

    // Major tick
    glBegin(GL_LINES);
    glVertex2f(x_center - tick_length_, y);
    glVertex2f(x_center + tick_length_, y);
    glEnd();

    // Label for major tick
    std::ostringstream label;

    label << std::fixed << std::setprecision(2) << y;
    draw_text(x_center - 2 * tick_length_, y, label.str());

    // Minor ticks
    for (int j = 1; j < minor_tick_count_; ++j) {
      float y_minor = y + static_cast<float>(j) * minor_tick_step_y;
      glBegin(GL_LINES);
      glVertex2f(x_center - tick_length_ / 2, y_minor);
      glVertex2f(x_center + tick_length_ / 2, y_minor);
      glEnd();
    }
  }
}

void Plot2DWindow::setYRange(float min_y, float max_y)
{
  y0_ = min_y;
  y1_ = max_y;

  expand_degenerate_range(y0_, y1_);

  raster_x_ = x0_ + pan_x_;
  raster_y_ = y0_ + pan_y_;

  range_x_ = safe_range(x0_, x1_);
  range_y_ = safe_range(y0_, y1_);

  gamma_x_ = static_cast<float>(width_) / range_x_;
  gamma_y_ = static_cast<float>(height_) / range_y_;

  autoscale_ = false;
}

void Plot2DWindow::setBatchSize(int batch_size, int step)
{
  if (batch_size <= 0 || step <= 0) {
    std::cerr << "Error: batch_size and step must be positive integers."
              << std::endl;
    return;
  }

  if (step > batch_size) {
    std::cerr << "Error: step must be <= batch_size." << std::endl;
    return;
  }

  const int effective_batch_size = batch_size / step;

  if (effective_batch_size <= 0) {
    std::cerr << "Error: invalid effective batch size." << std::endl;
    return;
  }

  if (buffer_length_ % effective_batch_size != 0) {
    std::cerr
        << "Error: buffer_length_ must be a multiple of batch_size / step."
        << std::endl;
    return;
  }

  batch_size_ = batch_size;
  step_ = step;

  std::cout << "Batch size set to " << batch_size_ << ", Step size set to "
            << step_ << std::endl;
}

///////////////////////////////////////////////////////////
// Plot3DWindow : public Window
///////////////////////////////////////////////////////////

Plot3DWindow::Plot3DWindow(const std::string &name, int width, int height,
                           int pos_x, int pos_y, int buffer_length)
    : Window(WindowType::Plot3, name, width, height, pos_x, pos_y),
      buffer_length_(buffer_length),
      autoscale_(true),
      camera_angle_x_(0.0f),
      camera_angle_y_(0.0f),
      camera_distance_(10.0f),
      pan_x_(0.0f),
      pan_y_(0.0f),
      is_rotating_(false),
      is_panning_(false),
      is_zooming_(false),
      ptr_count_(0),
      count_sat_(false),
      step_(1),
      batch_size_(1)
{
  // Allocate memory and check for failures (only needed for older compilers)
  for (int i = 0; i < 4; i++) {
    data_[i] = new float[buffer_length_];
    if (!data_[i]) {
      std::cerr << "Memory allocation failed in Plot3DWindow constructor!"
                << std::endl;
      // Free previously allocated memory and exit constructor
      for (int j = 0; j < i; j++) {
        delete[] data_[j];
        data_[j] = nullptr;
      }
      return;
    }
    // Initialize to 0
    for (int j = 0; j < buffer_length_; j++) {
      data_[i][j] = 0.0f;
    }
  }

  initializeColormap();

  for (int i = 0; i < 4; ++i) {
    source_[i] = DataSource{};
  }

  resetBounds();

  // OpenGL Window Initialization (only if id_ is valid)
  if (id_ > 0) {
    glutSetWindow(id_);
  }
}

void Plot3DWindow::resetBounds()
{
  min_x_ = std::numeric_limits<float>::max();
  max_x_ = std::numeric_limits<float>::lowest();

  min_y_ = std::numeric_limits<float>::max();
  max_y_ = std::numeric_limits<float>::lowest();

  min_z_ = std::numeric_limits<float>::max();
  max_z_ = std::numeric_limits<float>::lowest();

  min_v_ = std::numeric_limits<float>::max();
  max_v_ = std::numeric_limits<float>::lowest();
}

Plot3DWindow::~Plot3DWindow()
{
  for (int i = 0; i < 4; i++) {
    delete[] data_[i];
  }
}

int32_t Plot3DWindow::setSource(int source_index, Queue *queue, int item,
                                BufferType buf_type)
{
  if (source_index < 0 || source_index >= 4) {
    std::cerr << "setSource: invalid source index " << source_index
              << std::endl;
    return -1;
  }

  if (!queue) {
    std::cerr << "setSource: Queue is null" << std::endl;
    return -1;
  }

  if (readers_.find(queue) == readers_.end()) {
    readers_[queue] = std::make_unique<QueueReader>(queue);
  }

  QueueReader &reader = *readers_[queue];

  Node *nn = (buf_type == BufferType::Data) ? reader.dataSchema()->item(item)
                                            : reader.hdrSchema()->item(item);

  if (!nn) {
    std::cerr << "setSource: Schema item is null" << std::endl;
    return -1;
  }

  DataNode *dn = nullptr;
  if (nn->isDataNode()) {
    dn = static_cast<DataNode *>(nn);
  }
  else {
    std::cerr << "setSource: Item is not a DataNode" << std::endl;
    return -1;
  }

  size_t dn_offset = (buf_type == BufferType::Data)
                         ? reader.dataSchema()->streamedNodeOffset(dn)
                         : reader.hdrSchema()->streamedNodeOffset(dn);

  size_t msg_size = (buf_type == BufferType::Data) ? reader.dataSchema()->size()
                                                   : reader.hdrSchema()->size();

  source_[source_index] = {item,      buf_type, readers_[queue].get(), dn,
                           dn_offset, msg_size, dn->datatype()};

  return 0;
}

int32_t Plot3DWindow::setXSource(Queue *queue, int item, BufferType buf_type)
{
  return setSource(0, queue, item, buf_type);
}

int32_t Plot3DWindow::setYSource(Queue *queue, int item, BufferType buf_type)
{
  return setSource(1, queue, item, buf_type);
}

int32_t Plot3DWindow::setZSource(Queue *queue, int item, BufferType buf_type)
{
  return setSource(2, queue, item, buf_type);
}

int32_t Plot3DWindow::setVSource(Queue *queue, int item, BufferType buf_type)
{
  return setSource(3, queue, item, buf_type);
}

bool Plot3DWindow::sourceConfigured(int source_index) const
{
  if (source_index < 0 || source_index >= 4) return false;
  return source_[source_index].reader != nullptr &&
         source_[source_index].dn != nullptr;
}

bool Plot3DWindow::requiredSourcesConfigured() const
{
  // X, Y, and Z are required. V is optional.
  return sourceConfigured(0) && sourceConfigured(1) && sourceConfigured(2);
}

void Plot3DWindow::setBatchSize(int batch_size, int step)
{
  if (batch_size <= 0 || step <= 0) {
    std::cerr << "Error: batch_size and step must be positive integers."
              << std::endl;
    return;
  }

  if (step > batch_size) {
    std::cerr << "Error: step must be <= batch_size." << std::endl;
    return;
  }

  const int effective_batch_size = batch_size / step;

  if (effective_batch_size <= 0) {
    std::cerr << "Error: invalid effective batch size." << std::endl;
    return;
  }

  if (buffer_length_ % effective_batch_size != 0) {
    std::cerr
        << "Error: buffer_length_ must be a multiple of batch_size / step."
        << std::endl;
    return;
  }

  batch_size_ = batch_size;
  step_ = step;

  std::cout << "Batch size set to " << batch_size_ << ", Step size set to "
            << step_ << std::endl;
}

void Plot3DWindow::recomputeBounds(int limit)
{
  resetBounds();

  if (limit <= 0) {
    min_x_ = min_y_ = min_z_ = min_v_ = 0.0f;
    max_x_ = max_y_ = max_z_ = max_v_ = 1.0f;
    return;
  }

  const bool has_v = sourceConfigured(3);

  for (int i = 0; i < limit; ++i) {
    const float x = data_[0][i];
    const float y = data_[1][i];
    const float z = data_[2][i];
    const float v = has_v ? data_[3][i] : z;

    if (std::isfinite(x)) {
      min_x_ = std::min(min_x_, x);
      max_x_ = std::max(max_x_, x);
    }

    if (std::isfinite(y)) {
      min_y_ = std::min(min_y_, y);
      max_y_ = std::max(max_y_, y);
    }

    if (std::isfinite(z)) {
      min_z_ = std::min(min_z_, z);
      max_z_ = std::max(max_z_, z);
    }

    if (std::isfinite(v)) {
      min_v_ = std::min(min_v_, v);
      max_v_ = std::max(max_v_, v);
    }
  }

  expand_degenerate_range(min_x_, max_x_);
  expand_degenerate_range(min_y_, max_y_);
  expand_degenerate_range(min_z_, max_z_);
  expand_degenerate_range(min_v_, max_v_);
}

void Plot3DWindow::setBlocking(bool blocking)
{
  for (int i = 0; i < 4; i++)
    if (source_[i].reader != nullptr)
      source_[i].reader->setBlockingCalls(blocking);
}

void Plot3DWindow::_update()
{
  glutSetWindow(id_);

  read_success_ = false;

  if (!requiredSourcesConfigured()) {
    std::cerr << "ERROR: Plot3DWindow requires X, Y, and Z sources."
              << std::endl;
    return;
  }

  if (step_ <= 0 || batch_size_ <= 0) {
    std::cerr << "ERROR: Invalid batch_size or step." << std::endl;
    return;
  }

  bool read_started = false;

  auto abort_started_reads = [&]() {
    if (!read_started) return;
    for (auto &[queue, reader] : readers_) {
      (void)queue;
      reader->endReadAbort();
    }
  };

  for (auto &[queue, reader] : readers_) {
    (void)queue;
    const int err = reader->startRead(batch_size_, batch_size_);
    if (err < 0) {
      abort_started_reads();
      return;
    }
  }

  read_started = true;

  const int source_count = sourceConfigured(3) ? 4 : 3;
  int expected_total = -1;

  for (int i = 0; i < source_count; i++) {
    DataSource &ds = source_[i];

    QueueReader *reader = ds.reader;
    DataNode *dn = ds.dn;

    if (!reader || !dn) {
      std::cerr << "ERROR: Null reader or DataNode detected for source " << i
                << std::endl;
      abort_started_reads();
      return;
    }

    const int len_a = reader->lenA() / step_;
    const int len_b = reader->lenB() / step_;
    const int total = len_a + len_b;

    if (len_a < 0 || len_b < 0 || total < 0 ||
        ptr_count_ + total > buffer_length_) {
      std::cerr << "ERROR: message length exceeds buffer limits for source "
                << i << std::endl;
      abort_started_reads();
      return;
    }

    if (expected_total < 0) {
      expected_total = total;
    }
    else if (total != expected_total) {
      std::cerr << "ERROR: Plot3D source lengths do not match." << std::endl;
      abort_started_reads();
      return;
    }

    char *base_ptr = (ds.buffer_type == BufferType::Data) ? reader->dataPtrA()
                                                          : reader->hdrPtrA();

    if (!base_ptr) {
      std::cerr << "ERROR: Invalid buffer pointer A for source " << i
                << std::endl;
      abort_started_reads();
      return;
    }

    convertToFloat(&data_[i][ptr_count_], base_ptr + ds.dn_offset, ds.base_type,
                   len_a, step_ * ds.msg_size);

    base_ptr = (ds.buffer_type == BufferType::Data) ? reader->dataPtrB()
                                                    : reader->hdrPtrB();

    if (!base_ptr) {
      std::cerr << "ERROR: Invalid buffer pointer B for source " << i
                << std::endl;
      abort_started_reads();
      return;
    }

    convertToFloat(&data_[i][ptr_count_ + len_a], base_ptr + ds.dn_offset,
                   ds.base_type, len_b, step_ * ds.msg_size);
  }

  if (!sourceConfigured(3)) {
    // If V is not configured, use Z as the color/value channel.
    for (int i = 0; i < expected_total; ++i) {
      data_[3][ptr_count_ + i] = data_[2][ptr_count_ + i];
    }
  }

  const int new_ptr_count = ptr_count_ + expected_total;

  if (new_ptr_count >= buffer_length_) {
    ptr_count_ = 0;
    count_sat_ = true;
  }
  else {
    ptr_count_ = new_ptr_count;
  }

  for (auto &[queue, reader] : readers_) {
    (void)queue;
    reader->endRead();
  }

  read_started = false;
  read_success_ = true;

  if (autoscale_) {
    const int limit = count_sat_ ? buffer_length_ : ptr_count_;
    recomputeBounds(limit);

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    _reshape(w, h);
  }
}

// Function to map Z value to a color using the LUT
void get_color(float z, float &r, float &g, float &b)
{
  //  float normalizedZ = (z - minZ_) / (maxZ_ - minZ_);  // Normalize Z to
  //  [0, 1] std::vector<float> color = getColorFromLUT(normalizedZ,
  //  jetColormap);
  std::vector<float> color = getColorFromLUT(z, jetColormap);
  r = color[0];
  g = color[1];
  b = color[2];
}

void Plot3DWindow::_display()
{
  glutSetWindow(id_);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Compute bounding box center
  float center_x = (min_x_ + max_x_) / 2.0f;
  float center_y = (min_y_ + max_y_) / 2.0f;
  float center_z = (min_z_ + max_z_) / 2.0f;

  float size_x = std::max(max_x_ - min_x_, 1e-6f);
  float size_y = std::max(max_y_ - min_y_, 1e-6f);
  float size_z = std::max(max_z_ - min_z_, 1e-6f);
  float size = std::max({size_x, size_y, size_z});

  if (autoscale_) {
    // camera distance:
    camera_distance_ = std::max(size * 2.0f, 1.0f);

    // pan / rotation:
    camera_angle_x_ = 0.0f;  // Reset X rotation
    camera_angle_y_ = 0.0f;  // Reset Y rotation
    pan_x_ = 0.0f;           // Reset panning in X
    pan_y_ = 0.0f;           // Reset panning in Y
  }

  // Adjust the camera based on user input
  gluLookAt(center_x + pan_x_, center_y + pan_y_, center_z + camera_distance_,
            center_x + pan_x_, center_y + pan_y_, center_z, 0, 1, 0);

  // Apply rotation
  glRotatef(camera_angle_x_, 1, 0, 0);
  glRotatef(camera_angle_y_, 0, 1, 0);

  // Draw points
  //  std::cout << "count sat: " << count_sat_ << std::endl;

  glBegin(GL_POINTS);
  float r, g, b;
  int loop_count = count_sat_ ? buffer_length_ : ptr_count_;

  const float range_v = std::max(max_v_ - min_v_, 1e-12f);

  for (int i = 0; i < loop_count; i++) {
    const float normalized_v =
        std::clamp((data_[3][i] - min_v_) / range_v, 0.0f, 1.0f);

    get_color(normalized_v, r, g, b);
    glColor3f(r, g, b);
    glVertex3f(data_[0][i], data_[1][i], data_[2][i]);
  }
  glEnd();

  glutSwapBuffers();
}

void Plot3DWindow::_reshape(int w, int h)
{
  glutSetWindow(id_);

  w = std::max(w, 1);
  h = std::max(h, 1);

  glViewport(0, 0, w, h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  const float size = getBoundingBoxSize();
  const float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);

  const float near_plane = 0.01f;
  const float far_plane = std::max(size * 10.0f, near_plane + 1.0f);

  gluPerspective(45.0, aspect_ratio, near_plane, far_plane);

  glutPostRedisplay();
}

void Plot3DWindow::resetCamera()
{
  camera_distance_ = 10.0f;  // Default distance (adjust as needed)
  camera_angle_x_ = 0.0f;    // Reset X rotation
  camera_angle_y_ = 0.0f;    // Reset Y rotation
  pan_x_ = 0.0f;             // Reset panning in X
  pan_y_ = 0.0f;             // Reset panning in Y
  autoscale_ = true;         // Enable autoscale to adjust distance
}

float Plot3DWindow::getBoundingBoxSize()
{
  const float size_x = safe_range(min_x_, max_x_, 1.0f);
  const float size_y = safe_range(min_y_, max_y_, 1.0f);
  const float size_z = safe_range(min_z_, max_z_, 1.0f);

  return std::max({size_x, size_y, size_z, 1.0f});
}

void Plot3DWindow::_mouse(int button, int state, int x, int y)
{
  last_mouse_x_ = x;
  last_mouse_y_ = y;

  if (state == GLUT_DOWN) {
    if (button == GLUT_LEFT_BUTTON) {
      is_panning_ = true;
      autoscale_ = false;
    }
    else if (button == GLUT_MIDDLE_BUTTON) {
      //      resetCamera();  // Reset camera to default position and
      //      orientation
      autoscale_ = true;
      glutPostRedisplay();
    }
    else if (button == GLUT_RIGHT_BUTTON) {
      is_rotating_ = true;
      autoscale_ = false;
    }
    else if (button == 3) {  // Mouse wheel up
      float size = getBoundingBoxSize();
      camera_distance_ -=
          size * 0.1f;  // Zoom step relative to bounding box size
      camera_distance_ = std::max(camera_distance_, 1.0f);
      autoscale_ = false;
      glutPostRedisplay();
    }
    else if (button == 4) {  // Mouse wheel down
      float size = getBoundingBoxSize();
      camera_distance_ +=
          size * 0.1f;  // Zoom step relative to bounding box size
      autoscale_ = false;
      glutPostRedisplay();
    }
  }
  else if (state == GLUT_UP) {
    is_panning_ = false;
    is_rotating_ = false;
  }
}

void Plot3DWindow::_motion(int x, int y)
{
  int dx = x - last_mouse_x_;
  int dy = y - last_mouse_y_;

  if (is_panning_) {
    float size = getBoundingBoxSize();
    pan_x_ -= static_cast<float>(dx) * size * 0.002f;
    pan_y_ += static_cast<float>(dy) * size * 0.002f;
    // pan_x_ -= dx * 0.01f;
    // pan_y_ += dy * 0.01f;
  }
  else if (is_rotating_) {
    camera_angle_x_ += static_cast<float>(dy) * 0.5f;
    camera_angle_y_ += static_cast<float>(dx) * 0.5f;
  }

  last_mouse_x_ = x;
  last_mouse_y_ = y;
  glutPostRedisplay();
}

void Plot3DWindow::_motionOver(int /*x*/, int /*y*/)
{
}
void Plot3DWindow::_keyboard(unsigned char /*key*/, int /*x*/, int /*y*/)
{
}
void Plot3DWindow::_idle()
{
}

// // BMP SCREENSHOT
// #include <vector>
// #include <fstream>

// #pragma pack(push, 1)  // Ensure no padding for BMP structures
// struct BMPHeader {
//   uint16_t fileType{0x4D42}; // "BM"
//   uint32_t fileSize{0};      // Size of file in bytes
//   uint16_t reserved1{0};
//   uint16_t reserved2{0};
//   uint32_t offsetData{54};   // Offset to image data
// };

// struct BMPInfoHeader {
//   uint32_t size{40};         // Size of this header
//   int32_t width{0};          // Image width
//   int32_t height{0};         // Image height (negative for top-down)
//   uint16_t planes{1};
//   uint16_t bitCount{24};     // 24-bit color
//   uint32_t compression{0};   // No compression
//   uint32_t sizeImage{0};     // Image size (can be 0 for uncompressed)
//   int32_t xPixelsPerMeter{0};
//   int32_t yPixelsPerMeter{0};
//   uint32_t colorsUsed{0};
//   uint32_t colorsImportant{0};
// };
// #pragma pack(pop)  // Restore default struct alignment

// int32_t Plot3DWindow::saveFrameBuffer(std::string filename) {
//   int width = glutGet(GLUT_WINDOW_WIDTH);
//   int height = glutGet(GLUT_WINDOW_HEIGHT);

//   if (windowResized) {
//     printf("Window was resized: (%d, %d)\n", currentWidth, currentHeight);
//     windowResized = false;
//   } else {
//     printf("No reshape occurred.\n");
//   }

//   // Allocate memory for pixel data (GL_BGR format for BMP compatibility)
//   std::vector<unsigned char> pixels(width * height * 3);
//   glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

//   // Create BMP headers
//   BMPHeader fileHeader;
//   BMPInfoHeader infoHeader;
//   fileHeader.fileSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + (width *
//   height * 3); infoHeader.width = width; infoHeader.height = -height;  //
//   Negative height for top-down BMP infoHeader.sizeImage = width * height * 3;

//   // Open file for writing
//   std::ofstream file(filename, std::ios::binary);
//   if (!file) {
//     perror("Error opening file for writing");
//     return -1;
//   }

//   // Write headers
//   file.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
//   file.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

//   // Write pixel data (BMP stores rows bottom-to-top, but we use negative
//   height to avoid flipping)
//   file.write(reinterpret_cast<char*>(pixels.data()), pixels.size());

//   file.close();
//   printf("Framebuffer saved to %s\n", filename.c_str());
//   return 0;
// }

///////////////////////////////////////////////////////////
// ImageWindow : public Window
///////////////////////////////////////////////////////////

ImageWindow::ImageWindow(const std::string &name, int width, int height,
                         int pos_x, int pos_y)
    : Window(WindowType::Image, name, width, height, pos_x, pos_y)
{
  reader_ = nullptr;
  image_ = nullptr;
  norm_image_ = nullptr;

  display_queue_ = nullptr;
  probe_image_ = nullptr;
  probeSource_ = DataSource{};

  probe_enabled_ = false;
  probe_hover_ = false;
  probe_click_ = false;
  probe_units_ = "";
  probe_precision_ = 2;
  probe_throttle_ms_ = 100;
  last_probe_x_ = -1;
  last_probe_y_ = -1;
  last_probe_print_ = std::chrono::steady_clock::now();

  zoom_f_ = 1.0;
  window_w_ = width;
  window_h_ = height;
  aspect_ = 1.0f;
  raster_x_ = 0;
  raster_y_ = 0;
  x_prev_ = 0;
  y_prev_ = 0;
  img_w_ = width;
  img_h_ = height;

  texture_ = 0;
  data_type_ = GL_NONE;
  internal_data_format_ = GL_NONE;
  data_format_ = GL_NONE;
  gl_info_ = {};
  upload_supported_ = false;
  float_autoscale_ = true;

  normalize_ = false;
  compute_histogram_ = false;

  hist_window_ = new HistWindow(name + " histogram", width, height / 4, pos_x,
                                pos_y + height);
  glutSetWindow(hist_window_->id());
  glutHideWindow();

  glutSetWindow(id_);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glViewport(0, 0, window_w_, window_h_);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
             window_h_ / zoom_f_ - raster_y_);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  configureTextureState();

  // Create GLUT menu
  glutCreateMenu(ImageWindow::menuCallback);
  glutAddMenuEntry("Pause/Play- [p]", 1);
  glutAddMenuEntry("Histogram on/off - [h]", 2);
  glutAddMenuEntry("Normalize image on/off- [n]", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void ImageWindow::setFloatAutoscale(bool enabled)
{
  float_autoscale_ = enabled;
}

bool ImageWindow::configureUploadFormat(const ImageObject &img)
{
  gl_info_ =
      to_gl_info(img.pixelFormat(), img.width(), img.height(), img.channels());

  if (!gl_info_.directlyUploadable) {
    upload_supported_ = false;
    data_type_ = GL_NONE;
    data_format_ = GL_NONE;
    internal_data_format_ = GL_NONE;

    std::cerr << "ImageWindow: PixelFormat " << to_string(img.pixelFormat())
              << " is not directly uploadable to a single OpenGL texture";
    if (gl_info_.requiresUnpack) std::cerr << " (requires unpack)";
    if (gl_info_.requiresColorConversion)
      std::cerr << " (requires color conversion)";
    if (gl_info_.requiresPlanarUpload)
      std::cerr << " (requires planar/multi-plane handling)";
    std::cerr << std::endl;

    return false;
  }

  data_type_ = gl_info_.externalType;
  data_format_ = gl_info_.externalFormat;
  internal_data_format_ = gl_info_.internalFormat;
  upload_supported_ = true;
  return true;
}

void ImageWindow::setProbeOptions(bool enabled, bool hover, bool click,
                                  const std::string &units, int precision,
                                  int throttle_ms)
{
  probe_enabled_ = enabled;
  probe_hover_ = hover;
  probe_click_ = click;
  probe_units_ = units;
  probe_precision_ = std::max(0, precision);
  probe_throttle_ms_ = std::max(0, throttle_ms);
}

int32_t ImageWindow::setProbeSource(Queue *queue, int item, BufferType buf_type)
{
  if (!queue) {
    std::cerr << "ImageWindow probe: Queue is null" << std::endl;
    return -1;
  }

  if (display_queue_ && queue != display_queue_) {
    std::cerr << "ImageWindow probe: probe source must use the same queue as "
                 "the displayed image. Probe disabled."
              << std::endl;
    probe_enabled_ = false;
    return -1;
  }

  if (!reader_) {
    std::cerr << "ImageWindow probe: display reader is not configured"
              << std::endl;
    probe_enabled_ = false;
    return -1;
  }

  epf::Node *n = nullptr;
  if (buf_type == BufferType::Data)
    n = reader_->dataSchema()->item(item);
  else
    n = reader_->hdrSchema()->item(item);

  if ((!n) || (!n->isObjectNode())) {
    std::cerr << "ImageWindow probe: invalid schema item" << std::endl;
    probe_enabled_ = false;
    return -1;
  }

  epf::ObjectNode *onode = static_cast<epf::ObjectNode *>(n);
  if (onode->objecttype() != epf::EP_IMAGE_RAW) {
    std::cerr << "ImageWindow probe: object is not EP_IMAGE_RAW" << std::endl;
    probe_enabled_ = false;
    return -1;
  }

  delete probe_image_;
  probe_image_ = new epf::ImageObject(onode);

  if (!probe_image_) {
    std::cerr << "ImageWindow probe: failed to create probe ImageObject"
              << std::endl;
    probe_enabled_ = false;
    return -1;
  }

  if (image_ && (probe_image_->width() != image_->width() ||
                 probe_image_->height() != image_->height())) {
    std::cerr << "ImageWindow probe: probe image dimensions do not match "
                 "display image. Probe disabled."
              << std::endl;
    delete probe_image_;
    probe_image_ = nullptr;
    probe_enabled_ = false;
    return -1;
  }

  probeSource_ = {
      item, buf_type, reader_, nullptr, 0, 0, probe_image_->baseType()};

  return 0;
}

bool ImageWindow::windowToImagePixel(int x, int y, int &px, int &py) const
{
  if (!image_ || zoom_f_ <= 0.0) return false;

  const int y_gl = window_h_ - y - 1;

  const double image_x = static_cast<double>(x) / zoom_f_ - raster_x_;
  const double image_y = static_cast<double>(y_gl) / zoom_f_ - raster_y_;

  px = static_cast<int>(std::floor(image_x));
  py = static_cast<int>(std::floor(image_y));

  return px >= 0 && py >= 0 && px < image_->width() && py < image_->height();
}

bool ImageWindow::readPixelChannels(const ImageObject &img, int px, int py,
                                    std::vector<double> &values) const
{
  if (!img.data()) return false;
  if (px < 0 || py < 0 || px >= img.width() || py >= img.height()) return false;

  const int channels = std::max(1, img.channels());
  const size_t base =
      (static_cast<size_t>(py) * static_cast<size_t>(img.width()) +
       static_cast<size_t>(px)) *
      static_cast<size_t>(channels);

  values.clear();
  values.reserve(channels);

  switch (img.baseType()) {
    case EP_8U: {
      const auto *p = static_cast<const uint8_t *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    case EP_8S: {
      const auto *p = static_cast<const int8_t *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    case EP_16U: {
      const auto *p = static_cast<const uint16_t *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    case EP_16S: {
      const auto *p = static_cast<const int16_t *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    case EP_32S: {
      const auto *p = static_cast<const int32_t *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    case EP_32F: {
      const auto *p = static_cast<const float *>(img.data());
      for (int c = 0; c < channels; ++c) {
        const float v = p[base + c];
        values.push_back(std::isfinite(v)
                             ? static_cast<double>(v)
                             : std::numeric_limits<double>::quiet_NaN());
      }
      return true;
    }

    case EP_64F: {
      const auto *p = static_cast<const double *>(img.data());
      for (int c = 0; c < channels; ++c) values.push_back(p[base + c]);
      return true;
    }

    default:
      return false;
  }
}

void ImageWindow::printProbe(int x, int y, const char *event_name,
                             bool throttle)
{
  if (!probe_enabled_) return;

  int px = 0;
  int py = 0;
  if (!windowToImagePixel(x, y, px, py)) return;

  if (throttle) {
    const auto now = std::chrono::steady_clock::now();
    const auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - last_probe_print_)
                           .count();

    if (px == last_probe_x_ && py == last_probe_y_) return;
    if (dt_ms < probe_throttle_ms_) return;

    last_probe_print_ = now;
    last_probe_x_ = px;
    last_probe_y_ = py;
  }

  const ImageObject *src = probe_image_ ? probe_image_ : image_;
  if (!src) return;

  std::vector<double> values;
  if (!readPixelChannels(*src, px, py, values)) return;

  std::cout << name_ << " probe [" << event_name << "]: x=" << px << " y=" << py
            << " value=";

  std::cout << std::fixed << std::setprecision(probe_precision_);

  if (values.size() == 1) {
    std::cout << values[0];
  }
  else {
    std::cout << "(";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << values[i];
    }
    std::cout << ")";
  }

  if (!probe_units_.empty()) std::cout << " " << probe_units_;
  std::cout << std::endl;
}

void ImageWindow::configureTextureState()
{
  glBindTexture(GL_TEXTURE_2D, texture_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef GL_CLAMP_TO_EDGE
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
}

void ImageWindow::configureTextureSwizzle(bool is_single_channel)
{
#if defined(GL_TEXTURE_SWIZZLE_RGBA)
  glBindTexture(GL_TEXTURE_2D, texture_);

  if (is_single_channel) {
    GLint swizzle_mask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
  }
  else {
    GLint swizzle_mask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
  }
#else
  (void)is_single_channel;
#endif
}

bool ImageWindow::uploadImagePixels(const ImageObject &img, const void *pixels)
{
  if (!upload_supported_) return false;
  if (!pixels) return false;

  glutSetWindow(id_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  configureTextureState();

  // Important for Mono8, RGB8, BGR8, etc. when row size is not 4-byte aligned.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  const bool single_channel =
      (gl_info_.externalFormat == GL_RED && img.channels() == 1);
  configureTextureSwizzle(single_channel);

  glTexImage2D(GL_TEXTURE_2D, 0, internal_data_format_, img.width(),
               img.height(), 0, data_format_, data_type_, pixels);

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "ImageWindow: glTexImage2D failed with error code " << err
              << " for PixelFormat " << to_string(img.pixelFormat())
              << std::endl;
    return false;
  }

  return true;
}

void ImageWindow::menuCallback(int option)
{
  switch (option) {
    case 1:
      keyboard('p');
      break;
    case 2:
      keyboard('h');
      break;
    case 3:
      keyboard('n');
      break;
    // case 3:
    //   std::cout << "Exiting histogram window..." << std::endl;
    //   glutDestroyWindow(glutGetWindow());
    //   break;
    default:
      break;
  }
}

int32_t ImageWindow::setDataSource(Queue *queue, int item, BufferType buf_type)
{
  std::cout << " setDataSource " << std::endl;

  if (!queue) {
    std::cerr << "setDataSource: Queue is null" << std::endl;
    return -1;
  }

  display_queue_ = queue;

  if (reader_ != nullptr) {
    delete reader_;
    reader_ = nullptr;
  }
  if (image_ != nullptr) {
    delete image_;
    image_ = nullptr;
    delete probe_image_;
    probe_image_ = nullptr;
    probeSource_ = DataSource{};
  }
  if (norm_image_ != nullptr) {
    delete norm_image_;
    norm_image_ = nullptr;
  }

  reader_ = new QueueReader(queue);
  if (reader_ == nullptr) {
    std::cerr << "Display: setDataSource reader allocation error" << std::endl;
    return -1;
  }
  reader_->setBlockingCalls(false);

  epf::Node *n = nullptr;
  if (buf_type == BufferType::Data)
    n = reader_->dataSchema()->item(item);
  else
    n = reader_->hdrSchema()->item(item);

  if ((!n) || (!n->isObjectNode())) {
    std::cerr << "Display: setDataSource invalid schema item" << std::endl;
    return -1;
  }

  epf::ObjectNode *onode = static_cast<epf::ObjectNode *>(n);
  if (onode->objecttype() == epf::EP_IMAGE_RAW) {
    image_ = new epf::ImageObject(onode);
  }
  else {
    std::cerr << "Display: setDataSource object is not an EP_IMAGE_RAW"
              << std::endl;
    return -1;
  }

  if (image_ == nullptr) {
    std::cerr << "Display: failed to create ImageObject" << std::endl;
    return -1;
  }

  aspect_ = 1.0f;

  if (!configureUploadFormat(*image_)) {
    std::cerr << "ImageWindow: unsupported display path for PixelFormat "
              << to_string(image_->pixelFormat()) << std::endl;
    return -1;
  }

  glutSetWindow(id_);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glViewport(0, 0, window_w_, window_h_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  zoom_f_ = static_cast<double>(window_w_) / image_->width();

  gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
             window_h_ / zoom_f_ - raster_y_);

  // Allocate normalization image only when upload is supported.
  norm_image_ = new ImageObject("norm img", image_->width(), image_->height(),
                                image_->channels(), image_->pixelFormat());

  return 0;
}

void ImageWindow::setBlocking(bool blocking)
{  // todo
  if (reader_ != nullptr) reader_->setBlockingCalls(blocking);
}

ImageWindow::~ImageWindow()
{
  std::cout << " destructor ImageWindow " << std::endl;

  texture_ = 0;

  delete reader_;
  reader_ = nullptr;

  delete image_;
  image_ = nullptr;

  delete norm_image_;
  norm_image_ = nullptr;

  delete probe_image_;
  probe_image_ = nullptr;

  delete hist_window_;
  hist_window_ = nullptr;
}

void ImageWindow::_update()
{
  if (reader_ == nullptr || image_ == nullptr) {
    read_success_ = false;
    return;
  }

  if (!upload_supported_) {
    read_success_ = false;
    return;
  }

  int err = reader_->startRead();
  if (err < 0) {
    read_success_ = false;
    return;
  }

  reader_->dataMsg(0);  // update data message

  if (compute_histogram_) {
    hist_window_->computeHistogram(*image_);
    hist_window_->draw();
  }

  const ImageObject *upload_image = image_;
  const void *upload_pixels = image_->data();

  const bool should_autoscale_float = float_autoscale_ &&
                                      image_->baseType() == EP_32F &&
                                      image_->channels() == 1;

  // Two normalization modes:
  // 1) Manual normalization (existing behavior) when normalize_ == true
  // 2) Automatic visualization normalization for float32 single-channel images
  if (normalize_ || should_autoscale_float) {
    if (norm_image_ == nullptr) {
      std::cerr
          << "ImageWindow: normalization requested but norm_image_ is null"
          << std::endl;
    }
    else {
      float min_value = 0.0f;
      float max_value = 1.0f;
      bool have_valid_range = false;

      if (normalize_) {
        // Existing manual normalization path driven by histogram markers
        min_value = static_cast<float>(hist_window_->minVal());
        max_value = static_cast<float>(hist_window_->maxVal());
        have_valid_range = std::isfinite(min_value) &&
                           std::isfinite(max_value) && (max_value > min_value);
      }

      else if (should_autoscale_float) {
        // Automatic autoscale for display only
        const float *ptr = reinterpret_cast<const float *>(image_->data());
        const int total_pixels =
            static_cast<int>(image_->width() * image_->height());

        float min_v = std::numeric_limits<float>::infinity();
        float max_v = -std::numeric_limits<float>::infinity();

        for (int i = 0; i < total_pixels; ++i) {
          const float v = ptr[i];
          if (!std::isfinite(v)) continue;
          min_v = std::min(min_v, v);
          max_v = std::max(max_v, v);
        }

        if (std::isfinite(min_v) && std::isfinite(max_v)) {
          min_value = min_v;
          max_value = max_v;

          // Avoid zero-width range
          if (max_value <= min_value) {
            max_value = min_value + 1.0f;
          }

          have_valid_range = true;
        }
      }

      if (have_valid_range) {
        if (normalize_image(*image_, *norm_image_, min_value, max_value) == 0) {
          upload_image = norm_image_;
          upload_pixels = norm_image_->data();
        }
        else {
          std::cerr << "ImageWindow: normalize_image failed" << std::endl;
        }
      }
    }
  }

  read_success_ = uploadImagePixels(*upload_image, upload_pixels);

  reader_->endRead();
}

void ImageWindow::_display()
{
  glutSetWindow(id_);

  if (image_ == nullptr) return;
  if (!upload_supported_) return;

  img_w_ = image_->width();
  img_h_ = image_->height();

  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, texture_);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(0.0f, 0.0f);

  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(static_cast<float>(img_w_), 0.0f);

  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(static_cast<float>(img_w_), static_cast<float>(img_h_));

  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(0.0f, static_cast<float>(img_h_));
  glEnd();

  glutSwapBuffers();
}

void ImageWindow::_reshape(int width, int height)
{
  glutSetWindow(id_);

  window_w_ = width;
  window_h_ = height;

  glViewport(0, 0, window_w_, window_h_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
             window_h_ / zoom_f_ - raster_y_);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glutPostRedisplay();
}

void ImageWindow::_mouse(int button, int state, int x, int y)
{
  glutSetWindow(id_);

  if (probe_enabled_ && probe_click_ && state == GLUT_DOWN &&
      button == GLUT_LEFT_BUTTON) {
    printProbe(x, y, "click", false);
  }

  y = window_h_ - y - 1;

  if (button == GLUT_MIDDLE_BUTTON) {
    //    zoom_f_ = img_w_ / window_w_;
    zoom_f_ = static_cast<double>(window_w_) / img_w_;
    //    zoom(image_->width() / img_w_, x, y);
    x_prev_ = 0;
    y_prev_ = 0;
    raster_x_ = 0;
    raster_y_ = 0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
               window_h_ / zoom_f_ - raster_y_);
    glutPostRedisplay();
  }
  else if (state == GLUT_DOWN) {
    double result = x / zoom_f_ - raster_x_;
    x_prev_ = static_cast<int>(result);
    result = y / zoom_f_ - raster_y_;
    y_prev_ = static_cast<int>(result);
  }
  else if (button == 3) {  // Zoom in
    zoom(1.3f, x, y);
  }
  else if (button == 4) {  // Zoom out
    zoom(1 / 1.3f, x, y);
  }
}

void ImageWindow::_motion(int x, int y)
{
  glutSetWindow(id_);

  y = window_h_ - y - 1;
  double result = x / zoom_f_ - x_prev_;
  raster_x_ = static_cast<int>(result);
  result = y / zoom_f_ - y_prev_;
  raster_y_ = static_cast<int>(result);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
             window_h_ / zoom_f_ - raster_y_);
  glutPostRedisplay();
}

void ImageWindow::_motionOver(int x, int y)
{
  if (probe_enabled_ && probe_hover_) {
    printProbe(x, y, "hover", true);
  }
}

void ImageWindow::zoom(float factor, int /*x*/, int /*y*/)
{
  glutSetWindow(id_);

  double zoom_f_prev = zoom_f_;
  zoom_f_ *= factor;
  double dx = (window_w_ / zoom_f_ - window_w_ / zoom_f_prev) * 0.5;
  double dy = (window_h_ / zoom_f_ - window_h_ / zoom_f_prev) * 0.5;
  raster_x_ += int(dx);
  raster_y_ += int(dy);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-raster_x_, window_w_ / zoom_f_ - raster_x_, -raster_y_,
             window_h_ / zoom_f_ - raster_y_);
  glutPostRedisplay();
}

void ImageWindow::_idle()
{
}

void ImageWindow::_keyboard(unsigned char key, int /*x*/, int /*y*/)
{
  // Find the current window instance using the GLUT window ID
  if (key == 'h') {
    compute_histogram_ = !compute_histogram_;
    compute_histogram_ == true ? printf("h - histogram on\n")
                               : printf("h - histogram off\n");
    if (compute_histogram_) {
      glutSetWindow(hist_window_->id());
      glutShowWindow();
    }
    else {
      glutSetWindow(hist_window_->id());
      glutHideWindow();
    }
  }
  if (key == 'n') {
    normalize_ = !normalize_;
    normalize_ == true ? printf("n - normalize on\n")
                       : printf("h - normalize off\n");
  }
}

///////////////////////////////////////////////////////////
// HistWindow
///////////////////////////////////////////////////////////

HistWindow::HistWindow(const std::string &name, int width, int height,
                       int pos_x, int pos_y)
    : Window(WindowType::Hist, name, width, height, pos_x, pos_y)
{
  bins_ = 256;
  histogram_ = std::vector<float>(bins_, 0.0f);

  min_val_ = 0.0f;
  max_val_ = 255.0f;

  range_x_ = 255.0f;
  range_y_ = 1.0f;

  basetype_min_value_ = 0.0;
  basetype_max_value_ = 255.0;

  hist_min_value_ = 0.0;
  hist_max_value_ = 255.0;
  hist_valid_range_ = true;

  glutSetWindow(id_);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  setupViewport();
}

double HistWindow::markerToValue(float marker) const
{
  if (!hist_valid_range_) return 0.0;

  const double m = std::clamp(static_cast<double>(marker), 0.0,
                              static_cast<double>(bins_ - 1));
  const double t = m / static_cast<double>(std::max(bins_ - 1, 1));

  return hist_min_value_ + t * (hist_max_value_ - hist_min_value_);
}

HistWindow::~HistWindow()
{
}

void HistWindow::setupViewport()
{
  glViewport(0, 0, width_, height_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, range_x_, 0.0, range_y_);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

// mouse interaction
void HistWindow::_mouse(int button, int state, int x, int /*y*/)
{
  glutSetWindow(id_);

  if (state == GLUT_DOWN) {
    float selected_val = static_cast<float>(x) /
                         static_cast<float>(std::max(width_, 1)) * range_x_;

    selected_val = std::clamp(selected_val, 0.0f, range_x_);

    if (button == GLUT_LEFT_BUTTON) {
      min_val_ = selected_val;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
      max_val_ = selected_val;
    }

    if (min_val_ > max_val_) std::swap(min_val_, max_val_);

    glutPostRedisplay();
  }
}

// Drawing logic
void HistWindow::_display()
{
  glutSetWindow(id_);
  glClear(GL_COLOR_BUFFER_BIT);

  drawHistogram();
  drawMarkers();

  glutSwapBuffers();
}

void HistWindow::draw()
{
  glutSetWindow(id_);
  _display();
}

void HistWindow::computeHistogram(ImageObject &data)
{
  basetype_range(data.baseType(), basetype_min_value_, basetype_max_value_);

  double hist_min = 0.0;
  double hist_max = 0.0;

  const int32_t err =
      compute_histogram256(data, histogram_, &hist_min, &hist_max, 0.0f);

  if (err < 0) {
    hist_min_value_ = 0.0;
    hist_max_value_ = 1.0;
    hist_valid_range_ = false;
    std::fill(histogram_.begin(), histogram_.end(), 0.0f);
    return;
  }

  hist_min_value_ = hist_min;
  hist_max_value_ = hist_max;
  hist_valid_range_ = hist_max_value_ > hist_min_value_;
}

void HistWindow::drawHistogram()
{
  // Fill under the histogram curve with a solid gray color
  glColor3f(0.5f, 0.5f, 0.5f);  // Gray color for the fill
  glBegin(GL_QUADS);
  for (int i = 0; i < static_cast<int>(histogram_.size()) - 1; ++i) {
    float x1 = static_cast<float>(i);
    float y1 = histogram_[i];
    float x2 = static_cast<float>(i + 1);
    float y2 = histogram_[i + 1];

    // Draw a quad for each segment of the histogram
    glVertex2f(x1, 0.0f);  // Bottom-left
    glVertex2f(x1, y1);    // Top-left
    glVertex2f(x2, y2);    // Top-right
    glVertex2f(x2, 0.0f);  // Bottom-right
  }
  glEnd();

  // Draw the histogram line on top in a lighter gray
  glColor3f(0.8f, 0.8f, 0.8f);  // Light gray for the curve
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < static_cast<int>(histogram_.size()); ++i) {
    glVertex2f(static_cast<float>(i), histogram_[i]);
  }
  glEnd();
}

void HistWindow::drawMarkers()
{
  // Draw the minimum marker as a vertical gray line
  glColor3f(0.3f, 0.3f, 0.3f);  // Dark gray for markers
  glBegin(GL_LINES);
  glVertex2f(min_val_, 0.0f);
  glVertex2f(min_val_, range_y_);
  glEnd();

  // Draw the maximum marker as a vertical gray line
  glBegin(GL_LINES);
  glVertex2f(max_val_, 0.0f);
  glVertex2f(max_val_, range_y_);
  glEnd();
}

void HistWindow::_reshape(int width, int height)
{
  glutSetWindow(id_);

  width_ = width;
  height_ = height;

  glViewport(0, 0, width_, height_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, range_x_, 0.0, range_y_);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}
