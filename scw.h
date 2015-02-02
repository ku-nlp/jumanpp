
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <unordered_map>

class FeatureVector;
class DiagMat;

class FeatureVector{
    std::unordered_map<std::string,double> vec;
    
    public:
        FeatureVector(){vec.clear();};
        FeatureVector(const std::vector<std::string>& v1, const std::vector<std::string>& v2);
        void update(double alpha, double y, const DiagMat& sigma, const FeatureVector& xt);
        double operator* (const FeatureVector& fv) const;
        bool has_key(const std::string& str){
            return vec.find(str)!=vec.end();
        };
        double& operator[](const std::string& s){
            auto itr=vec.find(s);
            if(itr == vec.end()){
                vec[s] = 0;
                return vec[s];
            }else{
                return itr->second;
            }
        };
        inline void clear(){ vec.clear(); };
        //operator std::unordered_map<std::string,double>(){return vec;}
        std::unordered_map<std::string, double>& get_umap(){return vec;}
        std::unordered_map<std::string, double>::iterator begin(){return vec.begin();};
        std::unordered_map<std::string, double>::iterator end(){return vec.end();};
        std::unordered_map<std::string, double>::const_iterator begin()const{return vec.cbegin();};
        std::unordered_map<std::string, double>::const_iterator end()const{return vec.cend();};
        std::unordered_map<std::string, double>::const_iterator cbegin()const{return vec.cbegin();};
        std::unordered_map<std::string, double>::const_iterator cend()const{return vec.cend();};
};

class DiagMat{
    private:
        std::unordered_map<std::string, double> vec;
    public:
        void update(double beta, const FeatureVector& x);
        double get_value(const std::string& sp1)const;
        inline double& operator[](const std::string& s){ return ref_value(s);};
        double& ref_value(const std::string sp1, const std::string sp2);
        double& ref_value(const std::string sp1){ return ref_value(sp1,sp1);};
        std::unordered_map<std::string, double>::iterator begin(){return vec.begin();};
        std::unordered_map<std::string, double>::iterator end(){return vec.end();};
        std::unordered_map<std::string, double>::const_iterator begin()const{return vec.cbegin();};
        std::unordered_map<std::string, double>::const_iterator end()const{return vec.cend();};
        std::unordered_map<std::string, double>::const_iterator cbegin()const{return vec.cbegin();};
        std::unordered_map<std::string, double>::const_iterator cend()const{return vec.cend();};
};


class SCWClassifier {
#ifndef KKN_UNIT_TEST 
    private: 
#else
    public:
#endif
        double phi;
        double zeta;
        double C;
        double psi;
        FeatureVector& mu;
        DiagMat sigmat; //本来は二次元だが対角行列だけ扱う
        inline double calc_alpha(double vt, double mt){// scw 1
            double alpha = 1.0/(vt*zeta) * ( -mt *psi + std::sqrt( (mt*mt) * (phi*phi*phi*phi / 4.0) + vt *phi*phi*zeta ));
            if( alpha < 0.0)
                return 0.0;
            if( alpha > C)
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
        SCWClassifier(double in_C, double in_phi, FeatureVector& in_mu):C(in_C),phi(in_phi),zeta(1+ in_phi*in_phi),mu(in_mu),psi(1.0 + in_phi*in_phi){ };
        void update(double loss_value, const FeatureVector& vec);
};
