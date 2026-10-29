#include "IR/Binary.hpp"
#include "Expressions/Expression.hpp"
#include "IR/Operand.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {

namespace {
void
emit_binary_op(CodeGenContext&    context,
               BIN_OPERATOR        op,
               const std::string&  right) {
  switch (op) {
  case BIN_OPERATOR::ADDITION:
    context.text << std::format("\tadd rax, {}\n", right);
    break;
  case BIN_OPERATOR::SUBTRACTION:
    context.text << std::format("\tsub rax, {}\n", right);
    break;
  case BIN_OPERATOR::MULTIPLICATION:
    context.text << std::format("\timul rax, {}\n", right);
    break;
  case BIN_OPERATOR::DIVISION:
  case BIN_OPERATOR::MODULUS:
    // rdx is used by idiv for the high half, so the divisor goes in rcx.
    context.text << std::format("\tmov rcx, {}\n", right);
    context.text << "\tcqo\n";
    context.text << "\tidiv rcx\n";
    if (op == BIN_OPERATOR::MODULUS)
      context.text << "\tmov rax, rdx\n";
    break;
  case BIN_OPERATOR::BITWISE_AND:
    context.text << std::format("\tand rax, {}\n", right);
    break;
  case BIN_OPERATOR::BITWISE_OR:
    context.text << std::format("\tor rax, {}\n", right);
    break;
  case BIN_OPERATOR::BITWISE_XOR:
    context.text << std::format("\txor rax, {}\n", right);
    break;
  case BIN_OPERATOR::LEFT_SHIFT:
  case BIN_OPERATOR::RIGHT_SHIFT:
    context.text << std::format("\tmov rcx, {}\n", right);
    context.text << (op == BIN_OPERATOR::LEFT_SHIFT ? "\tshl rax, cl\n" : "\tsar rax, cl\n");
    break;
  case BIN_OPERATOR::LESS_THAN:
  case BIN_OPERATOR::LESS_THAN_EQUAL:
  case BIN_OPERATOR::GREATER_THAN:
  case BIN_OPERATOR::GREATER_THAN_EQUAL:
  case BIN_OPERATOR::EQUAL_TO:
  case BIN_OPERATOR::NOT_EQUAL_TO: {
    context.text << std::format("\tcmp rax, {}\n", right);
    switch (op) {
    case BIN_OPERATOR::LESS_THAN:
      context.text << "\tsetl al\n";
      break;
    case BIN_OPERATOR::LESS_THAN_EQUAL:
      context.text << "\tsetle al\n";
      break;
    case BIN_OPERATOR::GREATER_THAN:
      context.text << "\tsetg al\n";
      break;
    case BIN_OPERATOR::GREATER_THAN_EQUAL:
      context.text << "\tsetge al\n";
      break;
    case BIN_OPERATOR::EQUAL_TO:
      context.text << "\tsete al\n";
      break;
    case BIN_OPERATOR::NOT_EQUAL_TO:
      context.text << "\tsetne al\n";
      break;
    default:
      break;
    }
    context.text << "\tmovzx rax, al\n";
    break;
  }
  case BIN_OPERATOR::LOGICAL_AND:
    context.text << std::format("\tand rax, {}\n", right);
    break;
  case BIN_OPERATOR::LOGICAL_OR:
    context.text << std::format("\tor rax, {}\n", right);
    break;
  default:
    break;
  }
}
} // namespace

std::string
Binary::to_string() {
  return std ::format("{} = {} {} {}\n", m_store_to, m_left, to_symbol(m_operator), m_right);
}

void
Binary::generate_code(CodeGenContext& context) {
  const auto destination = resolve_destination(m_function_index, m_store_to);
  if (destination.empty())
    return;

  context.text << std::format("\tmov rax, {}\n", resolve_operand(m_function_index, m_left));
  emit_binary_op(context, m_operator, resolve_operand(m_function_index, m_right));
  context.text << std::format("\tmov {}, rax\n", destination);
}

} // namespace ag
