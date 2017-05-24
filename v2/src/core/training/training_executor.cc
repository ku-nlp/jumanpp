//
// Created by Arseny Tolmachev on 2017/04/06.
//

#include "training_executor.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

TrainingExecutionResult TrainingExecutorThread::waitForTrainer() {
  auto state = state_.load();
  u64 count = 0;
  while (state == ExecutorThreadState::HaveInput) {
    // ok, the computation did not start yet, let it do so
    auto msec = std::chrono::nanoseconds(10);
    std::this_thread::sleep_for(msec);
    state = state_.load();
    ++count;

    if (count > 10000) {
      Status status = Status::InvalidState() << "waiting for trainer forever";
      return {nullptr, status};
    }
  }
  switch (state) {
    case ExecutorThreadState::RunningComputation: {
      // when the lock is acquired
      // it means that the run function has moved to the next iteration
      // because the only place when it yield the lock is waiting
      // for input_ready_ conditional variable.
      // If that is the case, our value should be ready.
      std::unique_lock<std::mutex> lck{mutex_};
      JPP_DCHECK_EQ(state_, ExecutorThreadState::ComputationFinished);
      return {trainer_, processStatus_};
    }
    case ExecutorThreadState::ComputationFinished: {
      // in this case the result is already ready, so just return it
      // no need for locks
      return {trainer_, processStatus_};
    }
    default: {
      // otherwise we have an invalid state
      Status status = Status::InvalidState() << "thread was not initialized: "
                                             << static_cast<int>(state);
      return {nullptr, status};
    }
  }
}

void TrainingExecutorThread::publishTrainer(OwningTrainer *trainer) {
  std::unique_lock<std::mutex> lck{mutex_};
  // this lock is needed to avoid situation
  // when run() is not waiting on the condition variable
  trainer_ = trainer;
  state_ = ExecutorThreadState::HaveInput;
  input_ready_.notify_all();
}

TrainingExecutorThread::TrainingExecutorThread(const analysis::ScorerDef *conf)
    : scoreConf_{conf},
      processStatus_{Status::Ok()},
      thread_{TrainingExecutorThread::runMain, this} {}

TrainingExecutorThread::~TrainingExecutorThread() {
  state_ = ExecutorThreadState::Exiting;
  input_ready_.notify_all();
  thread_.join();
}

void TrainingExecutorThread::run() {
  std::unique_lock<std::mutex> lck{mutex_};
  while (true) {
    input_ready_.wait(lck);
    auto st = state_.load();
    if (st == ExecutorThreadState::Exiting) {
      return;
    }
    JPP_DCHECK_EQ(st, ExecutorThreadState::HaveInput);
    state_ = ExecutorThreadState::RunningComputation;
    // read trainer field only once
    auto t = trainer_;
    processStatus_ = t->prepare();
    if (processStatus_) {
      processStatus_ = t->compute(scoreConf_);
    }
    auto s2 = state_.exchange(ExecutorThreadState::ComputationFinished);
    if (s2 == ExecutorThreadState::Exiting) {
      return;
    }
  }
}

Status TrainingExecutor::initialize(const analysis::ScorerDef *sconf,
                                    u32 nthreads) {
  threads_.clear();
  try {
    for (u32 i = 0; i < nthreads; ++i) {
      threads_.emplace_back(new TrainingExecutorThread{sconf});
    }
    head_ = 0;
    tail_ = 0;
  } catch (std::system_error &e) {
    return Status::InvalidState()
           << "failed to initialize executor: " << e.code()
           << " msg: " << e.what();
  }
  return Status::Ok();
}

bool TrainingExecutor::runNext(OwningTrainer *next,
                               TrainingExecutionResult *result) {
  JPP_DCHECK_LE(available(), capacity());

  bool status = false;
  if (available() == 0) {
    *result = waitOne();
    JPP_DCHECK_GT(available(), 0);
    status = true;
  }

  u32 headIdx = head_ % capacity();
  auto &thread = threads_[headIdx];
  thread->publishTrainer(next);
  head_ += 1;
  //  LOG_TRACE() << "added example #" << next->line() << " to thr=" << headIdx
  //              << " head=" << head_ << ", tail=" << tail_
  //              << " status=" << status;
  return status;
}

i64 safeLine(const TrainingExecutionResult &res) {
  i64 val = -1;
  if (res.trainer != nullptr) {
    val = res.trainer->line();
  }
  return val;
}

TrainingExecutionResult TrainingExecutor::waitOne() {
  u32 lastId = tail_ % capacity();
  auto &thread = threads_[lastId];
  tail_ += 1;
  auto result = thread->waitForTrainer();
  //  LOG_TRACE() << "got example #" << safeLine(result) << " from thr=" <<
  //  lastId
  //              << " tail=" << tail_;
  return result;
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp