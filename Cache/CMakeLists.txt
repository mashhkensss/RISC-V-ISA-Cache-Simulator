add_library(cache CacheBase.cpp
        CacheLRU.cpp
        CachePLRU.cpp)

target_include_directories(cache PUBLIC ${PROJECT_SOURCE_DIR})
