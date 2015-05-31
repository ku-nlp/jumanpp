///////////////////////////////////////////////////////////////////////
//
// Recurrent neural network based statistical language modeling toolkit
// Version 0.4a
// (c) 2010-2012 Tomas Mikolov (tmikolov@gmail.com)
// (c) 2013 Cantab Research Ltd (info@cantabResearch.com)
//
///////////////////////////////////////////////////////////////////////

#ifndef _RNNLMLIB_H_
#define _RNNLMLIB_H_

#include <vector>

namespace RNNLM{

const int MAX_STRING=100;

#ifndef WEIGHTTYPE
#define WEIGHTTYPE double
#endif

typedef WEIGHTTYPE real;	// NN weights
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
    std::vector<int> history;
    std::vector<real> l1_neuron;
};

const unsigned int PRIMES[]={108641969, 116049371, 125925907, 133333309, 145678979, 175308587, 197530793, 234567803, 251851741, 264197411, 330864029, 399999781,
407407183, 459258997, 479012069, 545678687, 560493491, 607407037, 629629243, 656789717, 716048933, 718518067, 725925469, 733332871, 753085943, 755555077,
782715551, 790122953, 812345159, 814814293, 893826581, 923456189, 940740127, 953085797, 985184539, 990122807};
const unsigned int PRIMES_SIZE=sizeof(PRIMES)/sizeof(PRIMES[0]);

const int MAX_NGRAM_ORDER=20;

enum FileTypeEnum {TEXT, BINARY, COMPRESSED};		//COMPRESSED not yet implemented

class CRnnLM{
protected:
    char train_file[MAX_STRING];
    char valid_file[MAX_STRING];
    char test_file[MAX_STRING];
    char rnnlm_file[MAX_STRING];
    char lmprob_file[MAX_STRING];
    
    int rand_seed;
    
    int debug_mode;
    
    int version;
    int filetype;
    
    int use_lmprob;
    //File* mem_lmprob;
    real lambda;
    real gradient_cutoff;
    
    real dynamic;
    
    real alpha;
    real starting_alpha;
    int alpha_divide;
    double logp, llogp;
    float min_improvement;
    int iter;
    int vocab_max_size;
    int vocab_size;
    int train_words;
    int train_cur_pos;
    int counter;
    
    int one_iter;
    int maxIter;
    int anti_k;
    
    real beta;
    
    int class_size;
    int **class_words;
    int *class_cn;
    int *class_max_cn;
    int old_classes;
    
    struct vocab_word *vocab;
    void sortVocab();
    int *vocab_hash;
    int vocab_hash_size;
    
    int layer0_size;
    int layer1_size;
    int layerc_size;
    int layer2_size;
    
    long long direct_size;
    int direct_order;
    int history[MAX_NGRAM_ORDER];
    
    int bptt;
    int bptt_block;
    int *bptt_history;
    neuron *bptt_hidden;
    struct synapse *bptt_syn0;
    
    int gen;

    int independent;
    
    struct neuron *neu0;		//neurons in input layer
    struct neuron *neu1;		//neurons in hidden layer
    struct neuron *neuc;		//neurons in hidden layer (for compression)
    struct neuron *neu2;		//neurons in output layer

    struct synapse *syn0;		//weights between input and hidden layer //単語ベクトルを学習
    struct synapse *syn1;		//weights between hidden and output layer (or hidden and compression if compression>0)
    struct synapse *sync;		//weights between hidden and compression layer
    direct_t *syn_d;		//direct parameters between input and output layer (similar to Maximum Entropy model parameters)
    
    //backup used in training:
    struct neuron *neu0b;
    struct neuron *neu1b;
    struct neuron *neucb;
    struct neuron *neu2b;

    struct synapse *syn0b;
    struct synapse *syn1b;
    struct synapse *syncb;
    direct_t *syn_db;
    
    //backup used in n-bset rescoring:
    struct neuron *neu1b2;
    
    
public:

    int alpha_set, train_file_set;

    double lpenalty = false; 
    double lweight = 1.0; // linear penalty for UNK

    CRnnLM()		//constructor initializes variables
    {
	version=10;
	filetype=TEXT;
	
	use_lmprob=0;
	lambda=0.75;
	gradient_cutoff=15;
	dynamic=0;
    
	train_file[0]=0;
	valid_file[0]=0;
	test_file[0]=0;
	rnnlm_file[0]=0;
	
	alpha_set=0;
	train_file_set=0;
	
	alpha=0.1;
	beta=0.0000001;
	//beta=0.00000;
	alpha_divide=0;
	logp=0;
	llogp=-100000000;
	iter=0;
	
	min_improvement=1.003;
	
	train_words=0;
	train_cur_pos=0;
	vocab_max_size=100;
	vocab_size=0;
	vocab=(struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
	
	layer1_size=30;
	
	direct_size=0;
	direct_order=0;
	
	bptt=0;
	bptt_block=10;
	bptt_history=NULL;
	bptt_hidden=NULL;
	bptt_syn0=NULL;
	
	gen=0;

	independent=0;
	
	neu0=NULL;
	neu1=NULL;
	neuc=NULL;
	neu2=NULL;
	
	syn0=NULL;
	syn1=NULL;
	sync=NULL;
	syn_d=NULL;
	syn_db=NULL;
	//backup
	neu0b=NULL;
	neu1b=NULL;
	neucb=NULL;
	neu2b=NULL;
	
	neu1b2=NULL;
	
	syn0b=NULL;
	syn1b=NULL;
	syncb=NULL;
	//
	
	rand_seed=1;
	
	class_size=100;
	old_classes=0;
	
	one_iter=0;
  maxIter=0;
	
	debug_mode=1;
	srand(rand_seed);
	
	vocab_hash_size=100000000;
	vocab_hash=(int *)calloc(vocab_hash_size, sizeof(int));
    }
    
    ~CRnnLM()		//destructor, deallocates memory
    {
	int i;
	
	if (neu0!=NULL) {
	    free(neu0);
	    free(neu1);
	    if (neuc!=NULL) free(neuc);
	    free(neu2);
	    
	    free(syn0);
	    free(syn1);
	    if (sync!=NULL) free(sync);
	    
	    if (syn_d!=NULL) free(syn_d);

	    if (syn_db!=NULL) free(syn_db);

	    //
	    free(neu0b);
	    free(neu1b);
	    if (neucb!=NULL) free(neucb);
	    free(neu2b);

	    free(neu1b2);
	    
	    free(syn0b);
	    free(syn1b);
	    if (syncb!=NULL) free(syncb);
	    //
	    
	    for (i=0; i<class_size; i++) free(class_words[i]);
	    free(class_max_cn);
	    free(class_cn);
	    free(class_words);
	
	    free(vocab);
	    free(vocab_hash);

	    if (bptt_history!=NULL) free(bptt_history);
	    if (bptt_hidden!=NULL) free(bptt_hidden);
            if (bptt_syn0!=NULL) free(bptt_syn0);
	    
	    //todo: free bptt variables too
	}
    }
    
    real random(real min, real max);

    void setTrainFile(char *str);
    void setValidFile(char *str);
    void setTestFile(char *str);
    void setRnnLMFile(const char *str);
    void setLMProbFile(char *str) {strcpy(lmprob_file, str);}
    
    void setFileType(int newt) {filetype=newt;}
    
    void setClassSize(int newSize) {class_size=newSize;}
    void setOldClasses(int newVal) {old_classes=newVal;}
    void setLambda(real newLambda) {lambda=newLambda;}
    void setGradientCutoff(real newGradient) {gradient_cutoff=newGradient;}
    void setDynamic(real newD) {dynamic=newD;}
    void setGen(real newGen) {gen=newGen;}
    void setIndependent(int newVal) {independent=newVal;}
    
    void setLearningRate(real newAlpha) {alpha=newAlpha;}
    void setRegularization(real newBeta) {beta=newBeta;}
    void setMinImprovement(real newMinImprovement) {min_improvement=newMinImprovement;}
    void setHiddenLayerSize(int newsize) {layer1_size=newsize;}
    void setCompressionLayerSize(int newsize) {layerc_size=newsize;}
    void setDirectSize(long long newsize) {direct_size=newsize;}
    void setDirectOrder(int newsize) {direct_order=newsize;}
    void setBPTT(int newval) {bptt=newval;}
    void setBPTTBlock(int newval) {bptt_block=newval;}
    void setRandSeed(int newSeed) {rand_seed=newSeed; srand(rand_seed);}
    void setDebugMode(int newDebug) {debug_mode=newDebug;}
    void setAntiKasparek(int newAnti) {anti_k=newAnti;}
    void setOneIter(int newOneIter) {one_iter=newOneIter;}
    void setMaxIter(int newMaxIter) {maxIter=newMaxIter;}
    void setLweight(double newLw) {lpenalty=true; lweight=newLw;}
    
    int getWordHash(char *word);
    void readWord(char *word, FILE *fin);
    int searchVocab(char *word);
    int readWordIndex(FILE *fin);
    int addWordToVocab(char *word);
    void learnVocabFromTrainFile();		//train_file will be used to construct vocabulary

    void readWord(char *word, std::string* in_str);
    int readWordIndex(std::string* in_str);

    
    void saveWeights();			//saves current weights and unit activations
    void restoreWeights();		//restores current weights and unit activations from backup copy
    //void saveWeights2();		//allows 2. copy to be stored, useful for dynamic rescoring of nbest lists
    //void restoreWeights2();		
    void saveContext();
    void restoreContext();
    void saveContext2();
    void restoreContext2();

    void saveFullContext(context *dest);
    void restoreFullContext(const context *dest);
    void initNet();
    void saveNet();
    void goToDelimiter(int delim, FILE *fi);
    void restoreNet();
    void netFlush();
    void netReset();    //will erase just hidden layer state + bptt history + maxent history (called at end of sentences in the independent mode)
    
    void computeNet(int last_word, int word);
    void computeNet(int last_word, int word, context *c);
    void computeNet(int word, context *context);
    void learnNet(int last_word, int word);
    void copyHiddenLayerToInput();


    real test_word(context &c, context &new_c, std::string next_word);
    void get_initial_context(context *c);
    void initialize_test_sent();
    void trainNet();
    void useLMProb(int use) {use_lmprob=use;}
    real test_word(context *c, context *new_c, std::string next_word);
    void testNet();
    void testNbest();
    void testGen();
    real test_sent(std::string sent);
    
    void matrixXvector(struct neuron *dest, struct neuron *srcvec, struct synapse *srcmatrix, int matrix_width, int from, int to, int from2, int to2, int type);
};

}
#endif

