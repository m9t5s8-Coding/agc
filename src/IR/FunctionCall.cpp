#include "IR/FunctionCall.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
FunctionCall::to_string() {
  return std::format("call {}\n", m_name);
}

void
FunctionCall::generate_code(CodeGenContext& context) {
  context.text << std::format("\tcall {}\n", m_name);
}
} // namespace ag
