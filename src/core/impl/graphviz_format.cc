//
// Created by Arseny Tolmachev on 2017/05/17.
//

#include "graphviz_format.h"
#include "core/analysis/score_processor.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace format {

namespace detail {

void RenderCell::render(detail::RenderOutput *out, detail::RenderContext *ctx) {
  out->lit('<').lit(tag_);
  for (auto &attr : attributes_) {
    out->lit(' ').lit(attr.key).lit('=').q(attr.value);
  }
  out->lit('>');
  auto doBreak = children_.size() > 1;
  if (doBreak) {
    out->indent(1);
  }
  for (auto &child : children_) {
    if (doBreak) {
      out->line();
    }
    child->render(out, ctx);
  }
  if (doBreak) {
    out->line(-1);
  }
  out->lit("</").lit(tag_).lit('>');
}

}  // namespace detail

namespace d = ::jumanpp::core::format::detail;

void GraphVizBuilder::row(std::initializer_list<StringPiece> fields,
                          std::initializer_list<detail::Attribute> attrs) {
  auto container = tag("tr");

  for (auto &fldName : fields) {
    auto fld = new d::RenderStringField{fldName};
    resources_.emplace_back(fld);
    auto cell = tag("td");

    for (auto &attr : attrs) {
      cell.attr(attr.key, attr.value);
    }
    cell.child(fld);
    container.child(cell);
  }

  body_.push_back(container);
}

Status GraphVizBuilder::build(GraphVizFormat *format, int maxBeam) {
  auto header = makeHeader();
  auto footer = makeFooter(maxBeam);

  auto everything = tag("table");
  everything.attr("border", "0")
      .attr("cellpadding", "0")
      .attr("cellspacing", "0");
  everything.child(header);
  for (auto &r : body_) {
    everything.child(r);
  }
  everything.child(footer);

  d::GraphVizConfig config{};
  config.header = header;
  config.footer = footer;
  config.everything = everything;
  format->config_ = config;
  format->resources_ = std::move(resources_);
  format->chars_ = std::move(chars_);
  format->maxBeam_ = maxBeam;
  return Status::Ok();
}

detail::Renderable *GraphVizBuilder::makeHeader() {
  auto container = tag("tr");
  auto render = fn([](d::RenderOutput *out, d::RenderContext *ctx) {
    auto &w = ctx->walker();
    auto ptr = ctx->curNode();
    out->lit(ptr.boundary).lit(" ").lit(ptr.position);
    if (w.remaining() != 0) {
      out->lit(" (+ ").lit(w.remaining() + 1).lit(')');
    }
  });
  auto cell = tag("td").attr("colspan", "3").child(render);

  container.child(cell);

  return container;
}

detail::Renderable *GraphVizBuilder::makeFooter(int maxBeam) {
  auto container = tag("table");
  container.attr("border", "0").attr("cellborder", "1");

  for (int i = 0; i < maxBeam; ++i) {
    auto row = tag("tr");
    auto col = tag("td").attr("port", str("beam_", i));
    col.child(fn([i](d::RenderOutput *out, d::RenderContext *ctx) {
      auto lat = ctx->lattice();
      auto ptr = ctx->curNode();
      auto bnd = lat->boundary(ptr.boundary);
      auto beamData = bnd->starts()->beamData().row(ptr.position);
      if (i >= beamData.size()) {
        out->lit("***");
        return;
      }
      auto myBeam = beamData.at(i);
      if (analysis::EntryBeam::isFake(myBeam)) {
        out->lit("N/A");
      } else {
        auto sc = bnd->scores()->forPtr(myBeam.ptr);
        auto max = sc.size();
        for (int j = 0; j < max; ++j) {
          auto score = sc.at(j);
          out->lit(score);
          if (j != max - 1) {
            out->lit(' ');
          }
        }
      }
    }));
    row.child(col);
    container.child(row);
  }

  auto outer = tag("tr").child(tag("td").attr("colspan", "5").child(container));
  return outer;
}

Status GraphVizFormat::render(const analysis::Lattice *lat) {
  if (output_ == nullptr) {
    return Status::InvalidState() << "graphviz: output was not initialized";
  }

  JPP_RETURN_IF_ERROR(top1_.fillIn(lat));

  detail::RenderOutput out{&printer_};

  out.lit("digraph {").line(1);
  out.lit("rankdir = \"LR\";").line();

  if (top1_.numBoundaries() > 0) {
    out.lit("// nodes").line();
    JPP_RETURN_IF_ERROR(renderNodes(&out, lat));
    out.line().lit("// edges").line();
    JPP_RETURN_IF_ERROR(renderEdges(&out, lat));
  }

  out.line(-1).lit("}").line(0);

  return Status::Ok();
}

Status GraphVizFormat::renderNodes(detail::RenderOutput *out,
                                   const analysis::Lattice *lat) {
  auto nbnd = lat->createdBoundaryCount() - 1;

  out->lit("// BOS node").line();
  out->lit("node_1_0 [shape=record, label=").q("<beam_0> BOS").lit("];").line();

  auto walker = output_->nodeWalker();

  d::RenderContext ctx{lat, &walker};

  for (int i = 2; i < nbnd; ++i) {
    auto bnd = lat->boundary(i);
    auto nnodes = bnd->localNodeCount();
    for (int j = 0; j < nnodes; ++j) {
      d::NodePtr ptr{(u16)i, (u16)j};
      if (!output_->locate(ptr, &walker)) {
        return Status::InvalidState()
               << "could not find a node: " << i << ", " << j;
      }
      if (!walker.next()) {
        return Status::InvalidState() << "walker did not have the next node";
      }

      ctx.changeCurrent(ptr);
      out->lit("node_")
          .lit(i)
          .lit('_')
          .lit(j)
          .lit(" [shape=plaintext, label=<")
          .line(1);
      config_.everything->render(out, &ctx);
      out->line(-1).lit(">");
      if (isGold(ptr)) {
        out->lit(", color=gold");
      }
      out->lit("];").line();
    }
  }

  out->lit("// EOS node").line();
  out->lit("node_")
      .lit(nbnd)
      .lit("_0 [shape=plaintext, label=<<table><tr><td>EOS</td></tr>")
      .line(1);
  ctx.changeCurrent({(u16)nbnd, 0});
  config_.footer->render(out, &ctx);
  out->line(-1).lit("</table>>];").line();

  return Status::Ok();
}

Status GraphVizFormat::renderEdges(detail::RenderOutput *out,
                                   const analysis::Lattice *lat) {
  auto nbnd = lat->createdBoundaryCount();
  for (int i = 2; i < nbnd; ++i) {
    out->line();
    auto bnd = lat->boundary(i);
    auto nnodes = bnd->localNodeCount();

    for (int j = 0; j < nnodes; ++j) {
      auto beamData = bnd->starts()->beamData().row(j);
      auto maxBeam = std::min<i64>(maxBeam_, beamData.size());
      for (int k = 0; k < maxBeam; ++k) {
        auto beamObj = beamData.at(k);
        if (analysis::EntryBeam::isFake(beamObj)) {
          break;
        }

        auto prev = beamObj.ptr.previous;

        out->lit("node_").lit(prev->boundary).lit("_").lit(prev->right);
        if (beamObj.ptr.beam < maxBeam_) {
          out->lit(":beam_").lit(beamObj.ptr.beam).lit(":e");
        }
        out->lit(" -> node_")
            .lit(beamObj.ptr.boundary)
            .lit("_")
            .lit(beamObj.ptr.right)
            .lit(":beam_")
            .lit(k)
            .lit(":w");
        out->lit(" [");
        out->lit("label=").q(beamObj.totalScore);
        if (top1_.contains(*prev, beamObj.ptr)) {
          out->lit(",color=red");
        }
        out->lit("];").line();
      }
    }
  }
  return Status::Ok();
}

bool GraphVizFormat::isGold(detail::NodePtr nodePtr) const {
  return std::find(gold_.begin(), gold_.end(), nodePtr) != gold_.end();
}

Status GraphVizFormat::render(const analysis::Analyzer &ana) {
  return render(ana.impl()->lattice());
}

}  // namespace format
}  // namespace core
}  // namespace jumanpp