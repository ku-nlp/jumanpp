#ifndef JUMANPP_MMAP_IMPL_WIN32_HPP
#define JUMANPP_MMAP_IMPL_WIN32_HPP

#if defined(_WIN32_WINNT)

#include "win32_utils.h"

namespace jumanpp {
namespace util {

MappedFile::MappedFile(MappedFile &&o) noexcept
    : fd_{o.fd_},
      filename_{std::move(o.filename_)},
      type_{o.type_},
      size_{o.size_} {
  o.fd_ = nullptr;
}

MappedFile &MappedFile::operator=(MappedFile &&o) noexcept {
  if (this->fd_ != nullptr) {
    CloseHandle(this->fd_);
  }
  fd_ = o.fd_;
  filename_ = std::move(o.filename_);
  type_ = o.type_;
  size_ = o.size_;
  o.fd_ = nullptr;
  return *this;
}

MappedFile::~MappedFile() {
  if (this->fd_ != nullptr) {
    CloseHandle(this->fd_);
  }
}

Status MappedFile::open(StringPiece filename, MMapType type) {
  if (fd_ != nullptr) {
    return JPPS_INVALID_STATE << "mmap has already opened file "
                              << this->filename_;
  }

  this->filename_ = filename.str();
  this->type_ = type;

  DWORD access_mode = 0;
  DWORD open_mode = 0;

  switch (type) {
    case MMapType::ReadOnly:
      access_mode |= GENERIC_READ;
      open_mode = OPEN_EXISTING;
      break;
    case MMapType::ReadWrite:
      access_mode |= GENERIC_READ;
      access_mode |= GENERIC_WRITE;
      open_mode = CREATE_ALWAYS;
      break;
  }

  auto fd = CreateFileW(to_wide_string(filename).c_str(), access_mode,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, open_mode, FILE_ATTRIBUTE_NORMAL, NULL);

  if (fd == INVALID_HANDLE_VALUE) {
    return JPPS_INVALID_STATE << "could not access file: " << filename
                              << " errcode=" << GetLastError();
  } else {
    LARGE_INTEGER size;
    if (GetFileSizeEx(fd, &size)) {
      // size.QuadPart is int64
      this->size_ = static_cast<size_t>(size.QuadPart);
    } else {
      return JPPS_INVALID_STATE << "unable to get size of file: " << filename
                                << " errcode=" << GetLastError();
    }
  }

  if (type == MMapType::ReadWrite) {
    FILETIME now;
    ::GetSystemTimeAsFileTime(&now);
    ::SetFileTime(fd,
                  &now,
                  &now,
                  &now);
  }

  this->fd_ = fd;

  return Status::Ok();
}

inline static DWORD get_system_granuality() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwAllocationGranularity;
}

inline static bool set_file_len(void *handle, size_t size) {
  LARGE_INTEGER info_size;
  info_size.QuadPart = static_cast<__int64>(size);
  FILE_END_OF_FILE_INFO info = {info_size};

  // Makes minimum supported platform Windows Vista
  return static_cast<bool>(SetFileInformationByHandle(handle, FileEndOfFileInfo,
                                                      &info, sizeof(info)));
}

Status MappedFile::map(MappedFileFragment *view, size_t offset, size_t size,
                       bool useHuge) {
  if (!view->isClean()) {
    return JPPS_INVALID_STATE << "fragment for the file " << filename_
                              << " is already mapped";
  }

  DWORD protection;
  DWORD map_access;
  const auto file_end = offset + size;

  switch (type_) {
    case MMapType::ReadOnly:
      if (file_end > size_) {
        return JPPS_INVALID_PARAMETER
               << "offset=" << offset << " and size=" << size
               << " point after the end of file " << filename_
               << " of size=" << size_;
      }
      map_access = FILE_MAP_READ;
      protection = PAGE_READONLY;
      break;
    case MMapType::ReadWrite:
      if (file_end > size_) {
        if (!set_file_len(this->fd_, file_end)) {
          return JPPS_INVALID_STATE << "could not extend file " << filename_
                                    << " to " << file_end << "bytes";
        }
        size_ = std::max(file_end, size_);
      }
      map_access = FILE_MAP_WRITE;  // read/write access
      protection = PAGE_READWRITE;
      break;
  }

  const auto mapping =
      CreateFileMappingW(this->fd_, nullptr, protection, 0, 0, nullptr);
  if (mapping == NULL) {
    return JPPS_INVALID_STATE << "Cannot open file mapping for " << filename_
                              << ". Error: " << GetLastError();
  }

  DWORD granuality = get_system_granuality();

  uint64_t alignment = offset % static_cast<uint64_t>(granuality);
  uint64_t aligned_offset = offset - alignment;
  uint64_t aligned_len = size + alignment;

  auto map_view = MapViewOfFile(
      mapping, map_access, static_cast<DWORD>(aligned_offset >> 32),
      static_cast<DWORD>(aligned_offset & 0xffffffff), aligned_len);
  CloseHandle(mapping);

  if (map_view == NULL) {
    return JPPS_INVALID_STATE << "Cannot open mapping view for " << filename_
                              << ". Error: " << GetLastError();
  }

  view->address_ = static_cast<char *>(map_view) + alignment;
  view->size_ = size;

  return Status::Ok();
}

void MappedFile::close() {
  if (fd_ != nullptr) {
    CloseHandle(fd_);
    fd_ = nullptr;
    filename_.clear();
    size_ = 0;
  }
}

MappedFileFragment::MappedFileFragment() noexcept : address_{nullptr} {}

MappedFileFragment::~MappedFileFragment() {
  if (!isClean()) {
    this->unmap();
  }
}

void MappedFileFragment::unmap() {
  const size_t alignment = reinterpret_cast<size_t>(this->address_) %
                           static_cast<size_t>(get_system_granuality());
  const void *ptr = static_cast<char *>(this->address_) - alignment;
  UnmapViewOfFile(ptr);
  address_ = nullptr;
}

bool MappedFileFragment::isClean() { return address_ == nullptr; }

Status MappedFileFragment::flush() {
  if (FlushViewOfFile(this->address_, 0) == 0) {
    return Status::InvalidState()
           << "could not flush mapped contents, error: " << GetLastError();
  }
  return Status::Ok();
}
StringPiece MappedFileFragment::asStringPiece() const {
  StringPiece::pointer_t asChar =
      reinterpret_cast<StringPiece::pointer_t>(address_);
  return StringPiece(asChar, size_);
}

MappedFileFragment::MappedFileFragment(MappedFileFragment &&o) noexcept
    : address_{o.address_}, size_{o.size_} {
  o.address_ = nullptr;
}

MappedFileFragment &MappedFileFragment::operator=(
    MappedFileFragment &&o) noexcept {
  address_ = o.address_;
  size_ = o.size_;
  o.address_ = nullptr;
  return *this;
}

}  // namespace util
}  // namespace jumanpp

#endif  // _MSC_VER

#endif  // JUMANPP_MMAP_IMPL_WIN32_HPP
