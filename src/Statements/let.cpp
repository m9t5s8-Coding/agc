#include "Statements/let.hpp"
#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/GlobalVariable.hpp"
#include "IR/IRTable.hpp"
#include "IR/MemberLoad.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
static DataType
make_int_type() {
  DataType type;
  type.kind           = TypeKind::PRIMITIVE;
  type.primitive_type = PrimitiveType::INT;
  type.size_in_bytes  = 8;
  type.alignment      = 8;
  return type;
}

AG_scope<Statements>
LetStatement::ParseLet(Parser& parser) {
  parser.advance();

  auto let_stmt        = AG_cscope<LetStatement>();
  let_stmt->m_is_static = parser.take_pending_static();
  let_stmt->m_location = {parser.current_token().line, parser.current_token().column};

  if (parser.match(TokenName::AG_LEFT_PAREN)) {
    // Tuple destructuring: `let (a, b) = pair;`.
    let_stmt->m_is_destructuring = true;
    do {
      if (!parser.check(TokenName::AG_IDENTIFIER)) {
        print_error(parser.is_at_end() ? error::UnexpectedEndOfFile
                                       : error::ExpectedIdentifier,
                    parser);
        return nullptr;
      }
      let_stmt->m_pattern_names.push_back(parser.current_token().value);
      parser.advance();
    } while (parser.match(TokenName::AG_COMMA));

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN,
                        error::ExpectedRightParen, nullptr);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN,
                        error::ExpectedEqual, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, let_stmt->m_expr, nullptr);

    if (!parser.match(TokenName::AG_SEMICOLON) &&
        !parser.check(TokenName::AG_RIGHT_PAREN) &&
        !parser.check(TokenName::AG_COMMA)) {
      print_error(error::ExpectedSemicolon, parser);
      return nullptr;
    }
    return let_stmt;
  }

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, let_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  if (parser.check(TokenName::AG_LEFT_BRACE)) {
    // Struct destructuring: `let Point { x, y } = point;`.
    let_stmt->m_is_struct_destructuring = true;
    let_stmt->m_destructure_type        = std::string(let_stmt->m_name);
    parser.advance(); // `{`

    while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      if (parser.is_at_end()) {
        print_error(error::UnexpectedEndOfFile, parser);
        return nullptr;
      }
      if (!parser.check(TokenName::AG_IDENTIFIER)) {
        print_error(error::ExpectedFieldName, parser);
        return nullptr;
      }

      std::string field(parser.current_token().value);
      std::string binding = field;
      parser.advance();

      if (parser.match(TokenName::AG_COLON)) {
        if (!parser.check(TokenName::AG_IDENTIFIER)) {
          print_error(error::ExpectedIdentifier, parser);
          return nullptr;
        }
        binding = std::string(parser.current_token().value);
        parser.advance();
      }

      let_stmt->m_destructure_fields.push_back(std::move(field));
      let_stmt->m_destructure_bindings.push_back(std::move(binding));

      if (parser.match(TokenName::AG_COMMA))
        continue;
      if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
        print_error(error::ExpectedComma, parser);
        return nullptr;
      }
    }
    parser.advance(); // `}`

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN,
                        error::ExpectedEqual, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, let_stmt->m_expr, nullptr);

    if (!parser.match(TokenName::AG_SEMICOLON) &&
        !parser.check(TokenName::AG_RIGHT_PAREN) &&
        !parser.check(TokenName::AG_COMMA)) {
      print_error(error::ExpectedSemicolon, parser);
      return nullptr;
    }
    return let_stmt;
  }

  if (parser.match(TokenName::AG_COLON)) {
    auto parsed_type = parse_type(parser);
    if (!parsed_type)
      return nullptr;
    let_stmt->m_datatype           = std::move(*parsed_type);
    let_stmt->m_datatype.is_const = true;
  } else {
    // `let name = value;` infers the type from the initializer.
    let_stmt->m_type_inferred = true;
  }

  if (parser.match(TokenName::AG_ASSIGN)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, let_stmt->m_expr, nullptr);
  } else if (let_stmt->m_type_inferred) {
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

  return let_stmt;
}

AG_scope<Statements>
LetStatement::ParseConst(Parser& parser) {
  parser.advance();

  auto const_stmt        = AG_cscope<LetStatement>();
  const_stmt->m_is_static = parser.take_pending_static();
  const_stmt->m_location = {parser.current_token().line, parser.current_token().column};

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, const_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

  auto parsed_type = parse_type(parser);
  if (!parsed_type)
    return nullptr;
  const_stmt->m_datatype           = std::move(*parsed_type);
  const_stmt->m_datatype.is_const = true;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN, error::ExpectedInitializer, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, const_stmt->m_expr, nullptr);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

  return const_stmt;
}

void
LetStatement::generate_ir(LabelCounters& counter,
                          Parser&        parser) {
  if (counter.function_index < 0) {
    const std::string global_name = m_qualified_name.empty()
                                        ? parser.qualify_name(m_name)
                                        : m_qualified_name;
    auto global = get_ir_table().declare_global(global_name,
                                                get_type_size(m_datatype),
                                                true,
                                                global_initializer(m_expr.get()));
    if (global)
      parser.add_instructios(
          AG_cscope<GlobalVariable>(global->name, global->initializer, counter.function_index));
    return;
  }

  if (m_is_struct_destructuring) {
    if (!m_expr)
      return;

    const auto block    = m_expr->generate_ir(counter, parser);
    const auto resolved = parser.resolve_type_name(m_destructure_type);
    const auto* definition = get_struct_table().lookup(resolved);
    if (!definition)
      return;

    for (size_t i = 0; i < m_destructure_bindings.size(); ++i) {
      const auto* member = definition->find_member(m_destructure_fields[i]);
      if (!member)
        continue;

      const DataType type = i < m_pattern_types.size() ? m_pattern_types[i]
                                                      : member->type;
      const std::string& binding = m_destructure_bindings[i];
      auto variable = get_ir_table().declare_user(counter.function_index,
                                                  binding,
                                                  get_type_size(type));
      const std::string destination = variable ? variable->name : binding;
      parser.add_instructios(AG_cscope<MemberLoad>(
          destination,
          block,
          static_cast<uint32_t>(member->byte_offset),
          counter.function_index));
    }
    return;
  }

  if (m_is_destructuring) {
    if (!m_expr)
      return;

    const auto block = m_expr->generate_ir(counter, parser);
    for (size_t i = 0; i < m_pattern_names.size(); ++i) {
      const DataType type = i < m_pattern_types.size() ? m_pattern_types[i]
                                                       : make_int_type();
      auto variable = get_ir_table().declare_user(counter.function_index,
                                                  std::string(m_pattern_names[i]),
                                                  get_type_size(type));
      const std::string destination =
          variable ? variable->name : std::string(m_pattern_names[i]);
      parser.add_instructios(AG_cscope<MemberLoad>(
          destination,
          block,
          static_cast<uint32_t>(i * 8),
          counter.function_index));
    }
    return;
  }

  if (m_is_static) {
    generate_static_local(counter,
                          parser,
                          m_name,
                          m_datatype,
                          m_expr.get(),
                          true);
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
LetStatement::analyze_semantics(Parser& parser) {
  if (m_is_struct_destructuring) {
    if (!m_expr)
      return false;
    if (!m_expr->analyze_semantics(parser,
                                   m_expr->source_location(m_location)))
      return false;

    const auto resolved    = parser.resolve_type_name(m_destructure_type);
    const auto* definition = get_struct_table().lookup(resolved);
    if (!definition) {
      print_error(error::UndefinedType, parser, m_location);
      return false;
    }

    m_pattern_types.clear();
    for (size_t i = 0; i < m_destructure_bindings.size(); ++i) {
      const auto* member = definition->find_member(m_destructure_fields[i]);
      if (!member) {
        print_error(error::UndefinedMember, parser, m_location);
        return false;
      }

      DataType field_type   = member->type;
      field_type.is_const   = true;
      m_pattern_types.push_back(field_type);

      Declaration data;
      data.type     = field_type;
      data.timing   = ValueTiming::COMPILE_TIME;
      data.location = m_location;
      if (!get_symbol_table().insert(m_destructure_bindings[i], data)) {
        print_error(error::RedefinitionOfSymbol, parser, m_location);
        return false;
      }
    }
    return true;
  }

  if (m_is_destructuring) {
    if (!m_expr)
      return false;
    if (!m_expr->analyze_semantics(parser,
                                   m_expr->source_location(m_location)))
      return false;

    const auto tuple_type = m_expr->get_data_type();
    m_pattern_types.clear();
    for (size_t i = 0; i < m_pattern_names.size(); ++i) {
      DataType element_type =
          i < tuple_type.tuple_elements.size() ? *tuple_type.tuple_elements[i]
                                               : make_int_type();
      element_type.is_const = true;
      m_pattern_types.push_back(element_type);

      Declaration data;
      data.type     = element_type;
      data.timing   = ValueTiming::COMPILE_TIME;
      data.location = m_location;
      if (!get_symbol_table().insert(m_pattern_names[i], data)) {
        print_error(error::RedefinitionOfSymbol, parser, m_location);
        return false;
      }
    }
    return true;
  }

  // Module-level values live in the active module's namespace; local values
  // keep their source spelling.  `let` and `const` share this path.
  if (parser.current_function_return_type() == nullptr)
    m_qualified_name = parser.qualify_name(m_name);

  const auto& diagnostic_location
      = m_expr ? m_expr->source_location(m_location) : m_location;
  if (m_type_inferred && m_expr) {
    // The initializer determines the declaration's type.
    if (!m_expr->analyze_semantics(parser, diagnostic_location))
      return false;
    m_datatype           = m_expr->get_data_type();
    m_datatype.is_const = true;
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
