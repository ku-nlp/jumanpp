/*
  Darts -- Double-ARray Trie System

  $Id: mkdarts.cpp 1674 2008-03-22 11:21:34Z taku $;

  Copyright(C) 2001-2007 Taku Kudo <taku@chasen.org>
  All rights reserved.
*/
#include "darts.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "tagger.h"
#include "pos.h"
#include "node.h"
#include "feature.h"

using namespace Morph;

int progress_bar(size_t current, size_t total) {
    static char bar[] = "*******************************************";
    static int scale = sizeof(bar) - 1;
    static int prev = 0;

    int cur_percentage = static_cast<int>(100.0 * current / total);
    int bar_len = static_cast<int>(1.0 * current * scale / total);

    if (prev != cur_percentage) {
        printf("Making Double Array: %3d%% |%.*s%*s| ", cur_percentage, bar_len,
               bar, scale - bar_len, "");
        if (cur_percentage == 100)
            printf("\n");
        else
            printf("\r");
        fflush(stdout);
    }

    prev = cur_percentage;

    return 1;
};

template <class Iterator>
inline size_t tokenizeCSV(char *str, Iterator out, size_t max) {
    // inline size_t tokenizeCSV(char *str, char** out, size_t max) {
    char *eos = str + std::strlen(str);
    char *start = 0;
    char *end = 0;
    size_t n = 0;

    for (; str < eos; ++str) {
        // skip white spaces
        // while (*str == ' ' || *str == '\t') ++str;
        // bool inquote = false;
        if (*str == '"') {
            start = ++str;
            end = start;
            for (; str < eos; ++str) {
                if (*str == '"') {
                    str++;
                    if (*str != '"')
                        break;
                }
                *end++ = *str;
            }
            // inquote = true;
            str = std::find(str, eos, ',');
        } else {
            start = str;
            str = std::find(str, eos, ',');
            end = str;
        }
        if (max-- > 1)
            *end = '\0';
        *out++ = start;
        ++n;
        if (max == 0)
            break;
    }

    return n;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " File OutputBasename"
                  << std::endl;
        return -1;
    }

    std::string file = argv[argc - 2];
    std::string base_filename = argv[argc - 1];

    std::string index = base_filename + ".da";
    std::string dicbin = base_filename + ".bin";
    std::string pos_list = base_filename + ".pos";
    std::string spos_list = base_filename + ".spos";
    std::string form_list = base_filename + ".form";
    std::string form_type_list = base_filename + ".form_type";
    std::string base_list = base_filename + ".base";
    std::string rep_list = base_filename + ".rep";
    std::string imis_list = base_filename + ".imis";
    std::string read_list = base_filename + ".read";

    Darts::DoubleArray da;

    std::vector<std::pair<std::string, Token *>> dic;
    std::istream *is;

    if (file == "-") {
        is = &std::cin;
    } else {
        is = new std::ifstream(file.c_str());
    }

    if (!*is) {
        std::cerr << "Cannot Open: " << file << std::endl;
        return -1;
    }

    Pos pos_gen;
    Pos spos_gen;      //細分類
    Pos form_gen;      //活用形
    Pos form_type_gen; //活用型
    Pos base_gen;      //活用型
    Pos reading_gen;
    Pos rep_gen;  //代表表記
    Pos imis_gen; //意味情報

    std::string line;
    const int max_dic_column = 12;
    char *col[max_dic_column];
    std::string w;
    unsigned int line_number = 0;
    while (std::getline(*is, line)) {
        const size_t n = tokenizeCSV((char *)line.c_str(), col, max_dic_column);
        line_number++;
        if (n != max_dic_column) {
            std::cerr << ";; format error: " << line_number << " :" << line
                      << endl;
            continue;
        }
        if (std::string(col[4]) == std::string(";")) {
            std::cerr << ";; format error: " << line << endl;
            exit(1);
        }
        if (std::string(col[5]).size() == 0) {
            col[5] = (char *)"*";
        }
        if (std::string(col[6]).size() == 0) {
            col[6] = (char *)"*";
        }
        if (std::string(col[7]).size() == 0) {
            col[5] = (char *)"*";
        }
        if (std::string(col[9]).size() == 0) { //代表表記がない場合
            col[10] = (char *)"*";
        }
        if (std::string(col[10]).size() == 0) { //意味情報がない場合
            col[11] = (char *)"*";
        }

        w = col[0];

        Token *token = new Token;
        token->lcAttr = std::atoi(col[1]);
        token->rcAttr = std::atoi(col[2]);
        token->wcost = std::atoi(col[3]);
        token->posid = pos_gen.get_id(col[4]);
        token->spos_id = spos_gen.get_id(col[5]);
        token->form_id = form_gen.get_id(col[6]);
        token->form_type_id = form_type_gen.get_id(col[7]);
        token->base_id = base_gen.get_id(col[8]);
        token->reading_id = reading_gen.get_id(col[9]);
        // 代表表記
        token->rep_id = rep_gen.get_id(col[10]);
        // cout << token->representation << endl;
        // 意味情報(残り全て)
        token->imis_id = imis_gen.get_id(col[11]);
        // cout << col[10] << endl;

        dic.push_back(std::make_pair(w, token));
    }
    if (file != "-")
        delete is;

    std::sort(dic.begin(), dic.end());

    size_t bsize = 0;
    size_t idx = 0;
    std::string prev;
    std::vector<const char *> str;
    std::vector<size_t> len;
    std::vector<Darts::DoubleArray::result_type> val;

    for (size_t i = 0; i < dic.size(); ++i) {
        if (i != 0 && prev != dic[i].first) {
            str.push_back(dic[idx].first.c_str());
            len.push_back(dic[idx].first.size());
            val.push_back(bsize + (idx << 8));
            bsize = 1;
            idx = i;
        } else {
            ++bsize;
        }
        prev = dic[i].first;
    }
    str.push_back(dic[idx].first.c_str());
    len.push_back(dic[idx].first.size());
    val.push_back(bsize + (idx << 8));

    auto ret = da.build(str.size(), &str[0], &len[0], &val[0], &progress_bar);
    if (ret != 0) {
        std::cerr << "Error: cannot build double array (err code =" << ret
                  << ") " << file << std::endl;
    }

    if (da.save(index.c_str()) != 0) {
        std::cerr << "Error: cannot build double array  " << file << std::endl;
        return -1;
    };

    std::cout << "Done!, Compression Ratio: "
              << 100.0 * da.nonzero_size() / da.size() << " %" << std::endl;

    // write dic
    std::string tbuf;
    for (size_t i = 0; i < dic.size(); ++i) {
        tbuf.append(reinterpret_cast<const char *>(dic[i].second),
                    sizeof(Token));
        delete dic[i].second;
    }
    dic.clear();

    // needs to be 8byte(64bit) aligned
    while (tbuf.size() % 8 != 0) {
        Token dummy;
        memset(&dummy, 0, sizeof(Token));
        tbuf.append(reinterpret_cast<const char *>(&dummy), sizeof(Token));
    }

    std::ofstream bofs(dicbin.c_str(), std::ios::binary | std::ios::out);
    bofs.write(const_cast<const char *>(tbuf.data()), tbuf.size());
    bofs.close();

    // write pos list
    pos_gen.write_pos_list(pos_list);
    spos_gen.write_pos_list(spos_list);
    form_gen.write_pos_list(form_list);
    form_type_gen.write_pos_list(form_type_list);
    base_gen.write_pos_list(base_list);
    reading_gen.write_pos_list(read_list);
    rep_gen.write_pos_list(rep_list);
    imis_gen.write_pos_list(imis_list);

    return 0;
}
