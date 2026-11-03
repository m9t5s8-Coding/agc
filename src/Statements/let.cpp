#include "Statements/let.hpp"
#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/GlobalVariable.hpp"
#include "IR/IRTable.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
LetStatement::ParseLet(Parser& parser) {
  parser.advance();

  auto let_stmt = AG_cscope<LetStatement>();
  let_stmt->m_location = {parser.current_token().line, parser.current_token().column};

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, let_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

  auto parsed_type = parse_type(parser);
  if (!parsed_type)
    return nullptr;
  let_stmt->m_datatype           = std::move(*parsed_type);
  let_stmt->m_datatype.is_const = true;

  if (parser.match(TokenName::AG_ASSIGN)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, let_stmt->m_expr, nullptr);
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

  return let_stmt;
}

void
LetStatement::generate_ir(LabelCounters& counter,
                          Parser&        parser) {
  if (counter.function_index < 0) {
    auto global = get_ir_table().declare_global(std::string(m_name),
                                                get_type_size(m_datatype),
                                                true,
                                                global_initializer(m_expr.get()));
    if (global)
      parser.add_instructios(
          AG_cscope<GlobalVariable>(global->name, global->initializer, counter.function_index));
    return;
  }

  auto variable = get_ir_table().declare_user(counter.function_index,
                                              std::string(m_name),
                                              get_type_size(m_datatype));
  std::string destination = variable ? variable->name : std::string(m_name);

  if (!m_expr)
    return;

  auto expr = m_expr->generate_ir(counter, parser);
  parser.add_instructios(AG_cscope<Assign>(expr, destination, counter.function_index));
}

bool
LetStatement::analyze_semantics(Parser& parser) {
  Declaration data;
  data.type     = m_datatype;
  data.timing   = ValueTiming::COMPILE_TIME;
  data.location = m_location;
  if (!get_symbol_table().insert(m_name, data)) {
    print_error(error::RedefinitionOfSymbol, parser, m_location);
    return false;
  }

  if (!m_expr)
    return true;
  const auto& diagnostic_location = m_expr->source_location(m_location);
  if (!m_expr->analyze_semantics(parser, diagnostic_location)) {
    return false;
  }
  if (!is_compatible_type(m_datatype, m_expr->get_data_type())) {
    print_error(error::IncompatibleTypes, parser, diagnostic_location);
    return false;
  }

  return true;
}

} // namespace ag
