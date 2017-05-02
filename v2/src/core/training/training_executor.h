//
// Created by Arseny Tolmachev on 2017/04/06.
//

#ifndef JUMANPP_TRAINING_EXECUTOR_H
#define JUMANPP_TRAINING_EXECUTOR_H

#include "scw.h"
#include "trainer.h"

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
};

class TrainingExecutorThread {
  analysis::ScoreConfig* scoreConf_;
  Status processStatus_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable input_ready_;
  std::condition_variable output_ready_;
  OwningTrainer* trainer_;
  std::atomic<bool> finished_;
  std::atomic<bool> running_;

  void run() {
    while (true) {
      std::unique_lock<std::mutex> lck{mutex_};
      input_ready_.wait(lck);
      if (finished_) {
        return;
      }
      running_ = true;
      auto t = trainer_;  // read trainer field only once
      processStatus_ = t->prepare();
      if (processStatus_) {
        processStatus_ = t->compute(scoreConf_);
      }
      running_ = false;
      output_ready_.notify_all();
    }
  }

  static void runMain(TrainingExecutorThread* ctx) { ctx->run(); }

 public:
  TrainingExecutorThread(analysis::ScoreConfig* conf)
      : scoreConf_{conf},
        processStatus_{Status::Ok()},
        thread_{TrainingExecutorThread::runMain, this} {}

  void publishTrainer(OwningTrainer* trainer) {
    trainer_ = trainer;
    input_ready_.notify_all();
  }

  TrainingExecutionResult waitForTrainer() {
    if (!running_) {
      return {nullptr, Status::NotImplemented()};
    }
    std::unique_lock<std::mutex> lck{mutex_};
    output_ready_.wait(lck);
    return {trainer_, processStatus_};
  }

  ~TrainingExecutorThread() {
    finished_ = true;
    input_ready_.notify_all();
    thread_.join();
  }
};

class TrainingExecutor {};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINING_EXECUTOR_H
