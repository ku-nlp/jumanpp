//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_STANDALONE_TEST_H
#define JUMANPP_STANDALONE_TEST_H

#include <catch.hpp>
#include <util/status.hpp>

namespace Catch {
  template <> struct StringMaker<jumanpp::Status> {
    static std::string convert(jumanpp::Status const& value) {
      std::stringstream s;
      s << value;
      return s.str();
    }
  };
}

class OkStatusMatcher : public Catch::Matchers::Impl::MatcherBase<jumanpp::Status> {
public:
  OkStatusMatcher(OkStatusMatcher const&) = default;
  OkStatusMatcher() = default;

  virtual bool match(jumanpp::Status const& status) const override {
    return status.isOk();
  }

  virtual std::string toStringUncached() const override {
    return "";
  }
};

#define CHECK_OK(x) CHECK_THAT(x, OkStatusMatcher())

#endif //JUMANPP_STANDALONE_TEST_H
