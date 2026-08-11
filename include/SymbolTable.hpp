#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace ag {
enum class DATA_TYPES {
  NONE,
  STRING,
  CUSTOM,
};
enum class DECLARATION_TYPES : uint8_t {
  LET = 0,
  VAR
};
struct Declaration_Data {
  DATA_TYPES        data_type = DATA_TYPES::NONE;
  DECLARATION_TYPES decl_type = DECLARATION_TYPES::VAR;
  std::string       value;
};

class SymbolTable {
public:
  bool contains(const std::string& key) {
    auto it = m_symbol_table.find(key);
    if (it != m_symbol_table.end()) {
      return false;
    }
    return true;
  }

  std::optional<Declaration_Data> get_value(const std::string& key) {
    auto it = m_symbol_table.find(key);
    if (it != m_symbol_table.end()) {
      return it->second;
    }
    return {};
  }

  void insert_value(const std::string& key, const Declaration_Data& value) {
    m_symbol_table[key] = value;
  }

private:
  std::unordered_map<std::string, Declaration_Data> m_symbol_table;
};

inline SymbolTable& get_table() {
  static SymbolTable table;
  return table;
}
}  // namespace ag
