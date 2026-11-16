#include "IR/DerefStore.hpp"

#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
DerefStore::to_string() {
  return std::format("*{} = {}\n", m_address, m_value);
}

void
DerefStore::generate_code(CodeGenContext& context) {
  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_address));
  context.text << std::format("\tmov rcx, {}\n",
                              resolve_operand(m_function_index, m_value));
  if (m_is_byte)
    context.text << "\tmov byte [rax], cl\n";
  else
    context.text << "\tmov qword [rax], rcx\n";
}
} // namespace ag
