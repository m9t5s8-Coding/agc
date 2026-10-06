#pragma once

#include "Statements/Utils.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ag {

class SymbolTable {
public:
  bool
  find(const std::string_view key) {
    auto& map = m_scopes.back().first;
    if (map.find(key) != map.end()) {
      return true;
    }
    return false;
  }
  bool
  insert(const std::string_view key,
         const Declaration&     decl_type) {

    if (find(key)) {
      return false;
    }

    // SymbolEntry entry;
    // entry.data_type   = decl_type.data_type;
    // entry.decl_type   = decl_type.decl_type;
    // entry.scope_label = s_current_scope;
    //
    // m_current_stack_offset += 8;
    // entry.stack_offset = m_current_stack_offset;
    //
    // if (decl_type.decl_type == VARIABLE_TYPE::COMPILE) {
    //   if (decl_type.data_type.datatype_type == DATA_TYPES::CHAR) {
    //     entry.value = decl_type.value;
    //   } else {
    //   }
    // }
    //
    // m_scopes.back()[key] = entry;
    return true;
  }

  // 0 1 2
  std::optional<SymbolEntry>
  lookup(const std::string_view key) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      if (it->second) {
        auto& map   = it->first;
        auto  found = map.find(key);
        if (found != map.end()) {
          return found->second;
        }
      }
    }
    return std::nullopt;
  }

  void
  enter_scope() {
    m_scopes.push_back({});
    m_scopes.back().second = true;
    ++s_current_scope;
  }
  void
  exit_scope() {
    // m_scopes.pop_back();
    m_scopes.back().second = false;
  }
  void
  reset_stack_offset() {
    m_current_stack_offset = 0;
  }

  friend SymbolTable&
  get_symbol_table();

private:
  using SymbolTableVector = std::vector<std::pair<std::unordered_map<std::string_view, SymbolEntry>, bool>>;

  SymbolTableVector     m_scopes;
  int64_t               m_current_stack_offset = 0;
  inline static int64_t s_current_scope        = -1;

  SymbolTable() { m_scopes.push_back({}); }
};

class FunctionTable {
public:
  bool
  declare(const std::string& mangled_name,
          FunctionEntry&     entry) {
    if (m_functions.count(mangled_name))
      return false; // function redeclaration
    m_functions[mangled_name] = std::move(entry);
    return true;
  }

  std::optional<FunctionEntry>
  lookup(const std::string& mangled_name) {
    auto it = m_functions.find(mangled_name);
    if (it != m_functions.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  friend FunctionTable&
  get_function_table();

private:
  // in function support overloading for now in std::string function can be send
  // as "foo(int,int)" and "foo(int)"
  using FunctionTableMap = std::unordered_map<std::string, FunctionEntry>;
  FunctionTableMap m_functions;

  FunctionTable() {}
};

[[nodiscard]]
inline SymbolTable&
get_symbol_table() {
  static SymbolTable table;
  return table;
}

inline FunctionTable&
get_function_table() {
  static FunctionTable table;
  return table;
}

} // namespace ag
