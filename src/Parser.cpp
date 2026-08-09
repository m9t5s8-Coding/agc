#include "Parser.hpp"

#include "Statements/Exit.hpp"
#include "Token.hpp"

#include <fstream>

namespace ag {
void Parser::parse() {
  while (is_valid()) {
    const auto& token = get_current_token();
    scan_token(token);
    increase_token();
  }

  std::stringstream ss;
  ss << "section .text\n";
  ss << "global _start\n\n";
  ss << "_start:\n";

  for (const auto& ptr : m_statemets) {
    ptr->generate(ss);
  }
  std::ofstream outfile("out.asm");
  if (!outfile.is_open()) {
    return;
  }
  outfile << ss.str();
  outfile.close();
  system("nasm -f elf64 out.asm -o out.o");
  system("ld out.o -o out");
}

bool Parser::scan_token(const Token& token) {
  switch (token.token_name) {
    case ag::TokenName::AG_EXIT: {
      auto exit_ptr = ExitStatement::ParseExit(*this, token.token_name);
      add_statements(exit_ptr);
      return true;
    }
    default: return false;
  }
  return false;
}
}  // namespace ag
