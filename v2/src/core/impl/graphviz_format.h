//
// Created by Arseny Tolmachev on 2017/05/17.
//

#ifndef JUMANPP_GRAPHVIZ_FORMAT_H
#define JUMANPP_GRAPHVIZ_FORMAT_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/lattice_types.h"
#include "core/analysis/output.h"
#include "util/char_buffer.h"
#include "util/printer.h"

namespace jumanpp {
namespace core {
namespace format {

namespace detail {

using NodePtr = analysis::LatticeNodePtr;

struct GraphVizNode {
  NodePtr ptr;
  bool gold = false;
};

struct GraphVizEdge {
  NodePtr start;
  i32 startBeam;
  NodePtr end;
  i32 endBeam;
};

class RenderContext {
  analysis::Lattice* lattice_;
  analysis::NodeWalker* walker_;
  NodePtr curNode_;

 public:
  RenderContext(analysis::Lattice* lattice_, analysis::NodeWalker* walker_)
      : lattice_(lattice_), walker_(walker_) {}

  void changeCurrent(NodePtr ptr) { curNode_ = ptr; }

  const analysis::NodeWalker& walker() const { return *walker_; }

  NodePtr curNode() const { return curNode_; }

  analysis::Lattice* lattice() const { return lattice_; }
};

class RenderOutput {
  int currentIndent = 0;
  util::io::Printer* printer_;

 public:
  RenderOutput(util::io::Printer* printer_) : printer_(printer_) {}

  template <typename T>
  RenderOutput& lit(const T& o) {
    printer_->rawOutput(o);
    return *this;
  }

  template <typename T>
  RenderOutput& q(const T& o) {
    lit('"').lit(o).lit('"');
    return *this;
  }

  RenderOutput& line(int indent = 0) {
    this->indent(indent);
    lit('\n');
    for (int i = 0; i < currentIndent; ++i) {
      lit(' ');
    }
    return *this;
  }

  void indent(int i) { currentIndent += i; }
};

class Renderable {
 protected:
  Renderable() = default;

 public:
  virtual ~Renderable() = default;
  virtual void render(RenderOutput* out, RenderContext* ctx) = 0;
  virtual Status initialize(const analysis::OutputManager* omgr) {
    return Status::Ok();
  }
};

struct Attribute {
  StringPiece key;
  StringPiece value;
};

class RenderCell : public Renderable {
  StringPiece tag_;
  std::vector<Attribute> attributes_;
  std::vector<Renderable*> children_;

  friend class GraphVizBuilder;
  friend class RenderCellBuilder;

 public:
  RenderCell(StringPiece tag) : tag_{tag} {}

  void render(RenderOutput* out, RenderContext* ctx) override;

  Status initialize(const analysis::OutputManager* omgr) override {
    for (auto& chld : children_) {
      JPP_RETURN_IF_ERROR(chld->initialize(omgr));
    }
    return Status::Ok();
  }
};

class RenderStringField : public Renderable {
  StringPiece fieldName;
  analysis::StringField field;

 public:
  RenderStringField(StringPiece name) : fieldName{name} {}

  void render(RenderOutput* out, RenderContext* ctx) override {
    auto val = field[ctx->walker()];
    out->lit(val);
  }

  Status initialize(const analysis::OutputManager* omgr) override {
    return omgr->stringField(fieldName, &field);
  }
};

class FunctionRender : public Renderable {
 public:
  using FnType = std::function<void(RenderOutput*, RenderContext*)>;

 private:
  FnType func_;

 public:
  FunctionRender(const FnType& func_) : func_(func_) {}

  void render(RenderOutput* out, RenderContext* ctx) override {
    func_(out, ctx);
  }
};

class RenderCellBuilder {
  RenderCell* cell_;

 public:
  RenderCellBuilder(RenderCell* cell_) : cell_(cell_) {}

  RenderCellBuilder child(Renderable* obj) {
    cell_->children_.push_back(obj);
    return *this;
  }

  RenderCellBuilder attr(StringPiece key, StringPiece value) {
    cell_->attributes_.push_back({key, value});
    return *this;
  }

  operator Renderable*() { return cell_; }
};

struct GraphVizConfig {
  Renderable* header;
  Renderable* body;
  Renderable* footer;
  Renderable* everything;
};

inline void concat2(std::stringstream& ss) {}

template <typename T>
inline void concat2(std::stringstream& ss, const T& o) {
  ss << o;
}

template <typename T, typename... Args>
inline void concat2(std::stringstream& ss, const T& o, const Args&... args) {
  ss << o;
  detail::concat2<Args...>(ss, args...);
};

}  // namespace detail

class GraphVizFormat {
  std::vector<std::unique_ptr<detail::Renderable>> resources_;
  const analysis::OutputManager* output_ = nullptr;
  util::CharBuffer<1 << 15> chars_;
  detail::GraphVizConfig config_;
  util::io::Printer printer_;
  std::vector<detail::NodePtr> gold_;
  i32 maxBeam_;
  analysis::AnalysisPath top1_;

  friend class GraphVizBuilder;

  Status renderNodes(detail::RenderOutput* out, analysis::Lattice* lat);
  Status renderEdges(detail::RenderOutput* out, analysis::Lattice* lat);
  bool isGold(detail::NodePtr nodePtr) const;

 public:
  Status initialize(const analysis::OutputManager& om) {
    output_ = &om;
    return config_.everything->initialize(&om);
  }

  void reset() {
    gold_.clear();
    printer_.reset();
  }

  void markGold(analysis::LatticeNodePtr nodePtr) { gold_.push_back(nodePtr); }

  Status render(analysis::Lattice* lat);
  StringPiece result() const { return printer_.result(); }
};

class GraphVizBuilder {
  std::vector<std::unique_ptr<detail::Renderable>> resources_;
  std::vector<detail::Renderable*> body_;
  util::CharBuffer<1 << 15> chars_;

 public:
  detail::RenderCellBuilder tag(StringPiece tag) {
    auto cell = new detail::RenderCell{tag};
    resources_.emplace_back(cell);
    return detail::RenderCellBuilder{cell};
  }

  detail::Renderable* fn(const detail::FunctionRender::FnType& fn) {
    auto cell = new detail::FunctionRender{fn};
    resources_.emplace_back(cell);
    return cell;
  }

  template <typename... Args>
  StringPiece str(const Args&... args) {
    std::stringstream ss;
    detail::concat2<Args...>(ss, args...);
    std::string s = ss.str();
    StringPiece sp = s;
    chars_.import(&sp);
    return sp;
  }

  void row(std::initializer_list<StringPiece> fields,
           std::initializer_list<detail::Attribute> attrs = {});
  Status build(GraphVizFormat* format, int maxBeam = 3);
  detail::Renderable* makeHeader();
  detail::Renderable* makeFooter(int maxBeam);
};

}  // namespace format
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_GRAPHVIZ_FORMAT_H
