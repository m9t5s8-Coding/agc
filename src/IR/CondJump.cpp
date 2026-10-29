#include "IR/CondJump.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
CondJump::to_string() {
  return std::format("if false {} jump {}\n", m_cond, m_false_block);
}

void
CondJump::generate_code(CodeGenContext& context) {
  context.text << std::format("\tmov rax, {}\n", resolve_operand(m_function_index, m_cond));
  context.text << "\ttest rax, rax\n";
  context.text << std::format("\tjz {}\n", m_false_block);
}

} // namespace ag
