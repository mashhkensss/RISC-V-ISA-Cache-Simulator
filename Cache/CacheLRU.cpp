#include <vector>
#include "Cache/CacheBase.cpp"
#include <list>

class CacheLRU : public CacheBase {
public:
    std::vector<std::list<int>> lru_order;

    CacheLRU() {
        lru_order.resize(CACHE_SETS);
        for (int i = 0; i < CACHE_SETS; ++i) {
            for (int j = 0; j < CACHE_WAY; ++j) {
                lru_order[i].push_back(j);
            }
        }
    }

    bool isInCache(Address address, Type type) override {
        for (int elem = 0; elem < CACHE_WAY; ++elem) {
            if (lines[address.index][elem].valid && lines[address.index][elem].l_tag == address.a_tag) {
                if (type == Type(w)) { lines[address.index][elem].dirty = true; }
                updateLRU(address.index, elem);
                return true;
            }
        }
        return false;
    }
    void updateLRU(int index, int elem) {
        lru_order[index].remove(elem);
        lru_order[index].push_front(elem);
    }
    int findLineLRU(int index) {
        return lru_order[index].back();
    }
    void updateLine(Address address, Type type, std::vector<int8_t>& memory) override {
        int newIndex = findLineLRU(address.index);
        lines[address.index][newIndex].valid = true;
        lines[address.index][newIndex].l_tag = address.a_tag;
        lines[address.index][newIndex].dirty = (type == Type(w));
        updateLRU(address.index, newIndex);
    }
};