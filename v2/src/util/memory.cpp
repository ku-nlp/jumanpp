//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include "memory.hpp"
#include <sys/mman.h>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "logging.hpp"

namespace jumanpp {
namespace util {
namespace memory {

#if JUMANPP_USE_DEFAULT_ALLOCATION
void *PoolAlloc::allocate_memory(size_t size, size_t alignment) {
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
void* PoolAlloc::allocate_memory(size_t size, size_t alignment) {
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

constexpr size_t TWO_MEGS = 2 * 1024 * 1024;

MemoryPage Manager::newPage() {
  if (currentPage < pages_.size()) {
    auto& page = pages_[currentPage];
    currentPage += 1;
    JPP_DCHECK(clearPage(page.base, page.size));
    return page;
  } else {
    void* addr;
    if (page_size_ >= TWO_MEGS) {
      int status = posix_memalign(&addr, TWO_MEGS, page_size_);
      if (status != 0) {
        LOG_ERROR() << "Error when trying to get memory: " << strerror(status);
        throw std::bad_alloc();
      }
#ifdef MADV_HUGEPAGE
      status = madvise(addr, page_size_, MADV_HUGEPAGE);
      if (status == -1) {
        LOG_WARN() << "madvise failed: " << strerror(status);
      }
#endif
    } else {
      int status = posix_memalign(&addr, 4096, page_size_);
      if (status != 0) {
        LOG_ERROR() << "Error when trying to get memory: " << strerror(status);
        throw std::bad_alloc();
      }
    }
    auto page = MemoryPage{addr, page_size_};
    JPP_DCHECK(clearPage(page.base, page.size));
    pages_.push_back(page);
    currentPage += 1;
    return page;
  }
}

bool PoolAlloc::ensureAvailable(size_t size) {
  auto remaining = end_ - offset_;
  if (remaining >= size) {
    return true;
  } else {
    MemoryPage page = mgr_->newPage();
    if (page.size < size) {
      LOG_ERROR() << "page size (" << page.size
                  << " is lesser than object size (" << size
                  << "), please increase page size";
      throw new std::bad_alloc();
    }
    base_ = reinterpret_cast<char*>(page.base);
    offset_ = 0;
    end_ = page.size;
    return false;
  }
}

void Manager::reset() { currentPage = 0; }

Manager::~Manager() {
  for (auto page : pages_) {
    std::free(page.base);
  }
}

bool Manager::supportHugePages() {
#if defined(MADV_HUGEPAGE) || defined(MAP_HUGETLB)
  return true;
#else
  return false;
#endif
}

#endif  // JUMANPP_USE_DEFAULT_ALLOCATION

void PoolAlloc::reset() {
  base_ = nullptr;
  offset_ = 0;
  end_ = 0;
}

void *MallocEalloc::Allocate(size_t size, size_t align) {
  void *result;
  auto maxAlign = std::max<size_t>(align, 8);
  int error = posix_memalign(&result, maxAlign, size);
  if (error != 0) {
    LOG_ERROR() << "posix_memalign failed, errno=" << strerror(error);
    throw std::bad_alloc();
  }
  return result;
}

void MallocEalloc::Reclaim(void *pVoid) noexcept { free(pVoid); }

MallocEalloc *defaultEalloc() {
  static MallocEalloc alloc;
  return &alloc;
}
}
}  // namespace util
}  // namespace jumanpp