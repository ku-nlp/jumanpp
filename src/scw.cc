#include "scw.h"
#include <math.h>

void DiagMat::update(double beta, const FeatureVector &x) { // update sigma
    for (const auto &v : x) {
        this->ref_value(v.first, v.first) -= beta * this->ref_value(v.first) *
                                             v.second * v.second *
                                             this->ref_value(v.first);
        // sigma の対角要素以外を考える場合は v.first
        // と他のキーの組み合わせを計算
    }
};

double &DiagMat::ref_value(const unsigned long sp1, const unsigned long sp2) {
    auto key = sp1;
    // 対角行列以外は非サポートだが，とりあえず計算する
    murmur_combine(key, sp2);

    auto itr = vec.find(key);
    if (itr != vec.cend()) {
        return itr->second;
    } else {
        if (sp1 == sp2) {
            auto &ref = vec[key];
            ref = 1.0;
            return ref;
        } else {
            auto &ref = vec[key];
            ref = 0.0;
            std::cerr << "Error@DiagMat. unsupported method. " << std::endl;
            return ref;
        }
    }
};

double DiagMat::get_value(const unsigned long sp1) const {
    auto key = sp1;
    murmur_combine(key, sp1);

    auto itr = vec.find(key);
    if (itr != vec.cend())
        return itr->second;
    else
        return 1.0;
};

void SCWClassifier::update_mu(double alpha, double y, const DiagMat &sigma,
                              const FeatureVector &x) {
    for (const auto &v : x) {
        mu[v.first] += alpha * y * sigma.get_value(v.first) * v.second;
        // v の対角要素以外の計算は省略する.
    }
};

void SCWClassifier::update(double loss_value, const FeatureVector &vec) {
    if (loss_value < 0.00001)
        return;
    double score = mu * vec;

    double vt = calc_vt(vec);
    double alphat = calc_alpha(vt, loss_value * score);
    double ut = calc_ut(alphat, vt);
    double betat = calc_beta(alphat, ut, vt);
    if (!std::isfinite(betat))
        return;

    //    (double alpha, double y, const DiagMat sigma, const FeatureVector& x)
    // std::cerr << "alpha:" << alphat << " beta:" << betat << " score:" <<
    // score <<" y:"  << (double)loss_value << "\t";
    // std::cerr << "\ty:"  << (double)loss_value << "\t";

    if (vt == 0.0)
        return;
    update_mu(alphat, loss_value, sigmat, vec);
    sigmat.update(betat, vec);
}

void SCWClassifier::perceptron_update(const FeatureVector &vec) {
    double score = mu * vec;
    // double vt = calc_vt(vec);
    // double alphat = calc_alpha(vt, loss_value*score);
    // double ut = calc_ut(alphat, vt);
    // double betat = calc_beta(alphat, ut, vt);
    //    (double alpha, double y, const DiagMat sigma, const FeatureVector& x)
    std::cerr << "mu*vec:" << score << std::endl;

    update_mu(1.0, 1.0, sigmat, vec);
    // sigmat.update(betat, vec);
}
