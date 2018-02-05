//
// Created by Arseny Tolmachev on 2017/12/03.
//

#include <fstream>
#include <iostream>
#include "args.h"
#include "core/analysis/analyzer_impl.h"
#include "jpp_jumandic_cg.h"
#include "jumandic/shared/jumandic_env.h"

struct PathDiffConf {
  bool posAreOk;
  std::string modelFile;
  std::string inputFile;
  std::string outputFile;

  static PathDiffConf parse(int argc, const char* argv[]) {
    args::ArgumentParser parser{"Path Differ"};
    args::Positional<std::string> model{parser, "model", "model"};
    args::Positional<std::string> input{parser, "input", "input"};
    args::Flag negativeOnly{parser, "output pos", "output positive", {"pos"}};

    try {
      parser.ParseCLI(argc, argv);
    } catch (std::exception& e) {
      std::cerr << e.what();
      exit(1);
    }

    PathDiffConf inst;
    inst.modelFile = model.Get();
    inst.inputFile = input.Get();
    inst.posAreOk = negativeOnly.Get();
    return inst;
  }
};

using namespace jumanpp;

struct NamedStringField {
  std::string name;
  std::shared_ptr<core::analysis::StringField> field;
};

struct ScoredNode {
  core::analysis::LatticeNodePtr ptr;
  float score;
};

struct RenderContext {
  util::FlatMap<i32, NamedStringField> fields;
  i32 surfaceIdx;
  const core::analysis::OutputManager* om;

  Status init(const core::CoreHolder& core,
              const core::analysis::OutputManager& om) {
    auto& spec = core.spec();
    for (auto& trFld : spec.training.fields) {
      auto& dicFld = spec.dictionary.fields[trFld.fieldIdx];
      if (trFld.weight != 0) {
        auto& fld = fields[trFld.dicIdx];
        fld.field.reset(new core::analysis::StringField);
        JPP_RIE_MSG(om.stringField(dicFld.name, fld.field.get()),
                    "field=" << dicFld.name);
        fld.name = dicFld.name;
      }
    }

    surfaceIdx = spec.training.fields[spec.training.surfaceIdx].dicIdx;
    this->om = &om;
    return Status::Ok();
  }

  bool renderFull(std::ostream& os, core::analysis::LatticeNodePtr lnp,
                  StringPiece prefix) {
    auto w = om->nodeWalker();
    JPP_RET_CHECK(om->locate(lnp, &w));

    auto& sfld = fields[surfaceIdx];

    auto surface = (*sfld.field)[w];

    os << prefix << surface;
    for (auto& f : fields) {
      if (f.first == surfaceIdx) {
        continue;
      }

      auto value = (*f.second.field)[w];
      if (value.empty()) {
        continue;
      }

      if (value == surface) {
        continue;
      }

      os << "\t" << f.second.name << ":" << value;
    }

    return true;
  }

  bool renderSurface(std::ostream& os, core::analysis::LatticeNodePtr lnp) {
    auto w = om->nodeWalker();
    JPP_RET_CHECK(om->locate(lnp, &w));

    auto& sfld = fields[surfaceIdx];

    auto surface = (*sfld.field)[w];
    os << surface;

    return true;
  }
};

struct DiffNode {
  std::vector<ScoredNode> left;
  std::vector<ScoredNode> right;
  bool isEqual = true;

  bool isEqualNode() const { return isEqual; }

  void addLeft(const core::analysis::ConnectionPtr* ptr,
               const core::analysis::Lattice* l) {
    left.push_back(makeNode(ptr, l));
  }

  void addRight(const core::analysis::ConnectionPtr* ptr,
                const core::analysis::Lattice* l) {
    right.push_back(makeNode(ptr, l));
  }

  static ScoredNode makeNode(const core::analysis::ConnectionPtr* ptr,
                             const core::analysis::Lattice* l) {
    auto bnd = l->boundary(ptr->boundary)->starts();
    auto beam = bnd->beamData().row(ptr->right).at(ptr->beam);
    return {ptr->latticeNodePtr(), beam.totalScore};
  }

  std::ostream& render(std::ostream& o, RenderContext& leftCtx,
                       bool writeRight) const {
    if (left.empty()) {
      return o;
    }

    if (isEqual) {
      o << "\n";
    }
    for (auto it = left.rbegin(); it != left.rend(); ++it) {
      if (isEqual) {
        leftCtx.renderSurface(o, it->ptr);
      } else {
        leftCtx.renderFull(o, it->ptr, "\n\t");
      }
    }

    return o;
  }

  std::ostream& renderInvalid(std::ostream& o, RenderContext& leftCtx,
                              bool writeRight) const {
    for (auto it = right.rbegin(); it != right.rend(); ++it) {
      if (!isEqual) {
        leftCtx.renderFull(o, it->ptr, "\n# ");
      }
    }

    return o;
  }
};

struct PathDiffCalculator {
  jumanpp::core::JumanppEnv env;
  jumanpp::core::analysis::Analyzer full;
  jumanpp::core::analysis::Analyzer gbeam;
  RenderContext leftCtx;
  bool outputPos;

  core::analysis::rnn::RnnInferenceConfig rnnConf;

  std::vector<DiffNode> nodes;
  float topLeft;
  float topRight;

  PathDiffCalculator() {
    rnnConf.rnnWeight = 0;
    rnnConf.perceptronWeight = 1;
  }

  Status init(const PathDiffConf& conf) {
    JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile));
    env.setRnnConfig(rnnConf);
    jumanpp_generated::JumandicStatic staticFeatures;
    JPP_RETURN_IF_ERROR(env.initFeatures(&staticFeatures));

    env.setBeamSize(10);
    JPP_RETURN_IF_ERROR(env.makeAnalyzer(&full));
    env.setBeamSize(5);
    env.setGlobalBeam(5, 1, 5);
    JPP_RETURN_IF_ERROR(env.makeAnalyzer(&gbeam));

    JPP_RETURN_IF_ERROR(leftCtx.init(*env.coreHolder(), full.output()));
    outputPos = conf.posAreOk;

    return Status::Ok();
  }

  void fillDiff(const core::analysis::Lattice* l1,
                const core::analysis::Lattice* l2,
                const core::analysis::ConnectionPtr* top1x,
                const core::analysis::ConnectionPtr* top2x) {
    nodes.emplace_back();
    auto* b = &nodes.back();

    auto top1 = top1x;
    auto top2 = top2x;

    while (top1->boundary >= 2 && top2->boundary >= 2) {
      if (top1->latticeNodePtr() == top2->latticeNodePtr()) {
        if (!b->isEqualNode()) {
          nodes.emplace_back();
          b = &nodes.back();
        }
        if (top1 != top1x) {
          b->addLeft(top1, l1);
        }
        top1 = top1->previous;
        top2 = top2->previous;
      } else {
        if (b->isEqualNode()) {
          nodes.emplace_back();
          b = &nodes.back();
          b->isEqual = false;
        }
        if (top1->boundary == top2->boundary) {
          b->addLeft(top1, l1);
          b->addRight(top2, l2);
          top1 = top1->previous;
          top2 = top2->previous;
        } else if (top1->boundary > top2->boundary) {
          b->addLeft(top1, l1);
          top1 = top1->previous;
        } else {
          b->addRight(top2, l2);
          top2 = top2->previous;
        }
      }
    }
  }

  bool hasUnks(const core::analysis::ConnectionPtr* element,
               const core::analysis::Lattice* lat) {
    while (element->boundary > 1) {
      auto bnd = lat->boundary(element->boundary)->starts();
      auto& info = bnd->nodeInfo().at(element->right);
      if (info.entryPtr().isSpecial()) {
        return true;
      }
      element = element->previous;
    }

    return false;
  }

  Status computeDiff(StringPiece sentence) {
    JPP_RETURN_IF_ERROR(full.analyze(sentence));
    JPP_RETURN_IF_ERROR(gbeam.analyze(sentence));
    auto l1 = full.impl()->lattice();
    auto l2 = gbeam.impl()->lattice();

    auto top1 = l1->boundary(l1->createdBoundaryCount() - 1)
                    ->starts()
                    ->beamData()
                    .at(0);
    auto top2 = l2->boundary(l1->createdBoundaryCount() - 1)
                    ->starts()
                    ->beamData()
                    .at(0);

    nodes.clear();

    if (hasUnks(top1.ptr.previous, l1) || hasUnks(top2.ptr.previous, l2)) {
      // unks should not be here
      return Status::Ok();
    }

    if (outputPos && top2.totalScore > -0.1f) {
      return Status::Ok();  // known-ish example
    }

    if (top1.totalScore - top2.totalScore <= 0.1f) {
      return Status::Ok();  // scores are not distant enough
    }

    fillDiff(l1, l2, &top1.ptr, &top2.ptr);
    topLeft = top1.totalScore;
    topRight = top2.totalScore;

    return Status::Ok();
  }

  void renderDiff(std::ostream& os) {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
      it->render(os, leftCtx, false);
    }
  }

  void renderInvalid(std::ostream& os) {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
      it->renderInvalid(os, leftCtx, false);
    }
  }
};

int main(int argc, const char* argv[]) {
  auto conf = PathDiffConf::parse(argc, argv);

  PathDiffCalculator calc;
  Status s = calc.init(conf);
  if (!s) {
    std::cerr << s;
    return 1;
  }

  std::ifstream ifs{conf.inputFile};

  std::string comment;
  std::string data;
  i64 line = 0;
  while (std::getline(ifs, data)) {
    ++line;
    if (data.size() > 2 && data[0] == '#' && data[1] == ' ') {
      comment.assign(data.begin() + 2, data.end());
      continue;
    }

    s = calc.computeDiff(data);
    if (!s) {
      std::cerr << "failed to analyze " << comment << " [" << data << "]: " << s
                << "\n";
      continue;
    }
    if (!calc.nodes.empty()) {
      std::cout << "# scores: " << calc.topLeft << " " << calc.topRight;
      calc.renderInvalid(std::cout);
      if (!comment.empty()) {
        std::cout << "\n# " << comment;
      } else {
        std::cout << "\n# line-" << line;
      }
      calc.renderDiff(std::cout);
      std::cout << "\n\n";
    }
    comment.clear();
  }

  return 0;
}