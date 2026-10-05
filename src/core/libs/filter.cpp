// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "filter.h"

using namespace epf;

SinkPort::SinkPort(SinkPort &&other) noexcept
    : length_(other.length_),
      max_readers_(other.max_readers_),
      max_writers_(other.max_writers_),
      queue_type_(other.queue_type_),
      batch_size_(other.batch_size_),
      blocking_(other.blocking_),
      timestamp_(other.timestamp_)
{
}

SinkPort &SinkPort::operator=(SinkPort &&other) noexcept
{
  if (this != &other) {
    length_ = other.length_;
    max_readers_ = other.max_readers_;
    max_writers_ = other.max_writers_;
    queue_type_ = other.queue_type_;
    batch_size_ = other.batch_size_;
    blocking_ = other.blocking_;
    timestamp_ = other.timestamp_;
  }
  return *this;
}

SinkPort::SinkPort(int32_t length, int32_t max_readers, int32_t max_writers,
                   QueueType queue_type, int32_t batch_size, bool blocking)
    : length_(length),
      max_readers_(max_readers),
      max_writers_(max_writers),
      queue_type_(queue_type),
      batch_size_(batch_size),
      blocking_(blocking),
      timestamp_(false)
{
}

int32_t SinkPort::activate(std::unique_ptr<Message> data_schema,
                           std::unique_ptr<Message> hdr_schema)
{
  size_t ts_offset = 0;
  if (timestamp_) {
    if (!hdr_schema) {
      hdr_schema = std::make_unique<Message>();
    }
    auto ts_node = std::make_unique<DataNode>("timestamp", EP_64U,
                                              std::vector<size_t>{}, nullptr);
    DataNode *ts_raw = ts_node.get();
    hdr_schema->addItem(std::move(ts_node));
    ts_offset = hdr_schema->streamedNodeOffset(ts_raw);
  }

  queue_.setType(queue_type_);
  int32_t err = queue_.init(length_, std::move(data_schema),
                            std::move(hdr_schema), max_readers_, max_writers_);
  if (err < 0) {
    std::cout << "Error init port queue" << std::endl;
    return err;
  }

  err = writer_.subscribe(&queue_);
  if (err < 0) {
    std::cout << "Error init port queue" << std::endl;
    return err;
  }

  writer_.setBatchSize(batch_size_);
  writer_.setBlockingCalls(blocking_);
  if (timestamp_) {
    writer_.setTimestampOffset(ts_offset);
  }
  writer_.enableTimestamp(timestamp_);

  return 0;
}

bool SinkPort::SinkPort::isActivated()
{
  if (queue_.status() == 'c') {
    return true;
  }
  return false;
}

Queue *SinkPort::queue()
{
  return &queue_;
}

QueueWriter *SinkPort::writer()
{
  return &writer_;
}

int32_t SinkPort::deactivate()
{
  writer_.unsubscribe();
  queue_.free();
  return 0;
}

QueueType &SinkPort::queueType()
{
  return queue_type_;
}

int32_t &SinkPort::length()
{
  return length_;
};

int32_t &SinkPort::maxReaders()
{
  return max_readers_;
}

int32_t &SinkPort::maxWriters()
{
  return max_writers_;
}

int32_t &SinkPort::batchSize()
{
  return batch_size_;
}
bool &SinkPort::blocking()
{
  return blocking_;
}

bool &SinkPort::timestamp()
{
  return timestamp_;
}

int32_t SinkPort::setTimestamp(bool value)
{
  timestamp_ = value;
  if (isActivated()) {
    writer_.enableTimestamp(value);
  }
  return 0;
}

int32_t SinkPort::setBlocking(bool value)
{
  blocking_ = value;
  if (isActivated()) {
    return writer_.setBlockingCalls(value);
  }
  return 0;
}

SourcePort::SourcePort(int32_t message_window, int32_t message_stride,
                       bool blocking)
    : message_window_(message_window),
      message_stride_(message_stride),
      blocking_(blocking)
{
}

SourcePort::~SourcePort()
{
  disconnect();
}

int32_t SourcePort::connect(Queue *q)
{
  if (q == nullptr) {
    std::cerr << " SourcePort::connect(): queue expired/nullptr" << std::endl;
    return -1;
  }
  int32_t err = reader_.subscribe(q);

  if (err < 0) {
    std::cerr << "SourcePort::connect(): reader subscribe err" << std::endl;
    return -1;
  }

  queue_ = q;
  reader_.setBlockingCalls(blocking_);
  reader_.setMessageWindow(message_window_);
  reader_.setMessageStride(message_stride_);

  return 0;
}

bool SourcePort::isConnected()
{
  if (queue_ != nullptr) {
    return true;
  }
  return false;
}

int32_t SourcePort::disconnect()
{
  if (isConnected()) {
    reader_.unsubscribe();
    queue_ = nullptr;
    return 0;
  }
  return 0;
}

QueueReader *SourcePort::reader()
{
  return &reader_;
}

Queue *SourcePort::queue() const
{
  return queue_;
}

int32_t &SourcePort::messageWindow()
{
  return message_window_;
}

int32_t &SourcePort::messageStride()
{
  return message_stride_;
}

bool &SourcePort::blocking()
{
  return blocking_;
}

int32_t SourcePort::setBlocking(bool value)
{
  blocking_ = value;
  if (isConnected()) {
    return reader_.setBlockingCalls(value);
  }
  return 0;
}

Filter::Filter(YAML::Node config, int32_t n_sources, int32_t n_sinks)
    : yaml_config_(config),
      max_sources_(n_sources),
      max_sinks_(n_sinks)
{
  state_ = FilterState::DISCONNECTED;
  job_execution_model_ = JobExecutionModel::EXTERNAL_THREAD;

  source_ports_.resize(max_sources_);
  sink_ports_.resize(max_sinks_);

  pthread_mutex_init(&state_mtx_, NULL);

  name_ = "unnamed";
  if (!yaml_config_.IsNull() && yaml_config_.IsMap()) {
    if (yaml_config_["name"]) {
      name_ = yaml_config_["name"].as<std::string>();
    }
  }

  settings_ = Settings(name_);
  state_code_ = static_cast<int32_t>(state_.load());

  addSetting("name", name_, BASE_SETTING, "filter name", epf::R);
  addSetting("type", type_, BASE_SETTING, "filter type", epf::R);
  addSetting("state_", state_code_, BASE_SETTING, "filter state (numeric code)",
             epf::R);
  addCommand("open", std::bind(&Filter::open, this), CONTROL_SETTING, "open",
             epf::W_D);
  addCommand("set", std::bind(&Filter::set, this), CONTROL_SETTING, "set",
             epf::W_C);
  addCommand("start", std::bind(&Filter::start, this), CONTROL_SETTING, "start",
             epf::W_S);
  addCommand("stop", std::bind(&Filter::stop, this), CONTROL_SETTING, "stop",
             epf::W);
  addCommand("reset", std::bind(&Filter::reset, this), CONTROL_SETTING, "reset",
             epf::W);
  addCommand("close", std::bind(&Filter::close, this), CONTROL_SETTING, "close",
             epf::W);

  createSourcePortSettings();
  createSinkPortSettings();

  // if (!yaml_config_.IsNull() && yaml_config_.IsMap() &&
  //     yaml_config_["settings"]) {
  //   readSettings(yaml_config_["settings"]);
  // }
  if (!yaml_config_.IsNull() && yaml_config_.IsMap()) {
    readSettings(yaml_config_);
  }
}

Filter::~Filter()
{
  pthread_mutex_destroy(&state_mtx_);
}

JobExecutionModel Filter::jobExecutionModel() const
{
  return job_execution_model_;
}

FilterState Filter::state() const
{
  return state_;
}

const std::string &Filter::name() const
{
  return name_;
}

const Filter::SettingsSignal &Filter::settingsChanged() const
{
  return settings_changed_;
}

const Filter::StateSignal &Filter::stateChanged() const
{
  return state_changed_;
}

const Filter::ErrorSignal &Filter::errorOccurred() const
{
  return error_occurred_;
}

uint64_t Filter::settingsRevision() const
{
  return settings_revision_.load(std::memory_order_relaxed);
}

void Filter::notifySettingsChanged(SettingsChangeKind kind,
                                   const std::string &reason)
{
  const uint64_t revision =
      settings_revision_.fetch_add(1, std::memory_order_relaxed) + 1;

  // Emits on caller thread; UI must marshal to UI thread.
  settings_changed_(*this, revision, kind, reason);
}

void Filter::notifyStateChanged(FilterState old_state, FilterState new_state,
                                const std::string &reason)
{
  state_code_ = static_cast<int32_t>(new_state);
  // Emits on caller thread; UI must marshal to UI thread.
  state_changed_(*this, old_state, new_state, reason);
}

void Filter::notifyErrorOccurred(const std::string &reason)
{
  // Emits on caller thread; UI must marshal to UI thread.
  error_occurred_(*this, reason);
}

int32_t Filter::maxSources() const
{
  return static_cast<int32_t>(max_sources_);
}

int32_t Filter::maxSinks() const
{
  return static_cast<int32_t>(max_sinks_);
}

int32_t Filter::open()
{
  std::string error_reason;
  bool emit_state_changed = false;

  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::DISCONNECTED) {
    error_reason = "open failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // if (!yaml_config_.IsNull() && yaml_config_.IsMap()) {
  //   readSettings(yaml_config_["settings"]);
  // }
  // else {
  //   std::clog << "Filter \"" << name()
  //             << "\" has not received a valid config YAML, resorting to "
  //                "default filter config values."
  //             << std::endl;
  // }

  // virtual open()
  if (_open() < 0) {
    error_reason = "open failed: _open";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // TODO: settings addQueueSettings
  FilterState old_state = state_;
  state_ = FilterState::CONNECTED;
  FilterState new_state = state_;
  emit_state_changed = true;
  pthread_mutex_unlock(&state_mtx_);

  if (emit_state_changed) notifyStateChanged(old_state, new_state, "open");
  return 0;
}

int32_t Filter::set()
{
  std::string error_reason;
  bool emit_state_changed = false;

  pthread_mutex_lock(&state_mtx_);
  if (!((state_ == FilterState::CONNECTED) || (state_ == FilterState::SET))) {
    error_reason = "set failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // virtual set
  if (_set() < 0) {
    error_reason = "set failed: _set";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  FilterState old_state = state_;
  state_ = FilterState::SET;
  emit_state_changed = true;
  pthread_mutex_unlock(&state_mtx_);

  if (emit_state_changed) notifyStateChanged(old_state, state_, "set");

  return 0;
}

int32_t Filter::reset()
{
  FilterState timeout_old_state = state_;
  FilterState timeout_new_state = state_;
  bool emit_timeout_state_changed = false;
  bool emit_state_changed = false;
  bool emit_settings_changed = false;
  std::string error_reason;

  pthread_mutex_lock(&state_mtx_);

  if (state_ == FilterState::STOP_REQUEST) {
    pthread_mutex_unlock(&state_mtx_);  // Unlock before waiting

    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(500);  // Set a timeout

    while (state_ != FilterState::SET) {
      // std::this_thread::sleep_for(
      //     std::chrono::milliseconds(1));  // Avoid CPU burn
      usleep(1000);

      // Timeout check
      if (std::chrono::steady_clock::now() - start_time > timeout) {
        std::cerr << "Warning: reset() timeout. STOP_REQUEST Forcefully "
                     "setting state to SET.\n";
        _stop();
        timeout_old_state = state_;
        state_ = FilterState::SET;  // Forcefully reset state
        timeout_new_state = state_;
        emit_timeout_state_changed = true;
        break;
      }
    }

    pthread_mutex_lock(&state_mtx_);  // Re-lock mutex before proceeding
  }

  if (state_ != FilterState::SET) {
    error_reason = "reset failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    if (emit_timeout_state_changed) {
      notifyStateChanged(timeout_old_state, timeout_new_state, "reset timeout");
    }
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // virtual reset
  if (_reset() < 0) {
    error_reason = "reset failed: _reset";
    pthread_mutex_unlock(&state_mtx_);
    if (emit_timeout_state_changed) {
      notifyStateChanged(timeout_old_state, timeout_new_state, "reset timeout");
    }
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // Runtime connectivity teardown (self-contained reset path):
  // 1) Disconnect readers/subscribers first.
  // 2) Deactivate writer queues after all readers are gone.
  // This local teardown is intentionally kept even when a Pipeline already
  // performed coordinated graph teardown. Operations are guarded/idempotent,
  // so repeated disconnect/deactivate calls are acceptable by design.
  disconnectSourcePortsUnsafe();
  deactivateSinkPortsUnsafe();

  // Source filter subscriptions are runtime connectivity state.
  src_filters_.clear();
  emit_settings_changed = true;

  FilterState old_state = state_;
  state_ = FilterState::CONNECTED;
  emit_state_changed = true;
  pthread_mutex_unlock(&state_mtx_);

  if (emit_timeout_state_changed) {
    notifyStateChanged(timeout_old_state, timeout_new_state, "reset timeout");
  }
  if (emit_settings_changed) {
    notifySettingsChanged(SettingsChangeKind::StructureChanged,
                          "runtime connectivity reset");
  }
  if (emit_state_changed) notifyStateChanged(old_state, state_, "reset");
  return 0;
}

int32_t Filter::disconnectSourcePorts()
{
  pthread_mutex_lock(&state_mtx_);
  int32_t ret = disconnectSourcePortsUnsafe();
  pthread_mutex_unlock(&state_mtx_);
  return ret;
}

int32_t Filter::deactivateSinkPorts()
{
  pthread_mutex_lock(&state_mtx_);
  int32_t ret = deactivateSinkPortsUnsafe();
  pthread_mutex_unlock(&state_mtx_);
  return ret;
}

int32_t Filter::disconnectSourcePortsUnsafe()
{
  for (std::size_t i = 0; i < source_ports_.size(); i++) {
    if (source_ports_[i].isConnected()) {
      source_ports_[i].disconnect();
    }
  }
  return 0;
}

int32_t Filter::deactivateSinkPortsUnsafe()
{
  for (std::size_t i = 0; i < sink_ports_.size(); i++) {
    if (sink_ports_[i].isActivated()) {
      sink_ports_[i].deactivate();
    }
  }
  return 0;
}

int32_t Filter::start()
{
  FilterState timeout_old_state = state_;
  FilterState timeout_new_state = state_;
  bool emit_timeout_state_changed = false;
  bool emit_state_changed = false;
  std::string error_reason;

  pthread_mutex_lock(&state_mtx_);

  if (state_ == FilterState::STOP_REQUEST) {
    pthread_mutex_unlock(&state_mtx_);  // Unlock before waiting

    auto start_time = std::chrono::steady_clock::now();
    constexpr auto TIMEOUT = std::chrono::milliseconds(500);  // Set a timeout

    while (state_ != FilterState::SET) {
      // std::this_thread::sleep_for(
      //     std::chrono::milliseconds(1));  // Avoid CPU burn
      usleep(1000);

      // Timeout check
      if (std::chrono::steady_clock::now() - start_time > TIMEOUT) {
        std::cerr << "Warning: start() timeout. STOP_REQUEST Forcefully "
                     "setting state to SET.\n";
        _stop();
        timeout_old_state = state_;
        state_ = FilterState::SET;  // Forcefully reset state
        timeout_new_state = state_;
        emit_timeout_state_changed = true;
        break;
      }
    }

    pthread_mutex_lock(&state_mtx_);  // Re-lock mutex before proceeding
  }

  if (state_ != FilterState::SET) {
    error_reason = "start failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    if (emit_timeout_state_changed) {
      notifyStateChanged(timeout_old_state, timeout_new_state, "start timeout");
    }
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // Restore runtime queue access mode (stop() forces non-blocking to wake
  // workers); on restart we must re-apply configured blocking behavior.
  for (size_t i = 0; i < sink_ports_.size(); ++i) {
    if (sink_ports_[i].isActivated()) {
      sink_ports_[i].writer()->setBlockingCalls(sink_ports_[i].blocking());
    }
  }
  for (size_t i = 0; i < source_ports_.size(); ++i) {
    if (source_ports_[i].isConnected()) {
      source_ports_[i].reader()->setBlockingCalls(source_ports_[i].blocking());
    }
  }

  // Virtual start
  if (_start() < 0) {
    error_reason = "start failed: _start";
    pthread_mutex_unlock(&state_mtx_);
    if (emit_timeout_state_changed) {
      notifyStateChanged(timeout_old_state, timeout_new_state, "start timeout");
    }
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // Start thread
  FilterState old_state = state_;
  state_ = FilterState::RUNNING;
  emit_state_changed = true;
  pthread_mutex_unlock(&state_mtx_);

  if (emit_timeout_state_changed) {
    notifyStateChanged(timeout_old_state, timeout_new_state, "start timeout");
  }
  if (emit_state_changed) notifyStateChanged(old_state, state_, "start");

  return 0;
}

int32_t Filter::stop()
{
  std::string error_reason;
  bool emit_state_changed = false;

  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::RUNNING) {
    error_reason = "stop failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  FilterState old_state = state_;
  state_ = FilterState::STOP_REQUEST;  // Request stop
  emit_state_changed = true;

  for (size_t i = 0; i < sink_ports_.size(); ++i)
    if (sink_ports_[i].isActivated()) {
      sink_ports_[i].writer()->setBlockingCalls(false);
      sink_ports_[i].writer()->wakeUp();
    }

  for (size_t i = 0; i < source_ports_.size(); ++i)
    //    if (source_ports_[i].reader()->queue() ) {
    //    if (source_ports_[i].isConnected()) {
    if (source_ports_[i].isConnected()) {
      source_ports_[i].reader()->setBlockingCalls(false);
      source_ports_[i].reader()->wakeUp();
    }

  pthread_mutex_unlock(&state_mtx_);

  if (emit_state_changed) notifyStateChanged(old_state, state_, "stop");

  return 0;
}

int32_t Filter::close()
{
  FilterState timeout_old_state = state_;
  FilterState timeout_new_state = state_;
  bool emit_timeout_state_changed = false;
  std::string error_reason;
  bool emit_state_changed = false;

  pthread_mutex_lock(&state_mtx_);
  // Lifecycle-normalizing close:
  // RUNNING -> stop() -> SET -> reset() -> CONNECTED -> _close() ->
  // DISCONNECTED. This preserves structural topology while clearing only
  // runtime state in reset().
  if (state_ == FilterState::RUNNING) {
    pthread_mutex_unlock(&state_mtx_);
    if (stop() < 0) {
      notifyErrorOccurred("close warning: stop failed");
    }
    pthread_mutex_lock(&state_mtx_);
  }

  if (state_ == FilterState::STOP_REQUEST) {
    pthread_mutex_unlock(&state_mtx_);

    auto start_time = std::chrono::steady_clock::now();
    constexpr auto TIMEOUT = std::chrono::milliseconds(500);

    while (state_ != FilterState::SET) {
      usleep(1000);
      if (std::chrono::steady_clock::now() - start_time > TIMEOUT) {
        std::cerr << "Warning: close() timeout. STOP_REQUEST Forcefully "
                     "setting state to SET.\n";
        // Execute filter-specific stop hook outside state mutex to avoid
        // re-entrancy/deadlock risks in custom filter implementations.
        _stop();
        pthread_mutex_lock(&state_mtx_);
        timeout_old_state = state_;
        if (state_ == FilterState::STOP_REQUEST) {
          state_ = FilterState::SET;
          timeout_new_state = state_;
          emit_timeout_state_changed = true;
        }
        pthread_mutex_unlock(&state_mtx_);
        break;
      }
    }

    pthread_mutex_lock(&state_mtx_);
  }

  if (state_ == FilterState::SET) {
    pthread_mutex_unlock(&state_mtx_);
    if (reset() < 0) {
      notifyErrorOccurred("close warning: reset failed");
    }
    pthread_mutex_lock(&state_mtx_);
  }

  if (state_ == FilterState::DISCONNECTED) {
    pthread_mutex_unlock(&state_mtx_);
    if (emit_timeout_state_changed) {
      notifyStateChanged(timeout_old_state, timeout_new_state, "close timeout");
    }
    return 0;
  }

  const bool runtime_close_possible =
      (state_ == FilterState::CONNECTED || state_ == FilterState::SET);
  if (!runtime_close_possible) {
    error_reason = "close failed: invalid state";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  // close() is a best-effort final shutdown path. Before invoking _close(),
  // clear any remaining local runtime queue connectivity so standalone
  // filters can close safely even from partially connected states.
  disconnectSourcePortsUnsafe();
  deactivateSinkPortsUnsafe();

  // virtual close
  if (_close() < 0) {
    error_reason = "close failed: _close";
    pthread_mutex_unlock(&state_mtx_);
    notifyErrorOccurred(error_reason);
    return -1;
  }

  FilterState old_state = state_;
  state_ = FilterState::DISCONNECTED;
  emit_state_changed = true;
  pthread_mutex_unlock(&state_mtx_);

  if (emit_timeout_state_changed) {
    notifyStateChanged(timeout_old_state, timeout_new_state, "close timeout");
  }
  if (emit_state_changed) notifyStateChanged(old_state, state_, "close");
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
  int32_t err = -1;
  if (state_ == FilterState::RUNNING) {
    err = this->_job();
    return err;
  }
  else if (state_ == FilterState::STOP_REQUEST) {
    completeStopTransitionIfRequested("job stop");
    return -1;
  }
  return err;
}

int32_t Filter::completeStopTransitionIfRequested(const std::string &reason)
{
  pthread_mutex_lock(&state_mtx_);
  if (state_ != FilterState::STOP_REQUEST) {
    pthread_mutex_unlock(&state_mtx_);
    return 0;
  }
  pthread_mutex_unlock(&state_mtx_);

  const int32_t stop_ret = _stop();

  bool emit_state_changed = false;
  FilterState old_state = FilterState::STOP_REQUEST;
  FilterState new_state = FilterState::STOP_REQUEST;

  pthread_mutex_lock(&state_mtx_);
  old_state = state_;
  if (state_ == FilterState::STOP_REQUEST) {
    state_ = FilterState::SET;
    new_state = state_;
    emit_state_changed = true;
  }
  pthread_mutex_unlock(&state_mtx_);

  if (emit_state_changed) {
    notifyStateChanged(old_state, new_state, reason);
  }

  return (stop_ret < 0) ? -1 : 0;
}

int32_t Filter::setDeviceSettingValue(const char *key, const void *value)
{
  std::cout << "Filter::setDeviceSettingValue(" << key << "," << value << ")"
            << std::endl;
  return -1;
}

int32_t Filter::setDeviceSettingValueStr(const char *key, const char *value)
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

const Node *Filter::settingNode(std::string &name) const
{
  return settings_[name];
}

int32_t Filter::addCommand(std::string name, std::function<int()> command,
                           epf::SettingType setting_type, std::string tooltip,
                           AccessType accessmode)
{
  int32_t ret =
      settings_.addCommand(name, command, setting_type, tooltip, accessmode);
  if (ret >= 0) {
    notifySettingsChanged(SettingsChangeKind::CommandAdded,
                          "command added: " + name);
  }
  return ret;
}

int32_t Filter::runCommand(std::string name)
{
  std::optional<AccessType> acces_mode = settings_.settingAccessMode(name);
  if (acces_mode) {
    if (access_allowed_in_state(acces_mode.value(), state_))
      return settings_.runCommand(name);
    else {
      std::cout << "Not allowed to set setting " << name
                << " in \"state_=" << state2string(state_) << "\"."
                << std::endl;
      return -1;
    }
  }
  else {
    std::cerr << "runCommand: Command \"" << name << "\" not found."
              << std::endl;
    return -1;
  }
}

const Settings *Filter::settings() const
{
  return &settings_;
}

int32_t Filter::readSettings(const YAML::Node &filter_node)
{
  // Read basic settings
  if (filter_node["name"]) name_ = filter_node["name"].as<std::string>();
  if (filter_node["type"]) type_ = filter_node["type"].as<std::string>();

  settings_.setName(name_);

  // Read device settings
  settings_.fromYAML(filter_node);

  if (filter_node["device_settings"]) {
    if (state_ != DISCONNECTED) {
      deviceSettingsFromYAML(filter_node["device_settings"]);
    }
  }
  notifySettingsChanged(SettingsChangeKind::Loaded, "settings read");
  return 0;
}

int32_t Filter::writeSettings(YAML::Node &filter_node)
{
  // General settings
  filter_node["name"] = name_;
  filter_node["type"] = type_;

  settings_.toYAML(filter_node);

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

int32_t Filter::saveSettings(const std::string &filename)
{
  // Create a YAML node and write the current settings into it
  YAML::Node filter_node;
  writeSettings(filter_node);

  try {
    if (filename.empty()) {
      std::cout << filter_node << std::endl;
    }
    else {
      // Save the YAML node to a file
      std::ofstream fout(filename);
      fout << filter_node;
    }
  }
  catch (const std::exception &e) {
    std::cerr << "Error saving settings to file: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}

int32_t Filter::connect(int src_port_index, SinkPort *sink_port)
{
  if (!sink_port->isActivated()) {
    std::cerr << "Filter::connectSourcePort: target SinkPort is not activated"
              << std::endl;
    return -1;
  }

  if (src_port_index < 0) {
    std::cerr << "Filter::connectSourcePort port < 0" << std::endl;
    return -1;
  }

  SourcePort *src_port = sourcePort(src_port_index);

  pthread_mutex_lock(&state_mtx_);

  if ((state_ == FilterState::CONNECTED) && src_port) {
    int32_t ret = src_port->connect(sink_port->queue());
    pthread_mutex_unlock(&state_mtx_);
    return ret;
  }

  pthread_mutex_unlock(&state_mtx_);
  return -1;
}

std::vector<int> Filter::connectedSources()
{
  std::vector<int> connected_sources;
  // Iterate over all possible source queues
  for (size_t i = 0; i < source_ports_.size(); ++i) {
    // Check if the source queue is connected
    //    if (source_ports_[i].isConnected()) {
    if (source_ports_[i].isConnected()) {
      //    if (source_ports_[i].queue() != nullptr) {
      connected_sources.push_back(static_cast<int32_t>(i));
    }
  }
  return connected_sources;
}

std::vector<int> Filter::activatedSinks()
{
  std::vector<int> connected_sinks;
  // Iterate over all possible sink queues
  for (size_t i = 0; i < sink_ports_.size(); ++i) {
    // Check if the sink queue is connected
    if (sink_ports_[i].isActivated()) {
      connected_sinks.push_back(static_cast<int32_t>(i));
    }
  }
  return connected_sinks;
}

int32_t Filter::connectSourceFilter(Filter *src_filter)
{
  if (src_filter == nullptr) return -1;

  for (auto it : src_filters_) {
    if (it == src_filter) return -1;
  }
  src_filters_.push_back(src_filter);
  return 0;
}

SourcePort *Filter::sourcePort(int32_t port_index) noexcept
{
  if (port_index >= 0 &&
      static_cast<size_t>(port_index) < source_ports_.size()) {
    return &source_ports_[port_index];
  }
  return nullptr;
}

SinkPort *Filter::sinkPort(int32_t port_index) noexcept
{
  if (port_index >= 0 && static_cast<size_t>(port_index) < sink_ports_.size()) {
    return &sink_ports_[port_index];
  }
  return nullptr;
}

int32_t Filter::readDeviceSettings(const YAML::Node &yaml_node)
{
  if (yaml_node["device_settings"])
    deviceSettingsFromYAML(yaml_node["device_settings"]);
  return 0;
}

int32_t Filter::createSourcePortSettings()
{
  for (size_t i = 0; i < source_ports_.size(); i++) {
    std::unique_ptr<ObjectNode> node =
        std::make_unique<ObjectNode>(std::to_string(i));
    int32_t index =
        settings_.addSettingNode(std::move(node), SOURCE_PORT_SETTING);

    AccessType access = epf::W_D | epf::W_C;

    settings_.addSettingUnder(
        "message_window", source_ports_[i].messageWindow(), index,
        "Number of messages read by the reader in each iteration.", access);
    settings_.addSettingUnder(
        "message_stride", source_ports_[i].messageStride(), index,
        "Number of messages that the reader advances in each call.", access);
    settings_.addSettingUnder("blocking", source_ports_[i].blocking(), index,
                              "The reading calls are blocking or not", access);
  }

  notifySettingsChanged(SettingsChangeKind::StructureChanged,
                        "source port settings created");
  return 0;
}

int32_t Filter::createSinkPortSettings()
{
  for (size_t i = 0; i < sink_ports_.size(); i++) {
    std::unique_ptr<ObjectNode> node =
        std::make_unique<ObjectNode>(std::to_string(i));
    int32_t index =
        settings_.addSettingNode(std::move(node), SINK_PORT_SETTING);

    AccessType access = epf::W_D | epf::W_C;

    settings_.addSettingUnder("length", sink_ports_[i].length(), index,
                              "Queue lenght.", access);
    settings_.addSettingUnder("max_readers", sink_ports_[i].maxReaders(), index,
                              "Maximum readers of the port queue.", access);
    settings_.addSettingUnder("max_writers", sink_ports_[i].maxWriters(), index,
                              "Maximum readers of the port queue.", access);
    settings_.addSettingUnder("queue_type", sink_ports_[i].queueType(), index,
                              "Type of queue (lifo/fifo)", access);
    settings_.addEnumOptions(
        "sink_ports." + std::to_string(i) + ".queue_type",
        {{static_cast<int32_t>(epf::QueueType::fifo), "fifo", "FIFO"},
         {static_cast<int32_t>(epf::QueueType::lifo), "lifo", "LIFO"}});
    settings_.addSettingUnder("batch_size", sink_ports_[i].batchSize(), index,
                              "Size of the batch that writer write.", access);
    settings_.addSettingUnder("blocking", sink_ports_[i].blocking(), index,
                              "The writing calls are blocking or not", access);
    settings_.addSettingUnder("timestamp", sink_ports_[i].timestamp(), index,
                              "Enable automatic timestamp", access);
  }

  notifySettingsChanged(SettingsChangeKind::StructureChanged,
                        "sink port settings created");
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
      if (err < 0) {
        std::cout << "setSettingValue: " << key << "[FAILED]" << std::endl;
        //        return err;
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
