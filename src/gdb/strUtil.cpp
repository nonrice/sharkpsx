#include <algorithm>

#include "strUtil.hpp"
#include "logging.hpp"

namespace pse {

inline u64 b2i(char ch){
    if (ch >= '0' && ch <= '9'){
        return ch - '0';
    }

    if (ch >= 'A' && ch <= 'F'){
        return (ch - 'A') + 10;
    }

    if (ch >= 'a' && ch <= 'f'){
        return (ch - 'a') + 10;
    }

    return -1;
}

inline char i2b(u64 i){
    if (i >= 0 && i <= 9){
        return '0' + i;
    }

    if (i >= 10 && i <= 15){
        return 'a' + (i - 10);
    }

    return -1;
}

u64 s2i(std::string_view s, u64 b){
    assert(2 <= b && b <= 16);

    u64 total = 0;
    u64 pow = 1;
    for (ssize i=s.size()-1; i>=0; i--){
        total += pow * b2i(s[i]);
        pow *= b;
    }

    return total;
}

ssize i2sl(std::span<char> s, u64 x, u64 b){
    assert(2 <= b && b <= 16);

    usize i=0;
    while (i < s.size() && x > 0){
        s[i] = i2b(x % b);
        i += 1;
        x /= b;
    }

    if (x > 0){
        LOG_DBG("number too big for span");
        return -1;
    }

    // into big endian
    std::ranges::reverse(s.subspan(0, i));
    return i;
}

ssize i2sr(std::span<char> s, u64 x, u64 b){
    assert(2 <= b && b <= 16);

    ssize i = s.size()-1;
    ssize d = 0;
    while (i >= 0 && x > 0){
        s[i] = i2b(x % b);
        i -= 1;
        d += 1;
        x /= b;
    }

    if (x > 0){
        LOG_DBG("number too big for span");
        return -1;
    }

    std::ranges::fill(s.subspan(0, i+1), '0');

    return d;
}

ssize find(std::string_view s, char ch){
    for (usize i=0; i<s.size(); i++){
        if (s[i] == ch){
            return i;
        }
    }

    return -1;
}

}
