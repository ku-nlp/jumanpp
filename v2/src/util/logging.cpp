//
// Created by Arseny Tolmachev on 2017/02/17.
//

#include "logging.hpp"
#include <chrono>
#include <ctime>
#include <iostream>

namespace jumanpp {
namespace util {
namespace logging {

constexpr static auto localLogPath = "src/util/logging.hpp";
constexpr static auto fullLogPath = __FILE__;
constexpr static auto prefixLength = sizeof(fullLogPath) - sizeof(localLogPath);

WriteInDestructorLoggerImpl::~WriteInDestructorLoggerImpl() {
  if (level_ <= CurrentLogLevel) {
    auto time = std::chrono::system_clock::now();
    auto timet = std::chrono::system_clock::to_time_t(time);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch());
    auto milli_val = millis.count() % 1000;
    std::tm tm = {0};
    if (localtime_r(&timet, &tm) != nullptr) {
      char buffer[256];
      std::snprintf(buffer, 255, "%02d:%02d:%02d.%03d ", tm.tm_hour, tm.tm_min,
                    tm.tm_sec, (int)milli_val);
      std::cerr << buffer;
    }
    std::cerr << data_.str() << "\n";
  }
}

WriteInDestructorLoggerImpl::WriteInDestructorLoggerImpl(const char *file,
                                                         int line, Level lvl)
    : level_{lvl} {
  if (level_ > CurrentLogLevel) {
    return;
  }

  switch (lvl) {
    case Level::Debug:
      data_ << "D ";
      break;
    case Level::Info:
      data_ << "I ";
      break;
    case Level::Warning:
      data_ << "W ";
      break;
    case Level::Error:
      data_ << "E ";
      break;
    default:;
  }
  data_ << (file + prefixLength) << ":" << line << " ";
}

// Enable all logging by default
/*thread_local*/ Level CurrentLogLevel = Level::Debug;
}  // namespace logging
}  // namespace util
}  // namespace jumanpp
