/******************************************************************************/
//                          class FeatureVector                               //
/******************************************************************************/

#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

typedef std::unordered_map<std::string,double> Umap;
typedef unsigned long Ulkey;
typedef double Ulval;
typedef std::unordered_map<Ulkey, Ulval> Ulmap; //参考に2015/11 時点での素性数 2,395,735
typedef std::unordered_map<std::string, unsigned long> Fimap; 
typedef std::pair<unsigned long, double> weightPair;

// Boost::serialize
#include <boost/functional/hash.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
// これをinclude しないとリンクに失敗する．(2015/12/14 boost 1.59)
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>

class FeatureVector{// ただのunordered_map のラップ/*{{{*/
    Ulmap vec;
    // Memory mapped file
    bool mmap_flag = false;

    public:
        FeatureVector(){vec.clear();};
        // Memory mapped file を使用するフラグ を受け取る.
        FeatureVector(bool in_mmap_flag){ mmap_flag = in_mmap_flag; };

        FeatureVector(const std::vector<unsigned long>& v1, const std::vector<unsigned long>& v2);
        FeatureVector(const std::vector<unsigned long>& v1);

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
        
        size_t size() const{return vec.size();};

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

        bool serialize(std::ofstream& os){
            size_t vec_size = vec.size();
            os.write((char*)&vec_size,sizeof(vec_size));
            for(auto const& kv: vec){
                os.write((char*)&kv.first, sizeof(Ulkey));
                os.write((char*)&kv.second, sizeof(Ulval));
            }
            return true;
        };

        bool deserialize(std::ifstream& is){
            vec.clear();
            size_t size = 0;
            is.read((char*)&size,sizeof(size_t));
            vec.reserve(size);
            std::cerr << size << std::endl;
            for (size_t i = 0; i != size; ++i) {
                Ulkey key;
                Ulval value;
                is.read((char*)&key,sizeof(Ulkey));
                is.read((char*)&value,sizeof(Ulval));
                vec[key] = value;
            }
            std::cerr << vec.size() << std::endl;
            if(vec.size() != size)
                return false;
            return true;
        };

private:
//    friend class boost::serialization::access;
//    template<class Archive>
//        void serialize(Archive& ar, const unsigned int version)
//        {
//            if(version)
//                ar & vec;
//            else
//                ar & vec;
//
//        }

};/*}}}*/
