// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "pipeline.h"

using namespace epf;
using namespace std;

/* Pipeline */
Pipeline::~Pipeline()
{
  disconnectFilterSignals(true);
}

int32_t Pipeline::open()
{
  int32_t ret = 0;
  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->open();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name() << " open() failed."
                << std::endl;
      ret = -1;
    }
  }
  if (setting_sequence_.size() == 0) {
    this->topologicalSort();
    if (setting_sequence_.size() != vertices_.size()) {
      std::cerr << "Pipeline open() error: invalid/cyclic graph setting "
                   "sequence."
                << std::endl;
      return -1;
    }
  }

  return ret;
}

int32_t Pipeline::set()
{
  if (setting_sequence_.size() == 0) {
    this->topologicalSort();
    if (setting_sequence_.size() != vertices_.size()) {
      std::cerr << "Pipeline set() error: invalid/cyclic graph setting "
                   "sequence."
                << std::endl;
      return -1;
    }
  }

  int32_t ret = 0;

  for (auto index : setting_sequence_) {
    const auto &node = vertices_[index];

    Filter *src = node.ptr_;

    int32_t err = src->set();
    if (err < 0) {
      std::cout << "ERROR: " << src->name() << " set() failed." << std::endl;
      ret = -1;
    }

    for (const auto &neighbor : node.neighbors_) {
      Filter *sink = vertices_[neighbor.index_].ptr_;

      for (const auto &edge : neighbor.edges_) {
        SinkPort *src_sink_port = src->sinkPort(edge.source_);
        if ((src_sink_port != nullptr) && src_sink_port->isActivated()) {
          int32_t c1 = sink->connect(edge.destination_, src_sink_port);
          int32_t c2 = sink->connectSourceFilter(src);
          if (c1 < 0 || c2 < 0) {
            std::cerr << "Pipeline set() error: failed to connect "
                      << src->name() << "[" << edge.source_ << "] to "
                      << sink->name() << "[" << edge.destination_ << "]"
                      << std::endl;
            ret = -1;
          }
        }
        else {
          std::cerr << "Pipeline set() error: Filter " << src->name()
                    << " port " << edge.source_
                    << " is not activated, failed to connect to "
                    << sink->name() << std::endl;
          ret = -1;
        }
      }
    }
  }
  return ret;
}

int32_t Pipeline::reset()
{
  int32_t ret = 0;

  // Coordinated two-phase teardown across the full pipeline:
  // 1) Disconnect all source readers/subscribers first.
  // 2) Deactivate all sink queues after readers are gone.
  // Filter::reset() also performs local teardown for standalone filter usage.
  // The resulting duplicated teardown in pipeline-driven reset is intentional;
  // disconnect/deactivate operations are guarded and idempotent.
  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->disconnectSourcePorts();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name()
                << " disconnectSourcePorts() failed." << std::endl;
      ret = -1;
    }
  }

  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->deactivateSinkPorts();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name()
                << " deactivateSinkPorts() failed." << std::endl;
      ret = -1;
    }
  }

  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->reset();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name() << " reset() failed."
                << std::endl;
      ret = -1;
    }
  }

  // Observability policy (Policy A):
  // Always rebuild pipeline/filter signal wiring after reset attempt, even
  // when one or more filters fail reset(). This preserves state/error/settings
  // visibility for degraded partial-failure states. Return code still reports
  // reset success/failure independently.
  disconnectFilterSignals(false);
  for (const auto &entry : id_to_filter_) {
    connectFilterSignals(entry.second, entry.first);
  }
  return ret;
}

int32_t Pipeline::close()
{
  bool has_running = false;
  for (const auto &node : vertices_) {
    if (node.ptr_->state() == FilterState::RUNNING) {
      has_running = true;
    }
  }

  int32_t ret = 0;

  if (has_running && stop() < 0) {
    std::cerr << "WARNING: pipeline stop reported errors during close()."
              << std::endl;
  }

  if (has_running && threads_launched_ && join() < 0) {
    std::cerr << "WARNING: pipeline join reported errors during close()."
              << std::endl;
  }

  // Best-effort shutdown path:
  // - do not invoke reset() here (filters may never have reached SET),
  // - first disconnect all source ports pipeline-wide,
  // - then deactivate all sink ports pipeline-wide,
  // - and finally call close() on every filter.
  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->disconnectSourcePorts();
    if (err < 0) {
      std::cerr << "WARNING: " << node.ptr_->name()
                << " disconnectSourcePorts() failed during close()."
                << std::endl;
    }
  }

  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->deactivateSinkPorts();
    if (err < 0) {
      std::cerr << "WARNING: " << node.ptr_->name()
                << " deactivateSinkPorts() failed during close()." << std::endl;
    }
  }

  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->close();
    if (err < 0) {
      std::cerr << "ERROR: " << node.ptr_->name() << " close() failed."
                << std::endl;
      ret = -1;
    }
  }

  return ret;
}

int32_t Pipeline::start()
{
  int32_t ret = 0;
  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->start();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name() << " start() failed."
                << std::endl;
      ret = -1;
    }
    //    this->printFilters();
  }
  return ret;
}

int32_t Pipeline::stop()
{
  int32_t ret = 0;
  for (const auto &node : vertices_) {
    int32_t err = node.ptr_->stop();
    if (err < 0) {
      std::cout << "ERROR: " << node.ptr_->name() << " stop() failed."
                << std::endl;
      ret = -1;
    }
  }
  return ret;
}

int32_t Pipeline::join()
{
  int32_t ret = ThreadPool::join();

  for (const auto &node : vertices_) {
    if (node.ptr_->state() == FilterState::STOP_REQUEST) {
      int32_t err =
          node.ptr_->completeStopTransitionIfRequested("join stop convergence");
      if (err < 0) {
        std::cerr << "WARNING: " << node.ptr_->name()
                  << " stop completion returned error during join convergence."
                  << std::endl;
        ret = -1;
      }
    }
  }

  return ret;
}

int32_t Pipeline::run()
{
  if (open() < 0) return -1;
  if (set() < 0) return -1;
  if (start() < 0) return -1;
  if (launch() < 0) return -1;

  return 0;
}

int32_t Pipeline::halt()
{
  // Best-effort shutdown that tolerates partially completed startup (e.g.
  // run() failing after open()/set() but before full start/launch).
  // close() already performs conditional stop/join/reset based on current
  // filter states and thread-pool lifecycle.
  return close();
}

int Pipeline::filterCount()
{
  return static_cast<int32_t>(vertices_.size());
}

Filter *Pipeline::filter(int index)
{
  if (index >= 0) {
    size_t u_index = static_cast<size_t>(index);
    if (u_index < vertices_.size()) {
      return vertices_[u_index].ptr_;
    }
  }
  std::cerr << "Pipeline: filter(index) out of bounds" << std::endl;
  return nullptr;
}

FilterId Pipeline::filterId(const Filter *filter) const
{
  auto it = filter_to_id_.find(filter);
  if (it == filter_to_id_.end()) {
    return 0;
  }
  return it->second;
}

Filter *Pipeline::filterById(FilterId id) const
{
  auto it = id_to_filter_.find(id);
  if (it == id_to_filter_.end()) {
    return nullptr;
  }
  return it->second;
}

int32_t Pipeline::add(std::unique_ptr<Filter> ptr)
{
  Filter *raw = ptr.get();
  if ((raw->jobExecutionModel() == MAIN_LOOP) && (main_loop_filter_)) {
    std::cerr << "Pipeline: add - can only have one main loop filter"
              << std::endl;
    return -1;
  }

  if (raw->jobExecutionModel() == MAIN_LOOP) main_loop_filter_ = raw;

  int32_t ret = Graph::add(raw);
  if (ret == 0) {
    FilterId id = registerFilter(raw);
    connectFilterSignals(raw, id);
    filter_added_(*this, id, *raw);
    ptr.release();
  }
  return ret;
}

const Pipeline::FilterSettingsSignal &Pipeline::filterSettingsChanged() const
{
  return filter_settings_changed_;
}

const Pipeline::FilterStateSignal &Pipeline::filterStateChanged() const
{
  return filter_state_changed_;
}

const Pipeline::FilterErrorSignal &Pipeline::filterErrorOccurred() const
{
  return filter_error_occurred_;
}

const Pipeline::FilterAddedSignal &Pipeline::filterAdded() const
{
  return filter_added_;
}

const Pipeline::FilterRemovedSignal &Pipeline::filterRemoved() const
{
  return filter_removed_;
}

FilterId Pipeline::registerFilter(Filter *filter)
{
  if (filter == nullptr) {
    return 0;
  }

  auto existing = filter_to_id_.find(filter);
  if (existing != filter_to_id_.end()) {
    return existing->second;
  }

  FilterId id = next_filter_id_++;
  filter_to_id_[filter] = id;
  id_to_filter_[id] = filter;
  return id;
}

void Pipeline::connectFilterSignals(Filter *filter, FilterId id)
{
  if (!filter) {
    return;
  }

  auto settings_conn = filter->settingsChanged().connect(
      [this, id](const Filter &changed_filter, uint64_t revision,
                 SettingsChangeKind kind, const std::string &reason) {
        filter_settings_changed_(*this, id, changed_filter, revision, kind,
                                 reason);
      });
  settings_connections_[filter] = fteng::connection(settings_conn);

  auto state_conn = filter->stateChanged().connect(
      [this, id](const Filter &changed_filter, FilterState old_state,
                 FilterState new_state, const std::string &reason) {
        filter_state_changed_(*this, id, changed_filter, old_state, new_state,
                              reason);
      });
  state_connections_[filter] = fteng::connection(state_conn);

  auto error_conn = filter->errorOccurred().connect(
      [this, id](const Filter &changed_filter, const std::string &reason) {
        filter_error_occurred_(*this, id, changed_filter, reason);
      });
  error_connections_[filter] = fteng::connection(error_conn);
}

void Pipeline::disconnectFilterSignals(bool emit_removed)
{
  if (emit_removed) {
    for (const auto &entry : filter_to_id_) {
      filter_removed_(*this, entry.second);
    }
  }

  for (auto &entry : settings_connections_) {
    entry.second.disconnect();
  }
  settings_connections_.clear();

  for (auto &entry : state_connections_) {
    entry.second.disconnect();
  }
  state_connections_.clear();

  for (auto &entry : error_connections_) {
    entry.second.disconnect();
  }
  error_connections_.clear();

  if (emit_removed) {
    id_to_filter_.clear();
    filter_to_id_.clear();
  }
}

/* Graph */
Graph::~Graph()
{
  for (auto &v : vertices_) {
    delete v.ptr_;
    v.ptr_ = nullptr;
  }
  vertices_.clear();
  vertices_map_.clear();
  setting_sequence_.clear();
}

int32_t Graph::add(Filter *ptr)
{
  if (ptr == nullptr) {
    std::cerr << "Graph::add error: nullptr filter provided." << std::endl;
    return -1;
  }

  std::vector<Neighbor> vec;
  Vertex v = {ptr, vec};
  vertices_.push_back(v);
  vertices_map_.emplace(ptr, vertices_.size() - 1);

  return 0;
}

int32_t Graph::connect(Filter *f1, int q1, Filter *f2, int q2)
{
  // Check if f1 and f2 are not nullptr
  if (f1 == nullptr || f2 == nullptr) {
    std::cerr << "Error: Null pointer for Filter provided." << std::endl;
    return -1;
  }
  // Check if f1 and f2 exist in the vertices map
  if (vertices_map_.find(f1) == vertices_map_.end()) {
    std::cerr << "Error: Filter f1 not found in the graph." << std::endl;
    return -1;
  }
  if (vertices_map_.find(f2) == vertices_map_.end()) {
    std::cerr << "Error: Filter f2 not found in the graph." << std::endl;
    return -1;
  }

  int f1_index = vertices_map_[f1];
  int f2_index = vertices_map_[f2];

  // Check if f1_index and f2_index are within the bounds
  if (f1_index < 0 || f1_index >= static_cast<int>(vertices_.size())) {
    std::cerr << "Error: f1_index is out of bounds." << std::endl;
    return -1;
  }
  if (f2_index < 0 || f2_index >= static_cast<int>(vertices_.size())) {
    std::cerr << "Error: f2_index is out of bounds." << std::endl;
    return -1;
  }

  bool found = false;
  int index = 0;

  Vertex *v1 = &(vertices_[f1_index]);

  for (const auto &element : v1->neighbors_) {
    if (element.index_ == f2_index) {
      found = true;
      break;
    }
    index++;
  }
  if (found) {
    Edge e = {q1, q2};
    v1->neighbors_[index].edges_.push_back(e);
  }
  else {
    std::vector<Edge> vec = {{q1, q2}};
    Neighbor n = {f2_index, vec};
    v1->neighbors_.push_back(n);
  }
  return 0;
}

int32_t Graph::connect(int id1, int id2, int id3, int id4)
{
  // Check if id1, id2, id3, id4 are non-negative (assuming IDs should be
  // non-negative)
  if (id1 < 0 || id2 < 0 || id3 < 0 || id4 < 0) {
    std::cerr << "Error: IDs must be non-negative integers." << std::endl;
    return -1;
  }

  // Check if id1 is within the bounds of vertices_
  if (id1 >= static_cast<int>(vertices_.size())) {
    std::cerr << "Error: id1 (" << id1 << ") is out of bounds." << std::endl;
    return -1;
  }

  // Check if id3 is a valid potential neighbor index
  if (id3 >= static_cast<int>(vertices_.size())) {
    std::cerr << "Error: id3 (" << id3 << ") is out of bounds." << std::endl;
    return -1;
  }

  bool found = false;
  int index = 0;

  // Loop through the neighbors of vertex id1 to find if a connection to id3
  // exists
  for (const auto &element : vertices_[id1].neighbors_) {
    if (element.index_ == id3) {
      found = true;
      break;
    }
    index++;
  }
  if (found) {
    // Add the edge to the existing neighbor's edges
    Edge e = {id2, id4};
    vertices_[id1].neighbors_[index].edges_.push_back(e);
  }
  else {
    // Create a new neighbor with the specified edge
    std::vector<Edge> vec = {{id2, id4}};
    Neighbor n = {id3, vec};
    vertices_[id1].neighbors_.push_back(n);
  }
  return 0;
}

void Graph::printFilters()
{
  int index = 0;
  std::cout << "Filters:" << std::endl;
  for (const auto &vertex : vertices_) {
    std::cout << "[" << index++ << "]: " << vertex.ptr_->name() << " "
              << state2string(vertex.ptr_->state());

    std::cout << " sources: ";
    for (int i = 0; i < vertex.ptr_->maxSources(); i++) {
      if (vertex.ptr_->sourcePort(i)->isConnected()) std::cout << i << " ";
    }
    std::cout << " sinks: ";
    for (int i = 0; i < vertex.ptr_->maxSinks(); i++) {
      if (vertex.ptr_->sinkPort(i)->isActivated()) std::cout << i << " ";
    }

    std::cout << std::endl;
  }
}

void Graph::printGraph()
{
  std::cout << "Graph(" << vertices_.size() << "): " << std::endl;
  int index = 0;
  for (const auto &vertex : vertices_) {
    if (vertex.neighbors_.size() > 0)
      for (const auto &neighbor : vertex.neighbors_) {
        std::cout << "[" << index << "] -> [" << neighbor.index_ << "]"
                  << std::endl;
        for (const auto &edge : neighbor.edges_) {
          std::cout << "   " << edge.source_ << " -> " << edge.destination_
                    << std::endl;
        }
      }
    else
      std::cout << "[" << index << "]" << std::endl;
    index++;
  }
}

std::vector<int> Graph::topologicalSort()
{
  setting_sequence_.clear();

  // Vector to store indegree of each vertex
  std::vector<int> indegree(vertices_.size());
  for (unsigned int i = 0; i < vertices_.size(); i++) {
    for (const auto &it : vertices_[i].neighbors_) {
      indegree[it.index_]++;
    }
  }

  // Queue to store vertices with indegree 0
  std::queue<int> q;
  for (unsigned int i = 0; i < vertices_.size(); i++) {
    if (indegree[i] == 0) {
      q.push(i);
    }
  }
  while (!q.empty()) {
    int node = q.front();
    q.pop();
    setting_sequence_.push_back(node);

    // Decrease indegree of adjacent vertices as the
    // current node is in topological order
    for (const auto &it : vertices_[node].neighbors_) {
      indegree[it.index_]--;

      // If indegree becomes 0, push it to the queue
      if (indegree[it.index_] == 0) q.push(it.index_);
    }
  }

  // Check for cycle
  if (setting_sequence_.size() != vertices_.size()) {
    setting_sequence_.clear();
    std::cerr << "Graph contains cycle - please set sequence manually."
              << std::endl;
  }

  return setting_sequence_;
}

/* ThreadPool */
ThreadPool::ThreadPool(int N)
{
  num_threads_ = N;
  threads_ = nullptr;
  tasks_ = nullptr;
  state_ = nullptr;
  threads_launched_ = false;
  lifecycle_state_ = LifecycleState::UNINITIALIZED;

  if (num_threads_ == 0) {
    std::cerr << "Pipeline: thread count 0 is invalid. "
              << "Falling back to automatic thread assignment." << std::endl;
    num_threads_ = -1;
  }

  if (num_threads_ >= 0) {
    init();
  }
}

ThreadPool::~ThreadPool()
{
  printf("ThreadPool destructor\n");
  if (threads_launched_) {
    join();
  }
  if (threads_ != nullptr) {
    delete[] this->threads_;
    threads_ = nullptr;
  }
  if (tasks_ != nullptr) {
    for (int i = 0; i < num_threads_; ++i) {
      this->tasks_[i].clear();
    }
    delete[] this->tasks_;
    tasks_ = nullptr;
  }
  if (state_ != nullptr) {
    delete[] this->state_;
    state_ = nullptr;
  }
  lifecycle_state_ = LifecycleState::UNINITIALIZED;
}

int32_t ThreadPool::init()
{
  if (num_threads_ <= 0) {
    return -1;
  }
  this->tasks_ = new vector<epf::Filter *>[num_threads_];
  this->threads_ = new pthread_t[num_threads_];
  this->state_ = new char[num_threads_];
  memset(this->state_, 0, num_threads_);
  lifecycle_state_ = LifecycleState::INITIALIZED;

  return 0;
}

int32_t ThreadPool::assignTask(int32_t thread_id, Filter *f)
{
  if ((thread_id >= 0) && (thread_id < num_threads_)) {
    if (f->jobExecutionModel() == EXTERNAL_THREAD) {
      this->tasks_[thread_id].push_back(f);
      return 0;
    }
    else if (f->jobExecutionModel() == MAIN_LOOP) {
      std::cerr << "WARNING: Pipeline::assignTask() failed " << f->name()
                << " filter job model is MAIN_LOOP." << std::endl;
      return -1;
    }
    else {
      std::cerr << "WARNING: Pipeline::assignTask() failed " << f->name()
                << " filter job model is OWN_THREAD." << std::endl;
      return -1;
    }
  }
  else {
    std::cerr << "ERROR: Pipeline::assignTask() thread id out ouf bounds."
              << std::endl;
    return -1;
  }
}

int32_t Pipeline::launch()
{
  // check here thread assignment
  if (num_threads_ == -1) {
    std::cout << "Pipeline::launch(): auto-initializing thread pool using "
              << vertices_.size() << " worker(s)." << std::endl;
    num_threads_ = static_cast<int32_t>(vertices_.size());
    if (num_threads_ > 0) {
      if (init() < 0) return -1;
      for (int i = 0; i < num_threads_; i++) {
        ThreadPool::assignTask(i, vertices_[i].ptr_);
      }
    }
  }

  int err = 0;
  if (num_threads_ > 0) {
    err = ThreadPool::launch();
  }

  if (main_loop_filter_ != nullptr) {
    main_loop_filter_->doJob();
  }

  return err;
}

int32_t ThreadPool::launch()
{
  if (lifecycle_state_ == LifecycleState::LAUNCHED || threads_launched_) {
    std::cerr << "ERROR: ThreadPool::launch() called while already launched."
              << std::endl;
    return -1;
  }

  if (lifecycle_state_ == LifecycleState::UNINITIALIZED) {
    std::cerr << "ERROR: ThreadPool::launch() called before initialization."
              << std::endl;
    return -1;
  }

  if (num_threads_ <= 0 || tasks_ == nullptr || threads_ == nullptr ||
      state_ == nullptr) {
    return -1;
  }

  // Create and launch threads
  int launched_count = 0;
  for (int i = 0; i < num_threads_; ++i) {
    // Create a structure to hold both the thread ID and ThreadPool instance
    struct ThreadInfo {
        int thread_id;
        ThreadPool *pool;
    };
    ThreadInfo *info =
        new ThreadInfo{i, this};  // Create a new ThreadInfo object
    this->state_[i] = 1;

    int create_ret =
        pthread_create(&threads_[i], NULL, threadFunction,
                       info);  // Pass the ThreadInfo object to the thread
    if (create_ret != 0) {
      std::cerr << "ERROR: ThreadPool::launch() pthread_create() failed for "
                   "thread "
                << i << " with code " << create_ret << std::endl;
      this->state_[i] = 0;
      delete info;

      // Best-effort cleanup for already-created threads.
      for (int j = 0; j < launched_count; ++j) {
        this->state_[j] = 0;
      }
      for (int j = 0; j < launched_count; ++j) {
        void *result = nullptr;
        if (pthread_join(this->threads_[j], &result) != 0) {
          std::cerr << "ERROR: ThreadPool::launch() cleanup pthread_join() "
                       "failed for thread "
                    << j << std::endl;
        }
      }
      threads_launched_ = false;
      lifecycle_state_ = LifecycleState::INITIALIZED;
      return -1;
    }
    ++launched_count;
  }

  threads_launched_ = true;
  lifecycle_state_ = LifecycleState::LAUNCHED;

  return 0;
}

int32_t ThreadPool::join()
{
  if (!threads_launched_) {
    if (lifecycle_state_ == LifecycleState::INITIALIZED ||
        lifecycle_state_ == LifecycleState::JOINED) {
      lifecycle_state_ = LifecycleState::JOINED;
    }
    return 0;
  }

  if (num_threads_ <= 0) {
    threads_launched_ = false;
    lifecycle_state_ = LifecycleState::JOINED;
    return 0;
  }
  if (state_ == nullptr || threads_ == nullptr || tasks_ == nullptr) {
    std::cerr << "ERROR: ThreadPool::join() called with incomplete internal "
                 "state."
              << std::endl;
    threads_launched_ = false;
    lifecycle_state_ = LifecycleState::JOINED;
    return -1;
  }

  for (int i = 0; i < num_threads_; i++) this->state_[i] = 0;

  void *result;
  for (int i = 0; i < this->num_threads_; i++) {
    // set non blocking calls and wake up to avoid interlock
    for (auto filter : tasks_[i]) {
      for (int j = 0; j < filter->maxSources(); j++) {
        if (filter->sourcePort(j)->isConnected()) {
          //          filter->sourcePort(j)->setBlocking(false);
          // Temporary non-blocking wake-up for join only.
          // Do not mutate SourcePort::blocking() persistent setting here,
          // otherwise start/stop cycles cannot restore the configured mode.
          filter->sourcePort(j)->reader()->setBlockingCalls(false);
          filter->sourcePort(j)->queue()->wakeUpConsumers();
        }
      }

      for (int j = 0; j < filter->maxSinks(); j++) {
        if (filter->sinkPort(j)->isActivated()) {
          //          filter->sinkPort(j)->setBlocking(false);
          // TODO w->queue()->wakeUpConsumers();
          // Temporary non-blocking wake-up for join only.
          // Keep SinkPort::blocking() configuration untouched.
          filter->sinkPort(j)->writer()->setBlockingCalls(false);
          // TODO w->queue()->wakeUpConsumers();
          filter->sinkPort(j)->queue()->wakeUpProducers();
        }
      }
    }

    printf("join thread %d..", i);
    if (pthread_join(this->threads_[i], &result) != 0) {
      perror("pthread_join() error");
    }
    else
      printf("ok \n");

    // Print the result
    if (result != nullptr) {
      std::cout << "result: " << *(static_cast<int *>(result)) << std::endl;
    }  // else {
  }
  threads_launched_ = false;
  // Keep resources allocated so launch() can be called again.
  lifecycle_state_ = LifecycleState::JOINED;
  return 0;
}

void *ThreadPool::threadFunction(void *arg)
{
  // Cast the argument back to ThreadInfo
  struct ThreadInfo {
      int thread_id;
      ThreadPool *pool;
  };

  // Extract information and manage resources
  ThreadInfo *info = static_cast<ThreadInfo *>(arg);
  int thread_id = info->thread_id;
  ThreadPool *pool = info->pool;
  // Delete the ThreadInfo object to avoid memory leak
  delete info;
  auto &tasks = pool->tasks_[thread_id];
  //  vector<epf::Filter*>* tasks = &(pool->tasks_[thread_id]);

  if (tasks.size() == 0) pthread_exit(nullptr);

  // Run tasks loop until the thread is signaled to stop
  while (pool->state_[thread_id]) {
    bool any = false;
    for (auto const &task : tasks) {
      //      if (task->state() == FilterState::RUNNING) {
      int err = task->doJob();
      if (err >= 0) any = true;
      //}
    }
    if (!any) usleep(10000);  // sleep for 10 milliseconds
  }

  pthread_exit(nullptr);
}

int32_t Pipeline::writeSettings(YAML::Node &config) const
{
  YAML::Node filters_node;

  // Iterate through all filters in the pipeline
  for (const auto &filter : vertices_) {
    YAML::Node filter_node;
    // Use filter's writeSettings method to populate the filter_node
    // std::cout << filter.ptr_->name() << " "
    //           << state2string(filter.ptr_->state()) << std::endl;

    filter.ptr_->writeSettings(filter_node);

    filters_node.push_back(filter_node);
  }

  config["filters"] = filters_node;

  return 0;
}

int32_t Pipeline::saveSettings(const std::string &filename) const
{
  // Create a YAML node to store the pipeline settings
  YAML::Node node;
  writeSettings(node);

  try {
    // Save the YAML node to a file
    std::ofstream fout(filename);
    fout << node;
    fout.close();
  }
  catch (const std::exception &e) {
    std::cerr << "Error saving settings to file: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
