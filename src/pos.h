#ifndef POS_H
#define POS_H

#include "common.h"
#include "hash.h"

#define BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/unordered_map.hpp>
namespace bip = boost::interprocess;

typedef bip::managed_mapped_file::segment_manager mapped_file_manager;
typedef bip::allocator<char, mapped_file_manager> allocator_char;
typedef bip::allocator<void, mapped_file_manager> allocator_void;
typedef bip::basic_string<char, std::char_traits<char>, allocator_char>
    MappedString;
typedef bip::allocator<MappedString, bip::managed_mapped_file::segment_manager>
    allocator_string;

typedef uint64_t RDkey;
typedef MappedString RDval;
typedef std::pair<const RDkey, RDval> RDpair;

// 動的読み込みのための定義
typedef bip::allocator<RDpair, bip::managed_mapped_file::segment_manager>
    allocator_rdpair;

typedef boost::unordered_map<RDkey, RDval, DummyHash, std::equal_to<RDkey>,
                             allocator_rdpair> RDmap;

//ファイルの更新時間を調べる
// struct stat attrib;                 //1. create a file attribute structure
// stat("file_name", &attrib);         //2. get the attributes of afile.txt
// clock = gmtime(&(attrib.st_mtime)); //3. Get the last modified time and

namespace Morph {

class Pos {
    // unsigned long count;
    std::unordered_map<std::string, unsigned long> dic;
    MurMurHash3_str str_hash;
    std::unordered_map<unsigned long, std::string> rdic;
    RDmap *rd_dic;

  private:
    bip::managed_mapped_file *p_file = nullptr;
    bool mmap_flag = false;

  public:
    Pos();

    // 品詞名からIDを引く
    inline unsigned long get_id(const std::string &pos_str) {
        return str_hash(pos_str);
    }

    inline unsigned long get_id(char const *pos_str) {
        std::string pos_string = std::string(pos_str);
        return get_id(pos_string);
    }

    unsigned long register_pos(const std::string &pos_str);
    unsigned long register_pos(const char *pos_str);

    std::string *get_pos(unsigned long posid);
    bool write_pos_list(const std::string &pos_filename);
    bool read_pos_list(const std::string &pos_filename);

  private:
    bool valid_mmap(const std::string &pos_filename) { //{{{
        std::string mapfile = pos_filename + ".map";
        // TODO: mapfile が pos_filename より新しいことを確認する．
        return (access(mapfile.c_str(), F_OK) != -1);
    };                                             //}}}
    bool create_mmap(const std::string &mapfile) { //{{{
        mmap_flag = true;

        // 大きめに1GBとる (通常は40MB程度?，あとでshrink)
        unsigned long long map_size = 1024 * 1024 * 1024;
        // メモリマップファイルを作成
        p_file = new bip::managed_mapped_file(bip::create_only, mapfile.c_str(),
                                              map_size);
        auto rd_pos_map = p_file->construct<RDmap>("rdmap")(
            0, DummyHash(), std::equal_to<RDkey>(),
            p_file->get_allocator<RDpair>());
        rd_dic = rd_pos_map;
        return true;
    }                                                           //}}}
    bool load_selialized_map(const std::string &pos_filename) { //{{{
        std::string map_filename = pos_filename + ".map";
        create_mmap(map_filename);
        mmap_flag = true;

        rd_dic->clear();

        // 普通の読み込み
        std::ifstream pos_in(pos_filename.c_str(), std::ios::in);
        if (!pos_in.is_open()) {
            cerr << ";; cannot open pos list for reading ("
                 << pos_filename.c_str() << endl;
            return false;
        }

        std::string buffer;
        allocator_char charallocator(p_file->get_segment_manager());
        while (getline(pos_in, buffer)) {
            std::vector<std::string> line;
            split_string(buffer, "\t", line);
            auto key = strtoul(static_cast<const char *>(line[1].c_str()),
                               nullptr, 10);
            rdic[key] = line[0];

            RDval str(charallocator);
            str = line[0].c_str();
            rd_dic->insert(std::make_pair(key, str));
        }
        pos_in.close();

        // メモリマップへの書き込みを flush
        p_file->flush();
        // メモリマップのサイズ shrink
        bip::managed_mapped_file::shrink_to_fit(map_filename.c_str());

        return true;
    };                                                //}}}
    bool load_mmap(const std::string &pos_filename) { //{{{
        mmap_flag = true;
        std::string mapfile = pos_filename + ".map";
        p_file =
            new bip::managed_mapped_file(bip::open_read_only, mapfile.c_str());
        rd_dic = (p_file->find<RDmap>("rdmap").first);
        return true;
    } //}}}
};
}

#endif
