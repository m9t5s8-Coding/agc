#include "IR/Return.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Return::to_string() {
  return std::format("return {}\n", m_name);
}

void
Return::generate_code(CodeGenContext& context) {
  if (!m_name.empty())
    context.text << std::format("\tmov rax, {}\n", resolve_operand(m_function_index, m_name));

  context.text << "\tleave\n";
  context.text << "\tret\n";
}

} // namespace ag
