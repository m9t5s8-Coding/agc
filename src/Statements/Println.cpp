#include "Statements/Println.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"

namespace ag {

AG_scope<Statements> PrintlnStatement::PrintVariable(Parser& parser) {
  parser.consume_token();
  auto& token         = parser.get_current_token();
  auto  variable_name = string_view_2_string(token.value);

  if (!parser.expect_token(TokenName::AG_RIGHT_PAREN)) {
    print_error("Missing: )");
    return nullptr;
  }
  parser.consume_token();
  if (!parser.expect_token(TokenName::AG_SEMICOLON)) {
    print_error("Missing: ;");
    return nullptr;
  }
  parser.consume_token();
  return AG_cscope<PrintlnStatement>(variable_name, false);
}

AG_scope<Statements> PrintlnStatement::ParsePrintln(Parser& parser, TokenName token) {
  if (token != TokenName::AG_PRINTLN) {
    return nullptr;
  }

  if (!parser.expect_token(TokenName::AG_LEFT_PAREN)) {
    print_error("Missing: (");
    return nullptr;
  }
  parser.consume_token();
  switch (parser.peek_token_name()) {
    case TokenName::AG_STRING_LITERAL: {
      return PrintString(parser);
    }
    case TokenName::AG_IDENTIFIER: {
      return PrintVariable(parser);
    }
    default: {
      print_error("Only string literal can be printable!");
      return nullptr;
    }
  }
  return nullptr;
}

AG_scope<Statements> PrintlnStatement::PrintString(Parser& parser) {
  parser.consume_token();
  auto& token       = parser.get_current_token();
  auto  print_value = string_view_2_string(token.value);

  if (!parser.expect_token(TokenName::AG_RIGHT_PAREN)) {
    print_error("Missing: )");
    return nullptr;
  }
  parser.consume_token();
  if (!parser.expect_token(TokenName::AG_SEMICOLON)) {
    print_error("Missing: ;");
    return nullptr;
  }
  parser.consume_token();
  return AG_cscope<PrintlnStatement>(print_value);
}

void PrintlnStatement::generate(CodeGenContext& context) {
  std::string label = "str" + std::to_string(context.label++);

  std::string buffer_value;
  if (!is_literal) {
    auto opt_data = get_table().get_value(this->buffer);
    if (opt_data.has_value()) {
      buffer_value = opt_data->value;
    }
  } else {
    buffer_value = buffer;
  }

  context.data << label << " db \"" << buffer_value << "\" , 10\n"
               << label << "_len equ $ - " << label << "\n";

  context.code << "mov rax, 1\n"
               << "mov rdi, 1\n"
               << "lea rsi, [rel " << label << "]\n"
               << "mov rdx, " << label << "_len\n"
               << "syscall\n";
}
}  // namespace ag
