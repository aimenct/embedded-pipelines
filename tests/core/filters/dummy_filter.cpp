#include "dummy_filter.h"

using namespace epf;

DummyFilter::DummyFilter(const YAML::Node config)
    : epf::Filter(config, 1, 1)
{
  // std::cout << "DummyFilter constructor " << std::endl;
  type_ = "DummyFilter";

  /* filter settings */
  timeout_ = 10;
  counter_init_ = 0;
  metadata_ = "This is a dummy Filter.";

  addSetting("timeout", timeout_);
  addSetting("counter_init", counter_init_);
  addSetting("metadata", metadata_);

  /* device settings */
  addSetting("dummy_setting", dummy_setting_value_, DEVICE_SETTING,
             "dummy tooltip", epf::W);
  // add device settings
  addSetting(dummy_offset_name_, dummy_offset_value_, DEVICE_SETTING, "tooltip",
             epf::W);
  return;
}

DummyFilter::~DummyFilter()
{
}

int32_t DummyFilter::_open()
{
  return 0;
}

int32_t DummyFilter::_close()
{
  return 0;
}
int32_t DummyFilter::_set()
{
  if (!sourcePort(0)->isConnected()) {
    std::cout << "DummyFilter error sourcePort not connected" << std::endl;
    return -1;
  }

  // Message *data_schema = new Message(*(reader(0)->dataSchema()));
  // Message *hdr_schema = new Message(*(reader(0)->hdrSchema()));
  auto data_schema =
      std::make_unique<Message>(*(sourcePort(0)->reader()->dataSchema()));
  auto hdr_schema =
      std::make_unique<Message>(*(sourcePort(0)->reader()->hdrSchema()));
  int port = 0;

  //  addSinkQueue(port, std::move(data_schema), std::move(hdr_schema));
  sinkPort(port)->activate(std::move(data_schema), std::move(hdr_schema));

  return 0;
}

int32_t DummyFilter::_reset()
{
  return 0;
}

int32_t DummyFilter::_start()
{
  return 0;
}

int32_t DummyFilter::_stop()
{
  return 0;
}

int32_t DummyFilter::_job()
{
  int err = sinkPort(0)->writer()->startWrite();
  if (err >= 0) {
    int err1 = sourcePort(0)->reader()->startRead();
    if (err1 >= 0) {
      memcpy(sinkPort(0)->writer()->dataPtrA(),
             sourcePort(0)->reader()->dataPtrA(),
             sourcePort(0)->reader()->dataSchema()->size());

      sourcePort(0)->reader()->endRead();
      sinkPort(0)->writer()->endWrite();
      return 0;
    }
    else {
      sinkPort(0)->writer()->endWriteAbort();
      usleep(timeout_);
      return err1;
    }
  }
  usleep(timeout_);
  return err;
}

// implementation of this functions are needed only if the filter has device
// settings
int32_t DummyFilter::setDeviceSettingValue(const char *key, const void *value)
{
  std::string name = setting_simple_name(key);
  if (dummy_setting_name_ == name) {
    int number = *((int *)value);
    dummy_setting_value_ = number;
    return 0;
  }
  else if (dummy_offset_name_ == name) {
    int number = *((int *)value);
    dummy_offset_value_ = number;
    return 0;
  }
  std::cout << "setting not found: " << std::endl;
  return -1;
}

int32_t DummyFilter::setDeviceSettingValueStr(const char *key,
                                              const char *value)
{
  std::string name = setting_simple_name(key);
  if (dummy_setting_name_ == name) {
    try {
      int number = std::stoi(value);
      dummy_setting_value_ = number;
    }
    catch (const std::invalid_argument &e) {
      //      std::cout << "setDeviceSettingValue error "<< std::end;
      std::cerr << "err" << std::endl;
      return -1;
    }
    return 0;
  }
  else if (dummy_offset_name_ == name) {
    try {
      int number = std::stoi(value);
      dummy_offset_value_ = number;
    }
    catch (const std::invalid_argument &e) {
      //      std::cout << "setDeviceSettingValue error "<< std::end;
      std::cerr << "err" << std::endl;
      return -1;
    }
    return 0;
  }
  else {
    std::cout << "setting not found: " << std::endl;
    return -1;
  }
}

int32_t DummyFilter::deviceSettingValue(const char *key, void *value)
{
  std::string name = setting_simple_name(key);
  if (dummy_setting_name_ == name) {
    *(int *)value = dummy_setting_value_;
    return 0;
  }
  else if (dummy_offset_name_ == name) {
    *(int *)value = dummy_offset_value_;
    return 0;
  }
  std::cout << "setting not found: " << std::endl;
  return -1;
}
