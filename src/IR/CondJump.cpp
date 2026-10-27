#include "IR/CondJump.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
CondJump::to_string() {
  return std::format("if false {} jump {}\n", m_cond, m_false_block);
}

void
CondJump::generate_code(CodeGenContext&) {}

} // namespace ag
