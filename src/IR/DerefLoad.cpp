#include "IR/DerefLoad.hpp"

#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
DerefLoad::to_string() {
  return std::format("{} = *{}\n", m_store_to, m_address);
}

void
DerefLoad::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_store_to);
  if (destination.empty())
    return;

  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_address));
  if (m_is_byte)
    context.text << "\tmovzx rax, byte [rax]\n";
  else
    context.text << "\tmov rax, qword [rax]\n";
  context.text << std::format("\tmov {}, rax\n", destination);
}
} // namespace ag
