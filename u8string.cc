
#include "u8string.h"

std::ostream& operator << (std::ostream& os, Morph::U8string& u) {
    os << u.byte_str; 
        return os;
};


