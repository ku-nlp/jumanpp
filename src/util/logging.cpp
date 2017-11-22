//
// Created by Arseny Tolmachev on 2017/02/17.
//

#include "logging.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>

namespace jumanpp {
namespace util {
namespace logging {

template <size_t S1, size_t S2>
constexpr size_t computeLength(const char (&v1)[S1], const char (&v2)[S2]) {
  return S2 - S1;
};

static constexpr size_t prefixLength =
    computeLength("src/util/logging.hpp", __FILE__);

std::mutex log_mutex_;

WriteInDestructorLoggerImpl::~WriteInDestructorLoggerImpl() {
  if (level_ <= CurrentLogLevel) {
    data_ << '\n';
    auto str = data_.str();
    std::lock_guard<std::mutex> guard{log_mutex_};
    std::cerr.write(str.data(), str.size());
  }
}

WriteInDestructorLoggerImpl::WriteInDestructorLoggerImpl(const char *file,
                                                         int line, Level lvl)
    : level_{lvl} {
  if (level_ > CurrentLogLevel) {
    return;
  }

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
    data_ << buffer;
  }

  switch (lvl) {
    case Level::Trace:
      data_ << "T ";
      break;
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
  auto shifted = file + prefixLength;
  data_ << shifted << ":" << line << " ";
}

// Enable all logging by default
/*thread_local*/ Level CurrentLogLevel = Level::Trace;
}  // namespace logging
}  // namespace util
}  // namespace jumanpp
