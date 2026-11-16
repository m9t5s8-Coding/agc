#include "IR/AddressOf.hpp"

#include "IR/IRTable.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
AddressOf::to_string() {
  return std::format("{} = &{}\n", m_store_to, m_source);
}

void
AddressOf::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_store_to);
  if (destination.empty())
    return;

  if (auto variable = get_ir_table().lookup_data(m_function_index, m_source)) {
    context.text << std::format("\tlea rax, [rbp - {}]\n", variable->offset);
  } else if (auto variable = get_ir_table().lookup_source(m_function_index, m_source)) {
    context.text << std::format("\tlea rax, [rbp - {}]\n", variable->offset);
  } else if (auto global = get_ir_table().lookup_global_data(m_source)) {
    context.text << std::format("\tlea rax, [{}]\n", global->name);
  } else if (auto global = get_ir_table().lookup_global_source(m_source)) {
    context.text << std::format("\tlea rax, [{}]\n", global->name);
  } else {
    context.text << "\txor eax, eax\n";
  }

  context.text << std::format("\tmov {}, rax\n", destination);
}
} // namespace ag
