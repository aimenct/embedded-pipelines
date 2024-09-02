// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef PIPELINE_H
#define PIPELINE_H

#include <pthread.h>

#include <iostream>
#include <list>
#include <queue>
#include <unordered_map>
#include <vector>

#include "filter.h"
#include "queue.h"
#include "queue_handlers.h"

namespace ep {

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
    Filter* ptr_;                     /**< Pointer to the associated filter */
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
    int32_t add(Filter* ptr);

    /**
     * @brief Connects two filters with specified IDs.
     * @param f1 Pointer to the first filter.
     * @param id1 ID of the source queue in the first filter.
     * @param f2 Pointer to the second filter.
     * @param id2 ID of the destination queue in the second filter.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t connect(Filter* f1, int id1, Filter* f2, int id2);

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
    std::unordered_map<Filter*, int>
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
    int32_t assignTask(int thread_id, ep::Filter* f);

    /**
     * @brief Launches all threads in the pool to start processing tasks.
     */
    int32_t launch();

    /**
     * @brief Waits for all threads in the pool to complete their tasks.
     */
    int32_t join();

  private:
    /**
     * @brief Function executed by each thread in the pool.
     * @param arg Argument passed to the thread function.
     * @return Pointer to the result of the thread function.
     */
    static void* threadFunction(void* arg);

  protected:
    char* state_;     /**< State of the thread pool */
    int num_threads_; /**< Number of threads in the pool */
    std::vector<ep::Filter*>*
        tasks_;          /**< Queue of tasks (filter jobs) for the threads */
    pthread_t* threads_; /**< Array of thread identifiers */

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

    /**
     * @brief Retrieves a filter pointer by its index.
     * @param index Index of the filter.
     * @return Pointer to the filter.
     */
    Filter* filter(int index);

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
    int32_t add(Filter* ptr);
    using Graph::connect;      /**< Inherit the connect methods from Graph */
    using Graph::printFilters; /**< Inherit the printFilters method from Graph
                                */
    using Graph::printGraph;   /**< Inherit the printGraph method from Graph */
    using ThreadPool::assignTask; /**< Inherit the assignTask method from
                                     ThreadPool */
    using ThreadPool::join; /**< Inherit the join method from ThreadPool */
                            /**< Inherit the launch method from ThreadPool */
    /**
     * @brief Launches all threads in the pool to start processing tasks.
     */
    int32_t launch();

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
     * @brief Resets the pipeline to its initial state.
     * @return Error code: 0 for success, non-zero for failure.
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
    int32_t writeSettings(YAML::Node& config) const;

    /**
     * @brief Saves the current settings of the pipeline to a file.
     * @param filename Name of the file to save settings to.
     * @return Error code: 0 for success, non-zero for failure.
     */
    int32_t saveSettings(const std::string& filename) const;

  private:
    Filter* main_loop_filter_; /**< Pointer to the filter that has a main loop
                                  job */
};

}  // namespace ep

#endif  // PIPELINE_H
