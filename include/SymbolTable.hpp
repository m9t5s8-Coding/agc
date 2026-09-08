#pragma once

#include "Statements/Utils.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ag {
enum class VARIABLE_TYPE : uint8_t { COMPILE, RUNTIME };

struct DeclarationData {
  DATA_TYPES    data_type = DATA_TYPES::VOID;
  VARIABLE_TYPE decl_type = VARIABLE_TYPE::COMPILE;

  std::string value;
};
struct SymbolEntry {
  DATA_TYPES    data_type = DATA_TYPES::VOID;
  VARIABLE_TYPE decl_type = VARIABLE_TYPE::COMPILE;

  std::string value;

  int64_t stack_offset = 0;

  uint32_t scope_label = 0;
};

struct FunctionEntry {
  std::string             name; // actual function name
  DATA_TYPES              return_type = DATA_TYPES::VOID;
  std::vector<DATA_TYPES> param_types;
  bool                    is_defined  = false;
  bool                    is_variadic = false;
};

class SymbolTable {
public:
  bool
  insert(const std::string&     key,
         const DeclarationData& decl_type,
         uint32_t               current_scope) {

    if (m_scopes.back().find(key) != m_scopes.back().end()) {
      return false; // Redeclaration of variable
    }

    SymbolEntry entry;
    entry.data_type   = decl_type.data_type;
    entry.decl_type   = decl_type.decl_type;
    entry.scope_label = current_scope;

    m_current_stack_offset += 8;
    entry.stack_offset = m_current_stack_offset;

    if (decl_type.decl_type == VARIABLE_TYPE::COMPILE) {
      if (decl_type.data_type == DATA_TYPES::CHAR) {
        entry.value = decl_type.value;
      } else {
      }
    }

    m_scopes.back()[key] = entry;
    return true;
  }

  std::optional<SymbolEntry>
  lookup(const std::string& key) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
      auto found = it->find(key);
      if (found != it->end()) {
        return found->second;
      }
    }
    return std::nullopt;
  }

  void
  enter_scope() {
    m_scopes.push_back({});
  }
  void
  exit_scope() {
    m_scopes.pop_back();
  }
  void
  reset_stack_offset() {
    m_current_stack_offset = 0;
  }

  friend SymbolTable&
  get_symbol_table();

private:
  using SymbolTableVector = std::vector<std::unordered_map<std::string, SymbolEntry>>;

  SymbolTableVector m_scopes;
  int64_t           m_current_stack_offset = 0;

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
