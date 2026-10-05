#include <gtest/gtest.h>

#include <memory>

#include "core.h"
#include "filters/dummy_filter.h"
#include "filters/sink_filter.h"
#include "filters/src_filter.h"

using namespace epf;

class ExposedSrcFilter : public SrcFilter {
  public:
    using Filter::deactivateSinkPorts;
    using Filter::disconnectSourcePorts;
};

class ExposedDummyFilter : public DummyFilter {
  public:
    using DummyFilter::DummyFilter;
    using Filter::deactivateSinkPorts;
    using Filter::disconnectSourcePorts;
};

class ExposedSinkFilter : public SinkFilter {
  public:
    using Filter::deactivateSinkPorts;
    using Filter::disconnectSourcePorts;
};

class PipelineTest : public ::testing::Test {
  protected:
    Pipeline pipeline_{0};
    ExposedSrcFilter *src_{nullptr};
    ExposedDummyFilter *mid_{nullptr};
    ExposedSinkFilter *sink_{nullptr};

    void SetUp() override
    {
      src_ = pipeline_.add<ExposedSrcFilter>();
      mid_ = pipeline_.add<ExposedDummyFilter>(YAML::Node());
      sink_ = pipeline_.add<ExposedSinkFilter>();

      pipeline_.connect(src_, 0, mid_, 0);
      pipeline_.connect(mid_, 0, sink_, 0);
    }

    void TearDown() override
    {
      // No explicit deletion needed
    }
};

class FailingFilter : public epf::Filter {
  public:
    explicit FailingFilter(bool fail_set = false, bool fail_stop = false)
        : epf::Filter(YAML::Node(), 1, 1),
          fail_set_(fail_set),
          fail_stop_(fail_stop)
    {
    }

  protected:
    int32_t _job() override
    {
      return 0;
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
      if (fail_set_) {
        return -1;
      }
      std::unique_ptr<epf::Message> msg = std::make_unique<epf::Message>();
      std::unique_ptr<epf::DataNode> counter_n =
          std::make_unique<epf::DataNode>("Counter", EP_32S,
                                          std::vector<size_t>{1}, nullptr);
      msg->addItem(std::move(counter_n));
      sinkPort(0)->activate(std::move(msg), nullptr);
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
      return fail_stop_ ? -1 : 0;
    }

  private:
    bool fail_set_{false};
    bool fail_stop_{false};
};

class FailingResetCloseFilter : public epf::Filter {
  public:
    FailingResetCloseFilter()
        : epf::Filter(YAML::Node(), 1, 1)
    {
    }
    int close_calls{0};

  protected:
    int32_t _job() override
    {
      return 0;
    }
    int32_t _open() override
    {
      return 0;
    }
    int32_t _close() override
    {
      ++close_calls;
      return 0;
    }
    int32_t _set() override
    {
      std::unique_ptr<epf::Message> msg = std::make_unique<epf::Message>();
      std::unique_ptr<epf::DataNode> counter_n =
          std::make_unique<epf::DataNode>("Counter", EP_32S,
                                          std::vector<size_t>{1}, nullptr);
      msg->addItem(std::move(counter_n));
      sinkPort(0)->activate(std::move(msg), nullptr);
      return 0;
    }
    int32_t _reset() override
    {
      return -1;
    }
    int32_t _start() override
    {
      return 0;
    }
    int32_t _stop() override
    {
      return 0;
    }
};

class OwnThreadStopConvergenceFilter : public epf::Filter {
  public:
    OwnThreadStopConvergenceFilter()
        : epf::Filter(YAML::Node(), 0, 0)
    {
      job_execution_model_ = OWN_THREAD;
    }

    int stop_calls{0};

  protected:
    int32_t _job() override
    {
      return 0;
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
      ++stop_calls;
      return 0;
    }
};

TEST_F(PipelineTest, BasicLifecycle)
{
  EXPECT_EQ(pipeline_.filterCount(), 3);

  EXPECT_EQ(pipeline_.open(), 0);
  EXPECT_EQ(pipeline_.set(), 0);
  EXPECT_EQ(pipeline_.start(), 0);
  EXPECT_EQ(pipeline_.stop(), 0);
  EXPECT_EQ(pipeline_.reset(), 0);
  EXPECT_EQ(pipeline_.close(), 0);
}

TEST_F(PipelineTest, ZeroThreadCtorFallsBackToAutoInitOnLaunch)
{
  EXPECT_EQ(pipeline_.open(), 0);
  EXPECT_EQ(pipeline_.set(), 0);
  EXPECT_EQ(pipeline_.start(), 0);
  EXPECT_EQ(pipeline_.launch(), 0);
  EXPECT_EQ(pipeline_.halt(), 0);
}

TEST_F(PipelineTest, ResetRemainsSafeAfterManualPreTeardown)
{
  ASSERT_EQ(pipeline_.open(), 0);
  ASSERT_EQ(pipeline_.set(), 0);

  ASSERT_EQ(src_->disconnectSourcePorts(), 0);
  ASSERT_EQ(mid_->disconnectSourcePorts(), 0);
  ASSERT_EQ(sink_->disconnectSourcePorts(), 0);
  ASSERT_EQ(src_->deactivateSinkPorts(), 0);
  ASSERT_EQ(mid_->deactivateSinkPorts(), 0);
  ASSERT_EQ(sink_->deactivateSinkPorts(), 0);

  // Pipeline reset performs two-phase teardown and must tolerate repeated
  // teardown work before invoking Filter::reset() on each node.
  EXPECT_EQ(pipeline_.reset(), 0);
  EXPECT_EQ(pipeline_.close(), 0);
}

TEST(PipelineRunHaltTest, RunPropagatesOpenSetStartLaunchFailures)
{
  {
    Pipeline pipeline(0);
    auto *src = pipeline.add<SrcFilter>();
    auto *mid = pipeline.add<FailingFilter>(true, false);
    auto *sink = pipeline.add<SinkFilter>();
    pipeline.connect(src, 0, mid, 0);
    pipeline.connect(mid, 0, sink, 0);
    EXPECT_LT(pipeline.run(), 0);
    EXPECT_EQ(pipeline.halt(), 0);
  }

  {
    Pipeline pipeline(0);
    auto *src = pipeline.add<SrcFilter>();
    auto *sink = pipeline.add<SinkFilter>();
    // Duplicate connection triggers connectSourceFilter() failure.
    pipeline.connect(src, 0, sink, 0);
    pipeline.connect(src, 0, sink, 1);
    EXPECT_LT(pipeline.run(), 0);
    EXPECT_EQ(pipeline.halt(), 0);
    EXPECT_EQ(pipeline.close(), 0);
  }
}

TEST(PipelineRunHaltTest, HaltPropagatesShutdownFailures)
{
  Pipeline pipeline(0);
  auto *src = pipeline.add<SrcFilter>();
  auto *mid = pipeline.add<FailingFilter>(false, true);
  auto *sink = pipeline.add<SinkFilter>();
  pipeline.connect(src, 0, mid, 0);
  pipeline.connect(mid, 0, sink, 0);

  ASSERT_EQ(pipeline.run(), 0);
  EXPECT_EQ(pipeline.halt(), 0);
}

TEST(PipelineCloseTest, CloseBestEffortAfterPartialResetFailure)
{
  Pipeline pipeline(0);
  auto *src = pipeline.add<SrcFilter>();
  auto *mid = pipeline.add<FailingResetCloseFilter>();
  auto *sink = pipeline.add<SinkFilter>();
  pipeline.connect(src, 0, mid, 0);
  pipeline.connect(mid, 0, sink, 0);

  ASSERT_EQ(pipeline.open(), 0);
  ASSERT_EQ(pipeline.set(), 0);

  EXPECT_EQ(pipeline.close(), 0);
  EXPECT_EQ(src->state(), DISCONNECTED);
  EXPECT_EQ(mid->state(), DISCONNECTED);
  EXPECT_EQ(sink->state(), DISCONNECTED);
  EXPECT_EQ(mid->close_calls, 1);
}

TEST(PipelineJoinStopConvergenceTest, StopJoinConvergesStopRequestToSet)
{
  Pipeline pipeline(1);
  auto *f = pipeline.add<OwnThreadStopConvergenceFilter>();
  ASSERT_NE(f, nullptr);

  ASSERT_EQ(pipeline.open(), 0);
  ASSERT_EQ(pipeline.set(), 0);
  ASSERT_EQ(pipeline.start(), 0);
  ASSERT_EQ(pipeline.launch(), 0);
  ASSERT_EQ(pipeline.stop(), 0);

  // Without post-join convergence, OWN_THREAD filters can remain in
  // STOP_REQUEST because worker doJob() paths do not drive them.
  EXPECT_EQ(f->state(), STOP_REQUEST);

  ASSERT_EQ(pipeline.join(), 0);
  EXPECT_EQ(f->state(), SET);
  EXPECT_GE(f->stop_calls, 1);
}

TEST(PipelineJoinStopConvergenceTest, RestartAfterStopJoinWorksWithoutSet)
{
  Pipeline pipeline(1);
  auto *f = pipeline.add<OwnThreadStopConvergenceFilter>();
  ASSERT_NE(f, nullptr);

  ASSERT_EQ(pipeline.open(), 0);
  ASSERT_EQ(pipeline.set(), 0);
  ASSERT_EQ(pipeline.start(), 0);
  ASSERT_EQ(pipeline.launch(), 0);
  ASSERT_EQ(pipeline.stop(), 0);
  ASSERT_EQ(pipeline.join(), 0);
  ASSERT_EQ(f->state(), SET);

  ASSERT_EQ(pipeline.start(), 0);
  ASSERT_EQ(pipeline.launch(), 0);
  ASSERT_EQ(pipeline.stop(), 0);
  ASSERT_EQ(pipeline.join(), 0);
  EXPECT_EQ(f->state(), SET);
}

TEST(PipelineJoinStopConvergenceTest, JoinConvergesOnlyRemainingStopRequests)
{
  Pipeline pipeline(1);
  auto *external = pipeline.add<FailingFilter>(false, false);
  auto *own = pipeline.add<OwnThreadStopConvergenceFilter>();
  ASSERT_NE(external, nullptr);
  ASSERT_NE(own, nullptr);

  ASSERT_EQ(pipeline.open(), 0);
  ASSERT_EQ(pipeline.set(), 0);
  ASSERT_EQ(pipeline.start(), 0);
  ASSERT_EQ(pipeline.stop(), 0);

  // Simulate one filter already converged by worker/job path.
  ASSERT_EQ(external->completeStopTransitionIfRequested("test pre-converge"),
            0);
  EXPECT_EQ(external->state(), SET);
  EXPECT_EQ(own->state(), STOP_REQUEST);

  ASSERT_EQ(pipeline.join(), 0);
  EXPECT_EQ(external->state(), SET);
  EXPECT_EQ(own->state(), SET);
  EXPECT_GE(own->stop_calls, 1);
}
