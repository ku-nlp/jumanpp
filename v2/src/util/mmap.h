//
// Created by Arseny Tolmachev on 2017/02/15.
//

#ifndef JUMANPP_MMAP_HPP
#define JUMANPP_MMAP_HPP

#include <stdint.h>
#include <string>

#include "status.hpp"
#include "string_piece.h"

namespace jumanpp {

namespace util {

enum class MMapType { ReadOnly, ReadWrite };

class MappedFileFragment {
  void *address_;
  size_t size_;

 public:
  MappedFileFragment();
  ~MappedFileFragment();

  void *address() { return address_; }
  size_t size() { return size_; }
  bool isClean();
  Status flush();

  friend class MappedFile;

  StringPiece asStringPiece();
};

class MappedFile {
  int fd_ = 0;
  std::string filename_;
  MMapType type_;
  size_t size_ = 0;

 public:
  Status open(const StringPiece &filename, MMapType type);
  Status map(MappedFileFragment *view, size_t offset, size_t size);

  size_t size() const { return size_; }

  ~MappedFile();
};
}
}

#endif  // JUMANPP_MMAP_HPP
