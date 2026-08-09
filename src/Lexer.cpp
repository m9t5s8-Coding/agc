#include "Lexer.hpp"

#include "Token.hpp"

#include <iostream>
#include <utility>

namespace ag {
Lexer::Lexer() { m_tokens.reserve(code_size() / 8); }

// TODO : Sort in order for faster search
bool Lexer::contains(const std::string_view str_view, Token& out_token) {
  constexpr static std::pair<std::string_view, Token> s_keyword_map[] = {

      {  "exit",   {TokenName::AG_EXIT}},
      {  "func",   {TokenName::AG_FUNC}},
      {"return", {TokenName::AG_RETURN}},
  };

  for (const auto& [view, token] : s_keyword_map) {
    if (view == str_view) {
      out_token = token;
      return true;
    }
  }
  return false;
}

void Lexer::tokenize() {
  while (m_index < code_size()) {
    const char c = m_source_code[m_index];
    if (!scan_char(c))
      create_keyword(c);
    increase();
  }
}

bool Lexer::scan_char(const char c) {
  switch (c) {
    case '\n': {
      return true;
    }
    case '#': {
      while (peek() != '\n') {
        increase();
      }
      increase();
      return true;
    }
    case ';': {
      add_token({TokenName::AG_SEMICOLON});
      return true;
    }
    case '(': {
      add_token({TokenName::AG_LEFT_PAREN});
      return true;
    }
    case ')': {
      add_token({TokenName::AG_RIGHT_PAREN});
      return true;
    }
    case '{': {
      add_token({TokenName::AG_LEFT_BRACE});
      return true;
    }
    case '}': {
      add_token({TokenName::AG_RIGHT_BRACE});
      return true;
    }
    case '[': {
      add_token({TokenName::AG_LEFT_BRACKET});
      return true;
    }
    case ']': {
      add_token({TokenName::AG_RIGHT_BRACKET});
      return true;
    }
    case '+': {
      add_token({TokenName::AG_PLUS});
      return true;
    }
    case '-': {
      if (peek() == '>') {
        increase();
        add_token({TokenName::AG_ARROW});
        return true;
      }
      add_token({TokenName::AG_MINUS});
      return true;
    }
    case '*': {
      add_token({TokenName::AG_ASTRIC});
      return true;
    }
    case '/': {
      add_token({TokenName::AG_SLASH});
      return true;
    }
    case '=': {
      add_token({TokenName::AG_ASSIGN});
      return true;
    }
    case '!': {
      add_token({TokenName::AG_NOT});
      return true;
    }
    case '%': {
      add_token({TokenName::AD_MODULO});
      return true;
    }
    case '&': {
      add_token({TokenName::AG_ADDRESS});
      return true;
    }
    case ':': {
      add_token({TokenName::AG_COLON});
      return true;
    }
    case '"': {
      increase();
      create_stringn_literal();
      return true;
    }
    case '\'': {
      increase();
      create_char_literal();
      return true;
    }
    case '<': {
      if (peek() == '=') {
        increase();
        add_token({TokenName::AG_LESS_EQUAL});
        return true;
      }
      add_token({TokenName::AG_LESS});
      return true;
    }
    case '>': {
      if (peek() == '=') {
        increase();
        add_token({TokenName::AG_GREATER_EQUAL});
        return true;
      }
      add_token({TokenName::AG_GREATER});
      return true;
    }
    case ',': {
      add_token({TokenName::AG_COMMA});
      return true;
    }
    case '.': {
      add_token({TokenName::AG_DOT});
      return true;
    }
    default: return false;
  }
  return false;
}

void Lexer::create_keyword(const char c) {
  auto is_char = [](const char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
  };

  auto is_num = [](const char c) {
    return c >= '0' && c <= '9';
  };

  if (is_char(c)) {
    m_buffer = get();
    while (is_char(peek()) || is_num(peek())) {
      consume();
    }
    process_word_buffer();
  }

  if (is_num(c)) {
    m_buffer = get();
    while (is_num(peek())) {
      consume();
    }
    process_number_buffer();
  }
}

void Lexer::create_stringn_literal() {
  // auto is_ascii = [](const char c) {
  //   return c != '"' && c >= 32 && c <= 126;
  // };

  if (peek() == '"') {
    add_token({TokenName::AG_STRING_LITERAL});
    return;
  }
  m_buffer = get();
  clear_buffer();
  while (peek() != '"') {
    consume();
  }
  add_token({
      TokenName::AG_STRING_LITERAL,
      {m_buffer, buffer_size()}
  });
  increase();
  clear_buffer();
}

void Lexer::create_char_literal() {
  // auto is_ascii = [](const char c) {
  //   return c >= 32 && c <= 126;
  // };
}

void Lexer::process_word_buffer() {
  if (Token token; contains({m_buffer, buffer_size()}, token)) {
    add_token(token);
  } else {
    add_token({
        TokenName::AG_IDENTIFIER,
        {m_buffer, buffer_size()}
    });
  }
  clear_buffer();
}

void Lexer::process_number_buffer() {
  add_token({
      TokenName::AG_INT_LITERAL,
      {m_buffer, buffer_size()}
  });
  clear_buffer();
}

void Lexer::print_tokens() const {
  for (const auto t : m_tokens) {
    std::cout << static_cast<int>(t.token_name) << ' ';
  }
}

std::ostream& operator<<(std::ostream& ss, const Lexer& tokenizer) {
  for (const auto t : tokenizer.m_tokens) {
    ss << static_cast<int>(t.token_name) << ' ';
  }
  return ss;
}

}  // namespace ag
