//
// Created by Arseny Tolmachev on 2017/03/10.
//

#ifndef JUMANPP_MIKOLOV_RNN_IMPL_H
#define JUMANPP_MIKOLOV_RNN_IMPL_H

#include <array>
#include "mikolov_rnn.h"
#include "simple_rnn_impl.h"
#include "util/debug_output.h"
#include "util/logging.hpp"
#include "util/stl_util.h"

namespace jumanpp {
namespace rnn {
namespace mikolov {

using namespace jumanpp::rnn::impl;

class MikolovScoreCalculator {
  util::ArraySlice<u64> indices;
  util::ArraySlice<float> weights;
  u64 hashMax;

 public:
  MikolovScoreCalculator(const util::ArraySlice<u64> &indices,
                         const util::ArraySlice<float> &weights, u64 hashMax)
      : indices(indices), weights(weights), hashMax(hashMax) {}

  inline float calcScores1(i32 word) const {
    auto idx0 = static_cast<size_t>((indices[0] + word) % hashMax);
    return weights[idx0];
  }

  inline float calcScores2(i32 word) const {
    auto idx0 = static_cast<size_t>((indices[0] + word) % hashMax);
    auto idx1 = static_cast<size_t>((indices[1] + word) % hashMax);
    return weights[idx0] + weights[idx1];
  }

  inline float calcScores3(i32 word) const {
    auto idx0 = static_cast<size_t>((indices[0] + word) % hashMax);
    auto idx1 = static_cast<size_t>((indices[1] + word) % hashMax);
    auto idx2 = static_cast<size_t>((indices[2] + word) % hashMax);
    return weights[idx0] + weights[idx1] + weights[idx2];
  }

  inline float calcScores4(i32 word) const {
    auto idx0 = static_cast<size_t>((indices[0] + word) % hashMax);
    auto idx1 = static_cast<size_t>((indices[1] + word) % hashMax);
    auto idx2 = static_cast<size_t>((indices[2] + word) % hashMax);
    auto idx3 = static_cast<size_t>((indices[3] + word) % hashMax);
    return weights[idx0] + weights[idx1] + weights[idx2] + weights[idx3];
  }

  void addScores(util::ArraySlice<i32> words,
                 util::MutableArraySlice<float> result) const {
    switch (indices.size()) {
      case 0:
        util::fill(result, 0);
        break;
      case 1:
        for (int i = 0; i < words.size(); ++i) {
          result.at(i) += calcScores1(words.at(i));
        }
        break;
      case 2:
        for (int i = 0; i < words.size(); ++i) {
          result.at(i) += calcScores2(words.at(i));
        }
        break;
      case 3:
        for (int i = 0; i < words.size(); ++i) {
          result.at(i) += calcScores3(words.at(i));
        }
        break;
      case 4:
        for (int i = 0; i < words.size(); ++i) {
          result.at(i) += calcScores4(words.at(i));
        }
        break;
      default: {
        for (int i = 0; i < words.size(); ++i) {
          auto w = words[i];
          float res = 0;
          for (int j = 0; j < indices.size(); ++j) {
            auto idx = (indices[i] + w) % hashMax;
            res += weights[idx];
          }
          result.at(i) += res;
        }
      }
    }
  }
};

class MikolovIndexCalculator {
  u64 hashMax;

 public:
  MikolovIndexCalculator(u64 directMentsize, u64 vocabSize)
      : hashMax{directMentsize - vocabSize} {}

  void calcIndices(util::ArraySlice<i32> context,
                   util::MutableArraySlice<u64> result) const {
    for (int i = 0; i < result.size(); ++i) {
      u64 x = PRIMES[0] * PRIMES[1];
      for (int j = 1; j <= i; ++j) {
        auto primeIdxRaw = i * PRIMES[j] + j;
        auto primeIdx = primeIdxRaw % PRIMES_SIZE;
        auto historyItem = context[j - 1];
        u64 converted = (u64)(historyItem) + 1;
        auto oneval = PRIMES[primeIdx] * converted;
        x += oneval;
      }
      result.at(i) = x % hashMax;
    }
  }

  void addScores(util::ArraySlice<i32> context, util::ArraySlice<i32> words,
                 util::ArraySlice<float> weights,
                 util::MutableArraySlice<float> scores) {
    std::array<u64, 4> hashedIndex{{0}};
    JPP_DCHECK_IN(context.size(), 0, 4);
    util::MutableArraySlice<u64> slice{hashedIndex.data(), context.size() + 1};
    calcIndices(context, slice);
    MikolovScoreCalculator msc{slice, weights, hashMax};
    msc.addScores(words, scores);
  }
};

class MikolovRnnImpl {
  const MikolovRnn &rnn;

 public:
  MikolovRnnImpl(const MikolovRnn &rnn) : rnn(rnn) {}

  void computeNewContext(StepData *data) {
    auto esize = rnn.header.layerSize;
    auto nnWeight = impl::asMatrix(rnn.weights, esize, esize);

    auto beamSize = data->beamContext.numRows();
    auto oldCtx = impl::asMatrix(data->context, esize, beamSize);
    auto myEmb = impl::asMatrix(data->leftEmbedding, esize, 1);
    auto result = impl::asMatrix(data->beamContext, esize, beamSize);

    result.noalias() = nnWeight.transpose() * oldCtx;
    result.colwise() += myEmb.col(0);
    result = Eigen::inverse(1 + Eigen::exp(-result.array()));  // sigmoid
  }

  void computeContextScores(StepData *data) {
    i32 beamSize = (i32)data->context.numRows();
    i32 numEntries = (i32)data->scores.rowSize();
    auto esize = rnn.header.layerSize;
    auto context = impl::asMatrix(data->beamContext, esize, beamSize);
    auto embeddings = impl::asMatrix(data->rightEmbeddings, esize, numEntries);
    auto result = impl::asMatrix(data->scores, numEntries, beamSize);
    result.noalias() = embeddings.transpose() * context;
  }

  void computeMaxentScores(StepData *data) {
    MikolovIndexCalculator calc{rnn.header.maxentSize, rnn.header.vocabSize};
    auto contexts = data->contextIds;
    auto words = data->rightIds;
    auto scorePack = data->scores;
    for (int beam = 0; beam < contexts.numRows(); ++beam) {
      auto ctx = contexts.row(beam);
      auto scores = scorePack.row(beam);
      calc.addScores(ctx, words, rnn.maxentWeights, scores);
    }
  }

  void applyNceConstant(StepData *data) {
    i32 beamSize = (i32)data->context.numRows();
    i32 numEntries = (i32)data->scores.rowSize();
    auto result = impl::asMatrix(data->scores, numEntries, beamSize);
    result.array() -= rnn.rnnNceConstant;
  }

  // original formula
  // neu2[word].ac = std::exp( rnn_score + direct_score - nce_lnz );
  // however the value is not used as it is, the log_10 is taken from it
  // so we can use the value itself (divided by log_e 10).
  // Still, grid search tries to find a multiplier for the score in any case
  // so just output the raw score
  void apply(StepData *data) {
    computeNewContext(data);
    computeContextScores(data);
    computeMaxentScores(data);
    applyNceConstant(data);
  }
};

class MikolovRnnImplParallel {
  const MikolovRnn &rnn;

 public:
  MikolovRnnImplParallel(const MikolovRnn &rnn) : rnn(rnn) {}

  void computeNewContext(const ParallelContextData &data) {
    auto esize = rnn.header.layerSize;
    auto nnWeight = impl::asMatrix(rnn.weights, esize, esize);

    auto numItems = data.context.numRows();
    auto oldCtx = impl::asMatrix(data.context, esize, numItems);
    auto myEmb = impl::asMatrix(data.leftEmbeds, esize, numItems);
    auto result = impl::asMatrix(data.newContext, esize, numItems);

    result.noalias() = nnWeight.transpose() * oldCtx;
    result += myEmb;
    result = Eigen::inverse(1 + Eigen::exp(-result.array()));  // sigmoid
  }

  void computeContextScores(ParallelStepData *data) {
    size_t numEntries = data->scores.size();
    auto esize = rnn.header.layerSize;
    auto context = impl::asMatrix(data->context, esize, numEntries);
    auto embeddings = impl::asMatrix(data->nceEmbeds, esize, numEntries);
    auto result = impl::asMatrix(data->scores, 1, numEntries);
    result.noalias() = (embeddings.cwiseProduct(context)).colwise().sum();
  }

  void computeMaxentScores(ParallelStepData *data) {
    MikolovIndexCalculator calc{rnn.header.maxentSize, rnn.header.vocabSize};
    auto contexts = data->contextIds;
    auto words = data->rightIds;
    auto scorePack = data->scores;
    for (int item = 0; item < words.size(); ++item) {
      auto ctx = contexts.row(item);
      auto idx = static_cast<u32>(item);
      util::MutableArraySlice<float> scores(scorePack, idx, 1);
      util::ArraySlice<i32> wordSlice{words, idx, 1};
      calc.addScores(ctx, wordSlice, rnn.maxentWeights, scores);
    }
  }

  void applyNceConstant(ParallelStepData *data) {
    size_t numEntries = data->scores.size();
    auto result = impl::asMatrix(data->scores, numEntries, 1);
    result.array() -= rnn.rnnNceConstant;
  }

  // original formula
  // neu2[word].ac = std::exp( rnn_score + direct_score - nce_lnz );
  // however the value is not used as it is, the log_10 is taken from it
  // so we can use the value itself (divided by log_e 10).
  // Still, grid search tries to find a multiplier for the score in any case
  // so just output the raw score
  void apply(ParallelStepData *data) {
    computeContextScores(data);
    computeMaxentScores(data);
    applyNceConstant(data);
  }
};

}  // namespace mikolov
}  // namespace rnn
}  // namespace jumanpp

#endif  // JUMANPP_MIKOLOV_RNN_IMPL_H
