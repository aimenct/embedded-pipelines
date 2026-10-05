#include "sink_filter.h"

using namespace epf;

SinkFilter::SinkFilter()
    : Filter(YAML::Node(), 5, 0)
{
  std::cout << "SinkFilter constructor " << std::endl;
}

SinkFilter::~SinkFilter()
{
  std::cout << "SinkFilter destructor" << std::endl;
}

int32_t SinkFilter::_job()
{
  for (int32_t i = 0; i < maxSources(); i++) {
    if (sourcePort(i)->isConnected()) {
      int32_t err = sourcePort(i)->reader()->startRead();
      if (err >= 0) {
        std::cout << "Succesfully read from source port: " << i << std::endl;
        sourcePort(i)->reader()->endRead();
      }
    }
  }

  return 0;
}

int32_t SinkFilter::_open()
{
  return 0;
}
int32_t SinkFilter::_close()
{
  return 0;
}

int32_t SinkFilter::_start()
{
  return 0;
}

int32_t SinkFilter::_stop()
{
  return 0;
}

int32_t SinkFilter::_set()
{
  return 0;
}

int32_t SinkFilter::_reset()
{
  return 0;
}