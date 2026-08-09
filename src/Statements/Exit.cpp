#include "Statements/Exit.hpp"

#include "Token.hpp"

#include <iostream>
#include <string>

namespace ag {

AG_scope<Statements> ExitStatement::ExitWithSmallBracket(Parser& parser) {
  if (!parser.expect_token(TokenName::AG_LEFT_PAREN)) {
    return nullptr;
  }
  parser.consume_token();

  if (!parser.expect_token(TokenName::AG_INT_LITERAL)) {
    return nullptr;
  }
  int exit_value = 1;
  {
    std::string str_int(parser.peek_token().value);
    try {
      exit_value = std::stoi(str_int);
    } catch (...) {
      exit_value = 1;
    }
  }
  parser.consume_token();

  if (!parser.expect_token(TokenName::AG_RIGHT_PAREN)) {
    return nullptr;
  }
  parser.consume_token();

  if (!parser.expect_token(TokenName::AG_SEMICOLON)) {
    return nullptr;
  }
  parser.consume_token();

  return AG_cscope<ExitStatement>(exit_value);
}

AG_scope<Statements> ExitStatement::ExitWithoutSmallBracket(Parser& parser) {
  if (!parser.expect_token(TokenName::AG_INT_LITERAL)) {
    return nullptr;
  }

  int exit_value = 1;
  {
    std::string str_int(parser.peek_token().value);
    try {
      exit_value = std::stoi(str_int);
    } catch (...) {
      exit_value = 1;
    }
  }
  parser.consume_token();

  if (!parser.expect_token(TokenName::AG_SEMICOLON)) {
    return nullptr;
  }
  parser.consume_token();

  return AG_cscope<ExitStatement>(exit_value);
}

AG_scope<Statements> ExitStatement::ParseExit(Parser& parser, TokenName token_name) {
  if (token_name != TokenName::AG_EXIT) {
    return nullptr;
  }

  switch (parser.peek_token_name()) {
    case TokenName::AG_LEFT_PAREN: {
      return ExitWithSmallBracket(parser);
    }
    case TokenName::AG_INT_LITERAL: {
      return ExitWithoutSmallBracket(parser);
    }
    default: {
      std::cerr << "Invalid Syntax!\n";
      return nullptr;
    }
  }
  return nullptr;
}

void ExitStatement::generate(std::stringstream& ss) {
  ss << "mov rdi, " << this->exit_code << "\n";
  ss << "mov rax, 60\n";
  ss << "syscall\n";
}

}  // namespace ag
