//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "status.hpp"
#include <iosfwd>
#include <vector>

namespace jumanpp {

constexpr const char *const StatusNames[] = {
    "Ok", "InvalidParameter", "InvalidState", "NotImplemented", "MaxStatus"};

namespace status_impl {

struct StackFrameElement {
  StringPiece filename;
  int line;
  StringPiece function;
};

struct StatusDataImpl {
  StatusCode code;
  std::string message;
  std::vector<StackFrameElement> backtrace;

  StatusDataImpl(StatusCode code_, std::string message_)
      : code{code_}, message{std::move(message_)} {}

  void AddFrame(StringPiece file, int line, StringPiece function) {
    backtrace.push_back(StackFrameElement{file, line, function});
  }
};

StatusDataImpl *StatusOps::data() { return status_->data_; }

StatusOps &StatusOps::AddFrame(StringPiece file, int line,
                               StringPiece function) {
  data()->AddFrame(file, line, function);
  return *this;
}

StatusConstructor::operator Status() const {
  Status s{code_, msgbuilder_.str()};
  if (line_ != -1) {
    StatusOps(&s).AddFrame(file_, line_, func_);
  }
  return s;
}

void MessageBuilder::ReplaceMessage(StatusDataImpl *data) {
  if (data) {
    builder_ << " <- " << data->message;
    data->message = builder_.str();
  }
}

}  // namespace status_impl

std::ostream &operator<<(std::ostream &str, const Status &st) {
  auto data = status_impl::StatusOps(const_cast<Status *>(&st)).data();

  if (data == nullptr) {
    return str;
  }

  int idx = (int)data->code;
  auto *name = StatusNames[idx];
  str << name;
  auto &msg = data->message;
  if (!msg.empty()) {
    str << ": " << msg;
  }
  auto &bt = data->backtrace;
  if (!bt.empty()) {
    str << "\nbacktrace:";
    for (auto &e : bt) {
      str << "\n    " << e.function << " at " << e.filename << ':' << e.line;
    }
  }
  return str;
}

Status::Status(StatusCode code_, const std::string &message_)
    : data_{new status_impl::StatusDataImpl{code_, message_}} {}

Status::Status(StatusCode code_) {
  if (code_ != StatusCode::Ok) {
    data_ = new status_impl::StatusDataImpl{code_, ""};
  }
}

Status &Status::operator=(Status &&s) noexcept {
  if (data_ != nullptr) {
    delete data_;
  }
  data_ = s.data_;
  s.data_ = nullptr;
  return *this;
}

StringPiece Status::message() const {
  if (data_) {
    return data_->message;
  }
  return StringPiece{"OK"};
}

StatusCode Status::code() const {
  if (data_) {
    return data_->code;
  }
  return StatusCode::Ok;
}

void Status::DestroyData() noexcept { delete data_; }

}  // namespace jumanpp
