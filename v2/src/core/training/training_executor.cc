//
// Created by Arseny Tolmachev on 2017/04/06.
//

#include "training_executor.h"

namespace jumanpp {
namespace core {
namespace training {

TrainingExecutionResult TrainingExecutorThread::waitForTrainer() {
  switch (state_.load()) {
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
      return {nullptr, Status::InvalidState()};
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

TrainingExecutorThread::TrainingExecutorThread(analysis::ScoreConfig *conf)
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

void TrainingExecutor::initialize(analysis::ScoreConfig *sconf, u32 nthreads) {
  threads_.clear();
  for (u32 i = 0; i < nthreads; ++i) {
    threads_.emplace_back(new TrainingExecutorThread{sconf});
  }
  head_ = 0;
  tail_ = 0;
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
  return status;
}

TrainingExecutionResult TrainingExecutor::waitOne() {
  u32 lastId = tail_ % capacity();
  auto &thread = threads_[lastId];
  tail_ += 1;
  return thread->waitForTrainer();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp