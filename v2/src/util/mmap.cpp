//
// Created by Arseny Tolmachev on 2017/02/15.
//

#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
    auto errn = errno;
    if (type == MMapType::ReadWrite && errn == ENOENT) {
      // ok, we have no file, it will be created
      this->size_ = 0;
    } else {
      return Status::InvalidState()
             << "could not get information about file: " << filename
             << " errcode=" << strerror(errn);
    }
  } else {
    this->size_ = (size_t)statResult.st_size;
  }

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

  //use mode 0644 for file creation
  int fd = ::open(filename.c_str(), file_mode, 0644);

  if (fd == 0) {
    return Status::InvalidState() << "mmap could not open file: " << filename
                                  << " error code = " << strerror(errno);
  }

  this->fd_ = fd;

  return Status::Ok();
}

char ZERO[] = {'\0'};

Status mmap_file::map(mmap_view *view, size_t offset, size_t size) {
  int protection = 0;
  int flags = 0;

  auto endoffset = offset + size;

  switch (type_) {
    case MMapType::ReadOnly:
      if (endoffset > size_) {
        return Status::InvalidParameter()
               << "offset=" << offset << " and size=" << size
               << " point after the end of file " << filename_
               << " of size=" << size_;
      }
      protection = PROT_READ;
      flags = MAP_PRIVATE;
      break;
    case MMapType::ReadWrite:
      if (endoffset > size_) {
        off_t fileoffset = static_cast<off_t>(endoffset) - 1;
        auto retval = ::lseek(fd_, fileoffset, SEEK_SET);
        if (retval == -1) {
          return Status::InvalidState() << "[seek] could not extend file "
                                        << filename_ << " to " << endoffset
                                        << "bytes";
        }
        auto write_ret = write(fd_, ZERO, 1);
        if (write_ret == -1) {
          return Status::InvalidState() << "[write] could not extend file "
                                        << filename_ << " to " << endoffset
                                        << "bytes";
        }
      }
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
  if (!isClean()) {
    ::munmap(address_, size_);
  }
}

bool mmap_view::isClean() { return address_ == MAP_FAILED; }

Status mmap_view::flush() {
  int status = ::msync(address_, size(), MS_SYNC);
  if (status != 0) {
    return Status::InvalidState() << "could not flush mapped contents, error: "
                                  << strerror(errno);
  }
  return Status::Ok();
}
}
}