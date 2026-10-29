
#include "Statements/body.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"

namespace ag {
AG_scope<Statements>
BodyStatement::ParseBlock(Parser& parser) {
  parser.advance();

  auto body_statement = AG_cscope<BodyStatement>();

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    const size_t position_before_statement = parser.get_current_position();
    auto         statement                 = parser.parse_statement();
    if (!statement)
      return nullptr;

    body_statement->m_statements.emplace_back(std::move(statement));

    if (parser.get_current_position() == position_before_statement) {
      print_error(error::UnexpectedToken, parser);
      return nullptr;
    }
  }
  parser.advance();
  return body_statement;
}

void
BodyStatement::generate_ir(LabelCounters& counter,
                           Parser&         parser) {
  for (const auto& stmt : m_statements) {
    stmt->generate_ir(counter, parser);
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
