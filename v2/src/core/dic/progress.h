//
// Created by Arseny Tolmachev on 2017/10/31.
//

#ifndef JUMANPP_PROGRESS_H
#define JUMANPP_PROGRESS_H

#include "util/types.hpp"

namespace jumanpp {
namespace core {

class ProgressCallback {
 public:
  virtual ~ProgressCallback() = default;
  virtual void report(u64 current, u64 total) = 0;
};

template <typename Fn>
class FunctionProgressCallback : public ProgressCallback {
  Fn fn_;

 public:
  FunctionProgressCallback(Fn fn) : fn_{fn} {}

  void report(u64 current, u64 total) override { fn_(current, total); }
};

template <typename Fn>
FunctionProgressCallback<Fn> progressCallback(Fn fn) {
  return FunctionProgressCallback<Fn>{fn};
}

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PROGRESS_H
