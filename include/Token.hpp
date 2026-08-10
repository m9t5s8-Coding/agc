#pragma once
#include <string_view>

namespace ag {
enum class TokenName {
  AG_NONE,
  AG_FUNC,
  AG_IDENTIFIER,

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
  AG_MINUS,
  AG_ASTRIC,
  AG_SLASH,
  AG_ASSIGN,

  AG_ADDRESS,
  AD_MODULO,
  AG_NOT,
  AG_DOT,

  AG_GREATER,
  AG_GREATER_EQUAL,
  AG_LESS,
  AG_LESS_EQUAL,

  AG_ARROW,

  AG_PRINTLN,
  AG_RETURN,
  AG_EXIT,
};

struct Token {
  constexpr Token()
    : token_name{TokenName::AG_NONE},
      value{""} {}
  constexpr Token(const TokenName token)
    : token_name{token},
      value{""} {}

  Token(const TokenName token, const std::string_view value)
    : token_name{token},
      value{value} {}
  TokenName        token_name;
  std::string_view value;
};
}  // namespace ag
