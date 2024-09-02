// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "pipeline.h"

using namespace ep;
using namespace std;

/* Pipeline */
int32_t Pipeline::open()
{
  for (const auto& node : vertices_) {
    node.ptr_->open();
  }
  if (setting_sequence_.size() == 0) this->topologicalSort();

  return 0;
}

int32_t Pipeline::set()
{
  if (setting_sequence_.size() == 0) this->topologicalSort();

  int err = 0;

  for (auto index : setting_sequence_) {
    const auto& node = vertices_[index];

    Filter* src = node.ptr_;

    err = src->set();
    if (err < 0) std::cout << src->name() << " set Error" << std::endl;
    //      std::cout << src->name() << " set OK" << std::endl;
    //    else

    for (const auto& neighbor : node.neighbors_) {
      Filter* sink = vertices_[neighbor.index_].ptr_;

      for (const auto& edge : neighbor.edges_) {
        sink->connectSourceQueue(edge.destination_,
                                 src->sinkQueue(edge.source_), src);
      }
    }
  }
  return 0;
}

int32_t Pipeline::reset()
{
  for (const auto& node : vertices_) {
    node.ptr_->reset();
  }
  return 0;
}

int32_t Pipeline::close()
{
  for (const auto& node : vertices_) {
    node.ptr_->close();
  }
  return 0;
}

int32_t Pipeline::start()
{
  for (const auto& node : vertices_) {
    node.ptr_->start();
    //    this->printFilters();
  }
  return 0;
}

int32_t Pipeline::stop()
{
  for (const auto& node : vertices_) {
    node.ptr_->stop();
  }
  return 0;
}

int32_t Pipeline::run()
{
  this->open();

  this->set();

  this->start();

  this->launch();

  return 0;
}

int32_t Pipeline::halt()
{
  this->stop();
  this->join();
  this->reset();
  this->close();
  return 0;
}

int Pipeline::filterCount()
{
  return static_cast<int32_t>(vertices_.size());
}

Filter* Pipeline::filter(int index)
{
  if ((index >= 0) && (index <= static_cast<int>(vertices_.size())))
    return vertices_[index].ptr_;
  else {
    std::cerr << "Pipeline: filter(index) out of bounds" << std::endl;
    return nullptr;
  }
}

int32_t Pipeline::add(Filter* ptr)
{
  if ((ptr->jobExecutionModel() == MAIN_LOOP) && (main_loop_filter_)) {
    main_loop_filter_ = ptr;
    std::cerr << "Pipeline: add - can only have one main loop filter"
              << std::endl;
    return -1;
  }

  if (ptr->jobExecutionModel() == MAIN_LOOP) main_loop_filter_ = ptr;

  return Graph::add(ptr);
}

/* Graph */
Graph::~Graph()
{
  for (auto& v : vertices_) {
    delete v.ptr_;
    v.ptr_ = nullptr;
  }
  vertices_.clear();
  vertices_map_.clear();
  setting_sequence_.clear();
}

int32_t Graph::add(Filter* ptr)
{
  if (ptr == nullptr) {
    printf("Graph Add nullptr fiter\n");
    exit(0);
  }

  std::vector<Neighbor> vec;
  Vertex v = {ptr, vec};
  vertices_.push_back(v);
  vertices_map_.emplace(ptr, vertices_.size() - 1);

  return 0;
}

int32_t Graph::connect(Filter* f1, int q1, Filter* f2, int q2)
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

  Vertex* v1 = &(vertices_[f1_index]);

  for (const auto& element : v1->neighbors_) {
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
  for (const auto& element : vertices_[id1].neighbors_) {
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
  for (const auto& vertex : vertices_) {
    std::cout << "[" << index++ << "]: " << vertex.ptr_->name() << " "
              << state2string(vertex.ptr_->state());

    std::cout << " sources: ";
    for (int i = 0; i < vertex.ptr_->sourcePorts(); i++) {
      if (vertex.ptr_->sourceQueue(i) != nullptr) std::cout << i << " ";
    }
    std::cout << " sinks: ";
    for (int i = 0; i < vertex.ptr_->sinkPorts(); i++) {
      if (vertex.ptr_->sinkQueue(i) != nullptr) std::cout << i << " ";
    }

    std::cout << std::endl;
  }
}

void Graph::printGraph()
{
  std::cout << "Graph(" << vertices_.size() << "): " << std::endl;
  int index = 0;
  for (const auto& vertex : vertices_) {
    if (vertex.neighbors_.size() > 0)
      for (const auto& neighbor : vertex.neighbors_) {
        std::cout << "[" << index << "] -> [" << neighbor.index_ << "]"
                  << std::endl;
        for (const auto& edge : neighbor.edges_) {
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
    for (const auto& it : vertices_[i].neighbors_) {
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
    for (const auto& it : vertices_[node].neighbors_) {
      indegree[it.index_]--;

      // If indegree becomes 0, push it to the queue
      if (indegree[it.index_] == 0) q.push(it.index_);
    }
  }

  // Check for cycle
  if (setting_sequence_.size() != vertices_.size()) {
    std::cout << "Graph contains cycle!";
    std::cout << " - please add manually a seting sequence" << std::endl;
    exit(0);
  }

  return setting_sequence_;
}

/* ThreadPool */
ThreadPool::ThreadPool(int N)
{
  num_threads_ = N;

  if (num_threads_ >= 0) {
    init();
  }
}

ThreadPool::~ThreadPool()
{
  printf("ThreadPool destructor\n");
  delete[] this->threads_;
  for (int i = 0; i < num_threads_; ++i) {
    this->tasks_[i].clear();
  }
  delete[] this->tasks_;
  delete[] this->state_;
}

int32_t ThreadPool::init()
{
  this->tasks_ = new vector<ep::Filter*>[num_threads_];
  this->threads_ = new pthread_t[num_threads_];
  this->state_ = new char[num_threads_];
  memset(this->state_, 0, num_threads_);

  return 0;
}

int32_t ThreadPool::assignTask(int thread_id, Filter* f)
{
  if ((thread_id >= 0) && (thread_id < num_threads_)) {
    if (f->jobExecutionModel() == EXTERNAL_THREAD) {
      this->tasks_[thread_id].push_back(f);
      return 0;
    }
    else if (f->jobExecutionModel() == MAIN_LOOP) {
      std::cerr << "Pipeline: assignTask(): Filter job is a main loop"
                << std::endl;
      return -1;
    }
    else {
      std::cerr << "Pipeline: assignTask(): Filter job model is own thread"
                << std::endl;
      return -1;
    }
  }
  else {
    std::cerr << "Pipeline: assignTask(): Thread id out ouf bounds"
              << std::endl;
    return -1;
  }
}

int32_t Pipeline::launch()
{
  // check here thread assignment
  if (num_threads_ == -1) {
    std::cout << "Thread Pool Not Initialized" << std::endl;
    num_threads_ = static_cast<int32_t>(vertices_.size());
    init();
    for (int i = 0; i < num_threads_; i++) {
      ThreadPool::assignTask(i, vertices_[i].ptr_);
    }
  }

  int err = ThreadPool::launch();

  if (main_loop_filter_ != nullptr) {
    main_loop_filter_->doJob();
  }

  return err;
}

int32_t ThreadPool::launch()
{
  // Create and launch threads
  for (int i = 0; i < num_threads_; ++i) {
    // Create a structure to hold both the thread ID and ThreadPool instance
    struct ThreadInfo {
        int thread_id;
        ThreadPool* pool;
    };
    ThreadInfo* info =
        new ThreadInfo{i, this};  // Create a new ThreadInfo object
    this->state_[i] = 1;

    pthread_create(&threads_[i], NULL, threadFunction,
                   info);  // Pass the ThreadInfo object to the thread
  }

  return 0;
}

int32_t ThreadPool::join()
{
  for (int i = 0; i < num_threads_; i++) this->state_[i] = 0;

  void* result;
  for (int i = 0; i < this->num_threads_; i++) {
    // set non blocking calls and wake up to avoid interlock
    for (auto f : tasks_[i]) {
      for (int j = 0; j < f->sourcePorts(); j++) {
        if (auto* r = f->reader(j)) {
          r->setBlockingCalls(false);
          r->queue()->wakeUpConsumers();
        }
      }

      for (int j = 0; j < f->sinkPorts(); j++) {
        if (auto* w = f->writer(j)) {
          w->setBlockingCalls(false);
          w->queue()->wakeUpConsumers();
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
      std::cout << "result: " << *(static_cast<int*>(result)) << std::endl;
    }  // else {
  }
  return 0;
}

void* ThreadPool::threadFunction(void* arg)
{
  // Cast the argument back to ThreadInfo
  struct ThreadInfo {
      int thread_id;
      ThreadPool* pool;
  };

  // Extract information and manage resources
  ThreadInfo* info = static_cast<ThreadInfo*>(arg);
  int thread_id = info->thread_id;
  ThreadPool* pool = info->pool;
  // Delete the ThreadInfo object to avoid memory leak
  delete info;
  auto& tasks = pool->tasks_[thread_id];
  //  vector<ep::Filter*>* tasks = &(pool->tasks_[thread_id]);

  if (tasks.size() == 0) pthread_exit(nullptr);

  // Run tasks loop until the thread is signaled to stop
  while (pool->state_[thread_id]) {
    bool any = false;
    for (auto const& task : tasks) {
      if (task->state() == FilterState::RUNNING) {
        int err = task->doJob();
        if (err >= 0) any = true;
      }
    }
    if (!any) usleep(10000);  // sleep for 10 milliseconds
  }

  pthread_exit(nullptr);
}

int32_t Pipeline::writeSettings(YAML::Node& config) const
{
  YAML::Node filters_node;

  // Iterate through all filters in the pipeline
  for (const auto& filter : vertices_) {
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

int32_t Pipeline::saveSettings(const std::string& filename) const
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
  catch (const std::exception& e) {
    std::cerr << "Error saving settings to file: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
