///////////////////////////////////////////////////////////////////////
//
// Recurrent neural network based statistical language modeling toolkit
// Version 0.4a
// (c) 2010-2012 Tomas Mikolov (tmikolov@gmail.com)
// (c) 2013 Cantab Research Ltd (info@cantabResearch.com)
//
///////////////////////////////////////////////////////////////////////

// TODO: 学習用コードの削除．不要な変数の除去

#ifndef _RNNLMLIB_H_
#define _RNNLMLIB_H_

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

// STL
#include <vector>

#include "rnn/mikolov_rnn.h"

#ifndef WEIGHTTYPE
#define WEIGHTTYPE float
#endif

namespace RNNLM_legacy {
using namespace jumanpp::rnn::mikolov;

const int MAX_STRING = 100;

typedef WEIGHTTYPE real;      // NN weights
typedef WEIGHTTYPE direct_t;  // ME weights

struct neuron {
  real ac;  // actual value stored in neuron
  real er;  // error value in neuron, used by learning algorithm
};

struct synapse {
  real weight;  // weight of synapse
};

struct vocab_word {
  int cn;
  char word[MAX_STRING];

  real prob;
  int class_index;
};

struct context {
  int last_word;
  bool have_recurrent;
  std::vector<int> history;
  std::vector<real> l1_neuron;
  std::vector<real> recurrent;
};

const int MAX_NGRAM_ORDER = 20;

class CRnnLM {
  // 静的読み込みモデルと動的読み込みモデルの切り替えのための，インターフェースクラス
 public:
  CRnnLM() {}
  virtual ~CRnnLM() {}

  // 必要なインターフェースの定義
  virtual void setRnnLMFile(const char *str) = 0;
  virtual void setDebugMode(int newDebug) = 0;
  virtual void setLweight(double newLw) = 0;
  virtual void get_initial_context_FR(context *c) = 0;
  virtual real test_word_selfnm(context *c, context *new_c,
                                std::string next_word, size_t word_length) = 0;
};
}
#endif
