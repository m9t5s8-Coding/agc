#include "IR/FunctionLabel.hpp"
#include "IR/IRTable.hpp"
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
FunctionLabel::to_string() {
  return std::format("\n{}:\n", m_name);
}

void
FunctionLabel::generate_code(CodeGenContext& context) {
  const auto* function = get_ir_table().lookup(m_function_index);

  context.text << std::format("\n{}:\n", m_name);
  context.text << "\tpush rbp\n";
  context.text << "\tmov rbp, rsp\n";

  if (function && function->frame_size != 0)
    context.text << std::format("\tsub rsp, {}\n", function->frame_size);

  // The body addresses parameters through their stack slots, so spill the
  // incoming argument registers before the body starts executing.
  if (function) {
    for (const auto& variable : function->variables) {
      if (!variable->is_parameter || variable->param_index >= argument_registers.size())
        continue;

      context.text << std::format("\tmov qword [rbp - {}], {}\n",
                                  variable->offset,
                                  argument_registers[variable->param_index]);
    }
  }
}

} // namespace ag
