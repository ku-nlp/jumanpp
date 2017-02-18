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
  void *address_;
  size_t size_;

 public:
  mmap_view();
  ~mmap_view();

  void *address() { return address_; }
  size_t size() { return size_; }
  bool isClean();
  Status flush();

  friend class mmap_file;
};

class mmap_file {
  int fd_ = 0;
  std::string filename_;
  MMapType type_;
  size_t size_ = 0;

 public:
  Status open(const std::string &filename, MMapType type);
  Status map(mmap_view *view, size_t offset, size_t size);

  size_t size() const { return size_; }

  ~mmap_file();
};
}
}

#endif  // JUMANPP_MMAP_HPP
