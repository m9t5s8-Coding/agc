#include "Statements/var.hpp"
#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/GlobalVariable.hpp"
#include "IR/IRTable.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <iostream>

namespace ag {
AG_scope<Statements>
VarStatement::ParseVar(Parser& parser) {
  parser.advance();

  auto var_stmt        = AG_cscope<VarStatement>();
  var_stmt->m_is_static = parser.take_pending_static();
  var_stmt->m_location = {parser.current_token().line, parser.current_token().column};

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, var_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  if (parser.match(TokenName::AG_COLON)) {
    auto parsed_type = parse_type(parser);
    if (!parsed_type)
      return nullptr;
    var_stmt->m_datatype = std::move(*parsed_type);
  } else {
    // `var name = value;` infers the type from the initializer.
    var_stmt->m_type_inferred = true;
  }

  if (parser.match(TokenName::AG_ASSIGN)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, var_stmt->m_expr, nullptr);
  } else if (var_stmt->m_type_inferred) {
    print_error(error::ExpectedInitializer, parser);
    return nullptr;
  }

  // Inside a `with (...)` header the declaration ends at `)` rather than `;`.
  if (!parser.match(TokenName::AG_SEMICOLON) &&
      !parser.check(TokenName::AG_RIGHT_PAREN) &&
      !parser.check(TokenName::AG_COMMA)) {
    print_error(error::ExpectedSemicolon, parser);
    return nullptr;
  }

  return var_stmt;
}

void
VarStatement::generate_ir(LabelCounters& counter,
                          Parser&        parser) {
  if (counter.function_index < 0) {
    const std::string global_name = m_qualified_name.empty()
                                        ? parser.qualify_name(m_name)
                                        : m_qualified_name;
    auto global = get_ir_table().declare_global(global_name,
                                                get_type_size(m_datatype),
                                                false,
                                                global_initializer(m_expr.get()));
    if (global)
      parser.add_instructios(
          AG_cscope<GlobalVariable>(global->name, global->initializer, counter.function_index));
    return;
  }

  if (m_is_static) {
    generate_static_local(counter,
                          parser,
                          m_name,
                          m_datatype,
                          m_expr.get(),
                          false);
    return;
  }

  auto variable = get_ir_table().declare_user(counter.function_index,
                                              std::string(m_name),
                                              get_type_size(m_datatype));
  std::string destination = variable ? variable->name : std::string(m_name);

  if (!m_expr)
    return;

  auto expr = m_datatype.kind == TypeKind::REFERENCE
                  ? m_expr->generate_reference_ir(counter, parser)
                  : m_expr->generate_ir(counter, parser);
  expr = narrow_value(expr, m_datatype, counter, parser);
  parser.add_instructios(AG_cscope<Assign>(expr, destination, counter.function_index));
}

bool
VarStatement::analyze_semantics(Parser& parser) {
  // Module-level values live in the active module's namespace; local values
  // keep their source spelling.
  if (parser.current_function_return_type() == nullptr)
    m_qualified_name = parser.qualify_name(m_name);

  // let x : int = 3;
  const auto& diagnostic_location
      = m_expr ? m_expr->source_location(m_location) : m_location;
  if (m_type_inferred && m_expr) {
    // The initializer determines the declaration's type.
    if (!m_expr->analyze_semantics(parser, diagnostic_location))
      return false;
    m_datatype = m_expr->get_data_type();
  }

  Declaration data;
  data.type     = m_datatype;
  data.timing   = ValueTiming::COMPILE_TIME;
  data.location = m_location;
  const std::string_view symbol_name
      = m_qualified_name.empty() ? m_name : std::string_view(m_qualified_name);
  if (!get_symbol_table().insert(symbol_name, data)) {
    print_error(error::RedefinitionOfSymbol, parser, m_location);
    return false;
  }

  if (!m_expr)
    return true;
  if (!m_type_inferred &&
      !m_expr->analyze_semantics(parser, diagnostic_location)) {
    return false;
  }
  if (m_type_inferred)
    return true;
  const auto expr_declared = m_expr->get_declared_data_type();
  const bool reference_initializer =
      m_datatype.kind == TypeKind::REFERENCE &&
      expr_declared.kind == TypeKind::REFERENCE &&
      is_compatible_type(m_datatype, expr_declared);
  if (!is_compatible_type(m_datatype, m_expr->get_data_type()) &&
      !reference_initializer) {
    print_error(error::IncompatibleTypes, parser, diagnostic_location);
    return false;
  }
  return true;
}

} // namespace ag
