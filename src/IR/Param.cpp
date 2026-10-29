#include "IR/Param.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <array>
#include <format>
#include <string_view>

namespace ag {

namespace {
constexpr std::array<std::string_view, 6> argument_registers = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};
} // namespace

std::string
Param::to_string() {
  return std::format("param {}\n", m_name);
}

void
Param::generate_code(CodeGenContext& context) {
  if (context.arg_index >= argument_registers.size())
    return;

  context.text << std::format("\tmov {}, {}\n",
                              argument_registers[context.arg_index++],
                              resolve_operand(m_function_index, m_name));
}

} // namespace ag
