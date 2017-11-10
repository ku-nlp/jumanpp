//
// Created by Arseny Tolmachev on 2017/10/31.
//

#ifndef JUMANPP_PROGRESS_H
#define JUMANPP_PROGRESS_H

#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {

class ProgressCallback {
 public:
  virtual ~ProgressCallback() = default;
  virtual void report(u64 current, u64 total) = 0;
  virtual void recordName(StringPiece name) = 0;
};

template <typename Fn, typename Fn2>
class FunctionProgressCallback : public ProgressCallback {
  Fn fn_;
  Fn2 fn2_;

 public:
  FunctionProgressCallback(Fn fn, Fn2 fn2) : fn_{fn}, fn2_{fn2} {}
  void report(u64 current, u64 total) override { fn_(current, total); }
  void recordName(StringPiece name) override { fn2_(name); }
};

template <typename Fn, typename Fn2>
FunctionProgressCallback<Fn, Fn2> progressCallback(Fn fn, Fn2 fn2) {
  return FunctionProgressCallback<Fn, Fn2>{fn, fn2};
}

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PROGRESS_H
