#include "IR/MemberLoad.hpp"

#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
MemberLoad::to_string() {
  return std::format("{} = [{} + {}]\n", m_store_to, m_object, m_byte_offset);
}

void
MemberLoad::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_store_to);
  if (destination.empty())
    return;

  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_object));
  context.text << std::format("\tmov rax, qword [rax + {}]\n", m_byte_offset);
  context.text << std::format("\tmov {}, rax\n", destination);
}
} // namespace ag
