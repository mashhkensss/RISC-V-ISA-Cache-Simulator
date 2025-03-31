#include <vector>
#include "Cache/CacheBase.cpp"
#include <list>

class CachePLRU : public CacheBase {
public:
    CachePLRU() : CacheBase() {}

    bool isInCache(Address address, Type type) override {
        for (int elem = 0; elem < CACHE_WAY; ++elem) {
            if (lines[address.index][elem].valid && lines[address.index][elem].l_tag == address.a_tag) {
                if (type == Type(w)) {
                    lines[address.index][elem].dirty = true;
                }
                updatePLRU(address, elem);
                return true;
            }
        }
        return false;
    }

    void updatePLRU(Address address, int skip) {
        lines[address.index][skip].PlruBits = 1;
        int count = 0;
        for (int ch = 0; ch < CACHE_WAY; ++ch) {
            count += lines[address.index][ch].PlruBits;
        }
        if (count == CACHE_WAY) {
            for (int ch = 0; ch < CACHE_WAY; ++ch) {
                if (ch != skip) {
                    lines[address.index][ch].PlruBits = 0;
                }
            }
        }
    }
    int findLinePLRU(Address address) {
        for (int ch = 0; ch < CACHE_WAY; ++ch) {
            if (lines[address.index][ch].PlruBits == 0) {
                return ch;
            }
        }
        return 0;
    }

    void updateLine(Address address, Type type, std::vector<int8_t>& memory) override {
        int newIndex = findLinePLRU(address);
        lines[address.index][newIndex].valid = true;
        lines[address.index][newIndex].l_tag = address.a_tag;
        lines[address.index][newIndex].dirty = (type == Type(w));
        updatePLRU(address, newIndex);
    }
};
