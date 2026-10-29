#include "IR/Assign.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Assign::to_string() {
  return std::format("{} = {}\n", m_destination, m_source);
}

void
Assign::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_destination);
  if (destination.empty())
    return;

  context.text << std::format("\tmov rax, {}\n", resolve_operand(m_function_index, m_source));
  context.text << std::format("\tmov {}, rax\n", destination);
}

} // namespace ag
