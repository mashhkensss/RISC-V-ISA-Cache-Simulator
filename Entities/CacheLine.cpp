#include <cstdint>

struct CacheLine {  // 3 flags + tag + data
    bool valid;
    bool dirty;
    int PlruBits;

    uint16_t l_tag;    // uint16_t because CACHE_TAG_LEN = 9 bits;
};
