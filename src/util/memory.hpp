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
#include "util/common.hpp"
#include "util/sliceable_array.h"

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

class ErasedAllocator {
 public:
  virtual void *Allocate(size_t size, size_t align) = 0;
  virtual void Reclaim(void *ptr) noexcept = 0;
  virtual ~ErasedAllocator() noexcept = default;
};

class PoolAlloc final : public ErasedAllocator {
  Manager *mgr_;
  char *base_ = nullptr;
  size_t offset_ = 0;
  size_t end_ = 0;
  bool switchToNewPage(size_t size);

 public:
  PoolAlloc(Manager *manager) : mgr_(manager) {}
  PoolAlloc(const PoolAlloc &o) = delete;

  void *allocate_memory(size_t size, size_t alignment);

  template <typename T>
  T *allocate(size_t align = alignof(T)) {
    return (T *)allocate_memory(sizeof(T), align);
  };

  template <typename T>
  T *allocateArray(size_t count, size_t align = alignof(T)) {
    auto data = allocate_memory(sizeof(T) * count, align);
    auto valid = new (data) T[count];
    return valid;
  }

  template <typename T>
  T *allocateRawArray(size_t count, size_t align = alignof(T)) {
    auto data = allocate_memory(sizeof(T) * count, align);
    return reinterpret_cast<T *>(data);
  }

  template <typename T>
  util::MutableArraySlice<T> allocateBuf(size_t count,
                                         size_t align = alignof(T)) {
    auto mem = allocateArray<T>(count, align);
    return util::MutableArraySlice<T>{mem, count};
  }

  template <typename T>
  util::Sliceable<T> allocate2d(size_t rows, size_t cols,
                                size_t align = alignof(T)) {
    auto buf = allocateBuf<T>(rows * cols, align);
    return util::Sliceable<T>{buf, cols, rows};
  }

  template <typename T, typename... Args>
  T *make(Args &&... args) {
    T *buffer = allocate<T>(alignof(T));
    auto obj = new (buffer) T(std::forward<Args>(args)...);
    return obj;
  };

  template <typename T, typename... Args>
  ManagedPtr<T> make_unique(Args &&... args) {
    return ManagedPtr<T>(make<T, Args...>(std::forward<Args>(args)...));
  };

  u64 remaining() const { return end_ - offset_; }

  void reset();

  void *Allocate(size_t size, size_t align) override {
    return allocate_memory(size, align);
  }

  void Reclaim(void *pVoid) noexcept override {}
};

class Manager {
  size_t currentPage = 0;
  std::vector<MemoryPage> pages_;
  size_t page_size_;

 public:
  std::unique_ptr<PoolAlloc> core() {
    return std::unique_ptr<PoolAlloc>(new PoolAlloc{this});
  }

  Manager(size_t page_size) : page_size_{page_size} {}

#if JUMANPP_USE_DEFAULT_ALLOCATION
  void *allocate(size_t size, size_t align);
#else
  MemoryPage newPage();
#endif

  void reset();

  u64 used() const {
    u64 total = 0;
    for (auto &m : pages_) {
      total += m.size;
    }
    return total;
  }

  size_t pageSize() const { return page_size_; }

  static bool supportHugePages();

  ~Manager();
};

template <typename T>
class StlManagedAlloc {
  PoolAlloc *core_;

 public:
  using value_type = T;
  using pointer = value_type *;

  StlManagedAlloc(PoolAlloc *core) noexcept : core_{core} {}

  pointer allocate(size_t n) { return core_->allocateRawArray<T>(n); }

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

class MallocEalloc : public ErasedAllocator {
  void *Allocate(size_t size, size_t align) override;
  void Reclaim(void *pVoid) noexcept override;
};

MallocEalloc *defaultEalloc();

}  // namespace memory
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_MEMORY_HPP
