//
// Created by Arseny Tolmachev on 2017/02/17.
//

#include "logging.hpp"
#include <iostream>

namespace jumanpp {
namespace util {
namespace logging {

WriteInDestructorLoggerImpl::~WriteInDestructorLoggerImpl() {
  std::cerr << data_.str() << "\n";
}

WriteInDestructorLoggerImpl::WriteInDestructorLoggerImpl(const char *file,
                                                         int line, Level lvl)
    : level_{lvl} {
  switch (lvl) {
    case Level::Debug:
      data_ << "D ";
      break;
    default:;
  }
  data_ << file << ":" << line << " ";
}

// Enable all logging by default
/*thread_local*/ Level CurrentLogLevel = Level::Debug;
}
}
}
