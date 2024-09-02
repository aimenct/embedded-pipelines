// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filter.h"

using namespace ep;

Filter::Filter()
{
  state_ = FilterState::DISCONNECTED;

  pthread_mutex_init(&state_mtx_, NULL);

  name_ = "unamed";
  job_execution_model_ = JobExecutionModel::EXTERNAL_THREAD;
  settings_ = Settings2(name_);

  max_sinks_ = 0;
  max_sources_ = 0;
}

Filter::~Filter()
{
}

JobExecutionModel Filter::jobExecutionModel() const
{
  return job_execution_model_;
}

int Filter::open()
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::DISCONNECTED) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual open()
  if (_open() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  sink_ports_.resize(max_sinks_);
  source_ports_.resize(max_sources_);

  state_ = FilterState::CONNECTED;
  pthread_mutex_unlock(&state_mtx_);
  return 0;
}

int Filter::set()
{
  pthread_mutex_lock(&state_mtx_);
  if (!((state_ == FilterState::CONNECTED) || (state_ == FilterState::SET))) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual set
  if (_set() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  state_ = FilterState::SET;
  pthread_mutex_unlock(&state_mtx_);

  return 0;
}

int Filter::reset()
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::SET) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual reset
  if (_reset() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  for (std::size_t i = 0; i < sink_ports_.size(); i++) {
    sink_ports_[i].reset();
  }

  for (std::size_t i = 0; i < source_ports_.size(); i++) {
    source_ports_[i].reset();
  }

  state_ = FilterState::CONNECTED;
  pthread_mutex_unlock(&state_mtx_);
  return 0;
}

int Filter::start()
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::SET) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual start
  if (_start() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // start thread
  state_ = FilterState::RUNNING;
  pthread_mutex_unlock(&state_mtx_);

  return 0;
}

int Filter::stop()
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::RUNNING) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual stop
  if (_stop() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  state_ = FilterState::SET;

  pthread_mutex_unlock(&state_mtx_);
  return 0;
}

int Filter::close()
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ == FilterState::RUNNING) {
    pthread_mutex_unlock(&state_mtx_);
    if (stop() < 0) {
      return -1;
    }
    pthread_mutex_lock(&state_mtx_);
  }

  if (state_ == FilterState::SET) {
    pthread_mutex_unlock(&state_mtx_);
    if (reset() < 0) return -1;
    pthread_mutex_lock(&state_mtx_);
  }

  if (state_ != FilterState::CONNECTED) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  // virtual close
  if (_close() < 0) {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }

  sink_ports_.clear();
  source_ports_.clear();

  state_ = FilterState::DISCONNECTED;
  pthread_mutex_unlock(&state_mtx_);
  return 0;
}

void *Filter::doJob(void *self)
{
  if (static_cast<Filter *>(self)->state_ == FilterState::RUNNING)
    static_cast<Filter *>(self)->_job();
  return nullptr;
}

int32_t Filter::doJob()
{
  return this->_job();
}

std::shared_ptr<Queue> Filter::sinkQueue(int32_t index) const
{
  if ((index >= 0) && (index < static_cast<int32_t>(sink_ports_.size()))) {
    return sink_ports_[index].queue_;
  }
  else {
    return nullptr;
  }
}

std::shared_ptr<Queue> Filter::sourceQueue(int32_t index) const
{
  if ((index >= 0) && (index < static_cast<int32_t>(source_ports_.size()))) {
    return source_ports_[index].queue_;
  }
  else {
    return nullptr;
  }
}

std::vector<int> Filter::connectedSources() const
{
  std::vector<int> connectedSources;
  // Iterate over all possible source queues
  for (int i = 0; i < static_cast<int>(source_ports_.size()); ++i) {
    // Check if the source queue is connected
    if (source_ports_[i].queue_ != nullptr) {
      connectedSources.push_back(i);
    }
  }
  return connectedSources;
}

int Filter::addSinkQueue(int port, Message *data_schema, Message *hdr_schema)
{
  if (port < 0) {
    std::cerr << "TODO addSinkQueue port<0 " << std::endl;
    return -1;
  }

  //  instantiate sink queue
  SinkPort &p = sink_ports_[port];

  // Create a shared_ptr to manage the Queue
  std::shared_ptr<Queue> q = std::make_shared<Queue>();

  q->setQueueType(p.queue_settings_.type_);

  // (Queue gets the ownership of the Message)
  int err =
      q->init(p.queue_settings_.length_, data_schema, hdr_schema,
              p.queue_settings_.max_readers_, p.queue_settings_.max_writers_);
  if (err < 0) {
    std::cout << "Error init queue setSinkQueue" << std::endl;
    exit(0);
  }
  err = connectSinkQueue(port, q);
  return err;
}

// int Filter::connectSinkQueue(int port, Queue *q)
int Filter::connectSinkQueue(int port, std::shared_ptr<Queue> q)
{
  if (port < 0) {
    std::cerr << "connectSinkQueue port<0" << std::endl;
    return -1;
  }
  unsigned int u_port = port;

  if ((state_ == FilterState::CONNECTED) && (u_port < sink_ports_.size())) {
    SinkPort &p = sink_ports_[u_port];
    p.queue_ = q;
    p.writer_ = new QueueWriter(q);
    p.writer_->setBatchSize(p.writer_settings_.batch_);
    p.writer_->setBlockingCalls(p.writer_settings_.blocking_);
    return 0;
  }
  else {
    std::cout << "connect sink queue -1" << std::endl;
    return -1;
  }
}

int Filter::connectSourceQueue(int port, std::shared_ptr<Queue> q,
                               Filter *src_filter)
{
  if (q == nullptr) return -1;

  if (port < 0) {
    std::cerr << "connectSourceQueue port < 0" << std::endl;
    return -1;
  }
  unsigned int u_port = port;

  SourcePort &p = source_ports_[u_port];

  pthread_mutex_lock(&state_mtx_);

  if ((state_ == FilterState::CONNECTED) && (u_port < source_ports_.size())) {
    p.queue_ = q;
    p.reader_ = new QueueReader(q);
    QueueReader *r = p.reader_;
    r->setBatchSize(p.reader_settings_.batch_);
    r->setNewPerBatch(p.reader_settings_.new_per_batch_);
    r->setBlockingCalls(p.reader_settings_.blocking_);

    p.src_filter_ = src_filter;

    pthread_mutex_unlock(&state_mtx_);
    return 0;
  }
  else {
    pthread_mutex_unlock(&state_mtx_);
    return -1;
  }
}

int32_t Filter::sourcePorts() const
{
  return static_cast<int32_t>(source_ports_.size());
}

int32_t Filter::sinkPorts() const
{
  return static_cast<int32_t>(sink_ports_.size());
}

QueueReader *Filter::reader(int i)
{
  if ((i >= 0) && (i < static_cast<int32_t>(source_ports_.size()))) {
    return source_ports_[i].reader_;
  }
  else {
    return nullptr;
  }
}

QueueWriter *Filter::writer(int i)
{
  if ((i >= 0) && (i < static_cast<int32_t>(sink_ports_.size()))) {
    return sink_ports_[i].writer_;
  }
  else {
    return nullptr;
  }
}

const FilterState &Filter::state()
{
  return state_;
}

const std::string &Filter::name()
{
  return name_;
}

int32_t Filter::addCommand(std::string name, std::function<int()> command,
                           std::string tooltip, AccessType accessmode)
{
  return settings_.addCommand(name, command, tooltip, accessmode);
}

int32_t Filter::runCommand(std::string name)
{
  if (settings_.settingAccessMode(name) & state_)
    return settings_.runCommand(name);
  else {
    std::cout << "Not allowed to set setting " << name
              << " in \"state_=" << state2string(state_) << "\"." << std::endl;
    return -1;
  }
}

int Filter::setDeviceSettingValue(const char *key, const void *value)
{
  std::cout << "Filter::setDeviceSettingValue(" << key << "," << value << ")"
            << std::endl;
  return -1;
}

int Filter::setDeviceSettingValueStr(const char *key, const char *value)
{
  std::cout << "Filter::setDeviceSettingValueStr(" << key << "," << value << ")"
            << std::endl;
  return -1;
}

int32_t Filter::deviceSettingValue(const char *key, void *value)
{
  std::cout << "Filter::deviceSettingValue(" << key << "," << (char *)value
            << ")" << std::endl;
  return -1;
}

const Settings2 *Filter::settings()
{
  return &settings_;
}

const Node2 *Filter::settingNode(std::string &name) const
{
  return settings_[name];
}

int32_t Filter::readDeviceSettings(const YAML::Node &yaml_node)
{
  if (yaml_node["device_settings"])
    deviceSettingsFromYAML(yaml_node["device_settings"]);
  return 0;
}

int32_t Filter::deviceSettingsFromYAML(const YAML::Node &yaml_node)
{
  for (YAML::const_iterator it = yaml_node.begin(); it != yaml_node.end();
       ++it) {
    std::string key = it->first.as<std::string>();
    std::string value;

    if (yaml_node[key].Type() == YAML::NodeType::Scalar) {
      value = it->second.as<std::string>();

      int err = setSettingValue<std::string>(key, value);
      if (err >= 0) {
        // std::cout << "setSettingValue: " << key << ", " << value << "
        // [SUCCESS]"
        //<< std::endl;
        return 0;
      }
      else {
        //   std::cout << "setSettingValue: " << key << "[FAILED]" << std::endl;
        return err;
      }
    }
    else if (yaml_node[key].Type() == YAML::NodeType::Map) {
      deviceSettingsFromYAML(yaml_node[key]);
    }
    else {
      std::cout << "YAML NodeType not supported." << std::endl;
    }
  }
  return 0;
}

int32_t Filter::readSettings(const YAML::Node &filter_node)
{
  // Read basic settings
  if (filter_node["name"]) name_ = filter_node["name"].as<std::string>();
  if (filter_node["type"]) type_ = filter_node["type"].as<std::string>();

  // Read device settings
  settings_.fromYAML(filter_node);

  if (filter_node["device_settings"]) {
    if (this->state() != DISCONNECTED) {
      deviceSettingsFromYAML(filter_node["device_settings"]);
    }
  }

  if (filter_node["queue_settings"]) {
    const YAML::Node &queue_settings_node = filter_node["queue_settings"];

    // Read max sources and sinks
    if (queue_settings_node["max sources"]) {
      max_sources_ = queue_settings_node["max sources"].as<int>();
    }
    if (queue_settings_node["max sinks"]) {
      max_sinks_ = queue_settings_node["max sinks"].as<int>();
    }

    sink_ports_.resize(max_sinks_);
    source_ports_.resize(max_sources_);

    // Read sink queue settings
    if (queue_settings_node["sink queues"]) {
      const YAML::Node &queues_node = queue_settings_node["sink queues"];
      for (std::size_t i = 0; i < queues_node.size(); i++) {
        const YAML::Node &queue_node = queues_node[i];
        if (!queue_node["id"]) {
          std::cerr
              << "Warning: Missing 'id' in one of the sink queue settings."
              << std::endl;
          continue;
        }
        int id = queue_node["id"].as<int>();
        if (id < max_sinks_ && id >= 0) {
          readQueueSettings(queue_node, sink_ports_[id].queue_settings_);
        }
        else {
          std::cerr << "Warning: Queue ID " << id
                    << " out of bounds for sink queue settings." << std::endl;
        }
      }
    }

    // Read writer settings
    if (queue_settings_node["writers"]) {
      const YAML::Node &writers_node = queue_settings_node["writers"];
      for (std::size_t i = 0; i < writers_node.size(); i++) {
        const YAML::Node &writer_node = writers_node[i];
        if (!writer_node["id"]) {
          std::cerr << "Warning: Missing 'id' in one of the writer settings."
                    << std::endl;
          continue;
        }
        int id = writer_node["id"].as<int>();
        if (id < max_sinks_ && id >= 0) {
          readWriterSettings(writer_node, sink_ports_[id].writer_settings_);
        }
        else {
          std::cerr << "Warning: Writer ID " << id
                    << " is out of bounds for writer settings." << std::endl;
        }
      }
    }

    // Read reader settings
    if (queue_settings_node["readers"]) {
      const YAML::Node &readers_node = queue_settings_node["readers"];
      for (std::size_t i = 0; i < readers_node.size(); i++) {
        const YAML::Node &reader_node = readers_node[i];
        if (!reader_node["id"]) {
          std::cerr << "Warning: Missing 'id' in one of the reader settings."
                    << std::endl;
          continue;
        }
        int id = reader_node["id"].as<int>();
        if (id < max_sources_ && id >= 0) {
          readReaderSettings(reader_node, source_ports_[id].reader_settings_);
        }
        else {
          std::cerr << "Warning: Reader ID " << id
                    << " is out of bounds for reader settings." << std::endl;
        }
      }
    }
  }

  return 0;
}

// function to read queue settings
void Filter::readQueueSettings(const YAML::Node &queue_node,
                               SinkQueueSettings &queue_settings)
{
  if (queue_node["length"]) {
    queue_settings.length_ = queue_node["length"].as<int>();
  }
  if (queue_node["type"]) {
    std::string type_str = queue_node["type"].as<std::string>();
    queue_settings.type_ = (type_str == "lifo") ? lifo : fifo;
  }
  if (queue_node["max readers"]) {
    queue_settings.max_readers_ = queue_node["max readers"].as<int>();
  }
}

// function to read writer settings
void Filter::readWriterSettings(const YAML::Node &writer_node,
                                QueueHandlerSettings &writer_settings)
{
  if (writer_node["batch"]) {
    writer_settings.batch_ = writer_node["batch"].as<int>();
  }
  else {
    std::cerr << "Warning: 'batch' is missing. Using default value."
              << std::endl;
    writer_settings.batch_ = 1;
  }
  if (writer_node["blocking"]) {
    writer_settings.blocking_ = writer_node["blocking"].as<bool>();
  }
  else {
    std::cerr << "Warning: 'blocking' is missing. Using default value."
              << std::endl;
    writer_settings.blocking_ = false;
  }
}

// function to read reader settings
void Filter::readReaderSettings(const YAML::Node &reader_node,
                                QueueHandlerSettings &reader_settings)
{
  if (reader_node["batch"]) {
    reader_settings.batch_ = reader_node["batch"].as<int>();
  }
  else {
    std::cerr << "Warning: 'batch' is missing. Using default value."
              << std::endl;
    reader_settings.batch_ = 1;
  }
  if (reader_node["new per batch"]) {
    reader_settings.new_per_batch_ = reader_node["new per batch"].as<int>();
  }
  else {
    std::cerr << "Warning: 'new per batch' is missing. Using default value."
              << std::endl;
    reader_settings.new_per_batch_ = reader_settings.batch_;
  }
  if (reader_node["blocking"]) {
    reader_settings.blocking_ = reader_node["blocking"].as<bool>();
  }
  else {
    std::cerr << "Warning: 'blocking' is missing. Using default value."
              << std::endl;
    reader_settings.blocking_ = false;
  }
}

int32_t Filter::writeSettings(YAML::Node &filter_node)
{
  // General settings
  filter_node["name"] = name_;
  filter_node["type"] = type_;

  // // // ToDo ¿ refresh device settings ?
  // auto device_settings = settings_.listNames(1);  // get list of device
  // settings for (auto &s : device_settings) {
  //   char placeholder[256];  // update tree
  //   deviceSettingValue(s.c_str(), placeholder);
  // }

  // Write filter settings to the YAML node
  settings_.toYAML(filter_node);

  // Queue settings node
  YAML::Node queue_settings_node;
  queue_settings_node["max_sources"] = max_sources_;
  queue_settings_node["max_sinks"] = max_sinks_;

  // Sink queues and writers
  YAML::Node queues_node;
  YAML::Node writers_node;

  for (size_t i = 0; i < sink_ports_.size(); ++i) {
    if (sink_ports_[i].queue_) {
      SinkPort &p = sink_ports_[i];

      // Queue settings
      YAML::Node queue_node;
      queue_node["id"] = static_cast<int>(i);  // ID as integer
      queue_node["length"] = p.queue_settings_.length_;
      queue_node["type"] = p.queue_settings_.type_ == lifo ? "lifo" : "fifo";
      queue_node["max_readers"] = p.queue_settings_.max_readers_;
      queues_node.push_back(queue_node);

      // Writer settings
      YAML::Node writer_node;
      writer_node["id"] = static_cast<int>(i);  // ID as integer
      writer_node["batch"] = p.writer_settings_.batch_;
      writer_node["blocking"] = p.writer_settings_.blocking_;
      writers_node.push_back(writer_node);
    }
  }

  if (!queues_node.IsNull()) {
    queue_settings_node["sink_queues"] = queues_node;
  }
  if (!writers_node.IsNull()) {
    queue_settings_node["writers"] = writers_node;
  }

  // Source queues and readers
  YAML::Node readers_node;

  for (size_t i = 0; i < source_ports_.size(); ++i) {
    if (source_ports_[i].queue_) {
      SourcePort &p = source_ports_[i];

      // Reader settings
      YAML::Node reader_node;
      reader_node["id"] = static_cast<int>(i);  // ID as integer
      reader_node["batch"] = p.reader_settings_.batch_;
      reader_node["new_per_batch"] = p.reader_settings_.new_per_batch_;
      reader_node["blocking"] = p.reader_settings_.blocking_;
      readers_node.push_back(reader_node);
    }
  }

  if (!readers_node.IsNull()) {
    queue_settings_node["readers"] = readers_node;
  }

  filter_node["queue_settings"] = queue_settings_node;

  return 0;
}

int32_t Filter::saveSettings(const std::string &filename)
{
  // Create a YAML node and write the current settings into it
  YAML::Node filter_node;
  writeSettings(filter_node);

  try {
    // Save the YAML node to a file
    std::ofstream fout(filename);
    fout << filter_node;
  }
  catch (const std::exception &e) {
    std::cerr << "Error saving settings to file: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}

int32_t Filter::loadSettings(const std::string &filename)
{
  YAML::Node filter_node;

  try {
    // Load the YAML node from the file
    filter_node = YAML::LoadFile(filename);
  }
  catch (const std::exception &e) {
    std::cerr << "Error loading settings from file: " << e.what() << std::endl;
    return -1;
  }

  // Read the settings into the C++ variables
  return readSettings(filter_node);
}

namespace ep {

std::string state2string(const ep::FilterState &state)
{
  switch (state) {
    case FilterState::CONNECTED:
      return "connected";
      break;
    case FilterState::DISCONNECTED:
      return "disconnected";
      break;
    case FilterState::SET:
      return "set";
      break;
    case FilterState::RUNNING:
      return "running";
      break;
  }
  return "";
}

}  // namespace ep
