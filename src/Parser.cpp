#include "Parser.hpp"

#include "Statements/ExpressionStatement.hpp"
#include "Statements/Let.hpp"
#include "Statements/Statements.hpp"
#include "Statements/body.hpp"
#include "Statements/exit.hpp"
#include "Statements/function.hpp"
#include "Statements/read.hpp"
#include "Statements/return.hpp"
#include "Statements/write.hpp"
#include "Token.hpp"

#include <fstream>
#include <iostream>

namespace ag {
void
Parser::parse() {

  while (is_valid()) {
    if (!add_statements(parse_statement())) {
      m_invalid_syntax = true;
      break;
    }
    // advance();
  }
  if (m_invalid_syntax) {
    return;
  }

  CodeGenContext context;
  context.text << "_start:\n";
  for (const auto& ptr : m_statements) {
    ptr->generate(context);
  }

  std::ofstream outfile("out.asm");
  if (!outfile.is_open()) {
    return;
  }
  {
    std::stringstream ss;
    ss << "section .data\n"
       << context.data.str() << "\n"
       << "section .text\n"
       << "global _start\n\n"
       << context.text.str();
    outfile << ss.str();
    outfile.close();
    // system("nasm -f elf64 out.asm -o out.o");
    // system("ld out.o -o out");
  }
}

AG_scope<Statements>
Parser::parse_statement() {
  const auto& token = current_token();

  switch (token.token_name) {
  case ag::TokenName::AG_EXIT: {
    return ExitStatement::ParseExit(*this);
  }
  case TokenName::AG_WRITE: {
    return WriteStatement::ParseWrite(*this);
  }
  case TokenName::AG_READ: {
    return ReadStatement::ParseRead(*this);
  }
  case TokenName::AG_LET: {
    return LetStatement::ParseLet(*this);
  }
  case TokenName::AG_FUNC: {
    return FunctionStatement::ParseFunction(*this);
  }
  case TokenName::AG_LEFT_BRACE: {
    return BodyStatement::ParseBlock(*this);
  }
  case TokenName::AG_RETURN: {
    return ReturnStatement::ParseReturn(*this);
  }
  default:
    return ExpressionStatement::parse_expression_statement(*this);
  }
  return nullptr;
}
} // namespace ag
