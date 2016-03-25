#include "common.h"
#include "pos.h"

namespace Morph {
Pos::Pos() {}



unsigned long Pos::get_id(const std::string& pos_str) {
    if (dic.find(pos_str) == dic.end()) {
        // count する代わりにハッシュを計算することで，辞書に項目を追加しても学習したモデルを再利用できる
        dic[pos_str] = str_hash(pos_str); 
       
        // DEBUG: 衝突していないか，チェックする. //2015/11/06 チェックしたが衝突は無し
//        if( rdic.find(dic[pos_str]) != rdic.end() ){
//            std::cerr << "pos_str:" << pos_str << ", prev_str:" << rdic[dic[pos_str]] << std::endl;
//        }
        rdic[dic[pos_str]] = pos_str;
    } else {
        // 何もしない
    }
    return dic[pos_str];
}

unsigned long Pos::get_id(const char* pos_str) {
    std::string pos_string = pos_str;
    return get_id(pos_string);
}

std::string* Pos::get_pos(unsigned long posid) {
//    if (posid < count) {
//        return &(rdic[posid]);
//    } else {
//        cerr << ";; invalid posid:" << posid << endl;
//        return NULL;
//    }
      if(rdic.count(posid) == 0){
        cerr << ";; invalid posid:" << posid << endl;
        assert(rdic.count(posid) != 0);
        return NULL;
      }else{
        return &(rdic[posid]);
      }
}

bool Pos::write_pos_list(const std::string& pos_filename) {
    std::ofstream pos_out(pos_filename.c_str(), std::ios::out);
    if (!pos_out.is_open()) {
        cerr << ";; cannot open pos list for writing" << endl;
        return false;
    }

    for (std::unordered_map<std::string, unsigned long>::iterator it =
             dic.begin();
         it != dic.end(); it++) {
        pos_out << it->first << "\t" << it->second << endl;
    }

    pos_out.close();
    return true;
}

bool Pos::read_pos_list(const std::string& pos_filename) {
    std::ifstream pos_in(pos_filename.c_str(), std::ios::in);
    if (!pos_in.is_open()) {
        cerr << ";; cannot open pos list for reading (" << pos_filename.c_str()
             << endl;
        return false;
    }

    std::string buffer;
    while (getline(pos_in, buffer)) {
        std::vector<std::string> line;
        split_string(buffer, "\t", line);
        //dic[line[0]] = atoi(static_cast<const char*>(line[1].c_str()));
        //rdic[atoi(static_cast<const char*>(line[1].c_str()))] = line[0];
        dic[line[0]] = strtoul(static_cast<const char*>(line[1].c_str()),nullptr,10);
        rdic[dic[line[0]]] = line[0];
        //count++;
    }

    pos_in.close();
    return true;
}
}
