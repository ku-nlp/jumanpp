//
// Created by Arseny Tolmachev on 2017/12/08.
//

#include "lattice_dump_output.h"
#include "core/analysis/analyzer_impl.h"
#include "core/impl/feature_debug.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "lattice_dump.pb.h"

namespace jumanpp {
namespace core {
namespace output {

namespace {

struct ValueConverter {
  virtual ~ValueConverter() = default;
  virtual Status append(const analysis::NodeWalker& nw, LatticeNode* node) = 0;
};

struct IntValueConverter : public ValueConverter {
  analysis::IntField fld_;
  Status append(const analysis::NodeWalker& nw, LatticeNode* node) override {
    auto v = fld_[nw];
    node->add_values()->set_int_(v);
    return Status::Ok();
  }
};

struct StringValueConverter : public ValueConverter {
  analysis::StringField fld_;
  Status append(const analysis::NodeWalker& nw, LatticeNode* node) override {
    if (nw.eptr() == EntryPtr::EOS()) {
      node->add_values()->set_string("EOS");
    } else {
      auto v = fld_[nw];
      node->add_values()->set_string(v.data(), v.size());
    }

    return Status::Ok();
  }
};

struct StringListValueConverter : public ValueConverter {
  analysis::StringListField fld_;
  Status append(const analysis::NodeWalker& nw, LatticeNode* node) override {
    auto iter = fld_[nw];
    StringPiece v;
    auto res = node->add_values()->mutable_string_list();
    while (iter.next(&v)) {
      res->add_values(v.data(), v.size());
    }
    return Status::Ok();
  }
};

struct KVListValueConverter : public ValueConverter {
  analysis::KVListField fld_;
  Status append(const analysis::NodeWalker& nw, LatticeNode* node) override {
    auto iter = fld_[nw];
    auto res = node->add_values()->mutable_kvlist();
    while (iter.next()) {
      auto v = res->add_values();
      auto key = iter.key();
      v->set_key(key.data(), key.size());
      if (iter.hasValue()) {
        auto val = iter.value();
        v->set_value(val.data(), val.size());
      }
    }
    return Status::Ok();
  }
};

}  // namespace

struct LatticeDumpOutputImpl {
  features::FeaturesDebugger debugger_;
  google::protobuf::Arena arena_;
  LatticeDump* obj_;
  std::vector<std::string> fieldNames_;
  util::FlatMap<analysis::LatticeNodePtr, std::vector<i32>> ranks_;
  util::FlatMap<analysis::ConnectionPtr, std::vector<i32>> pathRanks_;
  std::vector<std::unique_ptr<ValueConverter>> converters_;
  core::features::DebugFeatures debugFeatures_;
  std::string buffer_;
  bool fillFeatures_;
  bool fillBuffer_;

  Status fillBeams(const analysis::AnalyzerImpl& ai,
                   analysis::LatticeRightBoundary* bnd, LatticeNode* node,
                   i32 nodeIdx) {
    auto beams = bnd->beamData().row(nodeIdx);
    auto& nspec = ai.core().spec().features.ngram;
    for (auto beamIdx = 0; beamIdx < beams.size(); ++beamIdx) {
      auto& el = beams.at(beamIdx);
      if (analysis::EntryBeam::isFake(el)) {
        break;
      }

      auto path = node->add_beams();

      auto& t0ptr = el.ptr;
      auto& t1ptr = *t0ptr.previous;
      auto& t2ptr = *t1ptr.previous;

      auto t2res = path->add_nodes();
      t2res->set_boundary(t2ptr.boundary);
      t2res->set_node(t2ptr.right);
      t2res->set_beam(t1ptr.beam);
      auto t1res = path->add_nodes();
      t1res->set_boundary(t1ptr.boundary);
      t1res->set_node(t1ptr.right);
      t1res->set_beam(t0ptr.beam);
      auto t0res = path->add_nodes();
      t0res->set_boundary(t0ptr.boundary);
      t0res->set_node(t0ptr.right);
      t0res->set_beam(beamIdx);

      path->set_cum_score(el.totalScore);

      auto rankit = pathRanks_.find(t0ptr);
      if (rankit != pathRanks_.end()) {
        for (auto& r : rankit->second) {
          path->add_ranks(r);
        }
      }

      auto scores =
          ai.lattice()->boundary(t0ptr.boundary)->scores()->forPtr(t0ptr);
      for (auto& s : scores) {
        path->add_raw_scores(s);
      }

      if (fillFeatures_) {
        debugFeatures_.features.clear();
        JPP_RETURN_IF_ERROR(debugger_.fill(&ai, &debugFeatures_, el.ptr));
        for (int ngram = 0; ngram < nspec.size(); ++ngram) {
          auto& ndef = nspec[ngram];
          auto& nres = debugFeatures_.features[ngram];
          auto inst = path->add_features();
          inst->set_index(ngram);
          for (auto ref : ndef.references) {
            inst->add_patterns(ref);
          }
          inst->set_repr(std::move(nres.repr));
          inst->set_raw_value(nres.rawHashCode);
          inst->set_masked_value(nres.maskedHashCode);
          inst->set_weight(nres.score);
        }
      }
    }

    return Status::Ok();
  }

  Status fillNodeValues(const analysis::NodeWalker& walker, LatticeNode* node) {
    for (auto& conv : converters_) {
      JPP_RETURN_IF_ERROR(conv->append(walker, node));
    }
    for (auto f : walker.features()) {
      node->add_value_ptrs(f);
    }
    for (auto f : walker.data()) {
      node->add_value_ptrs(f);
    }
    return Status::Ok();
  }

  Status fillNode(const analysis::AnalyzerImpl& ai, LatticeNode* node,
                  analysis::LatticeRightBoundary* bnd, i32 bndIdx,
                  i32 nodeIdx) {
    auto& om = ai.output();
    auto w = om.nodeWalker();
    auto& ni = bnd->nodeInfo().at(nodeIdx);
    if (!om.locate(ni.entryPtr(), &w)) {
      return JPPS_INVALID_STATE << "failed a find a node with ptr: "
                                << ni.entryPtr().rawValue();
    }

    StringPiece surf;
    if (ai.lattice()->createdBoundaryCount() - 1 == bndIdx) {
      surf = "EOS";
    } else {
      auto firstBnd = bndIdx - 2;
      surf = ai.input().surface(firstBnd, firstBnd + ni.numCodepoints());
    }

    if (!w.next()) {
      return JPPS_INVALID_STATE << "failed to resolve first features";
    }

    JPP_RETURN_IF_ERROR(fillNodeValues(w, node));
    while (w.next()) {
      auto variant = node->add_variants();
      JPP_RETURN_IF_ERROR(fillNodeValues(w, variant));
    }

    node->set_length(ni.numCodepoints());
    node->set_surface(surf.data(), surf.size());
    node->set_entry_ptr(ni.entryPtr().rawValue());
    auto rankit = ranks_.find(analysis::LatticeNodePtr::make(bndIdx, nodeIdx));
    if (rankit != ranks_.end()) {
      for (auto rank : rankit->second) {
        node->add_ranks(rank);
      }
    }
    auto patv = bnd->patternFeatureData().row(nodeIdx);
    for (auto pat : patv) {
      node->add_patterns(pat);
    }
    JPP_RETURN_IF_ERROR(fillBeams(ai, bnd, node, nodeIdx));

    return Status::Ok();
  }

  Status fillBoundaries(const core::analysis::Analyzer& analyzer) {
    auto l = analyzer.impl()->lattice();
    for (i32 bndIdx = 2; bndIdx < l->createdBoundaryCount(); ++bndIdx) {
      auto bnd = obj_->add_boundaries();
      auto bndstart = l->boundary(bndIdx)->starts();
      for (i32 nodeIdx = 0; nodeIdx < bndstart->numEntries(); ++nodeIdx) {
        auto nodeObj = bnd->add_nodes();
        JPP_RIE_MSG(
            fillNode(*analyzer.impl(), nodeObj, bndstart, bndIdx, nodeIdx),
            "bnd=" << bndIdx << " node=" << nodeIdx);
      }
    }
    return Status::Ok();
  }

  Status fillFieldNames() {
    for (auto& n : fieldNames_) {
      obj_->add_field_names(n);
    }
    return Status::Ok();
  }

  void resolveRanks(const analysis::Lattice* l) {
    auto eos =
        l->boundary(l->createdBoundaryCount() - 1)->starts()->beamData().row(0);
    for (i32 rank = 0; rank < eos.size(); ++rank) {
      auto& el = eos.at(rank);
      if (analysis::EntryBeam::isFake(el)) {
        break;
      }

      const analysis::ConnectionPtr* ptr = &el.ptr;
      while (ptr->boundary >= 2) {
        pathRanks_[*ptr].push_back(rank);
        auto& nodeRanks = ranks_[ptr->latticeNodePtr()];
        if (!nodeRanks.empty() && nodeRanks.back() != rank) {
          nodeRanks.push_back(rank);
        }
        ptr = ptr->previous;
      }
    }
  }

  Status format(const core::analysis::Analyzer& analyzer, StringPiece comment) {
    arena_.Reset();
    ranks_.clear_no_resize();
    pathRanks_.clear_no_resize();
    resolveRanks(analyzer.impl()->lattice());
    obj_ = google::protobuf::Arena::CreateMessage<LatticeDump>(&arena_);

    obj_->set_comment(comment.data(), comment.size());
    auto surface = analyzer.impl()->input().surface();
    obj_->set_surface(surface.begin(), surface.size());
    auto& spec = analyzer.impl()->core().spec();
    auto idxCol = spec.dictionary.indexColumn;
    auto& col = spec.dictionary.fields[idxCol];
    obj_->set_surface_idx(col.dicIndex);
    JPP_RETURN_IF_ERROR(fillBoundaries(analyzer));
    JPP_RETURN_IF_ERROR(fillFieldNames());

    if (fillBuffer_) {
      buffer_.clear();
      google::protobuf::io::StringOutputStream os{&buffer_};
      google::protobuf::io::CodedOutputStream cos{&os};
      cos.WriteVarint64(obj_->ByteSizeLong());
      if (!obj_->SerializeToCodedStream(&cos)) {
        return JPPS_INVALID_STATE << "failed to serialize LatticeDump";
      }
    }

    return Status::Ok();
  }

  Status initialize(const analysis::AnalyzerImpl* impl,
                    const analysis::WeightBuffer* weights) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    JPP_RETURN_IF_ERROR(debugger_.initialize(impl, *weights));
    auto& spec = impl->core().spec();
    auto numFields = spec.dictionary.fields.size();
    fieldNames_.resize(numFields);
    converters_.resize(numFields);
    auto& om = impl->output();
    for (auto& f : spec.dictionary.fields) {
      auto idx = f.dicIndex;
      if (idx < 0) {
        idx = spec.features.numDicFeatures + ~idx;
      }
      fieldNames_[idx] = f.name;
      switch (f.fieldType) {
        case spec::FieldType::Int: {
          auto p = new IntValueConverter;
          converters_[idx].reset(p);
          JPP_RETURN_IF_ERROR(om.intField(f.name, &p->fld_));
          break;
        }
        case spec::FieldType::String: {
          auto p = new StringValueConverter;
          converters_[idx].reset(p);
          JPP_RETURN_IF_ERROR(om.stringField(f.name, &p->fld_));
          break;
        }
        case spec::FieldType::StringList: {
          auto p = new StringListValueConverter;
          converters_[idx].reset(p);
          JPP_RETURN_IF_ERROR(om.stringListField(f.name, &p->fld_));
          break;
        }
        case spec::FieldType::StringKVList: {
          auto p = new KVListValueConverter;
          converters_[idx].reset(p);
          JPP_RETURN_IF_ERROR(om.kvListField(f.name, &p->fld_));
          break;
        }
        default:
          return JPPS_NOT_IMPLEMENTED << "field type " << f.fieldType
                                      << " was not implemented";
      }
    }
    return Status::Ok();
  }
};

LatticeDumpOutput::~LatticeDumpOutput() = default;

LatticeDumpOutput::LatticeDumpOutput(bool fillFeatures, bool fillBuffer)
    : fillFeatures_{fillFeatures}, fillBuffer_{fillBuffer} {}

Status LatticeDumpOutput::format(const core::analysis::Analyzer& analyzer,
                                 StringPiece comment) {
  JPP_RETURN_IF_ERROR(impl_->format(analyzer, comment));
  return Status::Ok();
}

StringPiece LatticeDumpOutput::result() const { return impl_->buffer_; }

Status LatticeDumpOutput::initialize(const analysis::AnalyzerImpl* impl,
                                     const analysis::WeightBuffer* weights) {
  impl_.reset(new LatticeDumpOutputImpl);
  impl_->fillFeatures_ = fillFeatures_;
  impl_->fillBuffer_ = fillBuffer_;
  JPP_RETURN_IF_ERROR(impl_->initialize(impl, weights));
  return Status::Ok();
}

const LatticeDump* LatticeDumpOutput::objectPtr() const { return impl_->obj_; }

}  // namespace output
}  // namespace core
}  // namespace jumanpp