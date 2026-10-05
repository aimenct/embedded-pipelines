# EPF Development Plan

## Priority 1: Immediate Correctness

1. **[DONE] Fix the Queue blocking getter bug.**
   Change `=` to comparison in `readBlocking()` and `writeBlocking()`. Add regression tests for both blocking modes.

2. **[DONE] Add concurrency protection to settings.**

   - Added a `std::shared_mutex` owned by `Filter` to protect `settings_`, with shared locking for reads and exclusive locking for mutations.
   - Setting operations now keep access validation, device access, and cached-value updates within the same lock boundary, while settings-change notifications are emitted after releasing the lock.
   - Changed setting value reads to return owned `std::optional<T>` values instead of pointers into the settings tree, and changed `Filter::settings()` to return a protected copy.
   - Added polymorphic `Node::clone()` support and `Filter::cloneSettingNode()` for protected copies of setting nodes and their owned reference subgraphs, including enum references.
   - Added `Filter::withSettingNode()` for callbacks that must inspect a node while the shared settings lock remains held. OPC-UA reads use this API to copy values directly into independently owned UA variants, while writes use the synchronized `Filter::setSettingValue()` path.
   - Added concurrency tests for simultaneous readers, serialized writers, atomic device reads and cache updates, cloned managed values and enum references, protected callback reads, and settings-change callback re-entry.

3. **[DONE] Add CI build and test coverage.**
   Start with a core-only job: `ENABLE_CAMERA=OFF`, `ENABLE_OPCUA=OFF`, `ENABLE_DOCS=OFF`. Build the library and tests, then run the gtest binaries or register them with CTest.

4. **[DONE] Remove process-level exits from `DataNode`.**
   `DataNode::read()` and `DataNode::write()` now return errors instead of terminating the process; affected callers handle those return values explicitly.

5. **[DONE] Resolve the `addSetting()` / `addSettingAuto()` half-transition.**
   Make the category-based API the normal public `addSetting()` path. Rename the parent-index-based API to something explicit like `addSettingUnder()` and deprecate/remove `addSettingAuto()`.

## Priority 2: Synchronization Foundation

1. **Refactor lifecycle synchronization from pthreads to standard C++.**
   Move `Filter` state synchronization to RAII-style `std::mutex`, `std::unique_lock`, and `std::condition_variable`.

2. **Replace stop-convergence polling.**
   Use a condition variable for `STOP_REQUEST -> SET` instead of `usleep()`. Timeouts should report lifecycle failure, not silently force state.

3. **Make the Queue synchronization boundary explicit.**
   Group the mutable state protected by `buffer_mtx_` into a dedicated synchronized-state object so the lock's protection domain is visible without tracing the complete Queue implementation. Consider renaming the mutex to reflect its broader role and restricting state access through an RAII locked handle where practical.

## Priority 3: Queue Refactor

1. **Add coverage before changing internals.**
   Cover blocking/non-blocking behavior, FIFO/LIFO behavior, abort paths, unsubscribe paths, and multi-producer/multi-consumer cases.

2. **Migrate Queue synchronization carefully.**
   Convert pthread mutexes/condition variables to `std::mutex` and `std::condition_variable` after tests are in place.

3. **Reduce duplicated barrier bookkeeping.**
   Extract helper functions for FIFO/LIFO barrier recomputation and producer/consumer state transitions.

4. **Clarify memory ownership.**
   Replace manual allocation where practical, or isolate it behind a small buffer-owning type.

## Priority 4: Cleanup

1. **Remove stale commented-out code.**
2. **Normalize ownership conventions in the information model.**
3. **Document lifecycle and threading invariants for `Filter`, `Queue`, and `Settings`.**

## Note on `ENABLE_ALL`

Do not make `ENABLE_ALL=ON` the first CI gate unless the optional camera and OPC-UA dependencies are installed reliably. Add that as a later full-feature job.
