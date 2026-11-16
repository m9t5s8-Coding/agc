#pragma once
#include "Statements/Utils.hpp"
#include <cstdint>
#include <string_view>

namespace ag {
enum class TokenName {
  AG_NONE,
  AG_EOF,

  AG_FUNC,
  AG_OPERATOR,
  AG_STRUCT,
  AG_ENUM,
  AG_IMPL,
  AG_TYPE,
  AG_EXTERN,
  AG_COMPTIME,
  AG_TRAIT,
  AG_AT,
  AG_GUARD,
  AG_ORELSE,
  AG_FOR,
  AG_LOOP,
  AG_BREAK,
  AG_CONTINUE,
  AG_MOVE,
  AG_BOX,
  AG_WITH,
  AG_DEFER,
  AG_ASYNC,
  AG_AWAIT,
  AG_UNSAFE,
  AG_TEST,
  AG_PUB,
  AG_PRIV,
  AG_STATIC,
  AG_MUT,
  AG_IMPORT,
  AG_MOD,
  AG_AS,
  AG_MATCH,
  AG_IDENTIFIER,
  AG_KEYWORD,

  AG_LET,
  AG_VAR,
  AG_CONST,

  AG_INT_LITERAL,
  AG_FLOAT_LITERAL,
  AG_BOOL_LITERAL,
  AG_STRING_LITERAL,
  AG_FSTRING,
  AG_CHAR_LITERAL,
  AG_NULLPTR,

  AG_LEFT_PAREN,
  AG_RIGHT_PAREN,
  AG_LEFT_BRACE,
  AG_RIGHT_BRACE,
  AG_LEFT_BRACKET,
  AG_RIGHT_BRACKET,

  AG_SEMICOLON,
  AG_COLON,
  AG_DOUBLE_COLON,
  AG_COMMA,

  AG_PLUS,
  AG_INCREMENT,
  AG_ADDITION_ASSIGNMENT,

  AG_MINUS,
  AG_DECREMENT,
  AG_SUBTRACTION_ASSIGNMENT,

  AG_ASTERISK,
  AG_MULTIPLY_ASSIGNMENT,

  AG_SLASH,
  AG_DIVISION_ASSIGNMENT,

  AG_ASSIGN,

  AG_REFERENCE,

  AG_MODULO,
  AG_MODULO_ASSIGNMENT,

  AG_LOGICAL_NOT,
  AG_LOGICAL_AND,
  AG_LOGICAL_OR,

  AG_BITWISE_NOT,
  AG_BITWISE_OR,
  AG_PIPE_GT,
  AG_BITWISE_XOR,
  AG_BITWISE_AND_ASSIGNMENT,
  AG_BITWISE_OR_ASSIGNMENT,
  AG_BITWISE_XOR_ASSIGNMENT,

  AG_LEFT_SHIFT,
  AG_RIGHT_SHIFT,
  AG_LEFT_SHIFT_ASSIGNMENT,
  AG_RIGHT_SHIFT_ASSIGNMENT,

  AG_EQUAL_TO,
  AG_NOT_EQUAL_TO,

  AG_DOT,
  AG_DOT_DOT,
  AG_ELLIPSIS,

  AG_GREATER,
  AG_GREATER_EQUAL,
  AG_LESS,
  AG_LESS_EQUAL,

  AG_ARROW,
  AG_CHANNEL_SEND,
  AG_FAT_ARROW,
  AG_QUESTION,
  AG_COALESCE,
  AG_COALESCE_ASSIGN,

  AG_RETURN,
  AG_IF,
  AG_ELSE,
  AG_WHILE,
  AG_UNTIL,
};

struct Token {
  constexpr Token()
      : token_name{TokenName::AG_NONE},
        value{""},
        line{1},
        column{1} {}

  constexpr Token(const TokenName token)
      : token_name{token},
        value{""},
        line{1},
        column{1} {}

  constexpr Token(const TokenName        token,
                  const std::string_view value)
      : token_name{token},
        value{value},
        line{1},
        column{1} {}

  constexpr Token(const TokenName        token,
                  const std::string_view value,
                  uint32_t               line,
                  uint32_t               column)
      : token_name{token},
        value{value},
        line{line},
        column{column} {}

  TokenName        token_name;
  std::string_view value;
  // For numeric and boolean literals, records the source-level type the
  // literal should infer.  `0` is an `int`, `0u` is a `uint`,
  // `0u8` is a `u8`, and `true`/`false` are `bool`.  Floats leave this
  // UNEXPECTED because their type is determined by the token kind.
  PrimitiveType    literal_type = PrimitiveType::UNEXPECTED;
  std::string_view literal_type_name;
  std::uint32_t    line;
  uint32_t         column;
  // SourceLocation   location; // TODO: single one
};
} // namespace ag
