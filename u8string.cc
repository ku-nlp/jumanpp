
#include "u8string.h"

std::ostream& operator << (std::ostream& os, Morph::U8string& u) {
    os << u.byte_str; 
        return os;
};

namespace Morph{
const std::unordered_set<std::string> U8string::lowercase{"ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゎ", "ヵ",
    "ァ", "ィ", "ゥ", "ェ", "ォ", "ヮ", "っ", "ッ", "ん", "ン",
    "ゃ", "ャ", "ゅ", "ュ", "ょ", "ョ"};
}


