#include <core.h>

class SinkFilter : public epf::Filter {
  public:
    SinkFilter();
    ~SinkFilter();

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();
};
