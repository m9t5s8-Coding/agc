#include "IR/Jump.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Jump::to_string() {
  return std::format("jump {}\n", m_name);
}

void
Jump::generate_code(CodeGenContext& context) {
  context.text << std::format("\tjmp {}\n", m_name);
}
} // namespace ag
