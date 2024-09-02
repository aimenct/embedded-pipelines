// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <core.h>

using namespace ep;

/* Dummy Device Setting */
std::string dummy_setting_name = "dummy_setting";
int dummy_setting_value = 0;
std::string dummy_offset_name = "dummy_offset";
int dummy_offset_value = 0;

/* Filter definition */
class DummyFilter : public Filter {
  public:
    DummyFilter(const YAML::Node &config);  // create settings
    ~DummyFilter();                         // delete settings

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
    YAML::Node yaml_config_;  // ¿move to base class?

    // Filter Settings
    uint32_t timeout_;
    int counter_init_;
    std::string metadata_;
};

/* filter implementation */
DummyFilter::DummyFilter(const YAML::Node &config)
    : Filter()
{
  std::cout << "DummyFilter constructor " << std::endl;
  yaml_config_ = config;

  /* filter name & type */
  name_ = "Dummy Filter";
  type_ = "DummyFilter";

  /* filter settings */
  timeout_ = 10;
  counter_init_ = 0;
  metadata_ = "This is a dummy Filter.";

  addSetting("timeout", timeout_);
  addSetting("counter init", counter_init_);
  addSetting("metadata", metadata_);

  /* device settings */
  addSetting("dummy_setting", dummy_setting_value, DEVICE_SETTING,
             "dummy tooltip", ep::W);

  /* queue settings */
  max_sinks_ = 1;
  max_sources_ = 1;

  /* update settings from yaml */
  readSettings(yaml_config_);

  return;
}
DummyFilter::~DummyFilter()
{
}
int32_t DummyFilter::_open()
{
  // device connection established
  // add device settings
  addSetting(dummy_offset_name, dummy_offset_value, DEVICE_SETTING, "tooltip",
             ep::W);

  // update device settings from yaml
  readSettings(yaml_config_);

  return 0;
}
int32_t DummyFilter::_close()
{
  return 0;
}
int32_t DummyFilter::_set()
{
  if (sourceQueue(0) == nullptr) {
    std::cout << "DummyFilter error source queue not connected" << std::endl;
    return -1;
  }

  Message *data_schema = new Message(*(reader(0)->dataSchema()));
  Message *hdr_schema = new Message(*(reader(0)->hdrSchema()));
  int port = 0;

  addSinkQueue(port, data_schema, hdr_schema);

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
  int err = writer(0)->startWrite();
  if (err >= 0) {
    int err1 = reader(0)->startRead();
    if (err1 >= 0) {
      memcpy(writer(0)->dataPtrA(), reader(0)->dataPtrA(),
             reader(0)->dataSchema()->size());

      reader(0)->endRead();
      writer(0)->endWrite();
      return 0;
    }
    else {
      writer(0)->endWriteAbort();
      usleep(timeout_);
      return err1;
    }
  }
  usleep(timeout_);
  return err;
}

// implementation of this functions are needed only if the filter has device
// settings
int DummyFilter::setDeviceSettingValue(const char *key, const void *value)
{
  if (strcmp(key, dummy_setting_name.c_str()) == 0) {
    int number = *((int *)value);
    dummy_setting_value = number;
    return 0;
  }
  else if (strcmp(key, dummy_offset_name.c_str()) == 0) {
    int number = *((int *)value);
    dummy_offset_value = number;
    return 0;
  }
  std::cout << "setting not found: " << std::endl;
  return -1;
}

int DummyFilter::setDeviceSettingValueStr(const char *key, const char *value)
{
  if (strcmp(key, dummy_setting_name.c_str()) == 0) {
    try {
      int number = std::stoi(value);
      dummy_setting_value = number;
    }
    catch (const std::invalid_argument &e) {
      //      std::cout << "setDeviceSettingValue error "<< std::end;
      std::cerr << "err" << std::endl;
      return -1;
    }
    return 0;
  }
  else if (strcmp(key, dummy_offset_name.c_str()) == 0) {
    try {
      int number = std::stoi(value);
      dummy_offset_value = number;
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

int DummyFilter::deviceSettingValue(const char *key, void *value)
{
  if (strcmp(key, dummy_setting_name.c_str()) == 0) {
    *(int *)value = dummy_setting_value;
    return 0;
  }
  else if (strcmp(key, dummy_offset_name.c_str()) == 0) {
    *(int *)value = dummy_offset_value;
    return 0;
  }
  std::cout << "setting not found: " << std::endl;
  return -1;
}

/* filter test */
int main()
{
  std::string fname = "../../../tests/core/filter_settings.yml";

  DummyFilter filter(YAML::LoadFile(fname));

  /* create source queue */
  std::shared_ptr<Queue> q = std::make_shared<Queue>();
  Message *data_schema = new Message();
  DataNode *counter_n = new DataNode("counter", EP_32S, {1}, nullptr);
  data_schema->addItem(counter_n);
  Message *hdr_schema = nullptr;
  int length = 10;
  q->init(length, data_schema, hdr_schema);

  int my_counter = 0;

  QueueWriter w(q);

  YAML::Node mySettings;
  filter.writeSettings(mySettings);
  filter.setSettingValue("dummy_setting", 1111);

  /* open() */
  filter.open();

  filter.setSettingValue("dummy_setting", 2222);

  int src_port = 0;
  filter.connectSourceQueue(src_port, q);

  /* set() */
  filter.set();

  filter.saveSettings("filter_settings.yml");
  filter.writeSettings(mySettings);
  std::cout << "\nAfter set() settings: \n" << mySettings << std::endl;

  int sink_port = 0;
  QueueReader r(filter.sinkQueue(sink_port));

  /* set variables before loop */
  DataNode *my_counter_node = static_cast<DataNode *>(w.dataSchema()->item(0));
  DataNode *my_counter_r = static_cast<DataNode *>(r.dataSchema()->item(0));

  int iterations_ = 0;
  int offset_ = 0;

  const int *iterations = filter.settingValue<int>("dummy_setting");
  if (iterations) iterations_ = *iterations;

  /* start() */
  filter.start();
  filter.setSettingValue("dummy_offset", 0);

  const int *offset = filter.settingValue<int>("dummy_offset");
  offset_ = offset ? *offset : offset_;  // check nullptr

  my_counter = offset_;

  int acc = 0;
  int err = 0;

  for (int i = 0; i < iterations_; i++) {
    err = w.startWrite();
    if (err >= 0) {
      my_counter_node->write(&my_counter);
      my_counter++;
      w.endWrite();
    }

    filter.doJob(&filter);

    err = r.startRead();
    if (err >= 0) {
      int a;
      my_counter_r->read(&a);
      acc = a;
      r.endRead();
    }
  }

  /* stop() */
  filter.stop();

  if (acc != (iterations_ - 1 + offset_)) {
    std::cout << "\n TEST1.1 FAIL" << std::endl;
  }
  else
    std::cout << "\n TEST1.1 PASSED" << std::endl;

  // TEST 1.2

  /* start() */
  filter.start();

  filter.setSettingValue("dummy_offset", 80);
  acc = 0;

  offset = filter.settingValue<int>("dummy_offset");
  offset_ = offset ? *offset : offset_;
  my_counter = offset_;

  iterations = filter.settingValue<int>("dummy_setting");
  iterations_ = iterations ? *iterations : iterations_;

  for (int i = 0; i < iterations_; i++) {
    err = w.startWrite();
    if (err >= 0) {
      my_counter_node->write(&my_counter);
      my_counter++;
      w.endWrite();
    }
    filter.doJob(&filter);
    err = r.startRead();
    if (err >= 0) {
      int a;
      my_counter_r->read(&a);
      acc = a;
      r.endRead();
    }
  }
  /* stop() */
  filter.stop();

  if (acc != (iterations_ - 1 + offset_)) {
    std::cout << "\n TEST1.2 FAIL" << std::endl;
  }
  else
    std::cout << "\n TEST1.2 PASSED" << std::endl;

  /* reset() */
  filter.reset();

  filter.writeSettings(mySettings);
  std::cout << "\nAfter reset \n" << mySettings << std::endl;

  /* close() */
  filter.close();

  filter.writeSettings(mySettings);
  std::cout << "\nAfter close \n" << mySettings << std::endl;

  q = nullptr;

  return 0;
}

