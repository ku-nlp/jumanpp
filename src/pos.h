#ifndef POS_H
#define POS_H

#include "common.h"

namespace Morph {

class Pos {
    // unsigned long count;
    std::unordered_map<std::string, unsigned long> dic;
    std::hash<std::string> str_hash;
    std::unordered_map<unsigned long, std::string> rdic;

  public:
    Pos();

    unsigned long get_id(const std::string &pos_str);
    unsigned long get_id(const char *pos_str);
    std::string *get_pos(unsigned long posid);
    bool write_pos_list(const std::string &pos_filename);
    bool read_pos_list(const std::string &pos_filename);
};
}

#endif
