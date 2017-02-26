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
#include <vector>
#include <memory>
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
  using ptr_t = typename std::unique_ptr<T, DestructorOnlyDeleter<T>>::pointer;
  void operator()(ptr_t ptr) const {
    ptr->~T(); //call destructor
  }
};

template <typename T>
using ManagedPtr = std::unique_ptr<T, DestructorOnlyDeleter<T>>;

class ManagedAllocatorCore {
  Manager *mgr_;
  char *current_ = nullptr;
  char *end_ = nullptr;
  bool ensureAvailable(size_t size);

 public:
  ManagedAllocatorCore(Manager *manager) : mgr_(manager) {}
  void *allocate_memory(size_t size, size_t alignment);

  template <typename T>
  T *allocate(size_t align) {
    return (T *)allocate_memory(sizeof(T), align);
  };

  template <typename T>
  T *allocateArray(size_t count, size_t align = alignof(T)) {
    return (T *)allocate_memory(sizeof(T) * count, align);
  }

  template <typename T, typename... Args>
  T *make(Args... args) {
    T *buffer = allocate<T>(alignof(T));
    auto obj = new (buffer) T(std::forward<Args>(args)...);
    return obj;
  };

  template <typename T, typename... Args>
  ManagedPtr<T> make_unique(Args... args) {
    return ManagedPtr<T>(make<T, Args...>(std::forward<Args...>(args...)));
  };
};

template <typename T, size_t Talign = alignof(T)>
class ManagedAllocator {
  ManagedAllocatorCore core_;

  static constexpr size_t Tsize = sizeof(T);

 public:
  ManagedAllocator(ManagedAllocatorCore core) : core_{core} {}

  template <typename... Args>
  T *make(Args... args) {
    void *buffer = core_.allocate_memory(Tsize, Talign);
    auto obj = new (buffer) T(std::forward<Args>(args)...);
    return obj;
  }
};

class Manager {
  std::vector<MemoryPage> pages_;
  size_t page_size_;

 public:
  template <typename T>
  ManagedAllocator<T> allocator() {
    return ManagedAllocator<T>{ManagedAllocatorCore{this}};
  }

  ManagedAllocatorCore core() {
    return ManagedAllocatorCore{this};
  }

  Manager(size_t page_size) : page_size_{page_size} {}

#if JUMANPP_USE_DEFAULT_ALLOCATION
  void *allocate(size_t size, size_t align);
#else
  MemoryPage newPage();
#endif

  ~Manager();
};

template <typename T>
class StlManagedAlloc {
  ManagedAllocatorCore* core_;
public:
  using value_type = T;
  using pointer = value_type*;

  StlManagedAlloc(ManagedAllocatorCore* core) noexcept : core_{core} {}

  pointer allocate(size_t n) {
    return core_->allocateArray<T>(n);
  }

  void deallocate(T* obj, size_t n) noexcept {
    //noop!
  }

  bool operator==(const StlManagedAlloc<T>& o) const noexcept {
    return core_ == o.core_;
  }

  bool operator!=(const StlManagedAlloc<T>& o) const noexcept {
    return !(*this == o);
  }
};

}
}
}

#endif  // JUMANPP_MEMORY_HPP
