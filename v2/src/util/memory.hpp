//
// Created by Arseny Tolmachev on 2017/02/16.
//

#ifndef JUMANPP_MEMORY_HPP
#define JUMANPP_MEMORY_HPP

#ifdef JUMANPP_DEBUG_MEMORY
#define JUMANPP_USE_DEFAULT_ALLOCATION 1
#else
#define JUMANPP_USE_DEFAULT_ALLOCATION 0
#endif

#include <cstddef>
#include <memory>
#include <vector>
#include "util/array_slice.h"
#include "util/common.hpp"

namespace jumanpp {
namespace util {
namespace memory {

inline bool IsPowerOf2(size_t val) { return (val & (val - 1)) == 0; }

inline bool IsAligned(size_t val, size_t alignment) {
  return (val & (alignment - 1)) == 0;
}

inline size_t Align(size_t val, size_t alignment) {
  JPP_DCHECK(IsPowerOf2(alignment));
  if (!IsAligned(val, alignment)) {
    return (val & ~(alignment - 1)) + alignment;
  }
  return val;
}

struct MemoryPage {
  void *base;
  size_t size;
};

class Manager;

template <typename T>
struct DestructorOnlyDeleter {
  using ptr_t = T *;
  void operator()(ptr_t ptr) const {
    ptr->~T();  // call destructor
  }
};

template <typename T>
using ManagedPtr = std::unique_ptr<T, DestructorOnlyDeleter<T>>;

class ManagedAllocatorCore {
  Manager *mgr_;
  char *base_ = nullptr;
  size_t offset_ = 0;
  size_t end_ = 0;
  bool ensureAvailable(size_t size);

 public:
  ManagedAllocatorCore(Manager *manager) : mgr_(manager) {}
  ManagedAllocatorCore(const ManagedAllocatorCore &o) = delete;

  void *allocate_memory(size_t size, size_t alignment);

  template <typename T>
  T *allocate(size_t align) {
    return (T *)allocate_memory(sizeof(T), align);
  };

  template <typename T>
  T *allocateArray(size_t count, size_t align = alignof(T)) {
    return (T *)allocate_memory(sizeof(T) * count, align);
  }

  template <typename T>
  util::MutableArraySlice<T> allocateBuf(size_t count,
                                         size_t align = alignof(T)) {
    auto mem = (T *)allocate_memory(sizeof(T) * count, align);
    return util::MutableArraySlice<T>{mem, count};
  }

  template <typename T, typename... Args>
  T *make(Args&&... args) {
    T *buffer = allocate<T>(alignof(T));
    auto obj = new (buffer) T(std::forward<Args>(args)...);
    return obj;
  };

  template <typename T, typename... Args>
  ManagedPtr<T> make_unique(Args&&... args) {
    return ManagedPtr<T>(make<T, Args...>(std::forward<Args...>(args...)));
  };

  void reset();
};

class Manager {
  size_t currentPage = 0;
  std::vector<MemoryPage> pages_;
  size_t page_size_;

 public:
  std::unique_ptr<ManagedAllocatorCore> core() {
    return std::unique_ptr<ManagedAllocatorCore>(
        new ManagedAllocatorCore{this});
  }

  Manager(size_t page_size) : page_size_{page_size} {}

#if JUMANPP_USE_DEFAULT_ALLOCATION
  void *allocate(size_t size, size_t align);
#else
  MemoryPage newPage();
#endif

  void reset();

  ~Manager();
};

template <typename T>
class StlManagedAlloc {
  ManagedAllocatorCore *core_;

 public:
  using value_type = T;
  using pointer = value_type *;

  StlManagedAlloc(ManagedAllocatorCore *core) noexcept : core_{core} {}

  pointer allocate(size_t n) { return core_->allocateArray<T>(n); }

  void deallocate(T *obj, size_t n) noexcept {
    // noop!
  }

  bool operator==(const StlManagedAlloc<T> &o) const noexcept {
    return core_ == o.core_;
  }

  bool operator!=(const StlManagedAlloc<T> &o) const noexcept {
    return !(*this == o);
  }
};

template <typename T>
using ManagedVector = std::vector<T, StlManagedAlloc<T>>;
}
}
}

#endif  // JUMANPP_MEMORY_HPP
