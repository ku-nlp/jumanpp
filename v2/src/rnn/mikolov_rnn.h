//
// Created by Arseny Tolmachev on 2017/03/08.
//

#ifndef JUMANPP_MIKOLOV_RNN_H
#define JUMANPP_MIKOLOV_RNN_H

#include <memory>
#include "util/sliceable_array.h"
#include "util/status.hpp"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace rnn {
namespace mikolov {

const uint64_t PRIMES[] = {
    108641969, 116049371, 125925907, 133333309, 145678979, 175308587,
    197530793, 234567803, 251851741, 264197411, 330864029, 399999781,
    407407183, 459258997, 479012069, 545678687, 560493491, 607407037,
    629629243, 656789717, 716048933, 718518067, 725925469, 733332871,
    753085943, 755555077, 782715551, 790122953, 812345159, 814814293,
    893826581, 923456189, 940740127, 953085797, 985184539, 990122807};
const uint64_t PRIMES_SIZE = sizeof(PRIMES) / sizeof(PRIMES[0]);

constexpr static size_t LayerNameMaxSize = 64;
constexpr static u64 VersionStepSize = 10000;

#define JPP_PACKED __attribute__((__packed__))

struct MikolovRnnModelHeader {
  u32 layerSize;
  u32 maxentOrder;
  u64 maxentSize;
  u64 vocabSize;
  float nceLnz;
};

struct StepData {
  // MaxEnt part
  util::Sliceable<i32> contextIds;  // (HMM ctx size - 1) x beam
  util::ArraySlice<i32> rightIds;   // rightSize

  // RNN part
  util::Sliceable<float> context;          // size x beam
  util::ArraySlice<float> leftEmbedding;   // size
  util::Sliceable<float> rightEmbeddings;  // size x rightSize

  // Output
  util::Sliceable<float> beamContext;  // size x beam
  util::Sliceable<float> scores;       // rightSize x beam
};

struct ContextStepData {
  // INPUT: context vectors for each element in the beam
  util::Sliceable<float> prevContext;
  // OUTPUT: updated context vectors for the beam
  util::Sliceable<float> curContext;
  // INPUT: an embedding of the current word
  util::ArraySlice<float> leftEmbedding;

  explicit ContextStepData(const StepData& sd)
      : prevContext{sd.context},
        curContext{sd.beamContext},
        leftEmbedding{sd.leftEmbedding} {}

  ContextStepData(const util::Sliceable<float>& outCtx,
                  const util::ArraySlice<float>& leftEmbedding,
                  const util::Sliceable<float>& prevCxt)
      : prevContext(prevCxt),
        curContext(outCtx),
        leftEmbedding(leftEmbedding) {}
};

struct InferStepData {
  // MaxEnt part
  util::Sliceable<i32> contextIds;  // (HMM ctx size - 1) x beam
  util::ArraySlice<i32> rightIds;   // rightSize

  // RNN part
  util::Sliceable<float> beamContext;      // size x beam
  util::Sliceable<float> rightEmbeddings;  // size x rightSize

  // Output
  util::Sliceable<float> scores;  // rightSize x beam

  InferStepData(const util::Sliceable<i32>& contextIds,
                const util::ArraySlice<i32>& rightIds,
                const util::Sliceable<float>& beamContext,
                const util::Sliceable<float>& rightEmbeddings,
                const util::Sliceable<float>& scores)
      : contextIds(contextIds),
        rightIds(rightIds),
        beamContext(beamContext),
        rightEmbeddings(rightEmbeddings),
        scores(scores) {}

  InferStepData slice(i32 from, i32 length) {
    util::ArraySlice<i32> rightIdSlice{rightIds, (u32)from, (u32)length};
    return InferStepData{contextIds, rightIdSlice, beamContext,
                         rightEmbeddings.rows(from, from + length),
                         scores.rows(from, from + length)};
  }
};

Status readHeader(StringPiece data, MikolovRnnModelHeader* header);

struct MikolovModelReaderData;

class MikolovModelReader {
  std::unique_ptr<MikolovModelReaderData> data_;

 public:
  MikolovModelReader();
  ~MikolovModelReader();
  Status open(StringPiece filename);
  Status parse();
  const MikolovRnnModelHeader& header() const;
  const std::vector<StringPiece>& words() const;
  util::ArraySlice<float> rnnMatrix() const;
  util::ArraySlice<float> embeddings() const;
  util::ArraySlice<float> nceEmbeddings() const;
  util::ArraySlice<float> maxentWeights() const;
};

class MikolovRnn {
  MikolovRnnModelHeader header;
  util::ArraySlice<float> weights;
  util::ArraySlice<float> maxentWeights;

  friend class MikolovRnnImpl;

 public:
  Status init(const MikolovRnnModelHeader& header,
              const util::ArraySlice<float>& weights,
              const util::ArraySlice<float>& maxentW);
  void apply(StepData* data);
  void calcNewContext(const ContextStepData& csd);
  void calcScoresOn(const InferStepData& isd, i32 from, i32 count);
};

}  // namespace mikolov
}  // namespace rnn
}  // namespace jumanpp

#endif  // JUMANPP_MIKOLOV_RNN_H
