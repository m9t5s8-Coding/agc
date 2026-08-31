#include "Statements/exit.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <iostream>

namespace ag {

AG_scope<Statements>
ExitStatement::ParseExit(Parser& parser) {
  auto exit_statement = AG_cscope<ExitStatement>();
  parser.advance();
  TokenName temp_token;
  if (!parser.match_one_of({TokenName::AG_LEFT_PAREN, TokenName::AG_IDENTIFIER}, &temp_token)) {
    std::cout << "Here Wrong!";
    return nullptr;
  }

  if (temp_token == TokenName::AG_LEFT_PAREN) {
    AG_EXPECT_GET_VALUE_OR_RETURN(
        parser, TokenName::AG_IDENTIFIER, ag::error::ExpectedIdentifier, exit_statement->value_or_name, nullptr);

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  }
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return nullptr;
}

void
ExitStatement::generate(CodeGenContext& context) {}

} // namespace ag
