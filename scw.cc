
#include "scw.h"

void FeatureVector::update(double alpha, double y,const DiagMat& sigma, const FeatureVector& x){//updated mu
    for(const auto& v:x){
        vec[v.first] += alpha * y * sigma.get_value(v.first) * v.second;
        // v の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};

void DiagMat::update(double beta, const FeatureVector& x){ // update sigma
    for(auto v:x){
        this->ref_value(v.first,v.first) -= beta * this->ref_value(v.first) * v.second * v.second * this->ref_value(v.first);
        // sigma の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};

double& DiagMat::ref_value(const std::string sp1, const std::string sp2){
    std::string sp = sp1 + "=" + sp2;
    auto itr = vec.find(sp);
    if( itr == vec.end() ){// キーが無い場合
        if(sp1 == sp2)//対角行列は1.0
            vec[sp] = 1.0;
        else
            vec[sp] = 0.0;
    }
    return vec[sp];
};

double DiagMat::get_value(const std::string& sp1)const{ 
    auto itr = vec.find(sp1+"="+sp1);
    if(itr != vec.cend())
        return itr->second;
    else
        return 1.0;
};

