//
// Created by Arseny Tolmachev on 2017/02/15.
//

#include <errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "mmap.hpp"

namespace jumanpp {

namespace util {

mmap_file::~mmap_file() {
  if (this->fd_ != 0) {
    ::close(fd_);
  }
}

Status mmap_file::open(const std::string &filename, MMapType type) {
  if (fd_ != 0) {
    return Status::InvalidState() << "mmap has already opened file "
                                  << this->filename_;
  }

  this->filename_ = filename;
  this->type_ = type;

  struct stat statResult;

  int code = stat(filename.c_str(), &statResult);
  if (code != 0) {
    return Status::InvalidState() << "could not get information about file: " << filename << " errcode=" << strerror(errno);
  }

  this->size_ = (size_t) statResult.st_size;

  int file_mode = 0;

  switch (type) {
    case MMapType::ReadOnly:
      file_mode |= O_RDONLY;
      break;
    case MMapType::ReadWrite:
      file_mode |= O_RDWR;
      file_mode |= O_CREAT;
      break;
  }

  int fd = ::open(filename.c_str(), file_mode);

  if (fd == 0) {
    return Status::InvalidState() << "mmap could not open file: " << filename
                                  << " error code = " << strerror(errno);
  }

  this->fd_ = fd;

  return Status::Ok();
}

Status mmap_file::map(mmap_view *view, size_t offset, size_t size) {
  int protection = 0;
  int flags = 0;

  switch (type_) {
    case MMapType::ReadOnly:
      protection = PROT_READ;
      flags = MAP_PRIVATE;
      break;
    case MMapType::ReadWrite:
      protection = PROT_READ | PROT_WRITE;
      flags = MAP_SHARED;
      break;
  }

  void *addr = ::mmap(NULL, size, protection, flags, this->fd_, (off_t)offset);
  if (addr == MAP_FAILED) {
    return Status::InvalidState() << "mmap call failed: error code = "
                                  << strerror(errno);
  }

  view->address_ = addr;
  view->size_ = size;

  return Status::Ok();
}

mmap_view::mmap_view() : address_{MAP_FAILED} {}

mmap_view::~mmap_view() {
  if (!clean()) {
    ::munmap(address_, size_);
  }
}

bool mmap_view::clean() { return address_ == MAP_FAILED; }
}
}