#include "IR/CondJump.hpp"
#include <format>

namespace ag {
std::string
CondJump::to_string() {
  return std::format("if false {} jump {}\n", m_cond, m_false_block);
}
} // namespace ag
