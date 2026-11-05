#include "Statements/enum.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <algorithm>
#include <string>

namespace ag {
namespace {
constexpr size_t kEnumTagSize  = 8;
constexpr size_t kEnumSlotSize = 8;

std::optional<EnumField>
parse_named_field(Parser& parser) {
  EnumField field;
  field.location = {parser.current_token().line, parser.current_token().column};

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, error::ExpectedFieldName, field.name, std::nullopt, TokenName::AG_IDENTIFIER);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, std::nullopt);

  auto type = parse_type(parser);
  if (!type)
    return std::nullopt;

  field.type = std::move(*type);
  return field;
}

std::optional<EnumField>
parse_positional_field(Parser& parser,
                       size_t  index) {
  EnumField field;
  field.location = {parser.current_token().line, parser.current_token().column};
  field.name     = std::to_string(index);

  // Positional payloads are written as types, but accepting `name: type` keeps
  // the two payload syntaxes consistent.
  if (parser.check(TokenName::AG_IDENTIFIER) &&
      parser.peek_token().token_name == TokenName::AG_COLON) {
    AG_EXPECT_GET_ID_OR_RETURN(
        parser, error::ExpectedFieldName, field.name, std::nullopt, TokenName::AG_IDENTIFIER);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, std::nullopt);
  }

  auto type = parse_type(parser);
  if (!type)
    return std::nullopt;

  field.type = std::move(*type);
  return field;
}
} // namespace

AG_scope<Statements>
EnumStatement::ParseEnum(Parser& parser) {
  const Token enum_token = parser.current_token();
  parser.advance(); // `enum`
  parser.take_pending_public(); // type visibility is not enforced yet

  auto enum_stmt        = AG_cscope<EnumStatement>();
  enum_stmt->m_location = {enum_token.line, enum_token.column};

  std::string raw_name;
  AG_EXPECT_GET_ID_OR_RETURN(
      parser, error::ExpectedEnumName, raw_name, nullptr, TokenName::AG_IDENTIFIER);

  enum_stmt->m_definition.name     = parser.qualify_name(raw_name);
  enum_stmt->m_definition.location = enum_stmt->m_location;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace, nullptr);

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    EnumVariant variant;
    variant.location = {parser.current_token().line, parser.current_token().column};
    variant.index    = enum_stmt->m_definition.variants.size();

    AG_EXPECT_GET_ID_OR_RETURN(
        parser, error::ExpectedEnumMemberName, variant.name, nullptr, TokenName::AG_IDENTIFIER);

    if (enum_stmt->m_definition.find_variant(variant.name) != nullptr) {
      print_error(error::DuplicateEnumMember, parser, variant.location);
      return nullptr;
    }

    if (parser.match(TokenName::AG_LEFT_PAREN)) {
      if (!parser.check(TokenName::AG_RIGHT_PAREN)) {
        size_t field_index = 0;
        do {
          auto field = parse_positional_field(parser, field_index);
          if (!field)
            return nullptr;
          variant.fields.push_back(std::move(*field));
          ++field_index;
        } while (parser.match(TokenName::AG_COMMA));
      }

      AG_EXPECT_OR_RETURN(
          parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
    } else if (parser.match(TokenName::AG_LEFT_BRACE)) {
      while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
        if (parser.is_at_end()) {
          print_error(error::UnexpectedEndOfFile, parser);
          return nullptr;
        }

        auto field = parse_named_field(parser);
        if (!field)
          return nullptr;

        if (variant.find_field(field->name) != nullptr) {
          print_error(error::DuplicateFieldName, parser, field->location);
          return nullptr;
        }

        variant.fields.push_back(std::move(*field));

        if (parser.match(TokenName::AG_COMMA) || parser.match(TokenName::AG_SEMICOLON))
          continue;
        if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
          print_error(error::ExpectedComma, parser);
          return nullptr;
        }
      }

      parser.advance(); // `}`
    }

    enum_stmt->m_definition.variants.push_back(std::move(variant));

    if (parser.match(TokenName::AG_COMMA) || parser.match(TokenName::AG_SEMICOLON))
      continue;
    if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      print_error(error::ExpectedComma, parser);
      return nullptr;
    }
  }

  parser.advance(); // `}`
  parser.match(TokenName::AG_SEMICOLON);

  size_t payload_slots = 0;
  for (auto& variant : enum_stmt->m_definition.variants) {
    payload_slots = std::max(payload_slots, variant.fields.size());
    for (size_t i = 0; i < variant.fields.size(); ++i)
      variant.fields[i].byte_offset = kEnumTagSize + i * kEnumSlotSize;
  }

  enum_stmt->m_definition.size_in_bytes
      = kEnumTagSize + payload_slots * kEnumSlotSize;
  enum_stmt->m_definition.alignment = kEnumSlotSize;

  if (!get_enum_table().declare(enum_stmt->m_definition)) {
    print_error(error::RedefinitionOfType, parser, enum_stmt->m_location);
    return nullptr;
  }

  return enum_stmt;
}

void
EnumStatement::generate_ir(LabelCounters&,
                           Parser&) {
  // Enum layouts live in the enum table; no runtime instruction is needed.
}

bool
EnumStatement::analyze_semantics(Parser&) {
  return true;
}
} // namespace ag
