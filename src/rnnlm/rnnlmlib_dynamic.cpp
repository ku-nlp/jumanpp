/* //////////////////////////////////////////////////////////////////////

    Recurrent Neural Network Language Model
    v0.95

    一部のコードは "Recurrent neural network based statistical language modeling toolkit Version 0.4a" を元にしています.

////////////////////////////////////////////////////////////////////// */

#include <stdio.h>
#include <inttypes.h>
#define __STDC_FORMAT_MACROS
//#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <math.h>

#include <time.h>
#include <cfloat>

#include <unordered_map>
#include <boost/unordered_map.hpp>
#include "rnnlmlib_dynamic.h"

namespace bip = boost::interprocess;

#ifndef exp10
    #define exp10(x) pow((double)10, (x))
#endif

namespace RNNLM{

    inline void FreadAllOrDie_dyn(void* ptr, size_t size, size_t count, FILE* fo, const char* message) {/*{{{*/
        size_t read = fread(ptr, size, count, fo);
        if (read != count) {
            fprintf(
                    stderr, "ERROR: expected to read %zu elements, but read %zu elements (%s)\n",
                    count, read, message);
            exit(1);
        }
    }/*}}}*/

    void CRnnLM_dyn::ReadFRHeader(FILE* file) {/*{{{*/
        const uint64_t kVersionStepSize = 10000;
        const int kCurrentVersion = 6;
        const unsigned kMaxLayerTypeName = 64; // maximum size of layer name type in bytes (including \0)
        const std::string kDefaultLayerType = "sigmoid";
        const char* error_message = "error@reading config";

        // READ ... 
        
        // leyer1 のサイズ
        uint64_t quazi_layer_size; //10000 *version + layer size が入っているらしい．．
        FreadAllOrDie_dyn(&quazi_layer_size, sizeof(int64_t), 1, file, "failed to read layer size.");
        unsigned int layer_size = quazi_layer_size % kVersionStepSize; //
        int version = quazi_layer_size / kVersionStepSize;
        if(version > kCurrentVersion || version < kCurrentVersion){
            fprintf(stderr, "unknown version: %ld", quazi_layer_size / kVersionStepSize );
            exit(1);
        }

        // direct
        unsigned long int maxent_hash_size;
        FreadAllOrDie_dyn(&maxent_hash_size, sizeof(int64_t), 1, file, error_message);
        unsigned int maxent_order;
        FreadAllOrDie_dyn(&maxent_order, sizeof(int), 1, file, error_message);

        // nce
        real nce_lnz_ = 9; // default value
        bool use_nce_;
        FreadAllOrDie_dyn(&use_nce_, sizeof(bool), 1, file, error_message);
        use_nce = use_nce_;
        FreadAllOrDie_dyn(&nce_lnz_, sizeof(real), 1, file, error_message);
        nce_lnz = nce_lnz_;

        // 読み込むだけのパラメタ(RNNLM toolkit に関係の無いパラメタ)
        bool reverse_sentence;
        char buffer[kMaxLayerTypeName];
        int layer_count;
        int hs_arity;
        FreadAllOrDie_dyn(&reverse_sentence, sizeof(bool), 1, file, error_message);
        FreadAllOrDie_dyn(&buffer, sizeof(char), kMaxLayerTypeName, file, error_message); // 固定長らしい
        FreadAllOrDie_dyn(&layer_count, sizeof(int), 1, file, error_message);
        FreadAllOrDie_dyn(&hs_arity, sizeof(int), 1, file, error_message);
        
        // 決め打ちでセットするパラメタ (faster-rnnlm には無いパラメタ)
        independent = true; // 文は独立

        // RNNLM 用のパラメタを設定
        layer0_size = vocab_size + layer_size;
        layer1_size = layer_size;
        layerc_size = 0;
        layer2_size = vocab_size;
        direct_size = maxent_hash_size;
        direct_order = maxent_order;

    }/*}}}*/

    real CRnnLM_dyn::calc_direct_score(int word, context* c){/*{{{*/
        unsigned long long hash[MAX_NGRAM_ORDER] = {};
        unsigned long long hash_max = direct_size - vocab_size;
        unsigned int a,b;
        unsigned int d_o = direct_order;
        double direct_score = 0;

        for (a=0; a<d_o; ++a) {
            b=0;
            hash[a]=PRIMES[0]*PRIMES[1];

            for (b=1; b<=a; b++){
                hash[a]+=PRIMES[(a*PRIMES[b]+b)%PRIMES_SIZE]*(unsigned long long)(c->history[b-1]+1);
            }
            hash[a]=(hash[a]%(hash_max));
        }

        for (a=0; a<d_o; ++a) {
            direct_score += (*syn_d)[(hash[a] + word)%hash_max]; 
            if(a>0 && c->history[a-1] == 0) break;
        }
        return direct_score;
    }/*}}}*/

    real CRnnLM_dyn::random(real min, real max)/*{{{*/
    {
        return rand()/(real)RAND_MAX*(max-min)+min;
    }/*}}}*/

    void CRnnLM_dyn::setRnnLMFile(const char *str)/*{{{*/
    {
        strcpy(rnnlm_file, str);
    }/*}}}*/

    int CRnnLM_dyn::searchVocab(char *word)
    {//{{{
        *mmfstr = word;
        auto vitr = vocab_map->find(*mmfstr);
        if( vitr == vocab_map->end()){
            return -1;
        }else{
            return vitr->second;
        }

        return -1; //return OOV if not found
    }//}}}

    void CRnnLM_dyn::saveFullContext(context *dest)		
    {//{{{
        int a;
            
        dest->l1_neuron.resize(layer1_size);
        dest->history.resize(MAX_NGRAM_ORDER);
        dest->last_word = history[0];
        dest->have_recurrent = false;
        
        //vector<real> l1_neuron = layer1_size;
        //vector<cahr> history = layer1_size;
        for (a=0; a<layer1_size; a++) dest->l1_neuron[a] = neu1[a].ac;
        for (a=0; a<MAX_NGRAM_ORDER; a++) dest->history[a] = history[a];
    }//}}}

    void CRnnLM_dyn::CacheRecurrent(context *dest)		
    {//{{{
        dest->have_recurrent = true;
        for (int a=0; a<layer1_size; a++) dest->l1_neuron[a] = neu1[a].ac;
    }//}}}

    void CRnnLM_dyn::restoreFullContext(const context *dest) 
    {//{{{
        //std::cerr << "restoreFullContext history_size:" << dest->history.size() << " " << (int)dest->last_word << " ," << (int)dest->history[0] << "," << (int)dest->history[1] << std::endl; //H?
        int a;
        for (a=0; a<layer1_size; a++) neu1[a].ac = dest->l1_neuron[a];
        for (a=0; a<MAX_NGRAM_ORDER; a++) history[a] = dest->history[a];
        if(dest->have_recurrent){
            for (a=0; a<layer1_size; a++) neu1[a].ac = dest->l1_neuron[a];
        }else{
            // TODO: 直接 neu0に 移したほうが速い... 下を後で試す
            for (a=0; a<layer1_size; a++) neu0[a+vocab_size].ac = dest->l1_neuron[a];

            //for (a=0; a<layer1_size; a++) neu1[a].ac = dest->l1_neuron[a];
            //copyHiddenLayerToInput(); // neu0 に移す
        }
    }//}}}

    void CRnnLM_dyn::initNetFR()
    {//{{{

        neu0=(struct neuron *)calloc(layer0_size, sizeof(struct neuron));
        neu1=(struct neuron *)calloc(layer1_size, sizeof(struct neuron));
        neu2=(struct neuron *)calloc(layer2_size, sizeof(struct neuron));

    }//}}}

    // RNNLM モデル，語彙の読み込みを行う(faster-rnnlm 対応版)
    void CRnnLM_dyn::restoreNet_FR()    //will read whole network structure
    {//{{{
        int a, b;
        float fl;

        // Filename
        const std::string model_vocab_file = rnnlm_file; 
        const std::string model_weight_file = model_vocab_file + ".nnet"; // ネットワークの重み

        // 語彙 (vocab, vocab_map)
        FILE *vocab_file = fopen(model_vocab_file.c_str(), "rb");
        if (vocab_file == NULL){
            fprintf(stderr, "Error: vocaburary file %s not found. \n", model_vocab_file.c_str());
            exit(1);
        }
            
        std::string MapVocabFilePath= model_vocab_file+".map";
        if( access( MapVocabFilePath.c_str(), F_OK ) != -1 ){ //読み込みのみ
            //std::cerr << "read mapped file" << std::endl;
            p_file_vocab = new bip::managed_mapped_file(ipc::open_only, MapVocabFilePath.c_str());
            vocab_map = p_file_vocab->find<umap_vocab>("map_vocab").first;
            mmfstr = p_file_vocab->find<MmfString>("mmfstring").first;
        }else{ //通常読み込み
            //std::cerr << "read original file" << std::endl;
            unsigned long long map_vocab_size = 1024 * 1024 * 1024; // 大きめに1GB (10M文で85MB程度)
            p_file_vocab = new bip::managed_mapped_file(ipc::create_only, MapVocabFilePath.c_str(), map_vocab_size ); 
            vocab_map = p_file_vocab->construct<umap_vocab>("map_vocab")( 0, boost::hash<MmfString>(), std::equal_to<MmfString>(), p_file_vocab->get_allocator<VocabPair>());
            mmfstr = p_file_vocab->construct<MmfString>("mmfstring")(p_file_vocab->get_allocator<MmfString>());
                 
            // 語彙の vocab, と vocab_map への登録
            for (int line_number = 0; !feof(vocab_file); ++line_number) {
                char buffer[MAX_STRING];
                uint64_t count; 
                if (fscanf(vocab_file, "%s %ld ", buffer, &count) != 2) {
                    fprintf(stderr, "WARNING: Skipping ill-formed line #%d in the vocabulary\n", line_number);
                    continue;
                }
                    
                int wid = vocab_map->size(); 
                *mmfstr = buffer;
                (*vocab_map)[*mmfstr] = wid; 
            }
            p_file_vocab->flush();
            bip::managed_mapped_file::shrink_to_fit(MapVocabFilePath.c_str());
            bip::managed_mapped_file::grow(MapVocabFilePath.c_str(),65535);
        }
        
        // vocab_map を全て読み込んでから，vocab を確保する(デバッグ用)
        vocab_size = vocab_map->size();
        if (debug_mode>0) {
            if (vocab!=NULL) free(vocab);
            vocab_max_size=vocab_size+1000;
            vocab=(struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));    //initialize memory for vocabulary

            for(auto& pw:(*vocab_map)){
                int wid = pw.second;
                std::stringstream ss;
                strncpy(vocab[wid].word, pw.first.c_str(), (pw.first.length()));
            }
        }
        fclose(vocab_file);
        
        // ネットワークの重み 読み出し
        FILE *model_file = fopen(model_weight_file.c_str(), "rb");
        if (model_file == NULL){
            fprintf(stderr, "Error: model file %s not found. \n", model_weight_file.c_str());
            exit(1);
        }
        
        ReadFRHeader(model_file);
            
        if (debug_mode>0) 
            std::cerr << "reading model file" << std::endl;
        
        if (neu0==NULL) initNetFR(); //memory allocation (neu0, neu1, neu2)
        
        if (use_nce != true){
            std::cerr << "err:use nce" << std::endl;
            exit(1); //DEB
        }

        const std::string MapFilePath( model_vocab_file + ".nnet.map" );
        if( access( MapFilePath.c_str(), F_OK ) != -1 ){ //読み込みのみ
            p_file_syn = new bip::managed_mapped_file(ipc::open_only, MapFilePath.c_str());
            if(debug_mode>0)
                std::cerr << "reading RNNLM model" << std::endl;
            syn_vocab_l1 = p_file_syn->find<vector_syn>("syn_vocab_l1").first;
            syn_rec      = p_file_syn->find<vector_syn>("syn_rec").first;
            syn_l1_l2    = p_file_syn->find<vector_syn>("syn_l1_l2").first;
        }else{ //通常読み込み
            unsigned long long syn_size = sizeof(real)*(vocab_size*layer1_size + layer1_size*layer1_size + layer2_size*layer1_size) + 4096;
            p_file_syn = new bip::managed_mapped_file(ipc::create_only, MapFilePath.c_str(), syn_size );
            syn_vocab_l1 = p_file_syn->construct<vector_syn>("syn_vocab_l1")(p_file_syn->get_segment_manager());
            syn_rec      = p_file_syn->construct<vector_syn>("syn_rec")(p_file_syn->get_segment_manager());
            syn_l1_l2    = p_file_syn->construct<vector_syn>("syn_l1_l2")(p_file_syn->get_segment_manager());

            if(debug_mode>0)
                std::cerr << "reading embedding" << std::endl;
            // embedding //Row-major // embeddings.resize(vocab.size(), cfg.layer_size);
            syn_vocab_l1->resize(vocab_size*layer1_size);
            for (a=0; a<vocab_size; a++) { //vocab_size < layer_0.size
                for (b=0; b<layer1_size; b++) {
                    fread(&fl, sizeof(fl), 1, model_file); //real
                    (*syn_vocab_l1)[b+a*layer1_size]=fl;
                }
            } 

            // TODO: 初期化．どうせ使う時初期化するはずなので，不要かも
            for (a=0; a<layer1_size; a++) {
                neu1[a].ac=0; 
            }

            if(debug_mode>0)
                std::cerr << "reading NCE layer " << std::endl; 
            // W:layer1 -> output (word embedding => layer1
            syn_l1_l2->resize(layer1_size*layer2_size);
            for (b=0; b<layer2_size; b++) { 
                for (a=0; a<layer1_size; a++) {
                    fread(&fl, sizeof(fl), 1, model_file);
                    (*syn_l1_l2)[a+b*layer1_size]=fl;
                }
            }
            
            if(debug_mode>0)
                std::cerr << "reading reccurent weight" << std::endl;
            // Recurrent weight
            syn_rec->resize(layer1_size*layer1_size);
            for (b=0; b<layer1_size; b++) {
                for (a=0; a<layer1_size; a++) {
                    fread(&fl, sizeof(fl), 1, model_file);
                    (*syn_rec)[a+b*layer1_size]=fl; 
                }
            }
            p_file_syn->flush();
        }
         
        if(debug_mode>0)
            std::cerr << "reading direct weight" << std::endl;

        const std::string FilePath( model_vocab_file + ".direct" );

        //auto const size = boost::filesystem::file_size(FilePath);
        if( access( FilePath.c_str(), F_OK ) != -1 ){ //読み込みのみ
            p_file_direct = new bip::managed_mapped_file( bip::open_only, FilePath.c_str() ); 
            syn_d = p_file_direct->find<vector_syn>("MyVector").first;
            if(debug_mode>0)
                std::cerr << "read finished"<< std::endl;
        }else{
            // 初回起動時は Memory mapped file に読み込み
            p_file_direct = new bip::managed_mapped_file( ipc::create_only, FilePath.c_str(), direct_size*sizeof(real)+4096 );
            syn_d = p_file_direct->construct<vector_syn>("MyVector")(p_file_direct->get_segment_manager());
            if(debug_mode>0)
                std::cerr << "Creating memory mapped file:" << std::endl;

            real fl;
            (*syn_d).resize(direct_size);
            for (unsigned long long b=0; b<direct_size; b++) {
                if(debug_mode>0)
                    if(b%10000 ==0)
                        std::cerr << "b:"<< b << "\r" << std::flush;
                fread(&fl, sizeof(real), 1, model_file); //real
                (*syn_d)[b]=fl;
            }
            p_file_direct->flush(); 
            if(debug_mode>0)
                std::cerr << "Creating memory mapped file: finished."<< std::endl;
        }
        
        fclose(model_file);
        return;
    }//}}}

    void CRnnLM_dyn::matrixXvector(struct neuron *dest, struct neuron *srcvec, vector_syn *srcmatrix, int matrix_width, int from, int to, int from2, int to2, int type)
    {//{{{
        int a, b;
        real val1, val2, val3, val4;
        real val5, val6, val7, val8;

        if (type==0) {		//ac mod
            for (b=0; b<(to-from)/8; b++) {
                val1=0;
                val2=0;
                val3=0;
                val4=0;

                val5=0;
                val6=0;
                val7=0;
                val8=0;

                for (a=from2; a<to2; a++) {
                    val1 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+0)*matrix_width];
                    val2 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+1)*matrix_width];
                    val3 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+2)*matrix_width];
                    val4 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+3)*matrix_width];

                    val5 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+4)*matrix_width];
                    val6 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+5)*matrix_width];
                    val7 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+6)*matrix_width];
                    val8 += srcvec[a].ac * (*srcmatrix)[a+(b*8+from+7)*matrix_width];
                }
                dest[b*8+from+0].ac += val1;
                dest[b*8+from+1].ac += val2;
                dest[b*8+from+2].ac += val3;
                dest[b*8+from+3].ac += val4;

                dest[b*8+from+4].ac += val5;
                dest[b*8+from+5].ac += val6;
                dest[b*8+from+6].ac += val7;
                dest[b*8+from+7].ac += val8;
            }

            for (b=b*8; b<to-from; b++) {
                for (a=from2; a<to2; a++) {
                    dest[b+from].ac += srcvec[a].ac * (*srcmatrix)[a+(b+from)*matrix_width];
                }
            }
        }
        else {		//er mod
            for (a=0; a<(to2-from2)/8; a++) {
                val1=0;
                val2=0;
                val3=0;
                val4=0;

                val5=0;
                val6=0;
                val7=0;
                val8=0;

                for (b=from; b<to; b++) {
                    val1 += srcvec[b].er * (*srcmatrix)[a*8+from2+0+b*matrix_width];
                    val2 += srcvec[b].er * (*srcmatrix)[a*8+from2+1+b*matrix_width];
                    val3 += srcvec[b].er * (*srcmatrix)[a*8+from2+2+b*matrix_width];
                    val4 += srcvec[b].er * (*srcmatrix)[a*8+from2+3+b*matrix_width];

                    val5 += srcvec[b].er * (*srcmatrix)[a*8+from2+4+b*matrix_width];
                    val6 += srcvec[b].er * (*srcmatrix)[a*8+from2+5+b*matrix_width];
                    val7 += srcvec[b].er * (*srcmatrix)[a*8+from2+6+b*matrix_width];
                    val8 += srcvec[b].er * (*srcmatrix)[a*8+from2+7+b*matrix_width];
                }
                dest[a*8+from2+0].er += val1;
                dest[a*8+from2+1].er += val2;
                dest[a*8+from2+2].er += val3;
                dest[a*8+from2+3].er += val4;

                dest[a*8+from2+4].er += val5;
                dest[a*8+from2+5].er += val6;
                dest[a*8+from2+6].er += val7;
                dest[a*8+from2+7].er += val8;
            }

            for (a=a*8; a<to2-from2; a++) {
                for (b=from; b<to; b++) {
                    dest[a+from2].er += srcvec[b].er * (*srcmatrix)[a+from2+b*matrix_width];
                }
            }

            if (gradient_cutoff>0)
                for (a=from2; a<to2; a++) {
                    if (dest[a].er>gradient_cutoff) dest[a].er=gradient_cutoff;
                    if (dest[a].er<-gradient_cutoff) dest[a].er=-gradient_cutoff;
                }
        }

        //this is normal implementation (about 3x slower):

        /*if (type==0) {		//ac mod
          for (b=from; b<to; b++) {
          for (a=from2; a<to2; a++) {
          dest[b].ac += srcvec[a].ac * srcmatrix[a+b*matrix_width].weight;
          }
          }
          }
          else 		//er mod
          if (type==1) {
          for (a=from2; a<to2; a++) {
          for (b=from; b<to; b++) {
          dest[a].er += srcvec[b].er * srcmatrix[a+b*matrix_width].weight;
          }
          }
          }*/
    }//}}}

    // 今までの状態を上書きし，破壊することに注意
    void CRnnLM_dyn::computeNet_selfnm(int word, context *context)
    {//{{{
        int a, b;
        int last_word_local = context->history[0];

        // 未知語に対しては計算しない
        if(word == -1) return; 
        
        // 語彙数次元のベクトル
        if (last_word_local!=-1) neu0[last_word_local].ac=1;
        
        // 0-(vocab)-(layer1)-layer0_size
        // 0...vocab(layer0_size-layer1_size -1) が語彙の次元
        // layer0_size-layer1_size > layer0_size がコンテクストの次元
        restoreFullContext(context); // コンテクストをneu0 のコンテクストへコピー，history をセット
        
        if(context->have_recurrent){
            // context が recurrent に含まれるなら，それを読み込むだけに留める.
            
        }else{
            //propagate 0->1
            for (a=0; a<layer1_size; a++) neu1[a].ac=0; //初期化
                
            // コンテクストvectorについての計算 context vector X reccurrent weight
            auto *syn_r = syn_rec; //syn0 +vocab_size*layer1_size;
            auto *neu_r = neu0 +vocab_size;
            matrixXvector(neu1, neu_r, syn_r, layer1_size, 0, layer1_size, 0, layer1_size, 0); 
                
            CacheRecurrent(context);
        }
                
        // last_word について計算(one hot)
        if(last_word_local != -1)
            for (b=0; b<layer1_size; b++) {
                a=last_word_local;
                neu1[b].ac += (*syn_vocab_l1)[b+a*layer1_size];
            }
            
        // layer 1 の activation(sigmoid)
        for (a=0; a<layer1_size; a++) {
            neu1[a].ac=1/(1+std::exp(-neu1[a].ac));
        }
                        
        //1->2 class
        double direct_score = 0.0;
        if (direct_size>0) { //RNNME を使う場合
            direct_score = calc_direct_score(word, context);
        }
                
        double rnn_score = 0.0;
        for (a=0; a<layer1_size; a++) {
            rnn_score += neu1[a].ac * (*syn_l1_l2)[a + word*layer1_size];
        }
            
        // exp 前 スコア
        neu2[word].ac = std::exp( rnn_score + direct_score - nce_lnz );
                
        if(debug_mode > 0)
            std::cerr << "p(" << vocab[word].word << "|" << vocab[context->history[0]].word << ", " << vocab[context->history[1]].word << ", ...) = " << neu2[word].ac << " (nn_score:" << rnn_score << " direct_score:" << direct_score << ")"<< std::endl;

    }//}}}
    
    void CRnnLM_dyn::copyHiddenLayerToInput()
    {//{{{
        //std::cout << "copyHiddenLayerToInput"<< std::endl;
        int a;

        for (a=0; a<layer1_size; a++) {
            neu0[a+layer0_size-layer1_size].ac=neu1[a].ac;
        }
    }//}}}

    void CRnnLM_dyn::get_initial_context_FR(context *c) 
    {//{{{
        if (debug_mode>0) std::cerr << "initializing RNNLM FR" << std::endl;
        restoreNet_FR(); // initialize モデル読込  重い
        //computeNet(0, 0); // initialize 
        copyHiddenLayerToInput();
        //saveContext(); 
        //saveContext2(); //必要？　
        for (int a=0; a<MAX_NGRAM_ORDER; a++) history[a]=-1;
        history[0]=0;
        saveFullContext(c); //文頭としてInitial context を作成
    }//}}}
    
    real CRnnLM_dyn::test_word_selfnm(context *c, context *new_c, std::string next_word, size_t word_length)
    {//{{{
        int last_word;
        last_word = c->last_word;
        float prob_other; //has to be float so that %f works in fscanf
        real senp;

        real lambda=1;
        real logp=0;
        senp=0; //

        int word = searchVocab((char*)next_word.c_str());

        // 文区切りを0に対応させる アドホックな対処
        if(next_word == "<EOS>" || next_word == "<BOS>")
            word = 0;
        
        // RNN の実行 (結果はneu2[word].ac に書き込まれる
        computeNet_selfnm(word,c);

        double ln_score = neu2[word].ac;
       
        if (word!=-1) { //OOVでない
            logp+=log10(ln_score);
            senp+=log10(ln_score*lambda + prob_other*(1-lambda));
        } else {
            //assign to OOVs some score to correctly rescore nbest lists, reasonable value can be less than 1/|V| or backoff LM score (in case it is trained on more data)
            //this means that PPL results from nbest list rescoring are not true probabilities anymore (as in open vocabulary LMs)
                
            // 文字の長さに対してlinear に設定する
            real oov_penalty=-5; //log penalty
            if(lpenalty){ //penalty を文字の長さに対して線形に与える.
               oov_penalty -= lweight * word_length;
            }
                
            logp+=oov_penalty;
            senp+=oov_penalty;
        }
            
        copyHiddenLayerToInput(); //必要？
        if (last_word!=-1) neu0[last_word].ac=0;  //delete previous activation
            
        // history の更新
        for (int a=MAX_NGRAM_ORDER-1; a>0; a--)
            history[a] = c->history[a-1];
        history[0] = word;
            
        // context の保存 // history の渡し方が暗黙的
        saveFullContext(new_c);

        return senp;
    }//}}}

}
