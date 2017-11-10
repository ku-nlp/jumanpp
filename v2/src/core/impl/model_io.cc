//
// Created by Arseny Tolmachev on 2017/03/04.
//

#include "model_io.h"
#include <util/printer.h>
#include <cstring>
#include <iostream>
#include "model_format_ser.h"
#include "util/debug_output.h"
#include "util/memory.hpp"
#include "util/mmap.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace model {

static const char ModelMagic[] = "jp2Mdl!";

struct ModelFile {
  StringPiece name;
  util::MappedFile mmap;
  util::MappedFileFragment fragment;
  ModelInfoRaw rawModel;
};

ModelSaver::ModelSaver() {}

ModelSaver::~ModelSaver() {}

Status ModelSaver::open(StringPiece name) {
  if (file_) {
    return Status::InvalidState()
           << "another file, " << file_->name << " is already opened";
  }
  file_.reset(new ModelFile);
  JPP_RETURN_IF_ERROR(file_->mmap.open(name, util::MMapType::ReadWrite));
  file_->name = name;
  return Status::Ok();
}

Status ModelSaver::save(const ModelInfo& info) {
  if (!file_) {
    return Status::InvalidState() << "can't save model, file is not opened";
  }

  ModelInfoRaw raw{};
  size_t offset = 4096;
  for (auto& part : info.parts) {
    ModelPartRaw rawPart;
    rawPart.kind = part.kind;
    rawPart.start = offset;

    for (auto& buf : part.data) {
      util::MappedFileFragment frag;
      JPP_RETURN_IF_ERROR(file_->mmap.map(&frag, offset, buf.size()));
      std::memcpy(frag.address(), buf.data(), buf.size());
      rawPart.data.push_back(BlockPtr{offset, buf.size()});
      offset = util::memory::Align(offset + buf.size(), 4096);
    }

    rawPart.end = offset;
    raw.parts.push_back(rawPart);
  }

  util::serialization::Saver sv;
  sv.save(raw);
  auto p1 = sv.result();

  auto p1s = p1.size();
  if (p1s > 4080) {
    return Status::NotImplemented()
           << " model header size >4080 bytes is not implemented";
  }

  util::CodedBuffer cb;
  cb.writeVarint(p1s);

  util::MappedFileFragment frag;
  JPP_RETURN_IF_ERROR(file_->mmap.map(&frag, 0, 4096));
  auto slice = frag.asMutableSlice();
  char* begin = slice.begin();
  std::memcpy(begin, ModelMagic, sizeof(ModelMagic));
  begin += sizeof(ModelMagic);
  std::memcpy(begin, cb.contents().begin(), cb.position());
  begin += cb.position();
  std::memcpy(begin, p1.data(), p1.size());

  JPP_RETURN_IF_ERROR(frag.flush());
  frag.unmap();

  return Status::Ok();
}

FilesystemModel::FilesystemModel() {}

FilesystemModel::~FilesystemModel() {}

Status FilesystemModel::open(StringPiece name) {
  if (file_) {
    return Status::InvalidState()
           << "another file, " << file_->name << " is already opened";
  }
  file_.reset(new ModelFile);
  JPP_RETURN_IF_ERROR(file_->mmap.open(name, util::MMapType::ReadOnly));
  file_->name = name;

  auto& hdrFrag = file_->fragment;
  JPP_RETURN_IF_ERROR(file_->mmap.map(&file_->fragment, 0, file_->mmap.size(), true));
  auto sp = hdrFrag.asStringPiece();
  auto magicSp = StringPiece{ModelMagic};
  if (sp.take(magicSp.size()) != magicSp) {
    return Status::InvalidState()
      << "model file " << file_->name << " has corrupted header";
  }

  auto rest = sp.slice(sizeof(ModelMagic), 4096 - sizeof(ModelMagic));
  util::CodedBufferParser cbp{rest};
  u64 hdrSize = 0;
  if (!cbp.readVarint64(&hdrSize)) {
    return Status::InvalidState()
      << "could not read header size from " << file_->name;
  }

  util::serialization::Loader l{
    rest.slice(cbp.numReadBytes(), cbp.numReadBytes() + hdrSize)};
  if (!l.load(&file_->rawModel)) {
    return Status::InvalidState()
      << "model file " << file_->name << " has corrupted model header";
  }

  return Status::Ok();
}

Status FilesystemModel::load(ModelInfo* info) {
  if (!file_) {
    return Status::InvalidState() << "can't load model, file is not opened";
  }

  auto& parts = info->parts;
  auto& mir = file_->rawModel;
  auto modelData = file_->fragment.asStringPiece();
  for (auto& p : mir.parts) {
    parts.emplace_back();
    auto& part = parts.back();
    part.kind = p.kind;
    for (auto& raw : p.data) {
      ptrdiff_t start = static_cast<ptrdiff_t>(raw.offset);
      ptrdiff_t end = static_cast<ptrdiff_t>(start + raw.size);
      part.data.push_back(modelData.slice(start, end));
    }
  }

  return Status::Ok();
}

StringPiece FilesystemModel::name() const {
  if (file_) {
    return file_->name;
  }
  return StringPiece("<not opened>");
}

void FilesystemModel::renderInfo() {
  util::io::Printer p;
  if (file_) {
    ModelInfo info;
    if (!load(&info)) {
      return;
    }

    for (int partNo = 0; partNo < info.parts.size(); ++partNo) {
      auto& mp = info.parts[partNo];
      auto& rawPart = file_->rawModel.parts[partNo];

      switch (mp.kind) {
        case ModelPartKind::Dictionary: {
          break;
        }
        case ModelPartKind::Perceprton: {
          break;
        }
        case ModelPartKind::Rnn: {
          break;
        }
        default: { p << "\nUnsupported Segment Type"; }
      }
    }
  }
  std::cerr << p.result();
}

}  // namespace model
}  // namespace core
}  // namespace jumanpp