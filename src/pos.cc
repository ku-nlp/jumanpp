#include "common.h"
#include "pos.h"

namespace Morph {
Pos::Pos() {}

// 新規の品詞を登録する（BOS，UNK など）
unsigned long Pos::register_pos(const std::string &pos_str) { //{{{
    auto hashval = str_hash(pos_str);
    auto itr = rdic.find(hashval);

    if (itr == rdic.end()) {
        rdic[hashval] = pos_str;
    }
    return hashval;
} //}}}

unsigned long Pos::register_pos(char const *pos_str) { //{{{
    std::string str = std::string(pos_str);
    return register_pos(str);
} //}}}

std::string *Pos::get_pos(unsigned long posid) { //{{{
    // メモリは余分に使うが，rd_dic で引いた結果を rdic に移してその結果を返す
    auto rdic_itr = rdic.find(posid);
    if (rdic_itr != rdic.end()) {
        return &(rdic_itr->second);
    } else {
        auto rd_dic_itr = rd_dic->find(posid);
        if (rd_dic_itr != rd_dic->end()) {
            rdic[posid] = std::string(rd_dic_itr->second.c_str());
            return &(rdic[posid]);
        }
    }

    cerr << ";; invalid posid:" << posid << endl;
    exit(1);
    return NULL;
} //}}}

bool Pos::write_pos_list(const std::string &pos_filename) { //{{{
    std::ofstream pos_out(pos_filename.c_str(), std::ios::out);
    if (!pos_out.is_open()) {
        cerr << ";; cannot open pos list for writing" << endl;
        return false;
    }

    for (auto it = rdic.begin(); it != rdic.end(); it++) {
        pos_out << it->second << "\t" << it->first << endl;
    }

    pos_out.close();
    return true;
} //}}}

bool Pos::read_pos_list(const std::string &pos_filename) { //{{{
    if (valid_mmap(pos_filename)) {
        load_mmap(pos_filename);
    } else {
        load_selialized_map(pos_filename);
    }
    return true;
} //}}}
}
