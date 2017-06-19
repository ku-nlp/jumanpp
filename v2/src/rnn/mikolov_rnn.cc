//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "mikolov_rnn.h"
#include "mikolov_rnn_impl.h"
#include "util/csv_reader.h"
#include "util/memory.hpp"
#include "util/mmap.h"

namespace jumanpp {
namespace rnn {
namespace mikolov {

struct JPP_PACKED MikolovRnnModePackedlHeader {
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

Status readHeader(StringPiece data, MikolovRnnModelHeader* header) {
  auto packed =
      reinterpret_cast<const MikolovRnnModePackedlHeader*>(data.begin());
  auto sv = packed->sizeVersion;
  auto vers = sv / VersionStepSize;
  if (vers != 6) {
    return Status::InvalidParameter()
           << "invalid rnn model version " << vers << " can handle only 6";
  }

  if (!packed->useNce) {
    return Status::InvalidParameter()
           << "model was trained without nce, we support only nce models";
  }

  auto piece = StringPiece{packed->layerType};
  if (piece != "sigmoid") {
    return Status::InvalidParameter()
           << "only sigmoid activation is supported, model had " << piece;
  }

  header->layerSize = (u32)(packed->sizeVersion % VersionStepSize);
  header->nceLnz = packed->nceLnz;
  header->maxentOrder = packed->maxentOrder;
  header->maxentSize = packed->maxEntTableSize;

  return Status::Ok();
}

void MikolovRnn::apply(StepData* data) {
  MikolovRnnImpl impl{*this};
  impl.apply(data);
}

void MikolovRnn::calcNewContext(const ContextStepData& csd) {
  MikolovRnnImpl impl{*this};
  impl.computeNewContext(csd);
}

void MikolovRnn::calcScoresOn(const InferStepData& isd, i32 from, i32 count) {
  MikolovRnnImpl impl{*this};
  impl.inferScores(isd, from, count);
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
  return Status::Ok();
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
  std::unique_ptr<float, FreeDeleter<float>> matrixData;
  std::unique_ptr<float, FreeDeleter<float>> embeddingData;
  std::unique_ptr<float, FreeDeleter<float>> nceEmbeddingData;
  std::unique_ptr<float, FreeDeleter<float>> maxentWeightData;
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

Status allocAligned(std::unique_ptr<float, FreeDeleter<float>>& result,
                    size_t size) {
  float* ptr;
  if (posix_memalign((void**)&ptr, 64, size * sizeof(float)) != 0) {
    return Status::InvalidState() << "could not allocate memory for matrix";
  }
  JPP_DCHECK(isAligned(ptr, 64));
  result.reset(ptr);
  return Status::Ok();
}

Status copyArray(StringPiece data,
                 std::unique_ptr<float, FreeDeleter<float>>& result,
                 size_t size, size_t* offset) {
  auto fullSize = size * sizeof(float);
  if (*offset + fullSize > data.size()) {
    return Status::InvalidParameter()
           << "can't copy rnn weight data, sizes are wrong";
  }
  memcpy(result.get(), data.begin() + *offset, fullSize);
  *offset += fullSize;
  return Status::Ok();
}

Status MikolovModelReader::parse() {
  auto contents = data_->modelFrag.asStringPiece();
  JPP_RETURN_IF_ERROR(readHeader(contents, &data_->header));
  util::CsvReader ssvReader{' '};
  JPP_RETURN_IF_ERROR(
      ssvReader.initFromMemory(data_->dictFrag.asStringPiece()));
  auto& wdata = data_->wordData;
  while (ssvReader.nextLine()) {
    wdata.push_back(ssvReader.field(0));
  }
  auto& hdr = data_->header;

  hdr.vocabSize = wdata.size();
  JPP_RETURN_IF_ERROR(
      allocAligned(data_->embeddingData, hdr.layerSize * hdr.vocabSize));
  JPP_RETURN_IF_ERROR(
      allocAligned(data_->nceEmbeddingData, hdr.layerSize * hdr.vocabSize));
  JPP_RETURN_IF_ERROR(
      allocAligned(data_->matrixData, hdr.layerSize * hdr.layerSize));
  JPP_RETURN_IF_ERROR(allocAligned(data_->maxentWeightData, hdr.maxentSize));
  size_t start = sizeof(MikolovRnnModePackedlHeader);
  JPP_RETURN_IF_ERROR(copyArray(contents, data_->embeddingData,
                                hdr.layerSize * hdr.vocabSize, &start));
  JPP_RETURN_IF_ERROR(copyArray(contents, data_->nceEmbeddingData,
                                hdr.layerSize * hdr.vocabSize, &start));
  JPP_RETURN_IF_ERROR(copyArray(contents, data_->matrixData,
                                hdr.layerSize * hdr.layerSize, &start));
  JPP_RETURN_IF_ERROR(
      copyArray(contents, data_->maxentWeightData, hdr.maxentSize, &start));
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
  return util::ArraySlice<float>{
      data_->matrixData.get(),
      data_->header.layerSize * data_->header.layerSize};
}

util::ArraySlice<float> MikolovModelReader::maxentWeights() const {
  return util::ArraySlice<float>{data_->maxentWeightData.get(),
                                 data_->header.maxentSize};
}

util::ArraySlice<float> MikolovModelReader::embeddings() const {
  return util::ArraySlice<float>{
      data_->embeddingData.get(),
      data_->header.vocabSize * data_->header.layerSize};
}

util::ArraySlice<float> MikolovModelReader::nceEmbeddings() const {
  return util::ArraySlice<float>{
      data_->nceEmbeddingData.get(),
      data_->header.vocabSize * data_->header.layerSize};
}
}  // namespace mikolov
}  // namespace rnn
}  // namespace jumanpp