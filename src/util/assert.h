//
// Created by Arseny Tolmachev on 2017/10/26.
//

#ifndef JUMANPP_ASSERT_H
#define JUMANPP_ASSERT_H

#include <cassert>
#include <functional>
#include <memory>
#include <ostream>
#include <utility>

#if defined(__GNUC__) || defined(__clang__)
#define JPP_NORETURN __attribute__((noreturn))
#else
#define JPP_NORETURN
#endif

#if defined(__clang__)
#define JPP_BREAK_TO_DEBUGGER_IF_ATTACHED() \
  if (::jumanpp::util::asserts::isDebuggerAttached()) __builtin_debugtrap()
#elif defined(_WIN32_WINNT)
#define JPP_BREAK_TO_DEBUGGER_IF_ATTACHED() __debugbreak()
#else
#define JPP_BREAK_TO_DEBUGGER_IF_ATTACHED() __builtin_trap()
#endif

namespace jumanpp {
namespace util {
namespace asserts {

bool isDebuggerAttached();

struct AssertData;

using Stringify = std::function<std::ostream&(std::ostream&)>;

template <typename T>
auto deduceStrArgs(std::ostream& o, const T& v) -> decltype(o << v);
void deduceStrArgs(...);

template <typename T, typename O = decltype(deduceStrArgs(
                          std::declval<std::ostream&>(),
                          std::declval<typename std::add_const<T>::type&>()))>
struct StringifyGate {
  using deduced = O;
  static constexpr bool enabled = std::is_same<std::ostream&, deduced>::value;
};

template <>
struct StringifyGate<std::nullptr_t> {
  static constexpr bool enabled = false;
};

template <typename T>
auto make_stringify(const T& v) ->
    typename std::enable_if<StringifyGate<T>::enabled, Stringify>::type {
  auto ref = std::cref(v);
  return Stringify{
      [ref](std::ostream& o) -> std::ostream& { return o << ref.get(); }};
}

class AssertException : public std::exception {
  std::shared_ptr<AssertData> data_;

 public:
  explicit AssertException(AssertData* data);
  ~AssertException() noexcept override;
  const char* what() const noexcept override;

  void doThrow() {
    if (isDebuggerAttached()) {
      std::fputs(what(), stderr);
      JPP_BREAK_TO_DEBUGGER_IF_ATTACHED();
    } else {
      throw *this;
    }
  }
};

class AssertBuilder {
  std::unique_ptr<AssertData> data_;

 public:
  // template <size_t N>
  // constexpr AssertBuilder(const char (&file)[N], int line) {}
  AssertBuilder(const char* file, int line);
  void AddExpr(const char* expr);
  void AddArgImpl(size_t n, const char* name, Stringify value);

  template <size_t N, typename T>
  auto AddArg(const char (&name)[N], const T& value) ->
      typename std::enable_if<StringifyGate<T>::enabled>::type {
    AddArgImpl(N, name, make_stringify(value));
  }

  // This overload has the lowest priority and should be selected
  // when it's impossible to convert an argument to a string
  template <size_t N, typename... Args>
  void AddArg(const char (&name)[N], const Args&... ignored) {}

  JPP_NO_INLINE void PrintStacktrace();

  AssertException MakeException();
  ~AssertBuilder() noexcept;
};

class UnwindPrinter {
  const char* name_;
  Stringify data_;
  const char* file_;
  const char* func_;
  int line_;

 public:
  UnwindPrinter(const char* name, Stringify data, const char* file,
                const char* func, int line)
      : name_{name},
        data_{std::move(data)},
        file_{file},
        func_{func},
        line_{line} {}
  ~UnwindPrinter() noexcept {
    if (JPP_UNLIKELY(std::uncaught_exception())) {
      DoPrint();
    }
  }

  void DoPrint() const noexcept;
};

JPP_NORETURN void die();

}  // namespace asserts
}  // namespace util
}  // namespace jumanpp

#define JPP_CONCAT(a, b) a##b

#define JPP_CAPTURE_NAME(line, ctr) JPP_CONCAT(__jpp_capture_, ctr)

#ifndef NDEBUG
#define JPP_DCHECK_IMPL1(expr) \
  JPP_UNLIKELY(!(expr)) ? \
  	[&]() JPP_NO_INLINE -> ::jumanpp::util::asserts::AssertException {\
  	::jumanpp::util::asserts::AssertBuilder bldr_{__FILE__, __LINE__}; \
    bldr_.AddExpr( #expr ); \
    bldr_.PrintStacktrace(); \
    return bldr_.MakeException(); \
  	}().doThrow() : (void)0;

#define JPP_DCHECK_IMPL2(expr, x, y) \
  JPP_UNLIKELY(!(expr)) ? \
  	[&]() JPP_NO_INLINE -> ::jumanpp::util::asserts::AssertException {\
  	::jumanpp::util::asserts::AssertBuilder bldr_{__FILE__, __LINE__}; \
    bldr_.AddExpr( #expr ); \
    bldr_.AddArg( #x, x ); \
    bldr_.AddArg( #y, y ); \
    bldr_.PrintStacktrace(); \
    return bldr_.MakeException(); \
  	}().doThrow() : (void)0;

#define JPP_DCHECK_IMPL3(expr, x, y, z) \
  JPP_UNLIKELY(!(expr)) ? \
  	[&]() JPP_NO_INLINE -> ::jumanpp::util::asserts::AssertException {\
  	::jumanpp::util::asserts::AssertBuilder bldr_{__FILE__, __LINE__}; \
    bldr_.AddExpr( #expr ); \
    bldr_.AddArg( #x, x ); \
    bldr_.AddArg( #y, y ); \
    bldr_.AddArg( #z, z ); \
    bldr_.PrintStacktrace(); \
    return bldr_.MakeException(); \
  	}().doThrow() : (void)0;

#define JPP_CAPTURE(x)                                                       \
  ::jumanpp::util::asserts::UnwindPrinter JPP_CAPTURE_NAME(__LINE__,         \
                                                           __COUNTER__) {    \
#x, ::jumanpp::util::asserts::make_stringify(x), __FILE__, __FUNCTION__, \
        __LINE__                                                             \
  }

#define JPP_INDEBUG(x) x
#else
#define JPP_DCHECK_IMPL1(x)
#define JPP_DCHECK_IMPL2(expr, x, y)
#define JPP_DCHECK_IMPL3(expr, x, y, z)
#define JPP_INDEBUG(x)
#define JPP_CAPTURE(x)
#endif

#define JPP_DCHECK(x) JPP_DCHECK_IMPL1(x)
#define JPP_DCHECK_NOT(x) JPP_DCHECK_IMPL1(!(x))
#define JPP_DCHECK_EQ(a, b) JPP_DCHECK_IMPL2((a) == (b), a, b)
#define JPP_DCHECK_NE(a, b) JPP_DCHECK_IMPL2((a) != (b), a, b)
#define JPP_DCHECK_GE(a, b) JPP_DCHECK_IMPL2((a) >= (b), a, b)
#define JPP_DCHECK_GT(a, b) JPP_DCHECK_IMPL2((a) > (b), a, b)
#define JPP_DCHECK_LE(a, b) JPP_DCHECK_IMPL2((a) <= (b), a, b)
#define JPP_DCHECK_LT(a, b) JPP_DCHECK_IMPL2((a) < (b), a, b)
#define JPP_DCHECK_IN(val, low, high) \
  JPP_DCHECK_IMPL3(((low) <= (val)) && ((val) < (high)), val, low, high)

#endif  // JUMANPP_ASSERT_H
