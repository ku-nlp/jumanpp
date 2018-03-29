//
// Created by Arseny Tolmachev on 2018/03/26.
//

#include "juman_pb_format.h"
#include <core/analysis/output.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "juman.pb.h"
#include "jumandic_id_resolver.h"
#include "jumanpp_pb_format.h"

namespace jumanpp {
namespace jumandic {

struct JumanPbFormatImpl {
  JumanSentence obj_;
  core::analysis::AnalysisPath path_;
  output::JumandicFields fields_;
  const JumandicIdResolver *idResolver_;
  std::string buffer_;
  bool fillBuffer_;

  Status initialize(const core::analysis::OutputManager &om,
                    const JumandicIdResolver *resolver, bool fill) {
    fillBuffer_ = fill;
    idResolver_ = resolver;
    JPP_RETURN_IF_ERROR(fields_.initialize(om));
    return Status::Ok();
  }

  static StringPiece ifEmpty(StringPiece first, StringPiece second) {
    if (first.empty()) {
      return second;
    }
    return first;
  }

  Status fillNode(JumanMorpheme *mrph,
                  const core::analysis::NodeWalker &walker) {
    JumandicPosId internalPos{
        fields_.pos.pointer(walker), fields_.subpos.pointer(walker),
        fields_.conjType.pointer(walker), fields_.conjForm.pointer(walker)};

    auto idPos = idResolver_->dicToJuman(internalPos);

    mrph->set_surface(fields_.surface[walker].str());
    mrph->set_reading(fields_.reading[walker].str());
    mrph->set_baseform(fields_.baseform[walker].str());

    auto cform = fields_.canonicForm[walker];
    if (!cform.empty()) {
      auto f = mrph->add_features();
      f->set_key("代表表記");
      f->set_value(cform.str());
    }

    auto features = fields_.features[walker];
    while (features.next()) {
      auto f = mrph->add_features();
      f->set_key(features.key().str());
      if (features.hasValue()) {
        f->set_value(features.value().str());
      }
    }

    auto pos = mrph->mutable_posinfo();
    pos->set_pos(idPos.pos);
    pos->set_subpos(idPos.subpos);
    pos->set_conj_type(idPos.conjType);
    pos->set_conj_form(idPos.conjForm);

    auto strPos = mrph->mutable_string_pos();
    strPos->set_pos(ifEmpty(fields_.pos[walker], "*").str());
    strPos->set_subpos(ifEmpty(fields_.subpos[walker], "*").str());
    strPos->set_conj_type(ifEmpty(fields_.conjForm[walker], "*").str());
    strPos->set_conj_form(ifEmpty(fields_.conjType[walker], "*").str());

    return Status::Ok();
  }

  Status format(const core::analysis::Analyzer &analyzer, StringPiece comment) {
    path_.reset();
    obj_.Clear();
    obj_.set_comment(comment.str());

    JPP_RETURN_IF_ERROR(path_.fillIn(analyzer.impl()->lattice()));

    core::analysis::NodeWalker walker;
    auto outputMgr = analyzer.output();

    while (path_.nextBoundary()) {
      core::analysis::ConnectionPtr cptr{};
      if (!path_.nextNode(&cptr)) {
        return JPPS_INVALID_STATE << "there were not even a single cptr at "
                                  << path_.currentBoundary();
      }

      if (!outputMgr.locate(cptr.latticeNodePtr(), &walker)) {
        return JPPS_INVALID_STATE << "no node";
      }

      if (!walker.next()) {
        return JPPS_INVALID_STATE << "can't walk to first node";
      }

      auto node = obj_.add_morphemes();
      JPP_RETURN_IF_ERROR(fillNode(node, walker));

      while (walker.next()) {
        JPP_RETURN_IF_ERROR(fillNode(node->add_variants(), walker));
      }

      while (path_.nextNode(&cptr)) {
        if (!outputMgr.locate(cptr.latticeNodePtr(), &walker)) {
          return JPPS_INVALID_STATE << "no node";
        }
        while (walker.next()) {
          JPP_RETURN_IF_ERROR(fillNode(node->add_variants(), walker));
        }
      }
    }

    if (fillBuffer_) {
      buffer_.clear();
      google::protobuf::io::StringOutputStream os{&buffer_};
      google::protobuf::io::CodedOutputStream cos{&os};
      auto messageSize = obj_.ByteSizeLong();
      cos.WriteVarint64(messageSize);
      buffer_.reserve(messageSize);
      if (!obj_.SerializeToCodedStream(&cos)) {
        return JPPS_INVALID_STATE << "failed to serialize LatticeDump";
      }
    }

    return Status::Ok();
  }

  StringPiece result() { return buffer_; }
};

Status JumanPbFormat::initialize(
    const jumanpp::core::analysis::OutputManager &om,
    const jumanpp::jumandic::JumandicIdResolver *resolver, bool fill) {
  impl_.reset(new JumanPbFormatImpl);
  return impl_->initialize(om, resolver, fill);
}

const JumanSentence *JumanPbFormat::objectPtr() const { return &impl_->obj_; }

StringPiece JumanPbFormat::result() const { return impl_->buffer_; }

Status JumanPbFormat::format(const core::analysis::Analyzer &analyzer,
                             StringPiece comment) {
  return impl_->format(analyzer, comment);
}

JumanPbFormat::JumanPbFormat() noexcept {}

JumanPbFormat::~JumanPbFormat() {}

}  // namespace jumandic
}  // namespace jumanpp
