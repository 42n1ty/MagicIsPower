#pragma once
#include "components.hpp"

#include <unordered_map>

namespace game {
  
  
  constexpr uint32_t Hash(const char* str, uint32_t hash = 2166136261u) {
    return *str ? Hash(str + 1, (hash ^ static_cast<uint32_t>(*str)) * 16777619u) : hash;
  }
  
  class StringTable {
    std::unordered_map<uint32_t, std::string> m_strings;
  public:
    uint32_t hash(std::string_view sv) { return Hash(sv.data()); }
    void regStr(uint32_t h, std::string s) { m_strings[h] = std::move(s); }
    const std::string& get(uint32_t h) const { return m_strings.at(h); }
    
  };
  
  
  //global data ???
  static StringTable itemNamesDB;
  
};