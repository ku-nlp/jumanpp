//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STANDALONE_TEST_H
#define JUMANPP_STANDALONE_TEST_H

#include <unistd.h>
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

  virtual std::string toStringUncached() const override { return ""; }
};

class TempFile {
  std::string filename_;

 public:
  TempFile() {
    char buffer[L_tmpnam];
// tmpnam is a security risk, but we use this for unit tests!
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
    std::tmpnam(buffer);
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
    filename_.assign(buffer);
  }

  bool isOk() const { return filename_.size() > 0; }

  const std::string &name() const { return filename_; }

  ~TempFile() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      WARN("temporary file name is: " << filename_);
    } else {
      // delete file
      unlink(filename_.c_str());
    }
  }
};

#define CHECK_OK(x) CHECK_THAT(x, OkStatusMatcher())
#define REQUIRE_OK(x) REQUIRE_THAT(x, OkStatusMatcher())

#endif  // JUMANPP_STANDALONE_TEST_H
