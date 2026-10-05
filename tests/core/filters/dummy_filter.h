#ifndef EP_DUMMY_FILTER
#define EP_DUMMY_FILTER

#include <core.h>

#include <string>

/* Filter definition */
class DummyFilter : public epf::Filter {
  public:
    DummyFilter(const YAML::Node config);  // create settings
    ~DummyFilter();                        // delete settings

  protected:
    int32_t _job();
    int32_t _open();   // establish connection with device
    int32_t _close();  // close connection with device
    int32_t _set();    // create queues and allocate memory
    int32_t _reset();  // delete queues and release memory
    int32_t _start();  // close connection with device
    int32_t _stop();   // create queues and allocate memory

    /* only needed if the filter manage settings from a external device sdk) */
    int setDeviceSettingValue(const char *key, const void *value);
    int setDeviceSettingValueStr(const char *key, const char *value);
    int deviceSettingValue(const char *key, void *value);

  private:
    /* Dummy Device Setting */
    std::string dummy_setting_name_ = "dummy_setting";
    int dummy_setting_value_ = 0;
    std::string dummy_offset_name_ = "dummy_offset";
    int dummy_offset_value_ = 0;

    // Filter Settings
    uint32_t timeout_;
    int counter_init_;
    std::string metadata_;
};

#endif  // EP_DUMMY_FILTER