#include "IR/MemberStore.hpp"

#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
MemberStore::to_string() {
  return std::format("[{} + {}] = {}\n", m_object, m_byte_offset, m_value);
}

void
MemberStore::generate_code(CodeGenContext& context) {
  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_object));
  context.text << std::format("\tmov rcx, {}\n",
                              resolve_operand(m_function_index, m_value));
  context.text << std::format("\tmov qword [rax + {}], rcx\n", m_byte_offset);
}
} // namespace ag
