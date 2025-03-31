#include <cstdint>

struct Address {
    uint16_t a_tag;     // CACHE_TAG_LEN
    uint8_t index;      // CACHE_INDEX_LEN
    uint8_t offset;     // CACHE_OFFSET_LEN

    Address(uint16_t tag, uint8_t idx, uint8_t offst) : a_tag(tag), index(idx), offset(offst) {};
};
