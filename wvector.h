
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
//#include <boost/foreach.hpp>

class WeightedVector {
    typedef std::pair<int, double>  Unit;
    typedef std::pair<std::string, int> mapping;
    typedef std::vector< Unit >  vector_imp;
#ifndef KKN_UNIT_TEST 
    private: 
#else
    public:
#endif
        static std::unordered_map<std::string,int> feature_map; 
        bool canonical;
        double product(vector_imp& r_vec );
        double operator*(vector_imp& r_vec ){return this->product(r_vec);};
        WeightedVector* operator+(vector_imp& r_vec );
        WeightedVector& add_without_check(int index, double value);
        WeightedVector& add_with_check(int index, double value);

    public:

        WeightedVector(){ vec.clear(); canonical = false; };
        inline WeightedVector& add(int index, double value){ 
            return add_without_check(index,value); 
        };
        WeightedVector& add(std::string feature, double value);
        WeightedVector& canonicalize();
            
        std::string str();
        std::string feature_map_str();
        double product( WeightedVector& r_wvec ){
            if(r_wvec.canonical)canonicalize(); 
            return this->product(r_wvec.vec); 
        };
        double operator*( WeightedVector& r_wvec ){
            if(r_wvec.canonical)canonicalize(); 
            return this->product(r_wvec);
        };
        WeightedVector* operator+( WeightedVector& r_wvec ){ 
            if(r_wvec.canonical)canonicalize(); 
            return *this + r_wvec.vec; 
        };
            
        WeightedVector& merge(double coef, WeightedVector& r_wvec);
        
        // TODO:
        //CW 的なadaptive margin 用の更新はどうするべきか.
        //イテレータだけ渡せば良い？
#ifndef KKN_UNIT_TEST 
    protected: 
#else:
    public:
#endif
        vector_imp vec;

};

