#include "Statements/struct.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <algorithm>
#include <string>

namespace ag {

AG_scope<Statements>
StructStatement::ParseStruct(Parser& parser) {
  const Token struct_token = parser.current_token();
  parser.advance(); // `struct`
  parser.take_pending_public(); // type visibility is not enforced yet

  auto struct_stmt        = AG_cscope<StructStatement>();
  struct_stmt->m_location = {struct_token.line, struct_token.column};

  std::string raw_name;
  AG_EXPECT_GET_ID_OR_RETURN(parser,
                             error::ExpectedStructName,
                             raw_name,
                             nullptr,
                             TokenName::AG_IDENTIFIER);
  struct_stmt->m_definition.name     = parser.qualify_name(raw_name);
  struct_stmt->m_definition.location = struct_stmt->m_location;

  // Generic parameters are accepted but not monomorphized yet.  Registering
  // their names lets field declarations use `T`, `T[]`, and similar forms.
  const bool has_generic_parameters = parser.check(TokenName::AG_LESS);
  if (has_generic_parameters) {
    auto generic_parameters = parse_generic_parameter_list(parser);
    if (!generic_parameters)
      return nullptr;
    parser.push_generic_parameters(std::move(*generic_parameters));
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace, nullptr);

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    StructMember member;
    if (parser.match(TokenName::AG_PUB))
      member.is_public = true;
    else if (parser.match(TokenName::AG_PRIV))
      member.is_public = false;
    member.location = {parser.current_token().line, parser.current_token().column};

    AG_EXPECT_GET_ID_OR_RETURN(
        parser, error::ExpectedFieldName, member.name, nullptr, TokenName::AG_IDENTIFIER);

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

    auto field_type = parse_type(parser);
    if (!field_type)
      return nullptr;
    member.type = std::move(*field_type);

    if (struct_stmt->m_definition.find_member(member.name) != nullptr) {
      print_error(error::DuplicateFieldName, parser, member.location);
      return nullptr;
    }

    const size_t field_size  = std::max<size_t>(1, get_type_size(member.type));
    const size_t field_align = std::min<size_t>(field_size, 8);
    size_t       offset      = struct_stmt->m_definition.members.empty()
                                   ? 0
                                   : struct_stmt->m_definition.size_in_bytes;
    offset = (offset + field_align - 1) / field_align * field_align;
    member.byte_offset = offset;
    struct_stmt->m_definition.members.push_back(std::move(member));
    struct_stmt->m_definition.size_in_bytes = offset + field_size;
    struct_stmt->m_definition.alignment
        = std::max(struct_stmt->m_definition.alignment, field_align);

    if (parser.match(TokenName::AG_COMMA) || parser.match(TokenName::AG_SEMICOLON))
      continue;
    if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      print_error(error::ExpectedComma, parser);
      return nullptr;
    }
  }

  parser.advance(); // `}`

  if (struct_stmt->m_definition.alignment == 0)
    struct_stmt->m_definition.alignment = 1;
  struct_stmt->m_definition.size_in_bytes
      = (struct_stmt->m_definition.size_in_bytes +
         struct_stmt->m_definition.alignment - 1) /
        struct_stmt->m_definition.alignment * struct_stmt->m_definition.alignment;
  struct_stmt->m_definition.size_in_bytes
      = std::max<size_t>(1, struct_stmt->m_definition.size_in_bytes);

  if (!get_struct_table().declare(struct_stmt->m_definition)) {
    print_error(error::RedefinitionOfType, parser, struct_stmt->m_location);
    return nullptr;
  }

  if (has_generic_parameters)
    parser.pop_generic_parameters();
  return struct_stmt;
}

void
StructStatement::generate_ir(LabelCounters&,
                             Parser&) {
  // Layout information lives in the struct table; no runtime instruction is
  // needed for the declaration itself.
}

bool
StructStatement::analyze_semantics(Parser&) {
  return true;
}
} // namespace ag
