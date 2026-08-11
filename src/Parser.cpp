#include "Parser.hpp"

#include "Statements/Exit.hpp"
#include "Statements/Let.hpp"
#include "Statements/Println.hpp"
#include "Statements/Statements.hpp"
#include "Token.hpp"

#include <fstream>

namespace ag {
void Parser::parse() {
  while (is_valid()) {
    const auto& token = get_current_token();
    scan_token(token);
    increase_token();
  }

  CodeGenContext context;
  context.code << "_start:\n";
  for (const auto& ptr : m_statemets) {
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
       << context.code.str();
    outfile << ss.str();
    outfile.close();
    system("nasm -f elf64 out.asm -o out.o");
    system("ld out.o -o out");
  }
}

bool Parser::scan_token(const Token& token) {
  switch (token.token_name) {
    case ag::TokenName::AG_EXIT: {
      auto exit_ptr = ExitStatement::ParseExit(*this, token.token_name);
      add_statements(exit_ptr);
      return true;
    }
    case TokenName::AG_PRINTLN: {
      auto print_ptr = PrintlnStatement::ParsePrintln(*this, token.token_name);
      add_statements(print_ptr);
      return true;
    }
    case TokenName::AG_LET: {
      auto let_ptr = LetStatement::ParseLet(*this, token.token_name);
      add_statements(let_ptr);
      return true;
    }
    default: return false;
  }
  return false;
}
}  // namespace ag
