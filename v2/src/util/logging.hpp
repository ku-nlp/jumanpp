//
// Created by Arseny Tolmachev on 2017/02/17.
//

#ifndef JUMANPP_LOGGING_HPP
#define JUMANPP_LOGGING_HPP

#include <sstream>

#include "common.hpp"

namespace jumanpp {
namespace util {
namespace logging {

enum class Level { None, Error, Warning, Info, Debug };

extern thread_local Level CurrentLogLevel;

class WriteInDestructorLoggerImpl {
  std::stringstream data_;
  Level level_;

 public:
  WriteInDestructorLoggerImpl(const char *file, int line, Level lvl);

  ~WriteInDestructorLoggerImpl();

  template <typename T>
  WriteInDestructorLoggerImpl &operator<<(const T &obj) {
    if (level_ >= CurrentLogLevel) data_ << obj;
    return *this;
  }
};
}
}
}

#define JPP_LOG(level)                                                       \
  (::jumanpp::util::logging::WriteInDestructorLoggerImpl{__FILE__, __LINE__, \
                                                         level})
#define LOG_DEBUG() JPP_LOG(::jumanpp::util::logging::Level::Debug)
#define LOG_INFO() JPP_LOG(::jumanpp::util::logging::Level::Info)
#define LOG_WARN() JPP_LOG(::jumanpp::util::logging::Level::Warning)
#define LOG_ERROR() JPP_LOG(::jumanpp::util::logging::Level::Error)

#endif  // JUMANPP_LOGGING_HPP
