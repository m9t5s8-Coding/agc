#include "IR/IndexAssign.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
IndexAssign::to_string() {
  return std::format("{}[{}] = {}\n", m_object, m_index, m_value);
}

void
IndexAssign::generate_code(CodeGenContext& context) {
  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_object));
  context.text << std::format("\tmov rcx, {}\n",
                              resolve_operand(m_function_index, m_index));
  context.text << std::format("\tmov rdx, {}\n",
                              resolve_operand(m_function_index, m_value));
  context.text << "\tmov byte [rax + rcx], dl\n";
}

} // namespace ag
