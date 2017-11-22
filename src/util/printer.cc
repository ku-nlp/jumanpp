//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "printer.h"

namespace jumanpp {
namespace util {
namespace io {

StringPiece impl::PrinterBuffer::contents() const {
  return StringPiece{pbase(), pptr()};
}

void impl::PrinterBuffer::reset() { setp(pbase(), epptr()); }
}  // namespace io
}  // namespace util
}  // namespace jumanpp