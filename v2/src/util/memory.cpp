//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include "memory.hpp"
#include <sys/mman.h>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include "logging.hpp"

namespace jumanpp {
namespace util {
namespace memory {

#if JUMANPP_USE_DEFAULT_ALLOCATION
void *ManagedAllocatorCore::allocate_memory(size_t size, size_t alignment) {
  assert(IsPowerOf2(alignment) && "alignment should be a power of 2");
  return mgr_->allocate(size, std::max<size_t>(alignment, 8));
}

void *Manager::allocate(size_t size, size_t align) {
  void *ptr = nullptr;
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

void Manager::reset() {
  for (auto &page : pages_) {
    free(page.base);
  }
  pages_.clear();
}

#else
void* ManagedAllocatorCore::allocate_memory(size_t size, size_t alignment) {
  auto address = Align(offset_, alignment);
  auto end = address + size;
  auto requiredSize = end - offset_;
  if (!JPP_LIKELY(ensureAvailable(requiredSize))) {
    return allocate_memory(size, alignment);
  }

  offset_ += requiredSize;
  // LOG_DEBUG() << "allocated " << requiredSize << " bytes";
  return base_ + address;
}

bool clearPage(void* begin, size_t size) {
  std::memset(begin, 0xdeadbeef, size);
  return true;
}

MemoryPage Manager::newPage() {
  if (currentPage < pages_.size()) {
    auto& page = pages_[currentPage];
    currentPage += 1;
    JPP_DCHECK(clearPage(page.base, page.size));
    return page;
  } else {
    void* addr = ::mmap(NULL, page_size_, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE, -1, 0);
    if (JPP_UNLIKELY(addr == MAP_FAILED)) {
      LOG_ERROR()
          << "mmap call failed, could not allocate memory, probably out "
             "of memory,"
          << " errcode=" << strerror(errno);
      throw std::bad_alloc();
    }
    auto page = MemoryPage{addr, page_size_};
    JPP_DCHECK(clearPage(page.base, page.size));
    pages_.push_back(page);
    currentPage += 1;
    return page;
  }
}

bool ManagedAllocatorCore::ensureAvailable(size_t size) {
  auto remaining = end_ - offset_;
  if (remaining >= size) {
    return true;
  } else {
    MemoryPage page = mgr_->newPage();
    assert(page.size > size &&
           "page size is lesser than object size, please increase page size");
    base_ = reinterpret_cast<char*>(page.base);
    offset_ = 0;
    end_ = page.size;
    return false;
  }
}

void Manager::reset() { currentPage = 0; }

Manager::~Manager() {
  for (auto page : pages_) {
    auto ret = ::munmap(page.base, page.size);
    if (ret == -1) {
      LOG_WARN() << "error when unmapping memory: errno=" << strerror(errno);
    }
  }
}

#endif  // JUMANPP_USE_DEFAULT_ALLOCATION

void ManagedAllocatorCore::reset() {
  base_ = nullptr;
  offset_ = 0;
  end_ = 0;
}
}
}
}