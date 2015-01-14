#include "common.h"
#include "pos.h"

namespace Morph {

Pos::Pos(): count(0) {}

unsigned long Pos::get_id(const std::string &pos_str) {
    if (dic.find(pos_str) == dic.end()) {
        // cerr << "DEBUG: " << pos_str << " -> " << count << " (new)" << endl;
        dic[pos_str] = count++;
        rdic[dic[pos_str]] = pos_str;
    }
    else {
        // cerr << "DEBUG: " << pos_str << " -> " << dic[pos_str] << endl;
    }
    return dic[pos_str];
}

unsigned long Pos::get_id(const char *pos_str) {
    std::string pos_string = pos_str;
    return get_id(pos_string);
}

std::string *Pos::get_pos(unsigned long posid) {
    if (posid < count) {
        return &(rdic[posid]);
    }
    else {
        cerr << ";; invalid posid:" << posid << endl;
        return NULL;
    }
}


bool Pos::write_pos_list(const std::string &pos_filename) {

    std::ofstream pos_out(pos_filename.c_str(), std::ios::out);
    if (!pos_out.is_open()) {
        cerr << ";; cannot open pos list for writing" << endl;
        return false;
    }

    for (std::map<std::string, unsigned long>::iterator it = dic.begin(); it != dic.end(); it++) {
        pos_out << it->first << "\t" << it->second << endl;
    }

    pos_out.close();
    return true;
}

bool Pos::read_pos_list(const std::string &pos_filename) {

    std::ifstream pos_in(pos_filename.c_str(), std::ios::in);
    if (!pos_in.is_open()) {
        cerr << ";; cannot open pos list for reading (" << pos_filename.c_str() << endl;
        return false;
    }

    std::string buffer;
    while (getline(pos_in, buffer)) {
        std::vector<std::string> line;
        split_string(buffer, "\t", line);
        dic[line[0]] = atoi(static_cast<const char *>(line[1].c_str()));
        rdic[atoi(static_cast<const char *>(line[1].c_str()))] = line[0];
        count++;
    }

    pos_in.close();
    return true;
}

}
