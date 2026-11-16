#include "IR/Narrow.hpp"

#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"

#include <format>

namespace ag {
std::string
Narrow::to_string() {
  return std::format("{} = trunc{}({})\n",
                     m_destination,
                     m_width * 8,
                     m_value);
}

void
Narrow::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_destination);
  if (destination.empty())
    return;

  context.text << std::format("\tmov rax, {}\n",
                              resolve_operand(m_function_index, m_value));
  switch (m_width) {
  case 1:
    context.text << std::format("\t{} rax, al\n",
                                m_is_signed ? "movsx" : "movzx");
    break;
  case 2:
    context.text << std::format("\t{} rax, ax\n",
                                m_is_signed ? "movsx" : "movzx");
    break;
  case 4:
    if (m_is_signed)
      context.text << "\tmovsxd rax, eax\n";
    else
      context.text << "\tmov eax, eax\n";
    break;
  default:
    break;
  }
  context.text << std::format("\tmov {}, rax\n", destination);
}
} // namespace ag
