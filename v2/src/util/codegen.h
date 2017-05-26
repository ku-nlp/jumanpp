//
// Created by Arseny Tolmachev on 2017/03/10.
//

#ifndef JUMANPP_CODEGEN_H
#define JUMANPP_CODEGEN_H

#include <vector>
#include "util/array_slice.h"
#include "util/printer.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace util {
namespace codegen {

template <typename T>
std::string to_str(const T& o) {
  std::stringstream ss;
  ss << o;
  return ss.str();
}

class PrintSemicolon {
  io::Printer& p;

 public:
  PrintSemicolon(io::Printer& p) : p(p) {}
  ~PrintSemicolon() { p << ";\n"; }
};

struct ConstDeclaration {
  std::string type;
  std::string name;
  std::vector<std::string> initialize;

  void print(io::Printer& p) {
    PrintSemicolon x{p};
    p << "constexpr const " << type << " " << name;
    if (initialize.size() == 0) {
      return;
    }
    p << " = ";
    if (initialize.size() == 1) {
      p << initialize[0];
    } else {
      p << "{";
      {
        io::Indent id{p, 2};
        for (int i = 0; i < initialize.size(); ++i) {
          if (i % 10 == 0) {
            p << "\n";
          }
          p << initialize[i] << ", ";
        }
      }
      p << "\n}";
    }
  }
};

class ResultAssign {
  i32 resultIdx_;
  std::vector<std::string> pieces_;
  std::stringstream ss_;
public:
  explicit ResultAssign(i32 resultIdx_) : resultIdx_(resultIdx_) {}

  ResultAssign& addHashConstant(u64 value) {
    ss_.str("");
    ss_ << value << "ULL";
    pieces_.push_back(ss_.str());
    return *this;
  }

  ResultAssign& addHashIndexed(StringPiece var, size_t idx) {
    ss_.str("");
    ss_ << var << '[' << idx << ']';
    pieces_.push_back(ss_.str());
    return *this;
  }

  void render(io::Printer& p) const {
    PrintSemicolon x{p};
    p << "result.at(" << resultIdx_ << ") = " <<
          "jumanpp::util::hashing::hashCtSeq"
      << '(';
    for (int i = 0; i < pieces_.size(); ++i) {
      p << pieces_[i];
      if (i != pieces_.size() - 1) {
        p << ", ";
      }
    }
    p << ')';
  }
};

class MethodBody {
  std::vector<ResultAssign> assigns_;
public:
  ResultAssign& resultInto(i32 index) {
    assigns_.emplace_back(index);
    return assigns_.back();
  }

  void render(io::Printer& p) {
    for (auto &a: assigns_) {
      a.render(p);
    }
  }
};

struct GeneratedClass {
  std::string name;
  std::string base;
  std::vector<ConstDeclaration> decls;

  void addI32Const(StringPiece name, i32 v) {
    ConstDeclaration decl{"jumanpp::i32", name.str(), {to_str(v)}};
    decls.push_back(decl);
  }

  void addI32Array(StringPiece name, util::ArraySlice<i32> vals) {
    ConstDeclaration c;
    c.type = "jumanpp::i32";
    c.name = name.str() + " []";
    for (auto i : vals) {
      c.initialize.push_back(to_str(i));
    }
  }
};

}  // namespace codegen
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_CODEGEN_H
