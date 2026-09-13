
#include "Statements/body.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Token.hpp"
#include <iostream>

namespace ag {
AG_scope<Statements>
BodyStatement::ParseBlock(Parser& parser) {
  parser.advance();

  auto body_statement = AG_cscope<BodyStatement>();

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    auto statement = parser.parse_statement();
    if (statement) {
      body_statement->m_statements.emplace_back(std::move(statement));
    } else {
      return nullptr;
    }
  }
  parser.advance();
  return body_statement;
}

void
BodyStatement::generate(CodeGenContext& context) {}
} // namespace ag
