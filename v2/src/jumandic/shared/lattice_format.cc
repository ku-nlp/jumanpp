//
// Created by Arseny Tolmachev on 2017/07/21.
//

#include "lattice_format.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status LatticeFormatInfo::fillInfo(const core::analysis::Analyzer& an,
                                   i32 topN) {
  auto ai = an.impl();
  auto lat = ai->lattice();

  info.clear_no_resize();

  if (lat->createdBoundaryCount() <= 3) {
    return Status::Ok();
  }

  auto eosBnd = lat->boundary(lat->createdBoundaryCount() - 1);
  auto eosBeam = eosBnd->starts()->beamData();

  auto maxN = std::min<i32>(eosBeam.size(), topN);
  for (int i = 0; i < maxN; ++i) {
    auto el = eosBeam.at(i);
    auto ptr = el.ptr.previous;

    while (ptr->boundary >= 2) {
      info[ptr->latticeNodePtr()].addElem(*ptr, i);
      ptr = ptr->previous;
    }
  }

  return Status::Ok();
}

void LatticeFormatInfo::publishResult(std::vector<LatticeInfoView>* view) {
  view->clear();
  view->reserve(info.size());
  for (auto& pair : info) {
    view->push_back({pair.first, &pair.second});
    pair.second.fixPrevs();
  }
  std::sort(view->begin(), view->end(),
            [](const LatticeInfoView& v1, const LatticeInfoView& v2) {
              if (v1.nodePtr().boundary == v2.nodePtr().boundary) {
                return v1.nodePtr().position < v2.nodePtr().position;
              }
              return v1.nodePtr().boundary < v2.nodePtr().boundary;
            });
  i32 firstId = 1;
  for (auto& v : *view) {
    v.assignId(firstId);
    ++firstId;
  }
}

i32 LatticeFormatInfo::idOf(LatticeNodePtr ptr) const {
  auto iter = info.find(ptr);
  if (iter != info.end()) {
    return iter->second.id;
  }
  return 0;
}

Status LatticeFormat::format(const core::analysis::Analyzer& analyzer,
                             StringPiece comment) {
  printer.reset();
  auto lat = analyzer.impl()->lattice();
  if (lat->createdBoundaryCount() == 3) {
    printer.reset();
    printer << "EOS\n";
    return Status::Ok();
  }

  JPP_RETURN_IF_ERROR(latticeInfo.fillInfo(analyzer, topN));

  latticeInfo.publishResult(&infoView);

  auto& om = analyzer.output();

  auto& f = flds;

  if (comment.size() > 0) {
    printer << "# " << comment << '\n';
  } else {
    printer << "# MA-SCORE\t";

    auto eos = lat->boundary(lat->createdBoundaryCount() - 1);
    auto eosBeam = eos->starts()->beamData();

    for (int i = 0; i < topN; ++i) {
      auto& bel = eosBeam.at(i);
      printer << "rank" << (i + 1) << ':' << bel.totalScore << ' ';
    }
    printer << '\n';
  }

  for (auto& n : infoView) {
    if (!om.locate(n.nodePtr(), &walker)) {
      return Status::InvalidState()
             << "failed to locate node: " << n.nodePtr().boundary << ":"
             << n.nodePtr().position;
    }

    auto& weights = analyzer.scorer()->scoreWeights;

    auto ptrit = std::max_element(
        n.nodeInfo().ptrs.begin(), n.nodeInfo().ptrs.end(),
        [lat, &weights](const core::analysis::ConnectionPtr& p1,
                        const core::analysis::ConnectionPtr& p2) {
          auto b1 = lat->boundary(p1.boundary);
          auto c1 = b1->connection(p1.left);
          auto s1 = c1->entryScores(p1.beam).row(p1.right);
          auto b2 = lat->boundary(p2.boundary);
          auto c2 = b2->connection(p2.left);
          auto s2 = c2->entryScores(p2.beam).row(p2.right);

          float total1 = 0, total2 = 0;
          for (int i = 0; i < weights.size(); ++i) {
            total1 += s1[i] * weights[i];
            total2 += s2[i] * weights[i];
          }
          return total1 > total2;
        });

    auto& cptr = *ptrit;
    auto bnd = lat->boundary(cptr.boundary);

    auto& ninfo = bnd->starts()->nodeInfo().at(cptr.right);

    while (walker.next()) {
      printer << "-\t";
      printer << n.nodeInfo().id << '\t';
      auto& prevs = n.nodeInfo().prev;
      for (int i = 0; i < prevs.size(); ++i) {
        auto id = latticeInfo.idOf(prevs[i]);
        printer << id;
        if (i != prevs.size() - 1) {
          printer << ';';
        }
      }
      printer << '\t';
      auto position = cptr.boundary - 2;  // we have 2 BOS
      printer << position << '\t';
      printer << position + ninfo.numCodepoints() << '\t';
      printer << f.surface[walker] << '\t';
      auto canFrm = f.canonicForm[walker];
      if (canFrm.size() > 0) {
        printer << f.canonicForm[walker];
      } else {
        printer << f.baseform[walker] << '/' << f.reading[walker];
      }
      printer << '\t';
      printer << f.reading[walker] << '\t';
      printer << f.baseform[walker] << '\t';

      JumandicPosId rawId{fieldBuffer[1], fieldBuffer[2],
                          fieldBuffer[4],  // conjForm and conjType are reversed
                          fieldBuffer[3]};

      auto newId = idResolver.dicToJuman(rawId);

      printer << f.pos[walker] << '\t' << newId.pos << '\t';
      printer << ifEmpty(f.subpos[walker], "*") << '\t' << newId.subpos << '\t';
      printer << ifEmpty(f.conjType[walker], "*") << '\t' << newId.conjType
              << '\t';
      printer << ifEmpty(f.conjForm[walker], "*") << '\t' << newId.conjForm
              << '\t';

      auto features = f.features[walker];
      while (features.next()) {
        printer << features.key();
        if (features.hasValue()) {
          printer << ':' << features.value();
        }
        printer << '|';
      }

      auto conn = bnd->connection(cptr.left);
      auto scores = conn->entryScores(cptr.beam).row(cptr.right);

      JPP_DCHECK_GE(weights.size(), 1);
      float totalScore = scores[0] * weights[0];

      printer << "特徴量スコア:" << totalScore << '|';
      if (weights.size() == 2) {  // have RNN
        float rnnScore = scores[1] * weights[1];
        printer << "言語モデルスコア:" << rnnScore << '|';
        totalScore += rnnScore;
      }

      printer << "形態素解析スコア:" << totalScore << '|';

      printer << "ランク:";
      auto& ranks = n.nodeInfo().ranks;
      for (int i = 0; i < ranks.size(); ++i) {
        printer << ranks[i] + 1;
        if (i != ranks.size() - 1) {
          printer << ';';
        }
      }
      printer << '\n';
    }
  }

  printer << "EOS\n";

  return Status::Ok();
}

StringPiece LatticeFormat::result() const { return printer.result(); }

Status LatticeFormat::initialize(const core::analysis::OutputManager& om) {
  JPP_RETURN_IF_ERROR(flds.initialize(om));
  JPP_RETURN_IF_ERROR(idResolver.initialize(om.dic()));
  return Status::Ok();
}

void LatticeNodeInfo::addElem(const core::analysis::ConnectionPtr& cptr,
                              i32 rank) {
  ranks.push_back(static_cast<u16>(rank));
  ptrs.insert(cptr);
  auto prevptr = cptr.previous->latticeNodePtr();
  if (std::find(prev.begin(), prev.end(), prevptr) == prev.end()) {
    prev.push_back(prevptr);
  }
}

void LatticeNodeInfo::fixPrevs() {
  if (prev.size() > 1) {
    std::sort(prev.begin(), prev.end(),
              [](const LatticeNodePtr& p1, const LatticeNodePtr& p2) {
                if (p1.boundary == p2.boundary)
                  return p1.position < p2.position;
                return p1.boundary < p2.boundary;
              });
  }
}
}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp