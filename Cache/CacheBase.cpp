#pragma once

#include <vector>
#include "Parameters/CacheConfig.cpp"
#include "Parameters/CommandTypes.cpp"
#include "Entities/CacheLine.cpp"
#include "Entities/Address.cpp"


class CacheBase {
public:
    std::vector<std::vector<CacheLine>> lines;

    CacheBase() {
        lines.resize(CACHE_SETS, std::vector<CacheLine>(CACHE_WAY));
    }

    virtual bool accessMemory(Address address, Type type, std::vector<int8_t>& memory) {
        if (isInCache(address, type)) {
            return true;
        } else {
            updateLine(address, type, memory);
            return false;
        }
    }

    virtual bool isInCache(Address address, Type type) = 0;
    virtual void updateLine(Address address, Type type, std::vector<int8_t>& memory) = 0;
};
