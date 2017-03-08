//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_LATTICE_PLUGIN_H
#define JUMANPP_LATTICE_PLUGIN_H

#include <utility>
#include "util/memory.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class LatticeRightBoundary;
class LatticeBoundaryConnection;

class LatticePlugin {
 public:
  virtual ~LatticePlugin() = default;
  virtual LatticePlugin* clone(
      util::memory::ManagedAllocatorCore* alloc) const = 0;
  virtual void install(LatticeRightBoundary* right) = 0;
  virtual void install(LatticeBoundaryConnection* connection) = 0;
};

template <typename T>
class LazyField {
  union {
    u64 flag_;
    T value_;
  };

 public:
  LazyField() : flag_{0xdeadbeefdeadbeefULL} {}

  bool isInitialized() const { return flag_ != 0xdeadbeefdeadbeefULL; }

  template <typename... Args>
  void initialize(Args&&... args) {
    new (&value_) T{std::forward<Args>(args)...};
  }

  T& value() {
    JPP_DCHECK(isInitialized());
    return value_;
  }

  const T& value() const {
    JPP_DCHECK(isInitialized());
    return value_;
  }

  ~LazyField() {
    if (isInitialized()) {
      value_.~T();
    }
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_PLUGIN_H
