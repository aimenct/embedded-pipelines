// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "core.h"

using namespace epf;

namespace {

class SignalFilter : public Filter {
  public:
    int value{0};

    SignalFilter()
        : Filter(YAML::Node(), 0, 0)
    {
      type_ = "SignalFilter";
    }

    int32_t _open() override
    {
      return 0;
    }
    int32_t _close() override
    {
      return 0;
    }
    int32_t _set() override
    {
      return 0;
    }
    int32_t _reset() override
    {
      return 0;
    }
    int32_t _start() override
    {
      return 0;
    }
    int32_t _stop() override
    {
      return 0;
    }
    int32_t _job() override
    {
      return 0;
    }
};

class FailingResetSignalFilter : public SignalFilter {
  public:
    int32_t _reset() override
    {
      return -1;
    }
};

}  // namespace

TEST(SignalsTest, SettingsSignalFiresOnAddAndUpdate)
{
  SignalFilter filter;
  std::vector<SettingsChangeKind> kinds;
  std::vector<uint64_t> revisions;
  const uint64_t start_revision = filter.settingsRevision();

  [[maybe_unused]] auto connection = filter.settingsChanged().connect(
      [&kinds, &revisions](const Filter &, uint64_t revision,
                           SettingsChangeKind kind, const std::string &) {
        kinds.push_back(kind);
        revisions.push_back(revision);
      });

  filter.addSetting("value", filter.value);
  EXPECT_EQ(filter.setSettingValue<int>("base.value", 4), 0);

  ASSERT_EQ(kinds.size(), 2u);
  EXPECT_EQ(kinds[0], SettingsChangeKind::Added);
  EXPECT_EQ(kinds[1], SettingsChangeKind::Updated);
  EXPECT_EQ(revisions[0], start_revision + 1);
  EXPECT_EQ(revisions[1], start_revision + 2);
  EXPECT_LT(revisions[0], revisions[1]);
}

TEST(SignalsTest, StateSignalFiresOnSuccessfulTransitionsOnly)
{
  SignalFilter filter;
  std::vector<std::pair<FilterState, FilterState>> transitions;

  [[maybe_unused]] auto state_connection = filter.stateChanged().connect(
      [&transitions](const Filter &, FilterState old_state,
                     FilterState new_state, const std::string &) {
        transitions.emplace_back(old_state, new_state);
      });

  EXPECT_EQ(filter.start(), -1);
  EXPECT_TRUE(transitions.empty());

  EXPECT_EQ(filter.open(), 0);
  EXPECT_EQ(filter.set(), 0);
  EXPECT_EQ(filter.start(), 0);
  EXPECT_EQ(filter.stop(), 0);
  EXPECT_EQ(filter.doJob(), -1);

  ASSERT_EQ(transitions.size(), 5u);
  EXPECT_EQ(transitions[0],
            (std::pair<FilterState, FilterState>{DISCONNECTED, CONNECTED}));
  EXPECT_EQ(transitions[1],
            (std::pair<FilterState, FilterState>{CONNECTED, SET}));
  EXPECT_EQ(transitions[2],
            (std::pair<FilterState, FilterState>{SET, RUNNING}));
  EXPECT_EQ(transitions[3],
            (std::pair<FilterState, FilterState>{RUNNING, STOP_REQUEST}));
  EXPECT_EQ(transitions[4],
            (std::pair<FilterState, FilterState>{STOP_REQUEST, SET}));
}

TEST(SignalsTest, ErrorSignalFiresOnInvalidPreconditions)
{
  SignalFilter filter;
  std::vector<std::string> errors;
  int state_signal_count = 0;

  [[maybe_unused]] auto error_connection = filter.errorOccurred().connect(
      [&errors](const Filter &, const std::string &reason) {
        errors.push_back(reason);
      });

  [[maybe_unused]] auto state_connection = filter.stateChanged().connect(
      [&state_signal_count](const Filter &, FilterState, FilterState,
                            const std::string &) { ++state_signal_count; });

  EXPECT_EQ(filter.set(), -1);
  EXPECT_EQ(filter.start(), -1);
  EXPECT_EQ(filter.stop(), -1);

  EXPECT_EQ(errors.size(), 3u);
  EXPECT_EQ(state_signal_count, 0);
}

TEST(SignalsTest, PipelineAggregatesFilterSettingsSignal)
{
  Pipeline pipeline{0};
  FilterId seen_id = 0;
  std::size_t signal_count = 0;

  [[maybe_unused]] auto pipeline_connection =
      pipeline.filterSettingsChanged().connect(
          [&seen_id, &signal_count](const Pipeline &, FilterId id,
                                    const Filter &, uint64_t,
                                    SettingsChangeKind, const std::string &) {
            seen_id = id;
            ++signal_count;
          });

  auto *filter = pipeline.add<SignalFilter>();
  ASSERT_NE(filter, nullptr);

  const FilterId expected_id = pipeline.filterId(filter);
  ASSERT_NE(expected_id, 0u);

  filter->addSetting("pipeline_value", filter->value);

  EXPECT_EQ(signal_count, 1u);
  EXPECT_EQ(seen_id, expected_id);
}

TEST(SignalsTest, PipelineRewiresSignalsAfterPartialResetFailure)
{
  Pipeline pipeline{0};
  FilterId seen_id = 0;
  std::size_t state_signal_count = 0;
  std::vector<std::pair<FilterState, FilterState>> transitions;

  [[maybe_unused]] auto state_connection =
      pipeline.filterStateChanged().connect(
          [&seen_id, &state_signal_count](const Pipeline &, FilterId id,
                                          const Filter &, FilterState,
                                          FilterState, const std::string &) {
            seen_id = id;
            ++state_signal_count;
          });

  [[maybe_unused]] auto filter_state_connection =
      pipeline.filterStateChanged().connect(
          [&transitions](const Pipeline &, FilterId, const Filter &,
                         FilterState old_state, FilterState new_state,
                         const std::string &) {
            transitions.emplace_back(old_state, new_state);
          });

  auto *filter = pipeline.add<FailingResetSignalFilter>();
  ASSERT_NE(filter, nullptr);
  ASSERT_EQ(pipeline.open(), 0);
  ASSERT_EQ(pipeline.set(), 0);

  const FilterId expected_id = pipeline.filterId(filter);
  ASSERT_NE(expected_id, 0u);

  // reset() fails because filter reset fails, but pipeline must still rewire
  // signals afterwards.
  EXPECT_EQ(pipeline.reset(), -1);

  // close() attempts reset() from SET state. Even if reset fails, close()
  // still performs best-effort runtime close and emits a final transition.
  EXPECT_EQ(filter->close(), 0);
  EXPECT_EQ(filter->state(), DISCONNECTED);
  EXPECT_EQ(transitions.back(),
            (std::pair<FilterState, FilterState>{SET, DISCONNECTED}));
  // open/set plus final close transition succeed in this scenario.
  EXPECT_EQ(state_signal_count, 3u);
  EXPECT_EQ(seen_id, expected_id);
}

TEST(SignalsTest, CloseEmitsWarningAndFinalStateAfterResetFailure)
{
  FailingResetSignalFilter filter;
  std::vector<std::string> errors;
  std::vector<std::pair<FilterState, FilterState>> transitions;

  [[maybe_unused]] auto error_connection = filter.errorOccurred().connect(
      [&errors](const Filter &, const std::string &reason) {
        errors.push_back(reason);
      });

  [[maybe_unused]] auto state_connection = filter.stateChanged().connect(
      [&transitions](const Filter &, FilterState old_state,
                     FilterState new_state, const std::string &) {
        transitions.emplace_back(old_state, new_state);
      });

  ASSERT_EQ(filter.open(), 0);
  ASSERT_EQ(filter.set(), 0);

  EXPECT_EQ(filter.close(), 0);
  EXPECT_EQ(filter.state(), DISCONNECTED);
  EXPECT_FALSE(transitions.empty());
  EXPECT_EQ(transitions.back(),
            (std::pair<FilterState, FilterState>{SET, DISCONNECTED}));
  EXPECT_FALSE(errors.empty());
  EXPECT_EQ(errors.back(), "close warning: reset failed");
}
