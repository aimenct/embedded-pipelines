// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _FILTER_H
#define _FILTER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "queue.h"
#include "queue_handlers.h"
#include "settings.h"

namespace ep {

/**
 * @brief Enum to represent the filter states.
 */
enum FilterState {
  DISCONNECTED = 0b0000001,
  CONNECTED = 0b0000010,
  SET = 0b0000100,
  RUNNING = 0b0001000,
};

std::string state2string(const FilterState &state);

/**
 * @brief Enum to represent different job execution models for the filter.
 */
enum JobExecutionModel { EXTERNAL_THREAD, OWN_THREAD, MAIN_LOOP };

class Filter;

// SinkQueueSettings class
class SinkQueueSettings {
  public:
    int length_{10};                   // Default length
    int max_readers_{5};               // Default max readers
    int max_writers_{1};               // Default max writers
    QueueType type_{QueueType::lifo};  // Default queue type

    SinkQueueSettings() = default;
    SinkQueueSettings(int len, int max_readers, int max_writers, QueueType type)
        : length_(len),
          max_readers_(max_readers),
          max_writers_(max_writers),
          type_(type)
    {
    }
    void reset()
    {
      length_ = 10;             // Default length
      max_readers_ = 5;         // Default max readers
      max_writers_ = 1;         // Default max writers
      type_ = QueueType::lifo;  // Default queue type
    }
};

// QueueHandlerSettings structure
struct QueueHandlerSettings {
    int batch_{1};          // Default batch size
    int new_per_batch_{1};  // Default items added per batch
    bool blocking_{true};   // Default to blocking mode

    QueueHandlerSettings() = default;
    QueueHandlerSettings(int b, int npb, bool blk)
        : batch_(b),
          new_per_batch_(npb),
          blocking_(blk)
    {
    }
    void reset()
    {
      batch_ = 1;          // Default batch size
      new_per_batch_ = 1;  // Default items added per batch
      blocking_ = true;    // Default to blocking mode
    }
};

// SinkPort class
class SinkPort {
  public:
    //    bool connected_{false};
    SinkQueueSettings queue_settings_;
    QueueHandlerSettings writer_settings_;
    QueueWriter *writer_{nullptr};
    std::shared_ptr<Queue> queue_{nullptr};

    explicit SinkPort(
        const SinkQueueSettings &q_settings = SinkQueueSettings(),
        const QueueHandlerSettings &w_settings = QueueHandlerSettings())
        : queue_settings_(q_settings),
          writer_settings_(w_settings)
    {
    }

    void reset()
    {
      queue_settings_.reset();
      writer_settings_.reset();
      delete writer_;  // unsubscribe from sink queue
      writer_ = nullptr;
      queue_->free();
      queue_ = nullptr;
    }
};

// SourcePort class
class SourcePort {
  public:
    QueueHandlerSettings reader_settings_;
    QueueReader *reader_{nullptr};
    std::shared_ptr<Queue> queue_{nullptr};
    Filter *src_filter_{nullptr};

    explicit SourcePort(
        const QueueHandlerSettings &r_settings = QueueHandlerSettings())
        : reader_settings_(r_settings)
    {
    }

    void reset()
    {
      reader_settings_.reset();
      delete reader_;  // unsubscribe from source queue
      reader_ = nullptr;
      queue_ = nullptr;
      src_filter_ = nullptr;
    }
};

// Filter class
class Filter {
  public:
    /**
     * @brief Default constructor. Initializes the Filter instance.
     */
    Filter();

    /**
     * @brief Destructor. Cleans up resources used by the Filter instance.
     */
    virtual ~Filter();

    /**
     * @brief Establishes a connection with a device (if applicable).
     *        Transitions the Filter to the connected state.
     *        Enables connection of data source queues for data flow.
     * @return Error code indicating success or failure.
     */
    int32_t open();

    /**
     * @brief Checks compatibility with data sources and initializes sinks.
     *        Allocates memory resources if necessary.
     *        Transitions the Filter to the set state.
     * @return Error code indicating success or failure.
     */
    int32_t set();

    /**
     * @brief Resets data sinks and releases resources.
     *        Transitions the Filter to the connected state.
     * @return Error code indicating success or failure.
     */
    int32_t reset();

    /**
     * @brief Starts the filter. Enables job execution.
     *        Transitions the Filter to the running state.
     * @return Error code indicating success or failure.
     */
    int32_t start();

    /**
     * @brief Stops the thread. Disables job execution.
     *        Transitions the Filter to the stopped state.
     * @return Error code indicating success or failure.
     */
    int32_t stop();

    /**
     * @brief Closes communication with the device (if applicable).
     *        Transitions the Filter to the disconnected state.
     * @return Error code indicating success or failure.
     */
    int32_t close();

    /**
     * @brief Static function to execute the job associated with the filter.
     * @param self Pointer to the Filter instance.
     * @return Pointer to the result (typically null for threads).
     */
    static void *doJob(void *self);

    /**
     * @brief function to execute the job associated with the filter.
     * @return Error code indicating success or failure.
     */
    int32_t doJob();

    /**
     * @brief Adds a setting to the filter.
     * @tparam T Type of the setting value.
     * @param name Name of the setting.
     * @param value Reference to the setting value.
     * @param setting_type Type of the setting.
     * @param tooltip Tooltip for the setting.
     * @param accessmode Access mode for the setting.
     * @return Error code indicating success or failure.
     */
    template <typename T>
    int32_t addSetting(std::string name, T &value,
                       ep::SettingType setting_type = ep::FILTER_SETTING,
                       std::string tooltip = "", AccessType accessmode = ep::W)
    {
      // check if setting exists in device getting value
      if (setting_type == DEVICE_SETTING) {
        int err = deviceSettingValue(name.c_str(), static_cast<void *>(&value));
        if (err < 0) {
          std::cout << "addDeviceSetting: " << name << " [FAILED]" << std::endl;
          return err;
        }
      }

      settings_.addSetting(name, value, setting_type, tooltip, accessmode);

      return 0;
    }

    /**
     * @brief Adds a command to the filter's settings object.
     * @param name Key name of the command.
     * @param command Function without arguments that returns an error code.
     * @return Error code indicating success or failure.
     */
    int32_t addCommand(std::string name, std::function<int32_t()> command,
                       std::string tooltip = "", AccessType accesstype = ep::W);

    /**
     * @brief Sets the setting value belonging to the device.
     * @param key Name of the setting.
     * @param value Value to set.
     * @return Error code indicating success or failure.
     */
    virtual int32_t setDeviceSettingValue(const char *key, const void *value);

    /**
     * @brief Sets the setting value belonging to the device (text interface).
     * @param key Name of the setting.
     * @param value Value to set as a string.
     * @return Error code indicating success or failure.
     */
    virtual int32_t setDeviceSettingValueStr(const char *key,
                                             const char *value);

    /**
     * @brief Retrieves the setting value belonging to the device (text
     * interface).
     * @param key Name of the setting.
     * @param value Pointer to store the retrieved value.
     * @return Error code indicating success or failure.
     */
    virtual int32_t deviceSettingValue(const char *key, void *value);

    /**
     * @brief Sets the value of a setting.
     * @tparam T Type of the setting value.
     * @param name Name of the setting.
     * @param value Value to set.
     * @return Error code indicating success or failure.
     */
    template <typename T>
    int32_t setSettingValue(std::string name, T value)
    {
      int32_t ret = 0;
      if (settings_.settingAccessMode(name) & state_) {
        if (settings_.isDeviceSetting(name)) {
          if constexpr (std::is_same<T, std::string>::value)
            ret = setDeviceSettingValueStr(name.c_str(), value.c_str());
          else
            ret = setDeviceSettingValue(name.c_str(), &value);
          if (ret >= 0) ret = settings_.setValue<T>(name, value);
          return ret;
        }
        else {
          ret = settings_.setValue<T>(name, value);
          if (ret < 0) std::cout << "setSettingValue: failed." << std::endl;
          return ret;
        }
      }
      else {
        std::cout << "Not allowed to set setting " << name
                  << " in \"state_=" << state2string(state_) << "\"."
                  << std::endl;
        return -1;
      }
    }

    /**
     * @brief Retrieves the value of a setting.
     * @tparam T Type of the setting value.
     * @param name Name of the setting.
     * @return Pointer to the setting value, or null if not found.
     */
    template <typename T>
    const T *settingValue(std::string name)
    {
      T value;
      if (settings_.isDeviceSetting(name)) {
        if constexpr (std::is_same<T, std::string>::value) {
          char placeholder[256];
          deviceSettingValue(name.c_str(), placeholder);
          value = std::string(placeholder);
          setSettingValue<T>(name, value);
        }
        else {
          deviceSettingValue(name.c_str(), &value);
          setSettingValue<T>(name, value);
        }
      }
      return settings_.value<T>(name);
    }

    /**
     * @brief Retrieves the node associated with a setting.
     * @param name Name of the setting.
     * @return Pointer to the setting node, or null if not found.
     */
    const Node2 *settingNode(std::string &name) const;

    /**
     * @brief Executes a command associated with the filter.
     * @param name Key name of the command.
     * @return Error code of the command execution.
     */
    int32_t runCommand(std::string name);

    /**
     * @brief Reads settings from a YAML configuration node.
     * @param config YAML node containing the configuration.
     * @return Error code indicating success or failure.
     */
    int32_t readSettings(const YAML::Node &config);

    /**
     * @brief Reads device-specific settings from a YAML configuration node.
     * @param config YAML node containing the configuration.
     * @return Error code indicating success or failure.
     */
    int32_t readDeviceSettings(const YAML::Node &config);

    /**
     * @brief Writes settings to a YAML configuration node.
     * @param config YAML node to write the configuration to.
     * @return Error code indicating success or failure.
     */
    int32_t writeSettings(YAML::Node &config);

    /**
     * @brief Loads settings from a YAML configuration file.
     * @param filename Path to the YAML configuration file.
     * @return Error code indicating success or failure.
     */
    int32_t loadSettings(const std::string &filename);

    /**
     * @brief Saves settings to a YAML configuration file.
     * @param filename Path to the YAML configuration file.
     * @return Error code indicating success or failure.
     */
    int32_t saveSettings(const std::string &filename);

    /**
     * @brief Connects a source queue to a specific port.
     * @param port_index Index of the port.
     * @param q Pointer to the queue to connect.
     * @param src_filter Pointer to the source filter (optional).
     * @return Error code indicating success or failure.
     */
    int32_t connectSourceQueue(int port_index, std::shared_ptr<Queue> q,
                               Filter *src_filter = nullptr);

    /**
     * @brief Retrieves a list of connected source port indices.
     * @return Vector of connected source port indices.
     */
    std::vector<int> connectedSources() const;

    /**
     * @brief Retrieves a list of connected sink port indices.
     * @return Vector of connected sink port indices.
     */
    std::vector<int> connectedSinks() const;

    /**
     * @brief Retrieves the number of sink ports.
     * @return Number of sink ports.
     */
    int32_t sinkPorts() const;

    /**
     * @brief Retrieves the number of source ports.
     * @return Number of source ports.
     */
    int32_t sourcePorts() const;

    /**
     * @brief Retrieves the writer associated with a specific port.
     * @param port_index Index of the port.
     * @return Pointer to the QueueWriter object, or null if not found.
     */
    QueueWriter *writer(int port_index);

    /**
     * @brief Retrieves the reader associated with a specific port.
     * @param port_index Index of the port.
     * @return Pointer to the QueueReader object, or null if not found.
     */
    QueueReader *reader(int port_index);

    /**
     * @brief Retrieves the sink queue associated with a specific port.
     * @param port_index Index of the port.
     * @return Pointer to the Queue object, or null if not found.
     */
    std::shared_ptr<Queue> sinkQueue(int port_index) const;

    /**
     * @brief Retrieves the source queue associated with a specific port.
     * @param port_index Index of the port.
     * @return Pointer to the Queue object, or null if not found.
     */
    std::shared_ptr<Queue> sourceQueue(int port_index) const;

    /** @brief Retrieves the execution mode of the job
     * @return jobExecutionModel
     */
    JobExecutionModel jobExecutionModel() const;

    /**
     * @brief Retrieves the current state of the filter.
     * @return Current state of the filter.
     */
    const FilterState &state();

    /**
     * @brief Retrieves the name of the filter.
     * @return Name of the filter.
     */
    const std::string &name();

    /**
     * @brief Retrieves the settings object of the filter.
     * @return Pointer to the Settings2 object.
     */
    const Settings2 *settings();

  protected:
    /**
     * @brief Starts the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _start() = 0;

    /**
     * @brief Stops the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _stop() = 0;

    /**
     * @brief Sets the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _set() = 0;

    /**
     * @brief Resets the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _reset() = 0;

    /**
     * @brief Opens the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _open() = 0;

    /**
     * @brief Closes the filter (internal implementation).
     *        Should be overridden by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _close() = 0;

    /**
     * @brief Pure virtual function to define the job execution.
     *        Must be implemented by derived classes.
     * @return Error code indicating success or failure.
     */
    virtual int32_t _job() = 0;

    /**
     * @brief Adds a sink queue at a specific port.
     * @param port Index of the port.
     * @param data_schema Pointer to the message schema.
     * @param hdr_schema Pointer to the header schema (optional).
     * @return Error code indicating success or failure.
     */
    int32_t addSinkQueue(int port, Message *data_schema,
                         Message *hdr_schema = nullptr);

    /**
     * @brief Connects a sink queue to a specific port.
     * @param port_index Index of the port.
     * @param q Pointer to the queue to connect.
     * @return Error code indicating success or failure.
     */
    int32_t connectSinkQueue(int port_index, std::shared_ptr<Queue> q);

    FilterState state_;                     /**< Current status of the filter */
    pthread_mutex_t state_mtx_;             /**< Mutex for status protection */
    std::string name_;                      /**< Name of the filter */
    std::string type_;                      /**< Type of the filter */
    JobExecutionModel job_execution_model_; /**< Job execution model */
    Settings2 settings_;                    /**< Settings object */

    std::vector<SinkPort> sink_ports_;     /**< Vector of sink ports */
    std::vector<SourcePort> source_ports_; /**< Vector of source ports */
    int max_sinks_{0};                     /**< Maximum number of sink ports */
    int max_sources_{0}; /**< Maximum number of source ports */

    void readQueueSettings(const YAML::Node &queue_node,
                           SinkQueueSettings &queue_settings);
    void readWriterSettings(const YAML::Node &writer_node,
                            QueueHandlerSettings &writer_settings);
    void readReaderSettings(const YAML::Node &reader_node,
                            QueueHandlerSettings &reader_settings);

  private:
    int32_t deviceSettingsFromYAML(const YAML::Node &config);
    int32_t deviceSettingsToYAML(YAML::Node &config);
};

}  // namespace ep

#endif  // _FILTER_H
