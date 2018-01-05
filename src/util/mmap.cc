//
// Created by Arseny Tolmachev on 2017/02/15.
//

#include <errno.h>
#include <string.h>

#include "logging.hpp"
#include "mmap.h"
#include "util/memory.hpp"

#if defined(_MSC_VER)
#include "mmap_impl_win32.h"
#else
#include "mmap_impl_unix.h"
#endif

namespace jumanpp {
namespace util {

Status FullyMappedFile::open(StringPiece filename, MMapType type) {
  if (!fragment_.isClean()) {
    if (file_.name() == filename) {
      // opening the same file
      return Status::Ok();
    }
    this->close();
  }
  JPP_RETURN_IF_ERROR(file_.open(filename, type));
  JPP_RETURN_IF_ERROR(file_.map(&fragment_, 0, file_.size()));
  return Status::Ok();
}

void FullyMappedFile::close() {
  fragment_.unmap();
  file_.close();
}

}  // namespace util
}  // namespace jumanpp
