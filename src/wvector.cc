#include "wvector.h"

std::unordered_map<std::string, int> WeightedVector::feature_map;

std::string WeightedVector::str(){
    if(!canonical) canonicalize();
    std::stringstream ss;
    BOOST_FOREACH( Unit x, vec){
        ss << x.first << ":" << x.second << " ";
    }
    return ss.str();
}

std::string WeightedVector::feature_map_str(){
    std::stringstream ss;
    BOOST_FOREACH( mapping x, feature_map){
        ss << x.first << ":" << x.second << " ";
    }
    return ss.str();
}

// O(1) 重複などを考慮しない，終わったらregularize 必要
WeightedVector& WeightedVector::add_without_check(int index, double value){
    this->vec.push_back(std::make_pair(index,value));
    canonical = false;
    return *this;
}

// 全部で\sum_1^N log2(n)+n, ソート不要// 効率悪そう
WeightedVector& WeightedVector::add_with_check(int index, double value){
    if(!canonical) canonicalize();

    auto result = std::lower_bound(this->vec.begin(), this->vec.end(), std::make_pair(index,value), [](auto x, auto y){return (x.first > y.first);} );
    if(result == this->vec.end()){
        this->vec.push_back(std::make_pair(index,value));
    }else if(result->first > index){ //通り過ぎた
        this->vec.insert(result, std::make_pair(index,value));
    }else{
        result->second += value;
    }
    return *this;
}

//ソートし，同じindex をまとめる M>N 最悪Mlog2 M + N
WeightedVector& WeightedVector::canonicalize(){
    std::sort(this->vec.begin(), this->vec.end());
    
    // 重複をまとめる
    for(auto it = this->vec.begin(); it!=this->vec.end();){
        if( (it+1) != this->vec.end() && it->first == (it+1)->first){
            (it+1)->second += it->second;
            it = vec.erase(it);
        }else{
            ++it;
        }
    }
    canonical = true;
    return *this;
}

WeightedVector& WeightedVector::add(std::string feature, double value){
    auto result = feature_map.find(feature);
    if(result == feature_map.end()){
        auto fsize = feature_map.size();
        return add(feature_map[feature]=fsize ,value);
    }else {
        return add(result->second, value);
    }
};

double WeightedVector::product( vector_imp& r_vec){
    if(!canonical) canonicalize();
    double result = 0.0;

    vector_imp::iterator it_l = this->vec.begin();
    vector_imp::iterator it_r = r_vec.begin();

    while(!(it_l == this->vec.end() && it_r == r_vec.end()) ){
        if( it_l == this->vec.end() || it_l->first > it_r->first ){
            it_r++;
        } else if( it_r == r_vec.end() || it_l->first < it_r->first){
            it_l++;
        } else if(it_l->first == it_r->first){
            result += it_l->second * it_r->second;
            it_l++;it_r++;
        }
    }
    return result;
}

WeightedVector& WeightedVector::merge(double coef, WeightedVector& r_wvec){
    if(!canonical) canonicalize();
    if(!r_wvec.canonical) r_wvec.canonicalize();

    vector_imp& l_vec = this->vec;
    vector_imp& r_vec = r_wvec.vec;

    auto it_l = 0;
    auto it_r = 0;
    auto r_end = r_wvec.vec.size();
    auto l_end = this->vec.size();

    while(!(it_l == l_end && it_r == r_end) ){
        if( it_l == l_end || l_vec[it_l].first > r_vec[it_r].first ){
            this->vec.push_back(std::make_pair(r_vec[it_r].first, r_vec[it_r].second * coef));
            it_r++;
        } else if( it_r == r_end || l_vec[it_l].first < r_vec[it_r].first){
            it_l++;
        } else if(l_vec[it_l].first == r_vec[it_r].first){
            l_vec[it_l].second = l_vec[it_l].second + coef*r_vec[it_r].second;
            it_l++;it_r++;
        }
    }
    std::sort(this->vec.begin(), this->vec.end());
    
    return *this;      
}

WeightedVector* WeightedVector::operator+ ( vector_imp& r_vec ){
    if(!canonical) canonicalize();
    WeightedVector* tmp = new WeightedVector();
    vector_imp::iterator it_l = this->vec.begin();
    vector_imp::iterator it_r = r_vec.begin();

    while(!(it_l == this->vec.end() && it_r == r_vec.end()) ){
        if( it_l == this->vec.end() || it_l->first > it_r->first ){
            tmp->vec.push_back( *it_r );
            it_r++;
        } else if( it_r == r_vec.end() || it_l->first < it_r->first){
            tmp->vec.push_back( *it_l ); 
            it_l++;
        } else if(it_l->first == it_r->first){
            tmp->vec.push_back( std::make_pair(it_l->first, it_l->second + it_r->second));
            it_l++;it_r++;
        }
    }

    return tmp;
}

//double kl_divergence(double mean1, double mean2, const WeightedVector& variance1, const WeightedVector& variance2) {
//
//}


