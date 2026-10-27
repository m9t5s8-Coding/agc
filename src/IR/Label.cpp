#include "IR/Label.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Label::to_string() {
  return std::format("\n{}:\n", m_name);
}

void
Label::generate_code(CodeGenContext& context) {
  context.text << std::format("\n{}:\n", m_name);
}
} // namespace ag
