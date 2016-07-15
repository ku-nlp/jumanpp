#include "feature_vector.h"

// 差分から新しいFeatureVectorオブジェクトを生成
FeatureVector::FeatureVector(const std::vector<unsigned long>& v1, const std::vector<unsigned long>& v2){/*{{{*/
    for(const auto& s:v1){
        vec[s] += 1.0;
    }
    for(const auto& s:v2){
        vec[s] -= 1.0;
    }
};/*}}}*/

// コピーコンストラクタ
FeatureVector::FeatureVector(const std::vector<unsigned long>& v1){/*{{{*/
    for(const auto& s:v1){
        vec[s] += 1.0;
    }
};/*}}}*/

double FeatureVector::operator* (const FeatureVector& fv) const{/*{{{*/
    double sum = 0.0;
    if(vec.size() > fv.vec.size()){
        for(const auto& f:fv){
            auto itr = vec.find(f.first);
            if(itr != vec.end())
                sum += f.second * itr->second;
        }
    }else{
        for(const auto& f:vec){
            auto itr = fv.vec.find(f.first);
            if(itr != fv.end())
                sum += f.second * itr->second;
        }
    }
    return sum;
};/*}}}*/

