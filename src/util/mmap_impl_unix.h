#ifndef JUMANPP_MMAP_IMPL_UNIX_HPP
#define JUMANPP_MMAP_IMPL_UNIX_HPP

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace jumanpp {
namespace util {

MappedFile::MappedFile(MappedFile &&o) noexcept
    : fd_{o.fd_},
      filename_{std::move(o.filename_)},
      type_{o.type_},
      size_{o.size_} {
  o.fd_ = 0;
}

MappedFile &MappedFile::operator=(MappedFile &&o) noexcept {
  if (this->fd_ != 0) {
    ::close(fd_);
  }
  fd_ = o.fd_;
  filename_ = std::move(o.filename_);
  type_ = o.type_;
  size_ = o.size_;
  o.fd_ = 0;
  return *this;
}

MappedFile::~MappedFile() {
  if (this->fd_ != 0) {
    ::close(fd_);
  }
}

struct PageSizeInfo {
  size_t normalPage = 4096;
  size_t largePage = size_t(1U << 21U);  // 2MB

  PageSizeInfo() {
#ifdef _SC_PAGESIZE
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize != -1) {
      normalPage = pageSize;
    }
#endif  // _SC_PAGESIZE
  }
};

Status MappedFile::open(StringPiece filename, MMapType type) {
  if (fd_ != 0) {
    return Status::InvalidState()
           << "mmap has already opened file " << this->filename_;
  }

  this->filename_ = filename.str();
  this->type_ = type;

  struct stat statResult;

  int code = stat(filename_.c_str(), &statResult);
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

  // use mode 0644 for file creation
  int fd = ::open(filename_.c_str(), file_mode, 0644);

  if (fd == -1) {
    return Status::InvalidState() << "mmap could not open file: " << filename
                                  << " error code = " << strerror(errno);
  }

  this->fd_ = fd;

  return Status::Ok();
}

const char ZERO[] = {'\0'};

Status MappedFile::map(MappedFileFragment *view, size_t offset, size_t size,
                       bool useHuge) {
  int protection = 0;
  int flags = 0;

  auto endoffset = offset + size;

  static PageSizeInfo psInfo{};
  size_t offsetPsAligned = (offset / psInfo.normalPage) * psInfo.normalPage;
  size_t sizePsAligned = endoffset - offsetPsAligned;
  size_t offsetFromStart = offset - offsetPsAligned;

  if (!view->isClean()) {
    return JPPS_INVALID_STATE << "fragment for the file " << filename_
                              << " is already mapped";
  }

  switch (type_) {
    case MMapType::ReadOnly:
      if (endoffset > size_) {
        return JPPS_INVALID_PARAMETER
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
          return JPPS_INVALID_STATE << "[seek] could not extend file "
                                    << filename_ << " to " << endoffset
                                    << "bytes";
        }
        auto write_ret = ::write(fd_, ZERO, 1);
        if (write_ret == -1) {
          return JPPS_INVALID_STATE << "[write] could not extend file "
                                    << filename_ << " to " << endoffset
                                    << "bytes";
        }
      }
      protection = PROT_READ | PROT_WRITE;
      flags = MAP_SHARED;
      break;
  }

  void *addr = MAP_FAILED;
  const size_t TWO_MEGS = (1 << 21);
  if (useHuge && size >= TWO_MEGS) {
    if (!util::memory::IsAligned(offset, TWO_MEGS)) {
      return JPPS_INVALID_PARAMETER << "offset is not aligned on 2MB boundary";
    }
#if defined(MAP_HUGETLB)
    int flags2 = flags;
    if (size >= TWO_MEGS) {
      flags2 |= MAP_HUGETLB;
    }
    addr = ::mmap(NULL, sizePsAligned, protection, flags2, this->fd_,
                  (off_t)offsetPsAligned);
#endif
  }
  // MAP_HUGETLB mmap call can fail if there are no resources,
  // try without MAP_HUGETLB
  if (addr == MAP_FAILED) {
    addr = ::mmap(NULL, sizePsAligned, protection, flags, this->fd_,
                  (off_t)offsetPsAligned);
  }

  if (addr == MAP_FAILED) {
    return JPPS_INVALID_STATE << "mmap call failed: error code = "
                              << strerror(errno);
  }

  view->baseAddress_ = addr;
  view->alignedSize_ = sizePsAligned;
  view->address_ = static_cast<char *>(addr) + offsetFromStart;
  view->size_ = size;

  return Status::Ok();
}

void MappedFile::close() {
  if (fd_ != 0) {
    ::close(fd_);
    fd_ = 0;
    filename_.clear();
    size_ = 0;
  }
}

MappedFileFragment::MappedFileFragment() noexcept : address_{MAP_FAILED} {}

MappedFileFragment::~MappedFileFragment() {
  if (!isClean()) {
    this->unmap();
  }
}

void MappedFileFragment::unmap() {
  ::munmap(baseAddress_, alignedSize_);
  baseAddress_ = MAP_FAILED;
  address_ = MAP_FAILED;
}

bool MappedFileFragment::isClean() { return address_ == MAP_FAILED; }

Status MappedFileFragment::flush() {
  int status = ::msync(address_, size(), MS_SYNC);
  if (status != 0) {
    return Status::InvalidState()
           << "could not flush mapped contents, error: " << strerror(errno);
  }
  return Status::Ok();
}

StringPiece MappedFileFragment::asStringPiece() const {
  const auto *asChar = reinterpret_cast<StringPiece::pointer_t>(address_);
  return StringPiece(asChar, size_);
}

MappedFileFragment::MappedFileFragment(MappedFileFragment &&o) noexcept
    : address_{o.address_}, size_{o.size_} {
  o.address_ = MAP_FAILED;
}

MappedFileFragment &MappedFileFragment::operator=(
    MappedFileFragment &&o) noexcept {
  address_ = o.address_;
  size_ = o.size_;
  o.address_ = MAP_FAILED;
  return *this;
}

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_MMAP_IMPL_UNIX_HPP
