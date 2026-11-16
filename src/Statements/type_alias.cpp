#include "Statements/type_alias.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <string>

namespace ag {
AG_scope<Statements>
TypeAliasStatement::ParseType(Parser& parser) {
  const Token type_token = parser.current_token();
  parser.advance(); // `type`
  parser.take_pending_public();

  auto type_stmt        = AG_cscope<TypeAliasStatement>();
  type_stmt->m_location = {type_token.line, type_token.column};

  std::string raw_name;
  AG_EXPECT_GET_ID_OR_RETURN(parser,
                             error::ExpectedAliasName,
                             raw_name,
                             nullptr,
                             TokenName::AG_IDENTIFIER);

  // Generic aliases (`type Predicate<T> = ...`) are accepted; the generic
  // parameters are recorded only for parsing purposes for now.
  if (parser.match(TokenName::AG_LESS)) {
    int depth = 1;
    while (depth > 0 && !parser.is_at_end()) {
      if (parser.check(TokenName::AG_LESS))
        ++depth;
      else if (parser.check(TokenName::AG_GREATER))
        --depth;
      parser.advance();
    }
    if (depth != 0) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN, error::ExpectedEqual, nullptr);

  const std::string qualified_name = parser.qualify_name(raw_name);

  // Function-type aliases have no callable representation yet.  Consume the
  // signature and store an opaque named type so declarations that mention the
  // alias still type-check.
  if (parser.check(TokenName::AG_FUNC)) {
    while (!parser.is_at_end() && !parser.check(TokenName::AG_SEMICOLON))
      parser.advance();
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

    DataType opaque;
    opaque.kind           = TypeKind::FUNCTION;
    opaque.primitive_type = PrimitiveType::USER_DEFINED;
    opaque.type_name      = qualified_name;
    if (!get_type_alias_table().declare(std::string(qualified_name), std::move(opaque))) {
      print_error(error::RedefinitionOfType, parser, type_stmt->m_location);
      return nullptr;
    }
    return type_stmt;
  }

  auto aliased_type = parse_type(parser);
  if (!aliased_type)
    return nullptr;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

  if (!get_type_alias_table().declare(std::string(qualified_name), std::move(*aliased_type))) {
    print_error(error::RedefinitionOfType, parser, type_stmt->m_location);
    return nullptr;
  }

  return type_stmt;
}
} // namespace ag
