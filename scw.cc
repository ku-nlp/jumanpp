
#include "scw.h"

FeatureVector::FeatureVector(const std::vector<std::string>& v1, const std::vector<std::string>& v2){
    for(const auto& s:v1){
        vec[s] = 1.0;
    }
    for(const auto& s:v2){
        vec[s] -= 1.0;
    }
};

FeatureVector::FeatureVector(const std::vector<std::string>& v1){
    for(const auto& s:v1){
        vec[s] = 1.0;
    }
};

double FeatureVector::operator* (const FeatureVector& fv) const{
    double sum = 0.0;
    for(const auto& f:fv){
        auto itr = vec.find(f.first);
        if(itr != vec.end())
            sum += f.second * itr->second;
    }
    return sum;
};

void FeatureVector::update(double alpha, double y,const DiagMat& sigma, const FeatureVector& x){//updated mu
    for(const auto& v:x){
        vec[v.first] += alpha * y * sigma.get_value(v.first) * v.second;
        // v の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};


void DiagMat::update(double beta, const FeatureVector& x){ // update sigma
    for(const auto& v:x){
        this->ref_value(v.first,v.first) -= beta * this->ref_value(v.first) * v.second * v.second * this->ref_value(v.first);
        // sigma の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};

double& DiagMat::ref_value(const std::string sp1, const std::string sp2){
    std::string sp = sp1 + "=" + sp2;
    auto itr = vec.find(sp);
    if( itr == vec.end() ){// キーが無い場合
        if(sp1 == sp2) //対角行列は1.0
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

void SCWClassifier::update(double loss_value, const FeatureVector& vec){
    //if(loss_value < 0.00001) return;
    //loss_value = 1.0; //ロスの使い方？
    double score = mu * vec;
    double vt = calc_vt(vec);
    double alphat = calc_alpha(vt, loss_value*score);
    double ut = calc_ut(alphat, vt);
    double betat = calc_beta(alphat, ut, vt);
    //    (double alpha, double y, const DiagMat sigma, const FeatureVector& x)
    std::cerr << "alpha:" << alphat << " beta:" << betat << " score:" << score <<" y:"  << (double)loss_value << std::endl;

    mu.update(alphat, loss_value, sigmat, vec);
    sigmat.update(betat, vec);
}

void SCWClassifier::perceptron_update(const FeatureVector& vec){
    double score = mu * vec;
    //double vt = calc_vt(vec);
    //double alphat = calc_alpha(vt, loss_value*score);
    //double ut = calc_ut(alphat, vt);
    //double betat = calc_beta(alphat, ut, vt);
    //    (double alpha, double y, const DiagMat sigma, const FeatureVector& x)
    std::cerr << "mu*vec:" << score << std::endl;

    mu.update(1.0, 1.0, sigmat, vec);
    //sigmat.update(betat, vec);
}


