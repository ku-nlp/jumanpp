//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STANDALONE_TEST_H
#define JUMANPP_STANDALONE_TEST_H

#ifndef _WIN32_WINNT
#include <unistd.h>
#else
#include <util/win32_utils.h>
#undef DELETE  // because FUCK YOU, WINDOWS
#endif

#include <util/string_piece.h>
#include <catch.hpp>
#include <util/status.hpp>

namespace Catch {
template <>
struct StringMaker<jumanpp::Status> {
  static std::string convert(jumanpp::Status const &value) {
    std::stringstream s;
    s << value;
    return s.str();
  }
};

template <>
struct StringMaker<jumanpp::StringPiece> {
  static std::string convert(jumanpp::StringPiece const &value) {
    std::stringstream s;
    s << '"' << value << '"';
    return s.str();
  }
};
}  // namespace Catch

class OkStatusMatcher
    : public Catch::Matchers::Impl::MatcherBase<jumanpp::Status> {
 public:
  OkStatusMatcher(OkStatusMatcher const &) = default;
  OkStatusMatcher() = default;

  virtual bool match(jumanpp::Status const &status) const override {
    return status.isOk();
  }

 protected:
  std::string describe() const override { return "Status == Ok"; }
};

class TempFile {
  std::string filename_;

 public:
#ifdef _WIN32_WINNT
  TempFile() {
    wchar_t tmpDirPath[512];
    wchar_t prefix[] = L"jpp";
    auto length = GetTempPathW(512, tmpDirPath);
    if (length == 0) {
      throw std::runtime_error("failed to get a temporary directory");
    }

    wchar_t tmpPath[512];
    auto result = GetTempFileNameW(tmpDirPath, prefix, 0, tmpPath);
    if (result == 0) {
      throw std::runtime_error("GetTempFileName call failed");
    }

    auto tmpBufLength =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmpPath, -1, nullptr,
                            0, nullptr, nullptr);

    if (tmpBufLength == 0) {
      throw std::runtime_error(
          "failed to get buffer length from WideCharToMultiByte");
    }

    filename_.resize(tmpBufLength, 0);
    auto convResult =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmpPath, -1,
                            &filename_[0], tmpBufLength, nullptr, nullptr);

    if (convResult == 0) {
      throw std::runtime_error(
          "failed to convert buffer with WideCharToMultiByte");
    }
  }

  void delete_file(const char* name) {
    _unlink(name);
  }
#else
  TempFile() {
    char buffer[L_tmpnam];
// tmpnam is a security risk, but we use this ONLY for unit tests!
// we actually need the access to file name and don't want to create
// the file beforehand, or open it exclusively
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    auto res = std::tmpnam(buffer);
    CHECK(res);
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    filename_.assign(buffer);
  }

  void delete_file(const char* name) {
    unlink(name);
  }
#endif  //_WIN32_WINNT

  bool isOk() const { return !filename_.empty(); }

  const std::string &name() const { return filename_; }

  ~TempFile() {
    // delete file
    delete_file(filename_.c_str());
  }
};

#define CHECK_OK(x) CHECK_THAT(x, OkStatusMatcher())
#define REQUIRE_OK(x) REQUIRE_THAT(x, OkStatusMatcher())

#endif  // JUMANPP_STANDALONE_TEST_H
