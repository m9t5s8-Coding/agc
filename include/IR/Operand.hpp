#pragma once

#include "IR/IRTable.hpp"
#include <format>
#include <string>
#include <string_view>

namespace ag {

// Turn an IR value into something that can be used as a source operand.
//
// Variables live on the stack and are addressed through the function's
// IRTable entry.  Literals and data labels (for example `str0`) are emitted
// as-is.
[[nodiscard]]
inline std::string
resolve_operand(int               function_index,
                std::string_view  value) {
  if (auto variable = get_ir_table().lookup_data(function_index, value))
    return std::format("qword [rbp - {}]", variable->offset);

  // Fallback for anything that still carries a raw source name.
  if (auto variable = get_ir_table().lookup_source(function_index, value))
    return std::format("qword [rbp - {}]", variable->offset);

  return std::string(value);
}

// Same as resolve_operand, but only succeeds for values that have a stack
// slot.  Used for destinations, which must be writable.
[[nodiscard]]
inline std::string
resolve_destination(int              function_index,
                    std::string_view value) {
  if (auto variable = get_ir_table().lookup_data(function_index, value))
    return std::format("qword [rbp - {}]", variable->offset);

  if (auto variable = get_ir_table().lookup_source(function_index, value))
    return std::format("qword [rbp - {}]", variable->offset);

  return {};
}

} // namespace ag
