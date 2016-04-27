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
#include <stddef.h>
#include <stdio.h>
#include <math.h>

// STL
#include <vector>

// Boost Libraries
#define  BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
namespace bip = boost::interprocess;

#include <boost/unordered_map.hpp>

#ifndef WEIGHTTYPE
#define WEIGHTTYPE float
#endif

namespace RNNLM{

const int MAX_STRING=100;

typedef WEIGHTTYPE real;	    // NN weights
typedef WEIGHTTYPE direct_t;	// ME weights

struct neuron {
    real ac;		//actual value stored in neuron
    real er;		//error value in neuron, used by learning algorithm
};

struct synapse {
    real weight;	//weight of synapse
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

const uint64_t PRIMES[]={
    108641969, 116049371, 125925907, 133333309, 145678979, 175308587, 197530793, 234567803, 
    251851741, 264197411, 330864029, 399999781, 407407183, 459258997, 479012069, 545678687, 
    560493491, 607407037, 629629243, 656789717, 716048933, 718518067, 725925469, 733332871, 
    753085943, 755555077, 782715551, 790122953, 812345159, 814814293, 893826581, 923456189, 
    940740127, 953085797, 985184539, 990122807};
const uint64_t PRIMES_SIZE=sizeof(PRIMES)/sizeof(PRIMES[0]);

const int MAX_NGRAM_ORDER=20;

class CRnnLM{
    // 静的読み込みモデルと動的読み込みモデルの切り替えのための，インターフェースクラス
public:
    CRnnLM(){}
    virtual ~CRnnLM(){}
    
    // 必要なインターフェースの定義
    virtual void setRnnLMFile(const char *str) = 0;
    virtual void setDebugMode(int newDebug) = 0;
    virtual void setLweight(double newLw) = 0;
    virtual void get_initial_context_FR(context *c) = 0;
    virtual real test_word_selfnm(context *c, context *new_c, std::string next_word, size_t word_length) = 0;
};

}
#endif

