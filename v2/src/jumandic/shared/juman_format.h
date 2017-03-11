//
// Created by Arseny Tolmachev on 2017/03/09.
//

#ifndef JUMANPP_JUMAN_FORMAT_H
#define JUMANPP_JUMAN_FORMAT_H

#include <array>
#include <sstream>
#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer.h"
#include "core/analysis/output.h"
#include "jumandic/shared/jumandic_spec.h"
#include "util/printer.h"

namespace jumanpp {
namespace jumandic {
namespace output {

struct JumandicFields {
  core::analysis::StringField surface;
  core::analysis::StringField pos;
  core::analysis::StringField subpos;
  core::analysis::StringField conjType;
  core::analysis::StringField conjForm;
  core::analysis::StringField baseform;
  core::analysis::StringField reading;
  core::analysis::StringField canonicForm;
  core::analysis::StringListField features;

  Status initialize(const core::analysis::OutputManager& om) {
    JPP_RETURN_IF_ERROR(om.stringField("surface", &surface));
    JPP_RETURN_IF_ERROR(om.stringField("pos", &pos));
    JPP_RETURN_IF_ERROR(om.stringField("subpos", &subpos));
    JPP_RETURN_IF_ERROR(om.stringField("conjtype", &conjType));
    JPP_RETURN_IF_ERROR(om.stringField("conjform", &conjForm));
    JPP_RETURN_IF_ERROR(om.stringField("baseform", &baseform));
    JPP_RETURN_IF_ERROR(om.stringField("reading", &reading));
    JPP_RETURN_IF_ERROR(om.stringField("canonic", &canonicForm));
    JPP_RETURN_IF_ERROR(om.stringListField("features", &features));
    return Status::Ok();
  }
};

class JumanFormat {
  JumandicFields flds;
  util::io::Printer printer;
  core::analysis::AnalysisResult analysisResult;
  core::analysis::AnalysisPath top1;
  std::array<i32, JumandicNumFields> fieldBuffer;
  core::analysis::NodeWalker walker;

 public:
  JumanFormat() : walker{&fieldBuffer} {}

  Status initialize(const core::analysis::OutputManager& om) {
    return flds.initialize(om);
  }

  bool formatOne(const core::analysis::OutputManager& om,
                 const core::analysis::ConnectionPtr& ptr, bool first);
  Status format(const core::analysis::Analyzer& analysis);
  StringPiece result() const { return printer.result(); }
};

}  // output
}  // jumandic
}  // jumanpp

#endif  // JUMANPP_JUMAN_FORMAT_H
