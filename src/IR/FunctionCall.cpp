#include "IR/FunctionCall.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
FunctionCall::to_string() {
  if (m_result.empty())
    return std::format("call {}\n", m_name);
  return std::format("{} = call {}\n", m_result, m_name);
}

void
FunctionCall::generate_code(CodeGenContext& context) {
  context.text << std::format("\tcall {}\n", m_name);

  if (!m_result.empty()) {
    const auto destination = resolve_destination(m_function_index, m_result);
    if (!destination.empty())
      context.text << std::format("\tmov {}, rax\n", destination);
  }

  context.arg_index = 0;
}
} // namespace ag
