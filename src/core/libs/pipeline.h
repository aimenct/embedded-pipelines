// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef PIPELINE_H
#define PIPELINE_H

#include <pthread.h>

#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "filter.h"
#include "queue.h"
#include "queue_handlers.h"

namespace epf {

/**
 * @struct Edge
 * @brief Represents a directed edge between two vertices in the graph.
 */
struct Edge {
    int source_;      /**< Source vertex ID */
    int destination_; /**< Destination vertex ID */
};

/**
 * @struct Neighbor
 * @brief Represents a neighbor of a vertex, containing an index and edges.
 */
struct Neighbor {
    int index_;               /**< Index of the neighboring vertex */
    std::vector<Edge> edges_; /**< List of edges to the neighboring vertex */
};

/**
 * @struct Vertex
 * @brief Represents a vertex in the graph, containing a filter pointer and a
 * list of neighbors.
 */
struct Vertex {
    Filter *ptr_;                     /**< Pointer to the associated filter */
    std::vector<Neighbor> neighbors_; /**< List of neighbors (outgoing edges) */
};

/**
 * @class Graph
 * @brief Represents a directed graph of filters and queues, managing
 * connections between filters.
 */
class Graph {
  public:
    /**
     * @brief Default constructor. Initializes an empty graph.
     */
    Graph()
        : setting_sequence_({})
    {
    }

    /**
     * @brief Destructor. Cleans up resources.
     */
    ~Graph();

    /**
     * @brief Adds a filter to the graph.
     * @param ptr Pointer to the filter to add.
     */
    int32_t add(Filter *ptr);

    /**
     * @brief Connects two filters with specified IDs.
     * @param f1 Pointer to the first filter.
     * @param id1 ID of the source queue in the first filter.
     * @param f2 Pointer to the second filter.
     * @param id2 ID of the destination queue in the second filter.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t connect(Filter *f1, int id1, Filter *f2, int id2);

    /**
     * @brief Connects two filters by specifying queue IDs directly.
     * @param id1 ID of the source filter.
     * @param id2 ID of the source queue.
     * @param id3 ID of the destination filter.
     * @param id4 ID of the destination queue.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t connect(int id1, int id2, int id3, int id4);

    /**
     * @brief Prints details of all filters in the graph.
     */
    void printFilters();

    /**
     * @brief Prints the entire graph structure.
     */
    void printGraph();

    /**
     * @brief Returns the number of vertices in the graph.
     * @return Number of vertices.
     */
    int32_t size() const
    {
      return static_cast<int32_t>(vertices_.size());
    }

    /**
     * @brief Sets the sequence of settings for the graph.
     * @param sequence Vector of integers representing the setting sequence.
     */
    void setSettingSequence(std::vector<int> sequence)
    {
      setting_sequence_ = sequence;
    }

    /**
     * @brief Prints the current setting sequence.
     */
    void printSettingSequence() const
    {
      for (auto i : setting_sequence_) {
        std::cout << i << " ";
      }
    }

    /**
     * @brief Performs a topological sort of the graph to determine the setting
     * sequence.
     * @return Vector of integers representing the topological order.
     */
    std::vector<int> topologicalSort();

  protected:
    std::vector<Vertex>
        vertices_; /**< List of vertices (filters) in the graph */
    std::unordered_map<Filter *, int>
        vertices_map_; /**< Map to store vertex indices by filter pointer */
    std::vector<int> setting_sequence_; /**< Sequence of settings for filters */
};

/**
 * @class ThreadPool
 * @brief Manages a pool of threads to distribute workload among filters.
 */
class ThreadPool {
  public:
    /**
     * @brief Lifecycle states for thread pool resources/workers.
     */
    enum class LifecycleState {
      UNINITIALIZED, /**< Resource arrays are not allocated yet. */
      INITIALIZED,   /**< Resource arrays are allocated and ready. */
      LAUNCHED,      /**< Worker threads are currently running. */
      JOINED         /**< Workers were launched and joined; pool is reusable. */
    };

    /**
     * @brief Constructor. Initializes the thread pool with a specified number
     * of threads.
     * @param N Number of threads to create in the pool.
     */
    ThreadPool(int N);

    /**
     * @brief Destructor. Cleans up resources used by the thread pool.
     */
    ~ThreadPool();

    /**
     * @brief Assigns a task (filter job) to a specific thread.
     * @param thread_id ID of the thread to assign the task to.
     * @param f Pointer to the filter whose job is to be assigned.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t assignTask(int32_t thread_id, epf::Filter *f);

    /**
     * @brief Launches all threads in the pool to start processing tasks.
     */
    int32_t launch();

    /**
     * @brief Waits for all threads in the pool to complete their tasks.
     *
     * After a successful join, the pool remains initialized and can be
     * launched again with the same task assignment.
     */
    int32_t join();

  private:
    /**
     * @brief Function executed by each thread in the pool.
     * @param arg Argument passed to the thread function.
     * @return Pointer to the result of the thread function.
     */
    static void *threadFunction(void *arg);

  protected:
    char *state_;                  /**< State of the thread pool */
    int num_threads_;              /**< Number of threads in the pool */
    bool threads_launched_{false}; /**< Whether worker threads were launched */
    LifecycleState lifecycle_state_{
        LifecycleState::UNINITIALIZED}; /**< Pool lifecycle state */
    std::vector<epf::Filter *>
        *tasks_;         /**< Queue of tasks (filter jobs) for the threads */
    pthread_t *threads_; /**< Array of thread identifiers */

    /**
     * @brief Initializes the thread pool resources.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t init();
};

/**
 * @class Pipeline
 * @brief Combines a graph of filters and a thread pool to manage and execute
 * filter operations.
 */
class Pipeline : protected ThreadPool, protected Graph {
  public:
    /**
     * @brief Constructor. Initializes the pipeline with a specified number of
     * threads.
     * @param threads Number of threads to use in the thread pool (default: -1,
     * which means automatic configuration).
     */
    Pipeline(int threads = -1)
        : ThreadPool(threads)
    {
      main_loop_filter_ = nullptr;
    }

    ~Pipeline();

    /**
     * @brief Retrieves a filter pointer by its index.
     * @param index Index of the filter.
     * @return Pointer to the filter.
     */
    Filter *filter(int index);

    FilterId filterId(const Filter *f) const;

    /**
     * @brief Retrieves a filter pointer by its unique id.
     * @param id Filter id.
     * @return Pointer to the filter.
     */
    Filter *filterById(FilterId id) const;

    /**
     * @brief Returns the number of filters in the pipeline.
     * @return Number of filters.
     */
    int filterCount();

    /**
     * @brief Runs the pipeline, starting all filters.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t run();

    /**
     * @brief Halts the pipeline, stopping all filters.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t halt();

    /**
     * @brief Adds a filter to the pipeline.
     * @param ptr Pointer to the filter to add.
     */
    int32_t add(std::unique_ptr<Filter> ptr);

    /**
     * @brief Adds a filter of type FilterT to the pipeline and returns its
     * pointer.
     *
     * This helper constructs the filter in-place and registers it into the
     * pipeline graph. The pipeline takes ownership of the created filter.
     *
     * @tparam FilterT   Type of the filter to be added. Must derive from
     * Filter.
     * @tparam Args      Argument types for the filter constructor.
     * @param args       Arguments to forward to the filter constructor.
     * @return Pointer to the newly created filter on success, nullptr on
     * failure.
     */
    template <typename FilterT, typename... Args>
    FilterT *add(Args &&...args)
    {
      static_assert(std::is_base_of<Filter, FilterT>::value,
                    "FilterT must derive from Filter");

      auto filter = std::make_unique<FilterT>(std::forward<Args>(args)...);
      FilterT *raw = filter.get();

      if ((raw->jobExecutionModel() == MAIN_LOOP) && (main_loop_filter_)) {
        std::cerr << "Pipeline: add - can only have one main loop filter"
                  << std::endl;
        return nullptr;
      }

      if (raw->jobExecutionModel() == MAIN_LOOP) main_loop_filter_ = raw;

      int32_t ret = Graph::add(raw);
      if (ret == 0) {
        FilterId id = registerFilter(raw);
        connectFilterSignals(raw, id);
        filter_added_(*this, id, *raw);
        filter.release();
        return raw;
      }

      return nullptr;
    }
    using Graph::connect;      /**< Inherit the connect methods from Graph */
    using Graph::printFilters; /**< Inherit the printFilters method from Graph
                                */
    using Graph::printGraph;   /**< Inherit the printGraph method from Graph */
    using ThreadPool::assignTask; /**< Inherit the assignTask method from
                                     ThreadPool */
    /**< Inherit the launch method from ThreadPool */
    /**
     * @brief Launches all threads in the pool to start processing tasks.
     */
    int32_t launch();

    /**
     * @brief Waits for worker completion and finalizes pending stop
     * convergence.
     *
     * Contract: after successful stop() + join(), pipeline-managed filters
     * will not remain in RUNNING/STOP_REQUEST when stop completion is
     * possible; remaining STOP_REQUEST states are converged to SET here.
     *
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t join();

    /**
     * @brief Opens the pipeline, preparing it for operation.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t open();

    /**
     * @brief Configures settings for the pipeline.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t set();

    /**
     * @brief Resets all filters and rebuilds pipeline/filter signal wiring.
     *        Pipeline reset performs a coordinated graph-level two-phase
     *        teardown first (disconnect all sources, then deactivate all
     *        sinks), then invokes Filter::reset() per node.
     *        Filter::reset() remains self-contained for standalone use, so
     *        teardown work may be repeated during Pipeline::reset(); this is
     *        intentional and valid because teardown operations are guarded and
     *        idempotent.
     *        Policy A: signal rewiring is always performed after the reset
     *        attempt, even if one or more filters fail reset().
     *        Return value still reports reset success/failure across filters;
     *        rewiring is an observability step and does not imply full success.
     * @return Error code: 0 for full success, -1 if any filter reset fails.
     */
    int32_t reset();

    /**
     * @brief Closes the pipeline, cleaning up resources.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t close();

    /**
     * @brief Starts the pipeline's operation.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t start();

    /**
     * @brief Stops the pipeline's operation.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t stop();

    /**
     * @brief Writes the current settings of the pipeline to a YAML
     * configuration node.
     * @param config YAML node to write settings to.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t writeSettings(YAML::Node &config) const;

    /**
     * @brief Saves the current settings of the pipeline to a file.
     * @param filename Name of the file to save settings to.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t saveSettings(const std::string &filename) const;

    using FilterSettingsSignal =
        fteng::signal<void(const Pipeline &, FilterId, const Filter &, uint64_t,
                           SettingsChangeKind, const std::string &)>;
    const FilterSettingsSignal &filterSettingsChanged() const;

    using FilterStateSignal =
        fteng::signal<void(const Pipeline &, FilterId, const Filter &,
                           FilterState, FilterState, const std::string &)>;
    const FilterStateSignal &filterStateChanged() const;

    using FilterErrorSignal = fteng::signal<void(
        const Pipeline &, FilterId, const Filter &, const std::string &)>;
    const FilterErrorSignal &filterErrorOccurred() const;

    using FilterAddedSignal =
        fteng::signal<void(const Pipeline &, FilterId, const Filter &)>;

    using FilterRemovedSignal = fteng::signal<void(const Pipeline &, FilterId)>;

    const FilterAddedSignal &filterAdded() const;
    const FilterRemovedSignal &filterRemoved() const;

  private:
    FilterId registerFilter(Filter *filter);
    void connectFilterSignals(Filter *filter, FilterId id);
    void disconnectFilterSignals(bool emit_removed);

    Filter *main_loop_filter_; /**< Pointer to the filter that has a main loop
                                  job */
    FilterId next_filter_id_ = 1;
    std::unordered_map<FilterId, Filter *> id_to_filter_{};
    std::unordered_map<const Filter *, FilterId> filter_to_id_{};
    std::unordered_map<const Filter *, fteng::connection>
        settings_connections_{};
    std::unordered_map<const Filter *, fteng::connection> state_connections_{};
    std::unordered_map<const Filter *, fteng::connection> error_connections_{};
    mutable FilterSettingsSignal filter_settings_changed_{};
    mutable FilterStateSignal filter_state_changed_{};
    mutable FilterErrorSignal filter_error_occurred_{};
    mutable FilterAddedSignal filter_added_{};
    mutable FilterRemovedSignal filter_removed_{};
};

}  // namespace epf

#endif  // PIPELINE_H
