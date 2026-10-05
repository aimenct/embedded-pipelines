// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef _FILTER_H
#define _FILTER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaml-cpp/yaml.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "queue.h"
#include "queue_handlers.h"
#include "settings.h"
#include "signals.h"

namespace epf {

class Pipeline;

// SinkPort class
class SinkPort {
  public:
    // Constructs a SinkPort with default settings.
    explicit SinkPort() = default;

    // Move constructor.
    SinkPort(SinkPort &&other) noexcept;

    // Move assignment operator.
    SinkPort &operator=(SinkPort &&other) noexcept;

    // Copy constructor is deleted.
    SinkPort(const SinkPort &) = delete;

    // Copy assignment operator is deleted.
    SinkPort &operator=(const SinkPort &) = delete;

    // Destroys the SinkPort instance.
    ~SinkPort() = default;

    // Constructs a SinkPort with the given configuration.
    SinkPort(int32_t length, int32_t max_readers, int32_t max_writers,
             QueueType queue_type, int32_t batch_size, bool blocking);

    // Activates the port with data and header schemas.
    int32_t activate(std::unique_ptr<Message> data_schema,
                     std::unique_ptr<Message> hdr_schema);

    // Returns whether the port is currently activated.
    bool isActivated();

    // Returns a pointer to the internal queue.
    Queue *queue();

    // Returns a pointer to the internal writer.
    QueueWriter *writer();

    // Deactivates the port.
    int32_t deactivate();

    // Returns a reference to the queue type (changes will take effect on
    // re-activation).
    QueueType &queueType();

    // Returns a reference to the queue length (changes will take effect on
    // re-activation).
    int32_t &length();

    // Returns a reference to the max reader count (changes will take effect on
    // re-activation).
    int32_t &maxReaders();

    // Returns a reference to the max writer count (changes will take effect on
    // re-activation)..
    int32_t &maxWriters();

    // Returns a reference to the batch size (changes will take effect on
    // re-activation)..
    int32_t &batchSize();

    // Returns a reference to the blocking flag (changes will take effect on
    // re-activation).
    bool &blocking();

    // Returns a reference to the timestamp flag (changes will take effect on
    // re-activation).
    bool &timestamp();

    // Enable or disable timestamping
    int32_t setTimestamp(bool value);

    // Sets the blocking mode.
    int32_t setBlocking(bool value);

  private:
    Queue queue_;
    QueueWriter writer_;

    int32_t length_{10};                     // Default length
    int32_t max_readers_{5};                 // Default max readers
    int32_t max_writers_{1};                 // Default max writers
    QueueType queue_type_{QueueType::lifo};  // Default queue type

    int32_t batch_size_{1};  // Default batch size
    bool blocking_{true};    // Default to blocking mode
    bool timestamp_{false};  // Timestamp disabled by default
};

// SourcePort class
// SourcePort class
class SourcePort {
  public:
    // Constructs a SourcePort with default settings.
    explicit SourcePort() = default;

    // Constructs a SourcePort with the given configuration.
    SourcePort(int32_t message_window, int32_t message_stride, bool blocking);

    // Destroys the SourcePort instance.
    ~SourcePort();

    // Connects the port to the given queue.
    int32_t connect(Queue *q);

    // Returns whether the port is currently connected.
    bool isConnected();

    // Disconnects the port and reader from the queue.
    int32_t disconnect();

    // Returns a pointer to the internal reader.
    QueueReader *reader();

    // Returns a pointer to the connected queue.
    Queue *queue() const;

    // Returns a reference to the batch size (changes will take effect on
    // re-activation).
    int32_t &messageWindow();

    // Returns a reference to the new items per batch (changes will take effect
    // on re-activation).
    int32_t &messageStride();

    // Returns a reference to the blocking flag (changes will take effect on
    // re-activation)..
    bool &blocking();

    // Sets the blocking mode.
    int32_t setBlocking(bool value);

  private:
    Queue *queue_{nullptr};
    QueueReader reader_;

    int32_t message_window_{1};  // Default batch size
    int32_t message_stride_{1};  // Default items added per batch
    bool blocking_{true};        // Default to blocking mode
};

// Filter class
class Filter {
  public:
    using SettingsSignal = fteng::signal<void(
        const Filter &, uint64_t, SettingsChangeKind, const std::string &)>;
    using StateSignal = fteng::signal<void(const Filter &, FilterState,
                                           FilterState, const std::string &)>;
    using ErrorSignal =
        fteng::signal<void(const Filter &, const std::string &)>;

    /**
     * @brief Default constructor. Initializes the Filter instance.
     */
    explicit Filter(YAML::Node config = YAML::Node(), int32_t n_sources = 1,
                    int32_t n_sinks = 1);

    /**
     * @brief Destructor. Cleans up resources used by the Filter instance.
     */
    virtual ~Filter();

    /** @brief Retrieves the execution mode of the job
     * @return jobExecutionModel
     */
    JobExecutionModel jobExecutionModel() const;

    /**
     * @brief Retrieves the current state of the filter.
     * @return Current state of the filter.
     */
    FilterState state() const;

    /**
     * @brief Retrieves the name of the filter.
     * @return Name of the filter.
     */
    const std::string &name() const;

    const SettingsSignal &settingsChanged() const;
    const StateSignal &stateChanged() const;
    const ErrorSignal &errorOccurred() const;

    uint64_t settingsRevision() const;

    int32_t maxSources() const;

    int32_t maxSinks() const;

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
     * @brief Resets runtime connectivity and releases runtime resources.
     *        This method is intentionally self-contained so a Filter can be
     *        used safely without a Pipeline instance.
     *        Sink/source port vectors are preserved (topology), while queue
     *        activation/subscriptions are cleared.
     *        Reset teardown is guarded/idempotent, so repeated
     *        disconnect/deactivate calls are acceptable by design.
     *        Valid transition is SET -> CONNECTED.
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
     * @brief Requests stop of job execution.
     *        Valid transition is RUNNING -> STOP_REQUEST.
     *        Convergence to SET happens later (normally via doJob()/ _stop()
     *        and guaranteed by Pipeline::join() for pipeline-managed filters).
     * @return Error code indicating success or failure.
     */
    int32_t stop();

    /**
     * @brief Closes communication with the device (if applicable).
     *        If needed, normalizes lifecycle first:
     *        RUNNING -> STOP_REQUEST -> SET -> CONNECTED
     *        then invokes the virtual close path.
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
     * @brief Completes a pending stop transition, if currently requested.
     *
     * If the filter is in STOP_REQUEST, this executes the stop-completion
     * path (_stop() + STOP_REQUEST->SET state convergence) and emits the
     * regular stateChanged signal when convergence happens.
     *
     * This helper is used by worker execution (doJob()) and by pipeline
     * post-join convergence so stop completion behavior remains consistent.
     *
     * @param reason Reason string used for stateChanged emission when
     * convergence occurs.
     * @return 0 if stop completion succeeded or no completion was needed, -1
     * when _stop() reported an error.
     */
    int32_t completeStopTransitionIfRequested(
        const std::string &reason = "job stop");

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
                       epf::SettingType setting_type = epf::BASE_SETTING,
                       std::string tooltip = "",
                       AccessType accessmode = epf::W);

    /**
     * @brief Retrieves the value of a setting.
     * @tparam T Type of the setting value.
     * @param name Name of the setting.
     * @return Pointer to the setting value, or null if not found.
     */
    template <typename T>
    const T *settingValue(std::string name);

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
    int32_t setSettingValue(std::string name, T value);

    /**
     * @brief Retrieves the node associated with a setting.
     * @param name Name of the setting.
     * @return Pointer to the setting node, or null if not found.
     */
    const Node *settingNode(std::string &name) const;

    /**
     * @brief Adds a command to the filter's settings object.
     * @param name Key name of the command.
     * @param command Function without arguments that returns an error code.
     * @return Error code indicating success or failure.
     */
    int32_t addCommand(std::string name, std::function<int32_t()> command,
                       epf::SettingType setting_type = epf::CONTROL_SETTING,
                       std::string tooltip = "",
                       AccessType accesstype = epf::W);

    /**
     * @brief Executes a command associated with the filter.
     * @param name Key name of the command.
     * @return Error code of the command execution.
     */
    int32_t runCommand(std::string name);

    /**
     * @brief Retrieves the settings object of the filter.
     * @return Pointer to the Settings object.
     */
    const Settings *settings() const;

    /**
     * @brief Reads settings from a YAML configuration node.
     * @param config YAML node containing the configuration.
     * @return Error code indicating success or failure.
     */
    int32_t readSettings(const YAML::Node &config);

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
    int32_t connect(int src_port_index, SinkPort *sink_port);

    /**
     * @brief Retrieves a list of connected source port indices.
     * @return Vector of connected source port indices.
     */
    //    std::vector<int> connectedSources();
    std::vector<int> connectedSources();

    /**
     * @brief Retrieves a list of connected sink port indices.
     * @return Vector of connected sink port indices.
     */
    std::vector<int> activatedSinks();

    /**
     * @brief Connects a source filter to the current filter.
     * @param src_filter Pointer to the source filter.
     * @return Error code indicating success or failure.
     */
    int32_t connectSourceFilter(Filter *src_filter);

    // Accessing filter's sinks ports. Returning nullptr if fails.
    SinkPort *sinkPort(int32_t port_index) noexcept;

    // Accessing filter's source ports. Returning nullptr if fails.
    SourcePort *sourcePort(int32_t port_index) noexcept;

  protected:
    void notifySettingsChanged(SettingsChangeKind kind,
                               const std::string &reason);
    void notifyStateChanged(FilterState old_state, FilterState new_state,
                            const std::string &reason);
    void notifyErrorOccurred(const std::string &reason);

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
     * @brief Reads device-specific settings from a YAML configuration node.
     * @param config YAML node containing the configuration.
     * @return Error code indicating success or failure.
     */
    int32_t readDeviceSettings(const YAML::Node &config);

    //    FilterState state_;
    std::atomic<FilterState> state_{
        DISCONNECTED}; /**< Current status of the filter */
    int32_t state_code_{static_cast<int32_t>(DISCONNECTED)}; /**< Numeric mirror
                                                              * of state_ for
                                                              * read-only
                                                              * settings
                                                              * exposure. */
    pthread_mutex_t state_mtx_; /**< Protects lifecycle critical sections
                                 * (not every state_ read). */
    std::string type_;          /**< Type of the filter */
    JobExecutionModel job_execution_model_{
        EXTERNAL_THREAD};    /**< Job execution model */
    Settings settings_;      /**< Settings object */
    YAML::Node yaml_config_; /**< YAML configuration node */

    std::vector<SinkPort> sink_ports_;     /**< Vector of sink ports */
    std::vector<SourcePort> source_ports_; /**< Vector of source ports */

    std::vector<Filter *>
        src_filters_{}; /**< Runtime list of connected source filters. */

    /**
     * @brief Teardown phase 1: disconnect all source ports (readers).
     *        Safe to call repeatedly.
     * @return Error code indicating success or failure.
     */
    int32_t disconnectSourcePorts();

    /**
     * @brief Teardown phase 2: deactivate all sink ports (writers/queues).
     *        Safe to call repeatedly.
     * @return Error code indicating success or failure.
     */
    int32_t deactivateSinkPorts();

  private:
    friend class Pipeline;

    std::string name_;      /**< Name of the filter */
    size_t max_sources_{1}; /**< Maximum number of source ports */
    size_t max_sinks_{1};   /**< Maximum number of sink ports */

    std::atomic<uint64_t> settings_revision_{0};
    mutable SettingsSignal settings_changed_{};
    mutable StateSignal state_changed_{};
    mutable ErrorSignal error_occurred_{};

    int32_t createSourcePortSettings();
    int32_t createSinkPortSettings();
    int32_t deviceSettingsFromYAML(const YAML::Node &config);
    int32_t disconnectSourcePortsUnsafe();
    int32_t deactivateSinkPortsUnsafe();
};

}  // namespace epf

#endif  // _FILTER_H
