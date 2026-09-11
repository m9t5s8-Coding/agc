#pragma once
#include <cstdint>
#include <string_view>

namespace ag {
enum class TokenName {
  AG_NONE,

  AG_FUNC,
  AG_IDENTIFIER,
  AG_KEYWORD,

  AG_LET,
  AG_VAR,

  AG_INT_LITERAL,
  AG_FLOAT_LITERAL,
  AG_STRING_LITERAL,
  AG_CHAR_LITERAL,

  AG_LEFT_PAREN,
  AG_RIGHT_PAREN,
  AG_LEFT_BRACE,
  AG_RIGHT_BRACE,
  AG_LEFT_BRACKET,
  AG_RIGHT_BRACKET,

  AG_SEMICOLON,
  AG_COLON,
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
  AG_BITWISE_XOR,

  AG_LEFT_SHIFT,
  AG_RIGHT_SHIFT,

  AG_EQUAL_TO,
  AG_NOT_EQUAL_TO,

  AG_DOT,

  AG_GREATER,
  AG_GREATER_EQUAL,
  AG_LESS,
  AG_LESS_EQUAL,

  AG_ARROW,
  AG_QUESTION,

  AG_WRITE,
  AG_READ,
  AG_RETURN,
  AG_EXIT,
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
  std::uint32_t    line;
  uint32_t         column;
};
} // namespace ag
