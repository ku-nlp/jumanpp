#include "scw.h"
//Fimap FeatureVector::str2id;

// 訓練時用だが，実際に使う？？
FeatureVector::FeatureVector(const std::vector<unsigned long>& v1, const std::vector<unsigned long>& v2){/*{{{*/
    for(const auto& s:v1){
        vec[s] += 1.0;
    }
    for(const auto& s:v2){
        vec[s] -= 1.0;
    }
};/*}}}*/

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

//updated mu
void FeatureVector::update(double alpha, double y,const DiagMat& sigma, const FeatureVector& x){/*{{{*/
    for(const auto& v:x){
        vec[v.first] += alpha * y * sigma.get_value(v.first) * v.second;
        // v の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};/*}}}*/

void DiagMat::update(double beta, const FeatureVector& x){ // update sigma
    for(const auto& v:x){
        this->ref_value(v.first,v.first) -= beta * this->ref_value(v.first) * v.second * v.second * this->ref_value(v.first);
        // sigma の対角要素以外を考える場合は v.first と他のキーの組み合わせを計算
    }
};

double& DiagMat::ref_value(const unsigned long sp1, const unsigned long sp2){
    auto key = sp1;
    // 対角行列以外は非サポートだが，とりあえず計算する
    boost::hash_combine(key,sp2);

    auto itr = vec.find(key);
    if(itr != vec.cend()){
        return itr->second;
    }else{
        if(sp1 == sp2){
            auto& ref = vec[key];
            ref = 1.0;
            return ref;
        }else{
            auto &ref = vec[key];
            ref = 0.0;
            std::cerr << "Error@DiagMat. unsupported method. " << std::endl;
            return ref;
        }
    }
};

double DiagMat::get_value(const unsigned long sp1)const{ 
    auto key = sp1;
    boost::hash_combine(key,sp1);
        
    auto itr = vec.find(key);
    if(itr != vec.cend())
        return itr->second;
    else
        return 1.0;
};

void SCWClassifier::update(double loss_value, const FeatureVector& vec){
    if(loss_value < 0.00001) return;
    double score = mu * vec;
    if(score == 0.0) return;

    double vt = calc_vt(vec);
    double alphat = calc_alpha(vt, loss_value*score);
    //double alphat = calc_alpha(vt, score);
    double ut = calc_ut(alphat, vt);
    double betat = calc_beta(alphat, ut, vt);
    //    (double alpha, double y, const DiagMat sigma, const FeatureVector& x)
    std::cerr << "alpha:" << alphat << " beta:" << betat << " score:" << score <<" y:"  << (double)loss_value << "\t";
    std::cerr << "\ty:"  << (double)loss_value << "\t";

    if( vt == 0.0 ) return;
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


