//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include "memory.hpp"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "logging.hpp"

#if defined(_WIN32_WINNT)
#include <malloc.h>
#include <errno.h>
#else
#include <sys/mman.h>
#endif

namespace jumanpp {
namespace util {
namespace memory {

#if defined(_WIN32_WINNT)
/**
 * posix_memalign replacement for Windows platform.
 *
 * Uses _aligned_alloc underhood and due to that memory cannot be freed using normal free() call
 */
static int posix_memalign(void **dst, size_t alignment, size_t size) {
  void *result = _aligned_malloc(size, alignment);

  if (result == nullptr) {
    return errno;
  }

  *dst = result;
  return 0;
}
#define free_impl _aligned_free
#else
#define free_impl free
#endif

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
    free_impl(obj.base);
  }
}

void Manager::reset() {
  for (auto &page : pages_) {
    free_impl(page.base);
  }
  pages_.clear();
}

bool Manager::supportHugePages() {
  return false;
}

#else
void* PoolAlloc::allocate_memory(size_t size, size_t alignment) {
  auto address = Align(offset_, alignment);
  auto objEnd = address + size;
  if (JPP_UNLIKELY(objEnd > end_)) {
    switchToNewPage(size);
    return allocate_memory(size, alignment);
  }
  offset_ = objEnd;
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

bool PoolAlloc::switchToNewPage(size_t size) {
  MemoryPage page = mgr_->newPage();
  if (page.size < size) {
    LOG_ERROR() << "page size (" << page.size << " is lesser than object size ("
                << size << "), please increase page size";
    throw new std::bad_alloc();
  }
  base_ = reinterpret_cast<char*>(page.base);
  offset_ = 0;
  end_ = page.size;
  return true;
}

void Manager::reset() { currentPage = 0; }

Manager::~Manager() {
  for (auto page : pages_) {
    free_impl(page.base);
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

void MallocEalloc::Reclaim(void *pVoid) noexcept { free_impl(pVoid); }

MallocEalloc *defaultEalloc() {
  static MallocEalloc alloc;
  return &alloc;
}
}
}  // namespace util
}  // namespace jumanpp
