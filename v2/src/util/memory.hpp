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

namespace jumanpp {
namespace util {
namespace memory {

inline bool IsPowerOf2(size_t val) { return (val & (val - 1)) == 0; }

struct MemoryPage {
  void* base;
  size_t size;
};

class Manager;

class ManagedAllocatorCore {
  Manager* mgr_;
  char* current_ = nullptr;
  char* end_ = nullptr;
  bool ensureAvailable(size_t size);

 public:
  ManagedAllocatorCore(Manager* manager) : mgr_(manager) {}
  void* allocate_memory(size_t size, size_t alignment);
};

template <typename T, size_t Talign = alignof(T)>
class ManagedAllocator {
  ManagedAllocatorCore core_;

  static constexpr size_t Tsize = sizeof(T);

 public:
  ManagedAllocator(ManagedAllocatorCore core) : core_{core} {}

  template <typename... Args>
  T* make(Args... args) {
    void* buffer = core_.allocate_memory(Tsize, Talign);
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

  Manager(size_t page_size) : page_size_{page_size} {}

#if JUMANPP_USE_DEFAULT_ALLOCATION
  void* allocate(size_t size, size_t align);
#else
  MemoryPage newPage();
#endif

  ~Manager();
};
}
}
}

#endif  // JUMANPP_MEMORY_HPP
