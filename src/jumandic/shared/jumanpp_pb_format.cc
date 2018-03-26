//
// Created by Arseny Tolmachev on 2018/03/23.
//

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <core/analysis/score_processor.h>
#include "jumanpp_pb_format.h"
#include "jumanpp.pb.h"
#include "juman_format.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace jumandic {

struct RankInfo {
  i32 rank;
  i32 beam;
};

struct LatticeNodeStats {
  util::InlinedVector<RankInfo, 4> ranks;
  util::InlinedVector<core::analysis::LatticeNodePtr, 4> previous;
  
  void addPrevious(const core::analysis::ConnectionPtr& ptr) {
    auto lptr = ptr.latticeNodePtr();
    if (lptr.boundary < 2) {
      return;
    }
    
    auto it = std::find(previous.begin(), previous.end(), lptr);
    if (it == previous.end()) {
      previous.push_back(lptr);
    }
  }
  
  void sortPrevious() {
    std::sort(previous.begin(), previous.end(), [](const core::analysis::LatticeNodePtr& p1, const core::analysis::LatticeNodePtr& p2) {
      if (p1.boundary == p2.boundary) {
        return p1.position < p2.position;
      }
      return p1.boundary < p2.boundary;
    });
  }
};

struct LatticeTopN {
  util::FlatMap<core::analysis::LatticeNodePtr, LatticeNodeStats> info_;
  std::vector<core::analysis::LatticeNodePtr> nodes_;
  std::vector<float> topScores_;

  void addNode(const core::analysis::ConnectionPtr& ptr, i32 rank, i32 beam) {
    auto lptr = ptr.latticeNodePtr();
    auto& info = info_[lptr];
    if (info.ranks.empty() || info.ranks.back().rank != rank) {
      info.ranks.push_back({rank, beam});
    }
    info.addPrevious(*ptr.previous);
  }

  void fillLattice(core::analysis::Lattice* lat, int topN) {
    info_.clear_no_resize();
    nodes_.clear();
    topScores_.clear();

    auto eos = lat->boundary(lat->createdBoundaryCount() - 1);
    auto beam = eos->starts()->beamData();
    auto maxN = std::min<i32>(static_cast<i32>(beam.size()), topN);
    for (int i = 0; i < maxN; ++i) {
      auto rank = i + 1;
      auto& el = beam.at(i);
      if (core::analysis::EntryBeam::isFake(el)) {
        break;
      }

      topScores_.push_back(el.totalScore);

      i32 beamIdx = el.ptr.beam;
      const core::analysis::ConnectionPtr* ptr = el.ptr.previous;
      while (ptr->boundary >= 2) {
        addNode(*ptr, rank, beamIdx);
        beamIdx = ptr->beam;
        ptr = ptr->previous;
      }
    }

    for (auto& it: info_) {
      nodes_.push_back(it.first);
      it.second.sortPrevious();
    }
    std::sort(nodes_.begin(), nodes_.end(), [](const core::analysis::LatticeNodePtr& p1, const core::analysis::LatticeNodePtr& p2) {
      if (p1.boundary == p2.boundary) {
        return p1.position < p2.position;
      }
      return p1.boundary < p2.boundary;
    });
  }
};

struct JumanppProtobufOutputImpl {
  Lattice object_;
  std::string buffer_;
  bool fillBuffer_ = true;
  output::JumandicFields fields_;
  i32 maxN_;
  LatticeTopN topN_;
  const JumandicIdResolver* idResolver_;

  Status initialize(const core::analysis::OutputManager &om, const JumandicIdResolver* resolver, i32 topN, bool fill) {
    fillBuffer_ = fill;
    idResolver_ = resolver;
    maxN_ = topN;
    JPP_RETURN_IF_ERROR(fields_.initialize(om));
    return Status::Ok();
  }

  static StringPiece ifEmpty(StringPiece first, StringPiece second) {
    if (first.empty()) {
      return second;
    }
    return first;
  }

  Status format(const core::analysis::Analyzer &analyzer, StringPiece comment) {
    auto lat = analyzer.impl()->lattice();
    topN_.fillLattice(lat, maxN_);
    object_.Clear();
    object_.set_comment(comment.str());
    
    core::analysis::NodeWalker walker;
    auto outputMgr = analyzer.output();

    for (auto& lptr: topN_.nodes_) {      
      if (!outputMgr.locate(lptr, &walker)) {
        return JPPS_INVALID_STATE << "failed to locate node: " << lptr;
      }
      
      auto& info = topN_.info_[lptr];
      
      u32 nodeId = lptr.boundary * 10000u + lptr.position;
      auto bnd = lat->boundary(lptr.boundary);
      auto bndStart = bnd->starts();
      auto& theInfo = bndStart->nodeInfo().at(lptr.position);
      
      while (walker.next()) {
        auto node = object_.add_nodes();
        node->set_nodeid(nodeId);
        for (auto& prev: info.previous) {
          node->add_prevnodes(prev.boundary * 10000u + prev.position);
        }

        node->set_startindex(theInfo.start());
        node->set_endindex(theInfo.end());
        node->set_surface(fields_.surface[walker].str());
        node->set_reading(fields_.reading[walker].str());
        node->set_dicform(fields_.baseform[walker].str());

        auto cform = fields_.canonicForm[walker];
        if (cform.empty()) {
          std::string canonic = node->dicform();
          canonic += '/';
          canonic += node->reading();
          node->set_canonic(std::move(canonic));
        } else {
          node->set_canonic(cform.str());
        }

        auto iter = fields_.features[walker];
        while (iter.next()) {
          auto feature = node->add_features();
          feature->set_key(iter.key().str());
          feature->set_value(iter.value().str());
        }
        
        auto scores = node->mutable_scores();

        JumandicPosId internalPos {
          fields_.pos.pointer(walker),
          fields_.subpos.pointer(walker),
          fields_.conjType.pointer(walker),
          fields_.conjForm.pointer(walker)
        };

        auto idPos = idResolver_->dicToJuman(internalPos);

        auto pos = node->mutable_pos();
        pos->set_pos(idPos.pos);
        pos->set_subpos(idPos.subpos);
        pos->set_conjtype(idPos.conjType);
        pos->set_conjform(idPos.conjForm);

        auto strPos = node->mutable_string_pos();
        strPos->set_pos(ifEmpty(fields_.pos[walker], "*").str());
        strPos->set_subpos(ifEmpty(fields_.subpos[walker], "*").str());
        strPos->set_conjform(ifEmpty(fields_.conjForm[walker], "*").str());
        strPos->set_conjtype(ifEmpty(fields_.conjType[walker], "*").str());

        for (auto& rankInfo: info.ranks) {
          node->add_rank(rankInfo.rank);
          auto& beam = bndStart->beamData().row(lptr.position).at(rankInfo.beam);
          scores->add_cumulative_scores(beam.totalScore);
          auto rawScores = bnd->scores()->forPtr(beam.ptr);
          auto detail = scores->add_score_details();
          detail->set_linear(rawScores.at(0));
          for (int i = 1; i < rawScores.size(); ++i) {
            detail->add_additional(rawScores.at(i));
          }
        }
      }
    }
    
    if (fillBuffer_) {
      buffer_.clear();
      google::protobuf::io::StringOutputStream os{&buffer_};
      google::protobuf::io::CodedOutputStream cos{&os};
      auto messageSize = object_.ByteSizeLong();
      cos.WriteVarint64(messageSize);
      buffer_.reserve(messageSize);
      if (!object_.SerializeToCodedStream(&cos)) {
        return JPPS_INVALID_STATE << "failed to serialize LatticeDump";
      }
    }

    return Status::Ok();
  }
};


Status JumanppProtobufOutput::format(const core::analysis::Analyzer &analyzer, StringPiece comment) {
  return impl_->format(analyzer, comment);
}

StringPiece JumanppProtobufOutput::result() const {
  return impl_->buffer_;
}

const Lattice *JumanppProtobufOutput::objectPtr() const {
  return &impl_->object_;
}

Status JumanppProtobufOutput::initialize(const core::analysis::OutputManager &om, const JumandicIdResolver* resolver, int topN, bool fill) {
  impl_.reset(new JumanppProtobufOutputImpl);
  return impl_->initialize(om, resolver, topN, fill);
}
} // namespace jumandic
} // namespace jumanpp