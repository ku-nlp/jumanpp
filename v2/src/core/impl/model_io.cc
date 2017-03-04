//
// Created by Arseny Tolmachev on 2017/03/04.
//

#include "model_io.h"
#include "model_format_ser.h"
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
  std::vector<util::MappedFileFragment> fragments;
};

ModelSaver::ModelSaver() {}

ModelSaver::~ModelSaver() {}

Status ModelSaver::open(StringPiece name) {
  if (file_) {
    return Status::InvalidState() << "another file, " << file_->name
                                  << " is already opened";
  }
  file_.reset(new ModelFile);
  JPP_RETURN_IF_ERROR(file_->mmap.open(name, util::MMapType::ReadWrite));
  return Status::Ok();
}

Status ModelSaver::save(const ModelInfo& info) {
  if (!file_) {
    return Status::InvalidState() << "can't save model, file is not opened";
  }

  ModelInfoRaw raw;

  raw.runtimeHash = info.runtimeHash;
  raw.specHash = info.specHash;

  size_t offset = 4096;
  for (auto& part : info.parts) {
    ModelPartRaw rawPart;
    rawPart.kind = part.kind;

    for (auto& buf : part.data) {
      util::MappedFileFragment frag;
      JPP_RETURN_IF_ERROR(file_->mmap.map(&frag, offset, buf.size()));
      std::memcpy(frag.address(), buf.data(), buf.size());
      rawPart.data.push_back(BlockPtr{offset, buf.size()});
      offset = util::memory::Align(offset + buf.size(), 4096);
    }
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

  return Status::Ok();
}

FilesystemModel::FilesystemModel() {}

FilesystemModel::~FilesystemModel() {}

Status FilesystemModel::open(StringPiece name) {
  if (file_) {
    return Status::InvalidState() << "another file, " << file_->name
                                  << " is already opened";
  }
  file_.reset(new ModelFile);
  JPP_RETURN_IF_ERROR(file_->mmap.open(name, util::MMapType::ReadOnly));
  return Status::Ok();
}

Status FilesystemModel::load(ModelInfo* info) {
  if (!file_) {
    return Status::InvalidState() << "can't load model, file is not opened";
  }

  util::MappedFileFragment hdrFrag;
  JPP_RETURN_IF_ERROR(file_->mmap.map(&hdrFrag, 0, 4096));
  auto sp = hdrFrag.asStringPiece();
  if (!std::strncmp(sp.char_begin(), ModelMagic, sizeof(ModelMagic))) {
    return Status::InvalidState() << "model file " << file_->name
                                  << " has corrupted header";
  }

  auto rest = sp.slice(sizeof(ModelMagic), 4096 - sizeof(ModelMagic));
  util::CodedBufferParser cbp{rest};
  u64 hdrSize = 0;
  if (!cbp.readVarint64(&hdrSize)) {
    return Status::InvalidState() << "could not read header size from "
                                  << file_->name;
  }

  util::serialization::Loader l{
      rest.slice(cbp.position(), cbp.position() + hdrSize)};
  ModelInfoRaw mir;
  if (!l.load(&mir)) {
    return Status::InvalidState() << "model file " << file_->name
                                  << " has corrupted model header";
  }

  info->specHash = mir.specHash;
  info->runtimeHash = mir.runtimeHash;

  auto& parts = info->parts;
  for (auto& p : mir.parts) {
    parts.emplace_back();
    auto& part = parts.back();
    part.kind = p.kind;

    for (auto& raw : p.data) {
      file_->fragments.emplace_back();
      auto frag = &file_->fragments.back();
      JPP_RETURN_IF_ERROR(file_->mmap.map(frag, raw.offset, raw.size));
      part.data.push_back(frag->asStringPiece());
    }
  }

  return Status::Ok();
}

}  // model
}  // core
}  // jumanpp