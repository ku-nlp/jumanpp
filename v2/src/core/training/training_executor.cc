//
// Created by Arseny Tolmachev on 2017/04/06.
//

#include "training_executor.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

TrainingExecutorThread::TrainingExecutorThread(
    const analysis::ScorerDef* conf,
    util::bounded_queue<OwningTrainer*>* trainers,
    util::bounded_queue<TrainingExecutionResult>* results)
    : scoreConf_{conf},
      trainers_{trainers},
      results_{results},
      thread_{TrainingExecutorThread::runMain, this} {}

void TrainingExecutorThread::run() {
  while (true) {
    auto trainer = trainers_->waitFor();
    if (trainer == nullptr) {
      return;
    }

    // read trainer field only once
    auto status = trainer->prepare();
    if (status) {
      status = trainer->compute(scoreConf_);
    }

    if (!status) {
      auto ops = ::jumanpp::status_impl::StatusOps(&status);
      ops.AddFrame(__FILE__, __LINE__, __FUNCTION__);
      ::jumanpp::status_impl::MessageBuilder mbld;
      mbld << "while processing example ["
           << trainer->trainer()->example().comment() << "] on line "
           << trainer->line();
      mbld.ReplaceMessage(ops.data());
    }
    TrainingExecutionResult result{trainer, std::move(status)};
    while (!results_->offer(std::move(result))) {
      std::this_thread::yield();
    }
  }
}

void TrainingExecutorThread::finish() { thread_.join(); }

Status TrainingExecutor::initialize(const analysis::ScorerDef* sconf,
                                    u32 nthreads) {
  threads_.clear();
  try {
    for (u32 i = 0; i < nthreads; ++i) {
      threads_.emplace_back(
          new TrainingExecutorThread{sconf, &trainers_, &results_});
    }
    trainers_.initialize(nthreads * 2);
    results_.initialize(nthreads * 2);
  } catch (std::system_error& e) {
    return JPPS_INVALID_STATE << "failed to initialize executor: " << e.code()
                              << " msg: " << e.what();
  }
  return Status::Ok();
}

TrainingExecutionResult TrainingExecutor::waitOne() {
  return results_.waitFor();
}

TrainingExecutor::~TrainingExecutor() {
  while (trainers_.offer(nullptr))
    ;  // do nothing
  trainers_.unblock_all();
  for (auto& t : threads_) {
    t->finish();
  }
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp