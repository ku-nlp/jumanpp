//
// Created by Arseny Tolmachev on 2018/01/19.
//

#include "partial_example_io.h"
#include "core/analysis/unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace input {

Status PartialExampleReader::initialize(TrainFieldsIndex* tio,
                                        char32_t noBreakToken) {
  tio_ = tio;
  fields_.clear();
  noBreakToken_ = noBreakToken;
  for (auto& x : tio->fields()) {
    fields_.insert(std::make_pair(x.name, &x));
  }

  return Status::Ok();
}

Status PartialExampleReader::readExample(PartialExample* result, bool* eof) {
  result->file_ = filename_;
  bool firstLine = true;
  i32 boundary = 2;
  result->boundaries_.clear();
  result->noBreak_.clear();
  result->surface_.clear();
  result->nodes_.clear();
  while (csv_.nextLine()) {
    if (firstLine) {
      result->line_ = csv_.lineNumber();
      firstLine = false;
    }

    auto wholeLine = csv_.line();
    if (wholeLine.size() > 2 && wholeLine[0] == '#' && wholeLine[1] == ' ') {
      wholeLine.slice(2, wholeLine.size() - 1).assignTo(result->comment_);
      continue;
    }

    if (csv_.numFields() == 1) {
      auto data = csv_.field(0);
      if (data.empty()) {
        auto& bnds = result->boundaries_;
        if (!bnds.empty()) {
          bnds.erase(bnds.end() - 1, bnds.end());
        }
        *eof = false;
        return Status::Ok();
      }
      codepts_.clear();
      JPP_RIE_MSG(chars::preprocessRawData(data, &codepts_),
                  "at " << filename_ << ":" << csv_.lineNumber());

      for (auto& codept : codepts_) {
        if (codept.codepoint == noBreakToken_) {
          result->noBreak_.push_back(boundary);
        } else {
          result->surface_.append(codept.bytes.begin(), codept.bytes.end());
          boundary += 1;
        }
      }

      result->boundaries_.push_back(boundary);
      continue;
    }

    if (!csv_.field(0).empty()) {
      return JPPS_INVALID_PARAMETER
             << "in file: " << filename_ << ":" << csv_.lineNumber()
             << " first field was not empty, but" << csv_.field(0);
    }

    NodeConstraint nc{};

    auto surface = csv_.field(1);
    codepts_.clear();
    JPP_RIE_MSG(chars::preprocessRawData(surface, &codepts_),
                surface << " at " << filename_ << ":" << csv_.lineNumber());
    surface.assignTo(nc.surface);
    nc.length = static_cast<i32>(codepts_.size());
    for (int i = 1; i < nc.length; ++i) {
      result->noBreak_.push_back(boundary + i);
    }
    nc.boundary = boundary;
    boundary += nc.length;
    result->surface_.append(nc.surface);
    result->boundaries_.push_back(boundary);

    for (int idx = 2; idx < csv_.numFields(); ++idx) {
      auto fldData = csv_.field(idx);
      auto it = std::find(fldData.begin(), fldData.end(), ':');
      if (it == fldData.end()) {
        return JPPS_INVALID_PARAMETER
               << "in file: " << filename_ << ":" << csv_.lineNumber()
               << " an entry [" << fldData
               << "] did not contain field name (<name>:<value>)";
      }
      StringPiece fldName = StringPiece{fldData.begin(), it};
      StringPiece fldValue = StringPiece{it + 1, fldData.end()};
      codepts_.clear();
      JPP_RIE_MSG(chars::preprocessRawData(fldValue, &codepts_),
                  fldValue << " at " << filename_ << ":" << csv_.lineNumber());
      auto mapIter = fields_.find(fldName);
      if (mapIter == fields_.end()) {
        return JPPS_INVALID_PARAMETER
               << "in file: " << filename_ << ":" << csv_.lineNumber()
               << " the field name of an entry [" << fldData
               << "] was not present in the dictionary spec";
      }

      auto idmap = mapIter->second->str2int;
      auto idValue = idmap->find(fldValue);
      if (idValue == idmap->end()) {
        auto id = analysis::hashUnkString(fldValue);
        nc.tags.push_back(TagConstraint{mapIter->second->dicFieldIdx, id});
      } else {
        nc.tags.push_back(
            TagConstraint{mapIter->second->dicFieldIdx, idValue->second});
      }
    }

    result->nodes_.push_back(std::move(nc));
  }
  *eof = true;
  return Status::Ok();
}

Status PartialExampleReader::setData(StringPiece data) {
  filename_ = "<memory>";
  return csv_.initFromMemory(data);
}

Status PartialExampleReader::openFile(StringPiece filename) {
  JPP_RETURN_IF_ERROR(file_.open(filename));
  JPP_RETURN_IF_ERROR(setData(file_.contents()));
  filename.assignTo(filename_);
  return Status::Ok();
}

}  // namespace input
}  // namespace core
}  // namespace jumanpp