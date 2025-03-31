add_library(parameters
CacheConfig.cpp
CacheReplacementPolicies.cpp
CommandTypes.cpp)

target_include_directories(parameters PUBLIC ${PROJECT_SOURCE_DIR})