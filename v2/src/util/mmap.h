//
// Created by Arseny Tolmachev on 2017/02/15.
//

#ifndef JUMANPP_MMAP_HPP
#define JUMANPP_MMAP_HPP

#include <stdint.h>
#include <string>

#include "array_slice.h"
#include "status.hpp"
#include "string_piece.h"

namespace jumanpp {

namespace util {

enum class MMapType { ReadOnly, ReadWrite };

class MappedFileFragment {
  void *address_;
  size_t size_;

  MappedFileFragment(const MappedFileFragment &) = delete;
  MappedFileFragment &operator=(const MappedFileFragment &) = delete;

 public:
  MappedFileFragment() noexcept;
  MappedFileFragment(MappedFileFragment &&o) noexcept;
  MappedFileFragment &operator=(MappedFileFragment &&o) noexcept;
  ~MappedFileFragment();

  void *address() { return address_; }
  size_t size() { return size_; }
  bool isClean();
  Status flush();

  friend class MappedFile;

  StringPiece asStringPiece();
  util::MutableArraySlice<char> asMutableSlice() {
    return util::MutableArraySlice<char>{reinterpret_cast<char *>(address()),
                                         size()};
  }
};

class MappedFile {
  int fd_ = 0;
  std::string filename_;
  MMapType type_;
  size_t size_ = 0;

  MappedFile(const MappedFile &) = delete;
  MappedFile &operator=(const MappedFile &) = delete;

 public:
  MappedFile() noexcept {}
  MappedFile(MappedFile &&o) noexcept;
  MappedFile &operator=(MappedFile &&o) noexcept;
  Status open(const StringPiece &filename, MMapType type);
  Status map(MappedFileFragment *view, size_t offset, size_t size);

  size_t size() const { return size_; }

  ~MappedFile();
};
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_MMAP_HPP
