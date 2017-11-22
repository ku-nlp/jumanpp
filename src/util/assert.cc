//
// Created by Arseny Tolmachev on 2017/10/26.
//

#include <backward.hpp>
#include <exception>
#include <sstream>
#include "logging.hpp"
#include "util/common.hpp"

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

namespace jumanpp {
namespace util {
namespace asserts {

void die() {
#ifdef NDEBUG
  std::terminate();
#else
  __builtin_trap();
#endif
}

struct AssertData {
  std::stringstream ss;
  std::string value;
};

#ifdef __APPLE__

bool isDebuggerAttachedOsx() {
  int junk;
  int mib[4];
  struct kinfo_proc info;
  size_t size;

  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.

  info.kp_proc.p_flag = 0;

  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  // Call sysctl.

  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  assert(junk == 0);

  // We're being debugged if the P_TRACED flag is set.

  return ((info.kp_proc.p_flag & P_TRACED) != 0);
}

#endif  // __APPLE__

bool isDebuggerAttached() {
#ifdef __APPLE__
  return isDebuggerAttachedOsx();
#else
  return false;
#endif  // __APPLE__
}

void AssertBuilder::PrintStacktrace() {
  namespace b = backward;
  b::StackTrace st;
  st.load_here(32);
  b::TraceResolver tr;
  tr.load_stacktrace(st);

  data_->ss << "\n Backtrace:";

  for (int i = 2; i < st.size(); ++i) {
    auto t = tr.resolve(st[i]);
    data_->ss << "\n #" << i << " "
              << " at " << t.source.filename << ":" << t.source.line << " of "
              << t.source.function << " (" << t.addr << " in "
              << t.object_filename << ") " << t.object_function;
  }
}

void AssertBuilder::AddExpr(const char *expr) {
  data_->ss << "\nExpression: " << expr;
}

AssertBuilder::AssertBuilder(const char *file, int line) {
  data_.reset(new AssertData());
  data_->ss << "Assertion failed in " << file << ":" << line;
}

void AssertBuilder::AddArgImpl(size_t n, const char *name, Stringify value) {
  data_->ss << "\n    ";
  data_->ss.write(name, n - 1);
  data_->ss << " = ";
  value(data_->ss);
}

AssertException AssertBuilder::MakeException() {
  return AssertException{data_.release()};
}

AssertBuilder::~AssertBuilder() noexcept = default;

AssertException::~AssertException() noexcept = default;

const char *AssertException::what() const noexcept {
  data_->value = data_->ss.str();
  return data_->value.c_str();
}

AssertException::AssertException(AssertData *data) : data_{data} {}

void UnwindPrinter::DoPrint() const noexcept {
  std::cerr << "Capture [" << name_ << "] = ";
  data_(std::cerr) << " at " << func_ << " on " << file_ << ":" << line_
                   << "\n";
}
}  // namespace asserts
}  // namespace util
}  // namespace jumanpp