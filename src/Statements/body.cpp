
#include "Statements/body.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"

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
BodyStatement::generate(CodeGenContext& context) {
  for (const auto& stmt : m_statements) {
    stmt->generate(context);
  }
}

bool
BodyStatement::analyze_semantics(Parser& parser) {
  get_symbol_table().enter_scope();

  for (const auto& stmt : m_statements) {
    if (!stmt->analyze_semantics(parser)) {
      return false;
    }
  }

  get_symbol_table().exit_scope();
  return true;
}
} // namespace ag
