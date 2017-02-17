//
// Created by Arseny Tolmachev on 2017/02/15.
//

#ifndef JUMANPP_MMAP_HPP
#define JUMANPP_MMAP_HPP

#include <stdint.h>
#include <string>

#include "status.hpp"

namespace jumanpp {

namespace util {

enum class MMapType { ReadOnly, ReadWrite };

class mmap_view {
  void* address_;
  size_t size_;

 public:
  mmap_view();
  ~mmap_view();

  void* address() { return address_; }
  size_t size() { return size_; }
  bool clean();

  friend class mmap_file;
};

class mmap_file {
  int fd_ = 0;
  std::string filename_;
  MMapType type_;

 public:
  Status open(const std::string& filename, MMapType type);
  Status map(mmap_view* view, size_t offset, size_t size);

  ~mmap_file();
};
}
}

#endif  // JUMANPP_MMAP_HPP
