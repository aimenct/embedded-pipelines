#include "core.h"

class BasicFilter : public epf::Filter {
  public:
    BasicFilter() = default;

  protected:
    int32_t _job() override
    {
      return 0;
    };
    int32_t _open() override
    {
      return 0;
    };
    int32_t _close() override
    {
      return 0;
    };
    int32_t _set() override
    {
      return 0;
    };
    int32_t _reset() override
    {
      return 0;
    };
    int32_t _start() override
    {
      return 0;
    };
    int32_t _stop() override
    {
      return 0;
    };
};
