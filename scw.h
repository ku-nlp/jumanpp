#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// これをinclude しないとリンクに失敗する．(2015/12/14 boost 1.59)
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>

typedef std::unordered_map<std::string,double> Umap;
typedef std::unordered_map<unsigned long, double> Ulmap; //参考に2015/11 時点での素性数 2,395,735
typedef std::unordered_map<std::string, unsigned long> Fimap; 

class FeatureVector;// 今後分離したほうが良い？
class DiagMat; //Matrix も扱うなら名前を変えるべき

// 現状ではただのunordered_map のラップになっている．
// TODO は，マップ先を固定長のvector にして feature hashing できるようにすること > for モデルのコンパクト化
class FeatureVector{// ただのunordered_map のラップ/*{{{*/
    Ulmap vec;
    public:
        FeatureVector(){vec.clear();};
        FeatureVector(const std::vector<unsigned long>& v1, const std::vector<unsigned long>& v2);
        FeatureVector(const std::vector<unsigned long>& v1);
        void update(double alpha, double y, const DiagMat& sigma, const FeatureVector& xt);

        double operator* (const FeatureVector& fv) const;
        double& operator[](const unsigned long s){ /*{{{*/
//            return vec[s]; // 0に初期化される?
            auto itr=vec.find(s);
            if(itr == vec.end()){
                auto& ref = vec[s];
                ref = 0;
                return ref;
            }else{
                return itr->second;
            }
        };/*}}}*/
            
        FeatureVector& merge(const FeatureVector &fv){/*{{{*/
            for(const auto &st:fv){
                vec[st.first] += st.second;
            }
            return *this;
        }/*}}}*/
        
        FeatureVector& diff(const FeatureVector &fv){/*{{{*/
            for(const auto &st:fv){
                vec[st.first] -= st.second;
            }
            return *this;
        }/*}}}*/

        Ulmap::iterator find(const unsigned long key){ /*{{{*/
            return vec.find(key); 
        }/*}}}*/
        
        Ulmap::const_iterator find(const unsigned long key)const{ /*{{{*/
            return vec.find(key); 
        }/*}}}*/
        
        const size_t size() const{return vec.size();};

        inline std::string str(){  /*{{{*/
            std::stringstream ss;
            for(const auto &st:vec){
                ss << st.first << ":" << st.second << " ";
            }
            ss << std::endl;
            return std::move(ss.str());
        };/*}}}*/

        inline void clear(){ vec.clear();};
        Ulmap& get_umap(){return vec;}
        Ulmap::iterator begin(){return vec.begin();};
        Ulmap::iterator end(){return vec.end();};
        Ulmap::const_iterator begin()const{return vec.cbegin();};
        Ulmap::const_iterator end()const{return vec.cend();};
        Ulmap::const_iterator cbegin()const{return vec.cbegin();};
        Ulmap::const_iterator cend()const{return vec.cend();};

private:
    friend class boost::serialization::access;
    template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & vec;
        }

};/*}}}*/

// Sparse Matrix
class DiagMat{/*{{{*/
    private:
        Ulmap vec;
    public:
        void update(double beta, const FeatureVector& x);
        double get_value(const unsigned long sp1)const;
        inline double& operator[](const unsigned long s){ return ref_value(s);};
        double& ref_value(const unsigned long sp1, const unsigned long sp2);
        double& ref_value(const unsigned long sp1){ return ref_value(sp1,sp1);};

        // ベクトルとの積を定義
//        std::vector<std::pair<unsigned long, double>> mXvt(const FeatureVector& v){/*{{{*/
//            std::unordered_map<unsigned long, double> sum;
//            std::vector<unsigned long> keys;
//            unsigned long key;
//            for(auto& v1: v){
//                // v1==v2 の場合
//                key = boost::combine_hash(v1.first, v1.first);
//                sum[key] += vec[key] * v1.second * 2;
//                
//                // v1 != v2 の場合
//                for(auto v2: keys){
//                    key = boost::combine_hash(v1.first, v2);
//                    sum += vec[key] * v1.second;
//                }
//                keys.push_back(v1.first);
//            }
//        };/*}}}*/
        
//        double tXm(const FeatureVector& v){/*{{{*/
//            std::vector<unsigned long> keys;
//
//            unsigned long key;
//            for(auto& v1: v){
//                // v1==v2 の場合
//                key = boost::combine_hash(v1.first, v1.first);
//                sum += vec[key] * v1.second;
//                
//                // v1 != v2 の場合
//                for(auto v2: keys){
//                    key = boost::combine_hash(v1.first, v2);
//                    sum += vec[key] * v1.second;
//                }
//                keys.push_back(v1.first);
//            }
//        };/*}}}*/

        Ulmap::iterator begin(){return vec.begin();};
        Ulmap::iterator end(){return vec.end();};
        Ulmap::const_iterator begin()const{return vec.cbegin();};
        Ulmap::const_iterator end()const{return vec.cend();};
        Ulmap::const_iterator cbegin()const{return vec.cbegin();};
        Ulmap::const_iterator cend()const{return vec.cend();};
};/*}}}*/

class SCWClassifier {/*{{{*/
#ifndef KKN_UNIT_TEST 
    private: 
#else
    public:
#endif
        double C;
        double phi;
        double zeta;
        FeatureVector& mu;
        double psi;
        DiagMat sigmat; //本来は二次元だが対角行列だけ扱う
        inline double calc_alpha(double vt, double mt){// scw 1
            double alpha = (1.0/(vt*zeta)) * (-mt *psi + std::sqrt( (mt*mt) * (phi*phi*phi*phi / 4.0) + vt *phi*phi*zeta ));
            if( alpha < 0.0 )
                return 0.0;
            if( alpha > C )
                return C;
            return alpha;
        };
        inline double calc_beta(double alphat, double ut, double vt){
            return (alphat * phi)/(std::sqrt(ut) + (vt*alphat*phi));
        };
        inline double calc_ut(double alphat, double vt){
            double t=(-alphat * vt * phi + std::sqrt(alphat*alphat * vt*vt*phi*phi+ 4*vt));
            return (1.0/4.0)*t*t;
        };
        inline double calc_vt(const FeatureVector& xt){
            double sum = 0.0;
            for(auto x:xt){
                sum += x.second* x.second* sigmat[x.first];
            }
            return sum;
        };
    public:    
        SCWClassifier(double in_C, double in_phi, FeatureVector& in_mu):C(in_C),phi(in_phi),zeta(1+ in_phi*in_phi),mu(in_mu),psi(1.0 + (in_phi*in_phi)/2.0){ };
        void update(double loss_value, const FeatureVector& vec);
        void perceptron_update(const FeatureVector& vec);
};/*}}}*/
