//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "mikolov_rnn.h"
#include "mikolov_rnn_impl.h"
#include "util/csv_reader.h"
#include "util/lazy.h"
#include "util/memory.hpp"
#include "util/mmap.h"

namespace jumanpp {
namespace rnn {
namespace mikolov {

struct PackedHeader {
  u64 sizeVersion;
  u64 maxEntTableSize;
  u32 maxentOrder;
  bool useNce;
  float nceLnz;
  bool reversedSentence;
  char layerType[LayerNameMaxSize];
  u32 layerCount;
  u32 hsArity;
};

template <typename T, typename P>
size_t toMember(P T::*ptr, T* base, const char* data, size_t offset) {
  P* dst = &(base->*ptr);
  std::memcpy(dst, data + offset, sizeof(P));  // unaligned read
  return offset + sizeof(P);
}

Status readHeader(StringPiece data, MikolovRnnModelHeader* header,
                  size_t* offset) {
  PackedHeader packed;

  std::memset(&packed, 0, sizeof(PackedHeader));
  size_t off = 0;
  off = toMember(&PackedHeader::sizeVersion, &packed, data.data(), off);
  off = toMember(&PackedHeader::maxEntTableSize, &packed, data.data(), off);
  off = toMember(&PackedHeader::maxentOrder, &packed, data.data(), off);
  off = toMember(&PackedHeader::useNce, &packed, data.data(), off);
  off = toMember(&PackedHeader::nceLnz, &packed, data.data(), off);
  off = toMember(&PackedHeader::reversedSentence, &packed, data.data(), off);
  off = toMember(&PackedHeader::layerType, &packed, data.data(), off);
  off = toMember(&PackedHeader::layerCount, &packed, data.data(), off);
  off = toMember(&PackedHeader::hsArity, &packed, data.data(), off);

  auto sv = packed.sizeVersion;
  auto vers = sv / VersionStepSize;
  if (vers != 6) {
    return JPPS_INVALID_PARAMETER << "invalid rnn model version " << vers
                                  << " can handle only 6";
  }

  if (!packed.useNce) {
    return JPPS_INVALID_PARAMETER
           << "model was trained without nce, we support only nce models";
  }

  auto piece = StringPiece::fromCString(packed.layerType);
  if (piece != "sigmoid") {
    return JPPS_INVALID_PARAMETER
           << "only sigmoid activation is supported, model had " << piece;
  }

  header->layerSize = static_cast<u32>(packed.sizeVersion % VersionStepSize);
  header->nceLnz = packed.nceLnz;
  header->maxentOrder = packed.maxentOrder;
  header->maxentSize = packed.maxEntTableSize;
  *offset = off;

  return Status::Ok();
}

void MikolovRnn::apply(StepData* data) {
  MikolovRnnImpl impl{*this};
  impl.apply(data);
}

Status MikolovRnn::init(const MikolovRnnModelHeader& header,
                        const util::ArraySlice<float>& weights,
                        const util::ArraySlice<float>& maxentW) {
  if (!isAligned(weights.data(), 64)) {
    return Status::InvalidState() << "weight matrix must be 64-aligned";
  }

  if (!isAligned(maxentW.data(), 64)) {
    return Status::InvalidState() << "maxent weights must be 64-aligned";
  }

  this->weights = weights;
  this->maxentWeights = maxentW;
  this->header = header;
  this->rnnNceConstant = header.nceLnz;
  return Status::Ok();
}

void MikolovRnn::applyParallel(ParallelStepData* data) const {
  MikolovRnnImplParallel impl{*this};
  impl.apply(data);
}

void MikolovRnn::computeNewParCtx(ParallelContextData* pcd) const {
  MikolovRnnImplParallel impl{*this};
  impl.computeNewContext(*pcd);
}

template <typename T>
struct FreeDeleter {
  void operator()(T* o) { free(o); }
};

struct MikolovModelReaderData {
  util::MappedFile rnnModel;
  util::MappedFile rnnDictionary;
  util::MappedFileFragment modelFrag;
  util::MappedFileFragment dictFrag;
  MikolovRnnModelHeader header;
  std::vector<StringPiece> wordData;
  util::Lazy<util::memory::Manager> memmgr;
  std::unique_ptr<util::memory::PoolAlloc> alloc;
  util::MutableArraySlice<float> matrixData;
  util::MutableArraySlice<float> embeddingData;
  util::MutableArraySlice<float> nceEmbeddingData;
  util::MutableArraySlice<float> maxentWeightData;
};

Status MikolovModelReader::open(StringPiece filename) {
  data_.reset(new MikolovModelReaderData);
  JPP_RETURN_IF_ERROR(
      data_->rnnDictionary.open(filename, util::MMapType::ReadOnly));
  auto nnetFile = filename.str() + ".nnet";
  JPP_RETURN_IF_ERROR(data_->rnnModel.open(nnetFile, util::MMapType::ReadOnly));
  JPP_RETURN_IF_ERROR(data_->rnnDictionary.map(&data_->dictFrag, 0,
                                               data_->rnnDictionary.size()));
  JPP_RETURN_IF_ERROR(
      data_->rnnModel.map(&data_->modelFrag, 0, data_->rnnModel.size()));
  return Status::Ok();
}

MikolovModelReader::MikolovModelReader() {}

MikolovModelReader::~MikolovModelReader() {}

Status copyArray(StringPiece data, util::MutableArraySlice<float> result,
                 size_t size, size_t* offset) {
  auto fullSize = size * sizeof(float);
  if (*offset + fullSize > data.size()) {
    return JPPS_INVALID_PARAMETER
           << "can't copy rnn weight data, from offset=" << *offset
           << " want to read " << fullSize << ", but there is only "
           << data.size() - *offset
           << " available, total length=" << data.size();
  }
  memcpy(result.data(), data.begin() + *offset, fullSize);
  *offset += fullSize;
  return Status::Ok();
}

Status MikolovModelReader::parse() {
  auto contents = data_->modelFrag.asStringPiece();
  size_t start = ~size_t{0};
  JPP_RETURN_IF_ERROR(readHeader(contents, &data_->header, &start));
  util::CsvReader ssvReader{' '};
  JPP_RETURN_IF_ERROR(
      ssvReader.initFromMemory(data_->dictFrag.asStringPiece()));
  auto& wdata = data_->wordData;
  while (ssvReader.nextLine()) {
    wdata.push_back(ssvReader.field(0));
  }
  auto& hdr = data_->header;

  hdr.vocabSize = wdata.size();
  auto maxBlock = std::max<u64>({hdr.layerSize * hdr.vocabSize, hdr.maxentSize,
                                 hdr.layerSize * hdr.layerSize});
  // 3 comes from rounding to next value + sizeof(float)
  auto pageSize = 1ULL << (static_cast<u32>(std::log2(maxBlock)) + 3);
  data_->memmgr.initialize(pageSize);
  data_->alloc = data_->memmgr.value().core();
  auto& alloc = data_->alloc;
  data_->embeddingData =
      alloc->allocateBuf<float>(hdr.layerSize * hdr.vocabSize, 64);
  data_->nceEmbeddingData =
      alloc->allocateBuf<float>(hdr.layerSize * hdr.vocabSize, 64);
  data_->matrixData =
      alloc->allocateBuf<float>(hdr.layerSize * hdr.layerSize, 64);
  data_->maxentWeightData = alloc->allocateBuf<float>(hdr.maxentSize, 64);

  JPP_RIE_MSG(copyArray(contents, data_->embeddingData,
                        hdr.layerSize * hdr.vocabSize, &start),
              "embeds");
  JPP_RIE_MSG(copyArray(contents, data_->nceEmbeddingData,
                        hdr.layerSize * hdr.vocabSize, &start),
              "nce embeds");
  JPP_RIE_MSG(copyArray(contents, data_->matrixData,
                        hdr.layerSize * hdr.layerSize, &start),
              "matrix");
  JPP_RIE_MSG(
      copyArray(contents, data_->maxentWeightData, hdr.maxentSize, &start),
      "maxent weights");
  if (start != contents.size()) {
    return Status::InvalidState() << "did not read rnn model file fully";
  }

  return Status::Ok();
}

const MikolovRnnModelHeader& MikolovModelReader::header() const {
  return data_->header;
}

const std::vector<StringPiece>& MikolovModelReader::words() const {
  return data_->wordData;
}

util::ArraySlice<float> MikolovModelReader::rnnMatrix() const {
  return data_->matrixData;
}

util::ArraySlice<float> MikolovModelReader::maxentWeights() const {
  return data_->maxentWeightData;
}

util::ArraySlice<float> MikolovModelReader::embeddings() const {
  return data_->embeddingData;
}

util::ArraySlice<float> MikolovModelReader::nceEmbeddings() const {
  return data_->nceEmbeddingData;
}

StringPiece MikolovRnn::matrixAsStringpiece() const {
  return StringPiece{reinterpret_cast<StringPiece::pointer_t>(weights.begin()),
                     weights.size() * sizeof(float)};
}

StringPiece MikolovRnn::maxentWeightsAsStringpiece() const {
  return StringPiece{
      reinterpret_cast<StringPiece::pointer_t>(maxentWeights.begin()),
      maxentWeights.size() * sizeof(float)};
}

}  // namespace mikolov
}  // namespace rnn
}  // namespace jumanpp