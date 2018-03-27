//
// Created by Arseny Tolmachev on 2017/11/07.
//

#include "feature_debug.h"
#include "core/analysis/analyzer_impl.h"
#include "core/impl/feature_impl_pattern.h"

namespace jumanpp {
namespace core {
namespace features {

namespace {
struct StringCopyDisplay : public Display {
  analysis::StringField field_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    if (ni.entryPtr() == EntryPtr::BOS()) {
      p << "BOS";
    } else if (ni.entryPtr() == EntryPtr::EOS()) {
      p << "EOS";
    } else {
      p << field_[walker];
    }
    return true;
  }
};

struct ShiftMaskDisplay : public Display {
  impl::ShiftMaskPrimFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << f_.access(ctx, ni, walker.features());
    return true;
  }
};

struct ProvidedDisplay : public Display {
  impl::ProvidedPrimFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p.write("{:X}", f_.access(ctx, ni, walker.features()));
    return true;
  }
};

struct ByteLengthDisplay : public Display {
  impl::ByteLengthPrimFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << f_.access(ctx, ni, walker.features());
    return true;
  }
};

struct CodepointLengthDisplay : public Display {
  impl::CodepointLengthPrimFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << f_.access(ctx, ni, walker.features());
    return true;
  }
};

struct SurfaceCodepointLengthDisplay : public Display {
  impl::SurfaceCodepointLengthPrimFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << f_.access(ctx, ni, walker.features());
    return true;
  }
};

struct CodepointDisplay : public Display {
  impl::CodepointFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << static_cast<char32_t>(f_.access(ctx, ni, walker.features()));
    return true;
  }
};

struct CodepointTypeDisplay : public Display {
  impl::CodepointTypeFeatureImpl f_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p.write("{:X}", f_.access(ctx, ni, walker.features()));
    return true;
  }
};

struct GroupDisplay : public Display {
  std::vector<std::unique_ptr<Display>> children_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << '[';
    for (int i = 0; i < children_.size(); ++i) {
      JPP_RET_CHECK(children_[i]->appendTo(p, walker, ni, ctx));
      if (i != children_.size() - 1) {
        p << ", ";
      }
    }
    p << ']';
    return true;
  }
};

struct TrueFalseDisplay : public Display {
  std::unique_ptr<Display> prim_;
  const impl::PrimitiveFeatureImpl* primFeature_;
  GroupDisplay true_;
  GroupDisplay false_;
  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    JPP_RET_CHECK(prim_->appendTo(p, walker, ni, ctx));
    p << ":";
    if (primFeature_->access(ctx, ni, walker.features()) != 0) {
      JPP_RET_CHECK(true_.appendTo(p, walker, ni, ctx));
    } else {
      JPP_RET_CHECK(false_.appendTo(p, walker, ni, ctx));
    }
    return true;
  }
};

struct PatternDisplay : public Display {
  std::vector<Display*> refs_;

  bool appendTo(util::io::FastPrinter& p, const analysis::NodeWalker& walker,
                const NodeInfo& ni,
                impl::PrimitiveFeatureContext* ctx) override {
    p << '{';
    for (int i = 0; i < refs_.size(); ++i) {
      refs_[i]->appendTo(p, walker, ni, ctx);
      if (i != refs_.size() - 1) {
        p << '-';
      }
    }
    p << '}';
    return true;
  }
};
}  // namespace

Status FeaturesDebugger::makeCombine(const analysis::AnalyzerImpl* impl_,
                                     i32 idx,
                                     std::unique_ptr<Display>& result) {
  auto& fspec = impl_->core().spec().features;
  auto& pat = fspec.computation[idx];

  if (pat.trueBranch.empty() && pat.falseBranch.empty()) {
    JPP_RETURN_IF_ERROR(makePrimitive(impl_, pat.primitiveFeature, result));
  } else {
    auto f = new TrueFalseDisplay;
    result.reset(f);
    f->primFeature_ = impl_->core().features().patternDynamic->primitive(
        pat.primitiveFeature);
    JPP_RETURN_IF_ERROR(makePrimitive(impl_, pat.primitiveFeature, f->prim_));
    for (auto fidx : pat.trueBranch) {
      f->true_.children_.emplace_back();
      auto& cf = f->true_.children_.back();
      JPP_RETURN_IF_ERROR(makePrimitive(impl_, fidx, cf));
    }
    for (auto fidx : pat.falseBranch) {
      f->false_.children_.emplace_back();
      auto& cf = f->false_.children_.back();
      JPP_RETURN_IF_ERROR(makePrimitive(impl_, fidx, cf));
    }
  }

  return Status::Ok();
}

Status FeaturesDebugger::makePrimitive(const analysis::AnalyzerImpl* impl,
                                       i32 idx,
                                       std::unique_ptr<Display>& result) {
  auto& fspec = impl->core().spec().features;
  auto& prim = fspec.primitive[idx];
  impl::FeatureConstructionContext fcc{&impl->core().dic().fields()};
  switch (prim.kind) {
    case PrimitiveFeatureKind::Copy: {
      auto f = new StringCopyDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(impl->output().stringField(prim.name, &f->field_));
      break;
    }
    case PrimitiveFeatureKind::SingleBit: {
      auto f = new ShiftMaskDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::Provided: {
      auto f = new ProvidedDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::ByteLength: {
      auto f = new ByteLengthDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::CodepointSize: {
      auto f = new CodepointLengthDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::SurfaceCodepointSize: {
      auto f = new SurfaceCodepointLengthDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::Codepoint: {
      auto f = new CodepointDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    case PrimitiveFeatureKind::CodepointType: {
      auto f = new CodepointTypeDisplay;
      result.reset(f);
      JPP_RETURN_IF_ERROR(f->f_.initialize(&fcc, prim));
      break;
    }
    default:
      return JPPS_NOT_IMPLEMENTED << "debug feature: " << prim.name
                                  << " was not implemented";
  }
  return Status::Ok();
}

Status FeaturesDebugger::initialize(const analysis::AnalyzerImpl* impl,
                                    const analysis::WeightBuffer& buf) {
  weights_ = &buf;
  auto& spec = impl->core().spec();
  for (auto& cf : spec.features.computation) {
    combines_.emplace_back();
    auto& res = combines_.back();
    JPP_RETURN_IF_ERROR(makeCombine(impl, cf.index, res));
  }
  for (auto& pf : spec.features.pattern) {
    auto f = new PatternDisplay;
    patterns_.emplace_back(f);
    for (auto idx : pf.references) {
      JPP_DCHECK_IN(idx, 0, combines_.size());
      f->refs_.push_back(combines_[idx].get());
    }
  }
  featureBuf_.resize(spec.features.ngram.size());
  return Status::Ok();
}

Status FeaturesDebugger::fill(const analysis::AnalyzerImpl* impl,
                              DebugFeatures* result,
                              const analysis::ConnectionPtr& ptr) {
  NgramFeatureRef tple{
      ptr.previous->previous->latticeNodePtr(),
      ptr.previous->latticeNodePtr(),
      ptr.latticeNodePtr(),
  };
  return fill(impl, result, tple);
}

Status FeaturesDebugger::fill(const analysis::AnalyzerImpl* impl,
                              DebugFeatures* result,
                              const NgramFeatureRef& nodes) {
  auto lat = impl->lattice();

  NgramFeaturesComputer nfc{lat, impl->core().features()};
  nfc.calculateNgramFeatures(nodes, &featureBuf_);
  auto& out = impl->output();
  auto t0w = out.nodeWalker();
  auto& ni0 = lat->boundary(nodes.t0.boundary)
                  ->starts()
                  ->nodeInfo()
                  .at(nodes.t0.position);
  if (!out.locate(nodes.t0, &t0w)) {
    return JPPS_INVALID_PARAMETER << "failed to find t0 " << nodes.t0;
  }
  auto t1w = out.nodeWalker();
  auto& ni1 = lat->boundary(nodes.t1.boundary)
                  ->starts()
                  ->nodeInfo()
                  .at(nodes.t1.position);
  if (!out.locate(nodes.t1, &t1w)) {
    return JPPS_INVALID_PARAMETER << "failed to find t1 " << nodes.t1;
  }
  auto t2w = out.nodeWalker();
  auto& ni2 = lat->boundary(nodes.t2.boundary)
                  ->starts()
                  ->nodeInfo()
                  .at(nodes.t2.position);
  if (!out.locate(nodes.t2, &t2w)) {
    return JPPS_INVALID_PARAMETER << "failed to find t2 " << nodes.t2;
  }
  impl::PrimitiveFeatureContext pfc{impl->extraNodesContext(),
                                    impl->dic().fields(), impl->dic().entries(),
                                    impl->input().codepoints()};
  u32 mask = static_cast<u32>(weights_->size() - 1);
  auto& ngrams = impl->core().spec().features.ngram;
  for (auto ngidx = 0; ngidx < featureBuf_.size(); ++ngidx) {
    auto& ngf = ngrams[ngidx];
    printer_.reset();
    printer_ << ngidx << '#';
    auto& refs = ngf.references;
    switch (refs.size()) {
      case 3:
        patterns_[refs[2]]->appendTo(printer_, t2w, ni2, &pfc);
      case 2:
        patterns_[refs[1]]->appendTo(printer_, t1w, ni1, &pfc);
      case 1:
        patterns_[refs[0]]->appendTo(printer_, t0w, ni0, &pfc);
        break;
      default:
        return JPPS_NOT_IMPLEMENTED << "ngram (" << ngidx
                                    << ") had size=" << refs.size();
    }

    result->features.emplace_back();
    auto& f = result->features.back();
    f.ngramIdx = ngidx;
    printer_.result().assignTo(f.repr);
    f.rawHashCode = featureBuf_[ngidx];
    f.maskedHashCode = f.rawHashCode & mask;
    f.score = weights_->at(f.maskedHashCode);
  }
  return Status::Ok();
}

}  // namespace features
}  // namespace core
}  // namespace jumanpp