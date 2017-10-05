//
// Created by Arseny Tolmachev on 2017/04/06.
//

#ifndef JUMANPP_TRAINING_EXECUTOR_H
#define JUMANPP_TRAINING_EXECUTOR_H

#include "scw.h"
#include "trainer.h"

#include <util/bounded_queue.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace jumanpp {
namespace core {
namespace training {

struct TrainingExecutionResult {
  OwningTrainer* trainer;
  Status processStatus;

  TrainingExecutionResult() : trainer{nullptr}, processStatus{Status::Ok()} {}
  TrainingExecutionResult(OwningTrainer* tr, Status status)
      : trainer{tr}, processStatus{std::move(status)} {}
};

class TrainingExecutorThread {
  const analysis::ScorerDef* scoreConf_;
  util::bounded_queue<OwningTrainer*>* trainers_;
  util::bounded_queue<TrainingExecutionResult>* results_;
  std::thread thread_;

  void run();

  // an entry poinf the the thread
  static void runMain(TrainingExecutorThread* ctx) { ctx->run(); }

 public:
  explicit TrainingExecutorThread(
      const analysis::ScorerDef* conf,
      util::bounded_queue<OwningTrainer*>* trainers,
      util::bounded_queue<TrainingExecutionResult>* results);
  void finish();
};

class TrainingExecutor {
  std::vector<std::unique_ptr<TrainingExecutorThread>> threads_;
  util::bounded_queue<OwningTrainer*> trainers_;
  util::bounded_queue<TrainingExecutionResult> results_;

 public:
  Status initialize(const analysis::ScorerDef* sconf, u32 nthreads);

  bool submitNext(OwningTrainer* next) {
    return trainers_.offer(std::move(next));
  }

  bool resultWithoutWait(TrainingExecutionResult* result) {
    return results_.recieve(result);
  }

  size_t nowReady() const { return results_.size(); }

  TrainingExecutionResult waitOne();
  ~TrainingExecutor();
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINING_EXECUTOR_H
