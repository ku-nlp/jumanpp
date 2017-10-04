//
// Created by Arseny Tolmachev on 2017/02/15.
//

#ifndef JUMANPP_STATUS_HPP
#define JUMANPP_STATUS_HPP

#include <util/string_piece.h>
#include <sstream>
#include <string>
#include <utility>

#include "common.hpp"

namespace jumanpp {

enum class StatusCode : int {
  // When adding a new status here do not forget add line to
  // StatusNames in status.cpp.
  Ok = 0,
  InvalidParameter = 1,
  InvalidState = 2,
  NotImplemented = 3,
  MaxStatus
};

class Status;

namespace status_impl {
struct StatusDataImpl;
class StatusConstructor {
  StatusCode code_;
  std::stringstream msgbuilder_;
  StringPiece file_;
  int line_ = -1;
  StringPiece func_;

 public:
  StatusConstructor(StatusCode code) : code_(code) {}

  StatusConstructor(StatusCode code, StringPiece file, int line,
                    StringPiece func)
      : code_{code}, file_{file}, line_{line}, func_{func} {}

  template <typename T>
  StatusConstructor &operator<<(const T &o) {
    msgbuilder_ << o;
    return *this;
  }

  operator Status() const;
};

class MessageBuilder {
  std::stringstream builder_;

 public:
  MessageBuilder() = default;

  template <typename T>
  MessageBuilder &operator<<(const T &obj) {
    builder_ << obj;
    return *this;
  }

  void ReplaceMessage(StatusDataImpl *data);
};

class StatusOps {
  Status *status_;

 public:
  explicit StatusOps(Status *status) : status_{status} {}
  StatusOps &AddFrame(StringPiece file, int line, StringPiece function);
  StatusDataImpl *data();
};
}  // namespace status_impl

class Status {
 private:
  status_impl::StatusDataImpl *data_;
  Status() noexcept : data_{nullptr} {}
  void DestroyData() noexcept;

 public:
  explicit Status(StatusCode code_);
  Status(StatusCode code_, const std::string &message_);
  Status(Status &&s) noexcept : data_{s.data_} { s.data_ = nullptr; }
  Status &operator=(Status &&s) noexcept;

  inline bool isOk() const noexcept { return data_ == nullptr; }

  inline explicit operator bool() const noexcept { return isOk(); }

  static Status Ok() { return Status{}; }

  static status_impl::StatusConstructor InvalidParameter() {
    return status_impl::StatusConstructor{StatusCode::InvalidParameter};
  }

  static status_impl::StatusConstructor InvalidState() {
    return status_impl::StatusConstructor{StatusCode::InvalidState};
  }

  static status_impl::StatusConstructor NotImplemented() {
    return status_impl::StatusConstructor{StatusCode::NotImplemented};
  }

  ~Status() {
    if (JPP_UNLIKELY(data_ != nullptr)) {
      DestroyData();
    }
  }

  StringPiece message() const;

  StatusCode code() const;

  friend class status_impl::StatusOps;
};

std::ostream &operator<<(std::ostream &str, const Status &st);

}  // namespace jumanpp

#define JPP_RETURN_IF_ERROR(expr)                      \
  {                                                    \
    Status __status__ = (expr);                        \
    if (JPP_UNLIKELY(!__status__.isOk())) {            \
      ::jumanpp::status_impl::StatusOps(&__status__)   \
          .AddFrame(__FILE__, __LINE__, __FUNCTION__); \
      return __status__;                               \
    }                                                  \
  }

#define JPP_RIE_MSG(expr, msg)                                   \
  {                                                              \
    Status __status__ = (expr);                                  \
    if (JPP_UNLIKELY(!__status__.isOk())) {                      \
      auto ops = ::jumanpp::status_impl::StatusOps(&__status__); \
      ops.AddFrame(__FILE__, __LINE__, __FUNCTION__);            \
      {                                                          \
        auto __data__ = ops.data();                              \
        ::jumanpp::status_impl::MessageBuilder __msgbld__{};     \
        __msgbld__ << msg;                                       \
        __msgbld__.ReplaceMessage(__data__);                     \
      }                                                          \
      return __status__;                                         \
    }                                                            \
  }

#define JPPS_INVALID_PARAMETER                                                \
  ::jumanpp::status_impl::StatusConstructor {                                 \
    ::jumanpp::StatusCode::InvalidParameter, __FILE__, __LINE__, __FUNCTION__ \
  }

#define JPPS_INVALID_STATE                                                \
  ::jumanpp::status_impl::StatusConstructor {                             \
    ::jumanpp::StatusCode::InvalidState, __FILE__, __LINE__, __FUNCTION__ \
  }

#define JPPS_NOT_IMPLEMENTED                                                \
  ::jumanpp::status_impl::StatusConstructor {                               \
    ::jumanpp::StatusCode::NotImplemented, __FILE__, __LINE__, __FUNCTION__ \
  }

#endif  // JUMANPP_STATUS_HPP
