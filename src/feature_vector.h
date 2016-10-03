/******************************************************************************/
//                          class FeatureVector                               //
/******************************************************************************/

#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include "hash.h"

#include <boost/unordered_map.hpp>
#define BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
namespace bip = boost::interprocess;

typedef std::unordered_map<std::string, double> Umap;
typedef uint64_t Ulkey;
typedef double Ulval;
typedef std::unordered_map<Ulkey, Ulval>
    Ulmap; //参考に2015/11 時点での素性数 2,395,735
typedef std::unordered_map<std::string, unsigned long> Fimap;
typedef std::pair<Ulkey, Ulval> weightPair;

// 動的読み込みのための定義
typedef bip::allocator<weightPair, bip::managed_mapped_file::segment_manager>
    allocator_dmap;

// 予め文字列をハッシュ化して与える
typedef boost::unordered_map<Ulkey, Ulval, DummyHash, std::equal_to<Ulkey>,
                             allocator_dmap> Uldmap;

class FeatureVector;
class FeatureVectorIterator;
class ConstFeatureVectorIterator;

// TODO: Static と Dynamic を別クラスに分けて，共通の親クラスにまとめたい
class FeatureVector {/*{{{*/
    friend FeatureVectorIterator;
    friend ConstFeatureVectorIterator;

    /* 本体 */
  private:
    Ulmap vec; // Static 用
    bip::managed_mapped_file *p_file_weight = nullptr;
    std::unique_ptr<Uldmap> dvec = nullptr; // Dynamic 用

    /* Dynamic読み込み用 */
  private:
    // メモリマップ操作用
    // Memory mapped file
    bool mmap_flag = false;

    /* 共用イテレータ */
  public:
    typedef FeatureVectorIterator iterator;
    typedef ConstFeatureVectorIterator const_iterator;
    FeatureVector::iterator begin();
    FeatureVector::iterator end();
    FeatureVector::const_iterator begin() const;
    FeatureVector::const_iterator end() const;

  public:
    FeatureVector() { vec.clear(); };
    // Memory mapped file を使用するフラグ を受け取る.
    FeatureVector(std::string mapfile, bool in_mmap_flag = true) { //{{{
        mmap_flag = in_mmap_flag;
        if (mmap_flag) {
            create_mmap(mapfile);
        }
    };                 //}}}
    ~FeatureVector() { //{{{
        //メモリマップは開放しない
        if (p_file_weight != nullptr) {
            p_file_weight->flush();
            delete (p_file_weight);
        }
        dvec.release();
    } //}}}

    FeatureVector(const std::vector<unsigned long> &v1,
                  const std::vector<unsigned long> &v2);
    FeatureVector(const std::vector<unsigned long> &v1);
    FeatureVector(const FeatureVector &);

    double operator*(const FeatureVector &fv) const;
    FeatureVector &operator=(const FeatureVector &fv);
    double &operator[](const unsigned long s) { /*{{{*/
        static double dummy = 0;
        if (mmap_flag) {
            auto itr = dvec->find(s);
            if (itr == dvec->end()) {
                // auto &ref = (*dvec)[s]; // 書き込み禁止なのでsegf
                dummy = 0;
                return dummy;
            } else {
                return itr->second;
            }
        } else {
            auto itr = vec.find(s);
            if (itr == vec.end()) {
                auto &ref = vec[s];
                ref = 0;
                return ref;
            } else {
                return itr->second;
            }
        }
    }; /*}}}*/

    double get_val(const unsigned long s) { /*{{{*/
        if (mmap_flag) {
            auto itr = dvec->find(s);
            if (itr == dvec->end()) {
                return 0;
            } else {
                return itr->second;
            }
        } else {
            auto itr = vec.find(s);
            if (itr == vec.end()) {
                return 0;
            } else {
                return itr->second;
            }
        }
    }; /*}}}*/

    inline size_t size() const { //{{{
        if (mmap_flag) {
            return dvec->size();
        } else {
            return vec.size();
        }
    };                    //}}}
    inline void clear() { //{{{
        if (mmap_flag) {
            dvec->clear();
        } else {
            vec.clear();
        }
    }; //}}}

    FeatureVector &merge(const FeatureVector &fv);
    FeatureVector &diff(const FeatureVector &fv);

    FeatureVector::iterator find(const unsigned long key);
    FeatureVector::const_iterator find(const unsigned long key) const;

    inline std::string str() { /*{{{*/
        std::stringstream ss;
        if (mmap_flag) {
            for (const auto &st : *dvec) {
                ss << st.first << ":" << st.second << " ";
            }
        } else {
            for (const auto &st : vec) {
                ss << st.first << ":" << st.second << " ";
            }
        }
        ss << std::endl;
        return std::move(ss.str());
    }; /*}}}*/

    bool serialize(std::ofstream &os) { //{{{
        size_t vec_size = vec.size();
        os.write((char *)&vec_size, sizeof(vec_size));
        for (auto const &kv : vec) {
            os.write((char *)&kv.first, sizeof(Ulkey));
            os.write((char *)&kv.second, sizeof(Ulval));
        }
        return true;
    };                                    //}}}
    bool deserialize(std::ifstream &is) { //{{{
        vec.clear();
        size_t size = 0;
        is.read((char *)&size, sizeof(size_t));
        vec.reserve(size);
        // std::cerr << size << std::endl;
        for (size_t i = 0; i != size; ++i) {
            Ulkey key;
            Ulval value;
            is.read((char *)&key, sizeof(Ulkey));
            is.read((char *)&value, sizeof(Ulval));
            vec[key] = value;
        }
        // std::cerr << vec.size() << std::endl;
        if (vec.size() != size)
            return false;
        return true;
    };                                            //}}}
    bool dynamic_serialize(std::string mapfile) { //{{{
        // メモリマップへの書き込みを flush
        p_file_weight->flush();
        // メモリマップのサイズ shrink
        bip::managed_mapped_file::shrink_to_fit(mapfile.c_str());
        return true;
    };                                                                 //}}}
    bool dynamic_deserialize(std::ifstream &is, std::string mapfile) { //{{{
        if (exist_mmap(mapfile)) {
            load_mmap(mapfile);
        } else {
            load_selialized_map(is, mapfile);
        }
        return true;
    }; //}}}

  private:
    // メモリマップファイルの存在を確認する．
    bool exist_mmap(std::string mapfile) { //{{{
        return (access(mapfile.c_str(), F_OK) != -1);
    };                                      //}}}
    bool create_mmap(std::string mapfile) { //{{{
        mmap_flag = true;

        // 大きめに1GBとる (通常は40MB程度?，あとでshrink)
        unsigned long long map_weight_size = 1024 * 1024 * 1024;
        // メモリマップファイルを作成
        p_file_weight = new bip::managed_mapped_file(
            bip::create_only, mapfile.c_str(), map_weight_size);
        // メモリマップ内にvocablary 用の領域を確保
        auto weight_map = p_file_weight->construct<Uldmap>("map_weight")(
            0, DummyHash(), std::equal_to<Ulkey>(),
            p_file_weight->get_allocator<weightPair>());
        dvec.reset(weight_map);
        return true;
    }                                                                  //}}}
    bool load_selialized_map(std::ifstream &is, std::string mapfile) { //{{{
        Ulkey key;
        Ulval value;
        create_mmap(mapfile);

        vec.clear();
        dvec->clear();
        size_t size = 0;

        is.read((char *)&size, sizeof(size_t));
        for (size_t i = 0; i != size; ++i) {
            is.read((char *)&key, sizeof(Ulkey));
            is.read((char *)&value, sizeof(Ulval));
            (*dvec)[key] = value;
        }

        // メモリマップへの書き込みを flush
        p_file_weight->flush();
        // メモリマップのサイズ shrink
        bip::managed_mapped_file::shrink_to_fit(mapfile.c_str());

        if (dvec->size() != size)
            return false;
        return true;
    };                                    //}}}
    bool load_mmap(std::string mapfile) { //{{{
        mmap_flag = true;
        p_file_weight =
            new bip::managed_mapped_file(bip::open_read_only, mapfile.c_str());
        dvec.reset(p_file_weight->find<Uldmap>("map_weight").first);
        return bool(dvec);
    } //}}}
};    /*}}}*/

class ConstFeatureVectorIterator
    : public std::iterator<std::forward_iterator_tag, Ulkey> { //{{{
    friend FeatureVector;
    //値の場所を示すメンバ変数と、取り扱う CMyClass へのポインタを private
    //に格納します。
  private:
    bool mmap_flag = false;
    // Static or Dynamic のFeatureVector iterator を持っておく

    FeatureVector const *fv;
    Ulmap::const_iterator itr;
    Uldmap::const_iterator ditr;

    // イテレータを構築するコンストラクタは private
    // に持たせ、外部で勝手に作れないようにします。
  private:
    ConstFeatureVectorIterator() { fv = nullptr; };
    ConstFeatureVectorIterator(FeatureVector *in_fv) {
        fv = in_fv;
        if (in_fv && in_fv->mmap_flag) {
            mmap_flag = true;
            ditr = std::begin(*(in_fv->dvec));
            return;
        } else if (in_fv) {
            mmap_flag = false;
            itr = std::begin(in_fv->vec);
            return;
        }
    };
    ConstFeatureVectorIterator(FeatureVector const *in_fv, bool begin) { //{{{
        fv = in_fv;
        if (begin) {
            if (in_fv && in_fv->mmap_flag) {
                mmap_flag = true;
                ditr = std::begin(*(in_fv->dvec));
                return;
            } else if (in_fv) {
                mmap_flag = false;
                itr = std::begin(in_fv->vec);
                return;
            }
        } else {
            if (in_fv && in_fv->mmap_flag) {
                mmap_flag = true;
                ditr = std::end(*(in_fv->dvec));
                return;
            } else if (in_fv) {
                mmap_flag = false;
                itr = std::end(in_fv->vec);
                return;
            }
        }
    }; //}}}

    ConstFeatureVectorIterator(FeatureVector const *in_fv,
                               Ulmap::const_iterator in_itr) { //{{{
        mmap_flag = false;
        fv = in_fv;
        itr = in_itr;
        return;
    }; //}}}

    ConstFeatureVectorIterator(FeatureVector const *in_fv,
                               Uldmap::const_iterator in_itr) { //{{{
        fv = in_fv;
        ditr = in_itr;
        return;
    }; //}}}

  public:
    ConstFeatureVectorIterator(const ConstFeatureVectorIterator &iterator) =
        default;

  public:
    ConstFeatureVectorIterator &
    operator=(const ConstFeatureVectorIterator &in) = default;

    ConstFeatureVectorIterator &operator++() { //{{{
        if (fv->mmap_flag) {
            (++ditr);
            return *this;
        } else {
            (++itr);
            return *this;
        }
    }; //}}}

    ConstFeatureVectorIterator operator++(int) { //{{{
        if (fv && fv->mmap_flag) {
            auto tmp = *this;
            (++ditr);
            return tmp;
        } else {
            auto tmp = *this;
            (++itr);
            return tmp;
        }
    }; //}}}

    auto operator*() const -> decltype(*ditr) { //{{{
        if (fv && fv->mmap_flag) {
            return (*ditr);
        } else {
            return (*itr);
        }
    }; //}}}

    bool operator==(const ConstFeatureVectorIterator &iterator) { //{{{
        // 両方 end ならtrue
        if (((mmap_flag && ditr == fv->dvec->end()) ||
             (!mmap_flag && itr == fv->vec.end())) &&
            ((iterator.mmap_flag &&
              iterator.ditr == iterator.fv->dvec->end()) ||
             (!iterator.mmap_flag && iterator.itr == iterator.fv->vec.end())))
            return true;

        if (iterator.mmap_flag != mmap_flag)
            return false;
        if ((mmap_flag && ditr == iterator.ditr) ||
            (!mmap_flag && itr == iterator.itr))
            return true;
        return false;
    }; //}}}

    bool operator!=(const ConstFeatureVectorIterator &iterator) { //{{{
        return !(*this == iterator);
    }; //}}}

    auto operator-> () -> decltype(&(*ditr)) { //{{{
        if (fv && fv->mmap_flag) {
            return &(*ditr);
        } else {
            return &(*itr);
        }
    }; //}}}
};     //}}}

class FeatureVectorIterator
    : public std::iterator<std::forward_iterator_tag, Ulkey> { //{{{
    friend FeatureVector;
    //値の場所を示すメンバ変数と、取り扱う CMyClass へのポインタを private
    //に格納します。
  private:
    bool mmap_flag = false;
    // Static or Dynamic のFeatureVector iterator を持っておく

    FeatureVector *fv;
    Ulmap::iterator itr;
    Uldmap::iterator ditr;

    // イテレータを構築するコンストラクタは private
    // に持たせ、外部で勝手に作れないようにします。
  private:
    FeatureVectorIterator() { fv = nullptr; };
    FeatureVectorIterator(FeatureVector *in_fv) { //{{{
        fv = in_fv;
        if (in_fv && in_fv->mmap_flag) {
            mmap_flag = true;
            ditr = std::begin(*(in_fv->dvec));
            return;
        } else if (in_fv) {
            mmap_flag = false;
            itr = std::begin(in_fv->vec);
            return;
        }
    };                                                        //}}}
    FeatureVectorIterator(FeatureVector *in_fv, bool begin) { //{{{
        fv = in_fv;
        if (begin) {
            if (in_fv && in_fv->mmap_flag) {
                mmap_flag = true;
                ditr = std::begin(*(in_fv->dvec));
                return;
            } else if (in_fv) {
                mmap_flag = false;
                itr = std::begin(in_fv->vec);
                return;
            }
        } else {
            if (in_fv && in_fv->mmap_flag) {
                mmap_flag = true;
                ditr = std::end(*(in_fv->dvec));
                return;
            } else if (in_fv) {
                mmap_flag = false;
                itr = std::end(in_fv->vec);
                return;
            }
        }
    }; //}}}

    FeatureVectorIterator(FeatureVector *in_fv, Ulmap::iterator in_itr) { //{{{
        mmap_flag = false;
        fv = in_fv;
        itr = (in_itr);
        return;
    }; //}}}

    FeatureVectorIterator(FeatureVector *in_fv, Uldmap::iterator in_itr) { //{{{
        fv = in_fv;
        ditr = (in_itr);
        return;
    }; //}}}

  public:
    FeatureVectorIterator(const FeatureVectorIterator &iterator) = default;
    FeatureVectorIterator &
    operator=(const FeatureVectorIterator &iterator) = default;

  public:
    FeatureVectorIterator &operator++() { //{{{
        if (fv->mmap_flag) {
            (++ditr);
            return *this;
        } else if (fv) {
            (++itr);
            return *this;
        }
        return *this;
    }; //}}}

    FeatureVectorIterator operator++(int) { //{{{
        if (fv && fv->mmap_flag) {
            auto tmp = *this;
            (++ditr);
            return tmp;
        } else {
            auto tmp = *this;
            (++itr);
            return tmp;
        }
    }; //}}}

    const weightPair operator*() const { //{{{
        if (fv && fv->mmap_flag) {
            return (*ditr);
        } else {
            return (*itr);
        }
    }; //}}}

    bool operator==(const FeatureVectorIterator &iterator) { //{{{
        // 両方 end ならtrue
        if (((mmap_flag && ditr == fv->dvec->end()) ||
             (!mmap_flag && itr == fv->vec.end())) &&
            ((iterator.mmap_flag &&
              iterator.ditr == iterator.fv->dvec->end()) ||
             (!iterator.mmap_flag && iterator.itr == iterator.fv->vec.end())))
            return true;

        if (iterator.mmap_flag != mmap_flag)
            return false;
        if ((mmap_flag && ditr == iterator.ditr) ||
            (!mmap_flag && itr == iterator.itr))
            return true;
        return false;
    }; //}}}

    bool operator!=(const FeatureVectorIterator &iterator) { //{{{
        return !(*this == iterator);
    }; //}}}

    auto operator-> () -> decltype(&(*ditr)) { //{{{
        if (fv && fv->mmap_flag) {
            return &(*ditr);
        } else {
            return &(*itr);
        }
    }; //}}}
};     //}}}

// TODO:FeatureVectorIteratorクラス, ConstFeatureVectorIterator クラスをtemplate
// で書き直す
