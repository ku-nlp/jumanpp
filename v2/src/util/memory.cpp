//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include "memory.hpp"
#include <sys/mman.h>
#include <cassert>
#include <cstddef>
#include <memory>
#include "logging.hpp"

namespace jumanpp {
namespace util {
namespace memory {

#if JUMANPP_USE_DEFAULT_ALLOCATION
void* ManagedAllocatorCore::allocate_memory(size_t size, size_t alignment) {
  assert(IsPowerOf2(alignment) && "alignment should be a power of 2");
  return mgr_->allocate(size, std::max<size_t>(alignment, 8));
}

void* Manager::allocate(size_t size, size_t align) {
  void* ptr = nullptr;
  int error = posix_memalign(&ptr, align, size);
  if (error != 0) {
    LOG_ERROR() << "posix_memalign failed, errno=" << strerror(error);
    throw std::bad_alloc();
  }
  pages_.emplace_back(MemoryPage{ptr, size});
  return ptr;
}

Manager::~Manager() {
  for (auto obj : pages_) {
    free(obj.base);
  }
}
#else
void* ManagedAllocatorCore::allocate_memory(size_t size, size_t alignment) {
  assert(IsPowerOf2(alignment) && "alignment should be a power of 2");

  // check if aligned
  size_t mask = alignment - 1;
  if ((reinterpret_cast<size_t>(current_) & mask) == 0) {
    ensureAvailable(size);
    void* addr = current_;
    current_ += size;
    return addr;
  } else {
    // need to align memory
    ensureAvailable(size + alignment);
    auto aligned =
        reinterpret_cast<char*>(reinterpret_cast<size_t>(current_) & ~mask);
    aligned += alignment;
    current_ = aligned + size;
    return aligned;
  }
}

MemoryPage Manager::newPage() {
  void* addr = ::mmap(NULL, page_size_, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (JPP_UNLIKELY(addr == MAP_FAILED)) {
    LOG_ERROR() << "mmap call failed, could not allocate memory, probably out "
                   "of memory,"
                << " errcode=" << strerror(errno);
    throw std::bad_alloc();
  }
  auto page = MemoryPage{addr, page_size_};
  pages_.push_back(page);
  return page;
}

bool ManagedAllocatorCore::ensureAvailable(size_t size) {
  auto remaining = end_ - current_;
  if (remaining >= size) {
    return true;
  } else {
    MemoryPage page = mgr_->newPage();
    assert(page.size > size &&
           "page size is lesser than object size, please increase page size");
    current_ = reinterpret_cast<char*>(page.base);
    end_ = current_ + page.size;
    return true;
  }
}

Manager::~Manager() {
  for (auto page : pages_) {
    auto ret = ::munmap(page.base, page.size);
    if (ret == -1) {
      LOG_WARN() << "error when unmapping memory: errno=" << strerror(errno);
    }
  }
}

#endif  // JUMANPP_USE_DEFAULT_ALLOCATION
}
}
}