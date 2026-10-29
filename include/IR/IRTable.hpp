#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ag {

// A single stack slot owned by a function.
//
// `offset` is stored as a positive number and the slot is addressed as
// [rbp - offset].  This keeps the table independent of the final frame size:
// the frame can grow while IR is being generated without invalidating any
// already recorded offset.
struct VariableData {
  // Unique name used inside the IR, e.g. "v.2.str0" or "t.0".  User names are
  // never used directly so they cannot collide with compiler-generated names.
  std::string name;

  // Original name written in the source file, e.g. "str0".  This is only used
  // to translate identifier expressions to their IR name.
  std::string source_name;

  uint32_t    size{};        // reserved size in bytes
  uint32_t    offset{};      // [rbp - offset]
  uint32_t    param_index{}; // register index when this is a parameter
  bool        is_parameter = false;
};

struct FunctionData {
  std::string name;
  std::string label;        // mangled assembly label
  uint32_t    frame_size{}; // always 16-byte aligned
  uint32_t    stack_offset{};
  std::vector<std::shared_ptr<VariableData>> variables;
};

class IRTable {
public:
  // Every slot starts at an 8 byte boundary, which is enough for all
  // primitive types currently supported by the language.
  static constexpr uint32_t kSlotAlignment  = 8;
  static constexpr uint32_t kFrameAlignment = 16;

  FunctionData*
  lookup(int index) {
    if (index < 0 || index >= static_cast<int>(m_IRTable.size()))
      return nullptr;
    return &m_IRTable[index];
  }

  const FunctionData*
  lookup(int index) const {
    if (index < 0 || index >= static_cast<int>(m_IRTable.size()))
      return nullptr;
    return &m_IRTable[index];
  }

  FunctionData*
  find_function(std::string_view name) {
    auto it = std::find_if(m_IRTable.begin(), m_IRTable.end(), [&name](const auto& function) {
      return function.name == name;
    });
    return it == m_IRTable.end() ? nullptr : &*it;
  }

  const FunctionData*
  find_function(std::string_view name) const {
    auto it = std::find_if(m_IRTable.begin(), m_IRTable.end(), [&name](const auto& function) {
      return function.name == name;
    });
    return it == m_IRTable.end() ? nullptr : &*it;
  }

  // Find a variable by the name used in the IR, e.g. "v.2.str0".
  std::shared_ptr<VariableData>
  lookup_data(int              index,
              std::string_view name) {
    auto data = lookup(index);
    if (!data)
      return nullptr;

    auto it = std::find_if(data->variables.begin(), data->variables.end(), [name](const auto& variable) {
      return variable->name == name;
    });

    return it == data->variables.end() ? nullptr : *it;
  }

  std::shared_ptr<const VariableData>
  lookup_data(int              index,
              std::string_view name) const {
    auto data = lookup(index);
    if (!data)
      return nullptr;

    auto it = std::find_if(data->variables.begin(), data->variables.end(), [name](const auto& variable) {
      return variable->name == name;
    });

    return it == data->variables.end() ? nullptr : *it;
  }

  // Find a variable by the name written in the source, e.g. "str0".
  std::shared_ptr<VariableData>
  lookup_source(int               index,
                std::string_view  source_name) {
    auto data = lookup(index);
    if (!data)
      return nullptr;

    auto it = std::find_if(data->variables.begin(), data->variables.end(), [source_name](const auto& variable) {
      return variable->source_name == source_name;
    });

    return it == data->variables.end() ? nullptr : *it;
  }

  // Allocate a stack slot for a user-declared name.
  //
  // The IR name gets a "v.<slot>." prefix (`num` becomes `v.0.num`).  Source
  // identifiers cannot contain '.', so this can never collide with a name the
  // user is allowed to write.
  std::shared_ptr<VariableData>
  declare_user(int              index,
               std::string      source_name,
               uint32_t         size,
               bool             is_parameter = false,
               uint32_t         param_index  = 0) {
    auto* function = lookup(index);
    if (!function || source_name.empty())
      return nullptr;

    if (auto existing = lookup_source(index, source_name))
      return existing;

    std::string ir_name = "v." + std::to_string(function->variables.size()) + "." + source_name;
    return allocate(*function, std::move(ir_name), std::move(source_name), size, is_parameter, param_index);
  }

  // Allocate a slot for a compiler-generated value (a temporary).  The caller
  // supplies an already unique name such as "t.0".
  std::shared_ptr<VariableData>
  declare_internal(int         index,
                   std::string name,
                   uint32_t    size) {
    auto* function = lookup(index);
    if (!function || name.empty())
      return nullptr;

    if (auto existing = lookup_data(index, name))
      return existing;

    return allocate(*function, name, name, size, false, 0);
  }

  bool
  insert_data(int                            index,
              std::shared_ptr<VariableData>& data) {
    auto* function = lookup(index);
    if (!function || !data)
      return false;

    if (auto existing = lookup_data(index, data->name))
      return existing == data;

    function->variables.push_back(data);
    return true;
  }

  int
  insert(std::string name  = {},
         std::string label = {}) {
    FunctionData function;
    function.name  = std::move(name);
    function.label = std::move(label);
    m_IRTable.push_back(std::move(function));
    return static_cast<int>(m_IRTable.size()) - 1;
  }

  void
  reset() {
    m_IRTable.clear();
  }

  size_t
  size() const {
    return m_IRTable.size();
  }

private:
  static uint32_t
  align_up(uint32_t value,
           uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
  }

  static std::shared_ptr<VariableData>
  allocate(FunctionData& function,
           std::string   name,
           std::string   source_name,
           uint32_t      size,
           bool          is_parameter,
           uint32_t      param_index) {
    if (size == 0)
      size = kSlotAlignment;

    const uint32_t start = align_up(function.stack_offset, kSlotAlignment);

    auto variable = std::make_shared<VariableData>();
    variable->name         = std::move(name);
    variable->source_name  = std::move(source_name);
    variable->size         = size;
    variable->offset       = start + size;
    variable->param_index  = param_index;
    variable->is_parameter = is_parameter;

    function.stack_offset = variable->offset;
    function.frame_size   = align_up(function.stack_offset, kFrameAlignment);
    function.variables.push_back(variable);

    return variable;
  }

  std::vector<FunctionData> m_IRTable;
};

[[nodiscard]]
inline IRTable&
get_ir_table() {
  static IRTable table;
  return table;
}
} // namespace ag
