#include "Lexer.hpp"

#include "Token.hpp"
#include "error_msg.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <format>
#include <iostream>
#include <utility>

namespace ag {
Lexer::Lexer() { m_tokens.reserve(code_size() / 8); }

// TODO : Sort in order for faster search
bool
Lexer::contains(const std::string_view str_view,
                Token&                 out_token) {
  constexpr static std::pair<std::string_view, Token> s_keyword_map[] = {

      {"and", {TokenName::AG_LOGICAL_AND}},
      {"or", {TokenName::AG_LOGICAL_OR}},
      {"not", {TokenName::AG_LOGICAL_NOT}},
      {"func", {TokenName::AG_FUNC}},
      {"operator", {TokenName::AG_OPERATOR}},
      {"struct", {TokenName::AG_STRUCT}},
      {"enum", {TokenName::AG_ENUM}},
      {"impl", {TokenName::AG_IMPL}},
      {"type", {TokenName::AG_TYPE}},
      {"extern", {TokenName::AG_EXTERN}},
      {"comptime", {TokenName::AG_COMPTIME}},
      {"trait", {TokenName::AG_TRAIT}},
      {"guard", {TokenName::AG_GUARD}},
      {"orelse", {TokenName::AG_ORELSE}},
      {"for", {TokenName::AG_FOR}},
      {"loop", {TokenName::AG_LOOP}},
      {"break", {TokenName::AG_BREAK}},
      {"continue", {TokenName::AG_CONTINUE}},
      {"move", {TokenName::AG_MOVE}},
      {"box", {TokenName::AG_BOX}},
      {"with", {TokenName::AG_WITH}},
      {"defer", {TokenName::AG_DEFER}},
      {"async", {TokenName::AG_ASYNC}},
      {"await", {TokenName::AG_AWAIT}},
      {"unsafe", {TokenName::AG_UNSAFE}},
      {"test", {TokenName::AG_TEST}},
      {"pub", {TokenName::AG_PUB}},
      {"priv", {TokenName::AG_PRIV}},
      {"static", {TokenName::AG_STATIC}},
      {"mut", {TokenName::AG_MUT}},
      {"import", {TokenName::AG_IMPORT}},
      {"mod", {TokenName::AG_MOD}},
      {"as", {TokenName::AG_AS}},
      {"match", {TokenName::AG_MATCH}},
      {"true", {TokenName::AG_BOOL_LITERAL, "1"}},
      {"false", {TokenName::AG_BOOL_LITERAL, "0"}},
      // Both spellings produce the same typed null pointer literal.
      {"null", {TokenName::AG_NULLPTR, "0"}},
      {"nullptr", {TokenName::AG_NULLPTR, "0"}},
      {"return", {TokenName::AG_RETURN}},
      {"char", {TokenName::AG_KEYWORD, "char"}},
      {"int", {TokenName::AG_KEYWORD, "int"}},
      {"uint", {TokenName::AG_KEYWORD, "uint"}},
      {"u8", {TokenName::AG_KEYWORD, "u8"}},
      {"i8", {TokenName::AG_KEYWORD, "i8"}},
      {"u16", {TokenName::AG_KEYWORD, "u16"}},
      {"i16", {TokenName::AG_KEYWORD, "i16"}},
      {"u32", {TokenName::AG_KEYWORD, "u32"}},
      {"i32", {TokenName::AG_KEYWORD, "i32"}},
      {"u64", {TokenName::AG_KEYWORD, "u64"}},
      {"i64", {TokenName::AG_KEYWORD, "i64"}},
      {"float", {TokenName::AG_KEYWORD, "float"}},
      {"double", {TokenName::AG_KEYWORD, "double"}},
      {"bool", {TokenName::AG_KEYWORD, "bool"}},
      {"void", {TokenName::AG_KEYWORD, "void"}},
      {"string", {TokenName::AG_KEYWORD, "string"}},
      {"let", {TokenName::AG_LET}},
      {"var", {TokenName::AG_VAR}},
      {"const", {TokenName::AG_CONST}},
      {"if", {TokenName::AG_IF}},
      {"else", {TokenName::AG_ELSE}},
      {"while", {TokenName::AG_WHILE}},
      {"until", {TokenName::AG_UNTIL}},
  };

  for (const auto& [view, token] : s_keyword_map) {
    if (view == str_view) {
      out_token = token;
      return true;
    }
  }
  return false;
}

void
Lexer::tokenize() {
  m_line_starts.push_back(0);
  while (m_index < code_size()) {
    m_token_start_line   = m_current_line;
    m_token_start_column = m_current_column;

    const char c = m_source_code[m_index];

    // Newlines are handled here so every scanner path leaves the cursor on
    // its last character and the next line always starts at column 0.
    if (c == '\n') {
      process_new_line();
      ++m_index;
      continue;
    }

    if (!scan_char(c))
      create_keyword(c);

    // Comments stop on the newline; let the branch above consume it.
    if (m_index < code_size() && m_source_code[m_index] == '\n')
      continue;

    increase();
  }
}

bool
Lexer::scan_char(const char c) {
  switch (c) {
  case '#': {
    // Leave the cursor on the newline (or at EOF) so tokenize() can update
    // the line table uniformly.
    while (m_index < code_size() && m_source_code[m_index] != '\n')
      increase();

    return true;
  }

  case '@': {
    add_token({TokenName::AG_AT});
    return true;
  }

  case '"': {
    increase();
    // Three adjacent quotes start a multi-line string literal.
    const bool triple = m_index + 1 < code_size() &&
                        m_source_code[m_index] == '"' &&
                        m_source_code[m_index + 1] == '"';
    if (triple) {
      increase();
      increase();
    }
    create_string_literal(false, false, triple);
    return true;
  }

  case '\'': {
    increase();
    create_char_literal();
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

  case ';': {
    add_token({TokenName::AG_SEMICOLON});
    return true;
  }

  case ':': {
    if (peek() == ':') {
      increase();
      add_token({TokenName::AG_DOUBLE_COLON});
    } else {
      add_token({TokenName::AG_COLON});
    }
    return true;
  }

  case ',': {
    add_token({TokenName::AG_COMMA});
    return true;
  }

  case '+': {
    char next = peek();

    if (next == '+') {
      increase();
      add_token({TokenName::AG_INCREMENT});
    } else if (next == '=') {
      increase();
      add_token({TokenName::AG_ADDITION_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_PLUS});
    }

    return true;
  }

  case '-': {
    char next = peek();

    if (next == '-') {
      increase();
      add_token({TokenName::AG_DECREMENT});
    } else if (next == '=') {
      increase();
      add_token({TokenName::AG_SUBTRACTION_ASSIGNMENT});
    } else if (next == '>') {
      increase();
      add_token({TokenName::AG_ARROW});
    } else {
      add_token({TokenName::AG_MINUS});
    }

    return true;
  }

  case '*': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_MULTIPLY_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_ASTERISK});
    }

    return true;
  }

  case '/': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_DIVISION_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_SLASH});
    }

    return true;
  }

  case '=': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_EQUAL_TO});
    } else if (peek() == '>') {
      increase();
      add_token({TokenName::AG_FAT_ARROW});
    } else {
      add_token({TokenName::AG_ASSIGN});
    }

    return true;
  }

  case '&': {
    if (peek() == '&') {
      increase();
      add_token({TokenName::AG_LOGICAL_AND});
    } else if (peek() == '=') {
      increase();
      add_token({TokenName::AG_BITWISE_AND_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_REFERENCE});
    }

    return true;
  }

  case '%': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_MODULO_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_MODULO});
    }

    return true;
  }

  case '!': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_NOT_EQUAL_TO});
    } else {
      add_token({TokenName::AG_LOGICAL_NOT});
    }

    return true;
  }

  case '|': {
    if (peek() == '|') {
      increase();
      add_token({TokenName::AG_LOGICAL_OR});
    } else if (peek() == '=') {
      increase();
      add_token({TokenName::AG_BITWISE_OR_ASSIGNMENT});
    } else if (peek() == '>') {
      increase();
      add_token({TokenName::AG_PIPE_GT});
    } else {
      add_token({TokenName::AG_BITWISE_OR});
    }

    return true;
  }

  case '~': {
    add_token({TokenName::AG_BITWISE_NOT});
    return true;
  }

  case '^': {
    if (peek() == '=') {
      increase();
      add_token({TokenName::AG_BITWISE_XOR_ASSIGNMENT});
    } else {
      add_token({TokenName::AG_BITWISE_XOR});
    }
    return true;
  }

  case '<': {
    if (peek() == '<') {
      increase();
      if (peek() == '=') {
        increase();
        add_token({TokenName::AG_LEFT_SHIFT_ASSIGNMENT});
      } else {
        add_token({TokenName::AG_LEFT_SHIFT});
      }
    } else if (peek() == '=') {
      increase();
      add_token({TokenName::AG_LESS_EQUAL});
    } else if (peek() == '-') {
      increase();
      add_token({TokenName::AG_CHANNEL_SEND});
    } else {
      add_token({TokenName::AG_LESS});
    }

    return true;
  }

  case '>': {
    if (peek() == '>') {
      increase();
      if (peek() == '=') {
        increase();
        add_token({TokenName::AG_RIGHT_SHIFT_ASSIGNMENT});
      } else {
        add_token({TokenName::AG_RIGHT_SHIFT});
      }
    } else if (peek() == '=') {
      increase();
      add_token({TokenName::AG_GREATER_EQUAL});
    } else {
      add_token({TokenName::AG_GREATER});
    }

    return true;
  }

  case '.': {
    if (peek() == '.') {
      increase();
      if (peek() == '.') {
        increase();
        add_token({TokenName::AG_ELLIPSIS});
      } else {
        add_token({TokenName::AG_DOT_DOT});
      }
    } else {
      add_token({TokenName::AG_DOT});
    }
    return true;
  }

  case '?': {
    if (peek() == '?') {
      increase();
      if (peek() == '=') {
        increase();
        add_token({TokenName::AG_COALESCE_ASSIGN});
      } else {
        add_token({TokenName::AG_COALESCE});
      }
    } else {
      add_token({TokenName::AG_QUESTION});
    }
    return true;
  }

  default:
    return false;
  }
}

void
Lexer::create_keyword(const char c) {
  auto is_char = [](const char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };

  auto is_num = [](const char c) { return c >= '0' && c <= '9'; };

  if (is_char(c)) {
    m_buffer = get();
    while (is_char(peek()) || is_num(peek())) {
      consume();
    }
    process_word_buffer();
  }

  if (is_num(c)) {
    m_buffer = get();

    // Base-prefixed literals may contain letters after the prefix, so the
    // scanner must consume the prefix before continuing with digits.
    const bool has_base_prefix =
        c == '0' && (peek() == 'x' || peek() == 'X' ||
                     peek() == 'b' || peek() == 'B' ||
                     peek() == 'o' || peek() == 'O');
    if (has_base_prefix) {
      consume();
      while (is_num(peek()) ||
             (peek() >= 'a' && peek() <= 'f') ||
             (peek() >= 'A' && peek() <= 'F') ||
             peek() == '_') {
        consume();
      }
    } else {
      // Decimal literals may contain a fractional part and an exponent.
      // `1..5` is a range, so only treat `.` as a decimal point when a digit
      // follows it.
      const auto char_at = [&](size_t offset) -> char {
        const size_t position = m_index + offset;
        return position < code_size() ? m_source_code[position] : '\0';
      };

      bool saw_dot = false;
      while (is_valid()) {
        const char next = peek();
        if (is_num(next) || next == '_') {
          consume();
          continue;
        }
        if (next == '.' && !saw_dot && is_num(char_at(2))) {
          saw_dot = true;
          consume();
          continue;
        }
        if ((next == 'e' || next == 'E') &&
            (is_num(char_at(2)) ||
             ((char_at(2) == '+' || char_at(2) == '-') &&
              is_num(char_at(3))))) {
          consume(); // e/E
          if (peek() == '+' || peek() == '-')
            consume();
          while (is_num(peek()) || peek() == '_')
            consume();
          break;
        }
        break;
      }
    }

    // Optional literal-width suffix: `u` (uint), `u8`, `i8`, ... `u64`,
    // `i64`.  Consume it here so `0u8` is one token rather than `0` followed
    // by an identifier.
    if (peek() == 'u' || peek() == 'U' || peek() == 'i' || peek() == 'I') {
      consume();
      while (is_num(peek()))
        consume();
    }
    process_number_buffer();
  }
}

void
Lexer::create_string_literal(bool raw,
                             bool fstring,
                             bool triple) {
  // On entry m_index points at the first character after the opening quote
  // (or after the third quote of a `"""` literal).  Leave m_index on the
  // closing quote; tokenize() advances past it.
  const uint32_t start_line   = m_current_line;
  const uint32_t start_column = m_current_column > 0 ? m_current_column - 1 : 0;
  bool           closed       = false;
  std::string    decoded;

  // Braces inside an f-string delimit embedded expressions.  While nested,
  // quotes and braces belong to the expression text rather than the literal,
  // so do not let them close the outer token.
  int  brace_depth  = 0;
  char quote_context = 0;

  while (m_index < code_size()) {
    const char current = m_source_code[m_index];

    if (brace_depth == 0) {
      if (triple) {
        if (current == '"' && m_index + 2 < code_size() &&
            m_source_code[m_index + 1] == '"' &&
            m_source_code[m_index + 2] == '"') {
          m_index += 2;
          m_current_column += 2;
          closed = true;
          break;
        }
      } else if (current == '"') {
        closed = true;
        break;
      }
    }

    if (current == '\n') {
      decoded.push_back(current);
      ++m_index;
      ++m_current_line;
      m_current_column = 0;
      m_line_starts.push_back(m_index);
      continue;
    }

    if (fstring && brace_depth > 0) {
      if (quote_context != 0) {
        decoded.push_back(current);
        if (current == '\\' && m_index + 1 < code_size()) {
          decoded.push_back(m_source_code[m_index + 1]);
          m_index += 2;
          m_current_column += 2;
          continue;
        }
        if (current == quote_context)
          quote_context = 0;
        ++m_index;
        ++m_current_column;
        continue;
      }

      if (current == '"' || current == '\'') {
        quote_context = current;
      } else if (current == '{') {
        ++brace_depth;
      } else if (current == '}') {
        --brace_depth;
      }

      decoded.push_back(current);
      ++m_index;
      ++m_current_column;
      continue;
    }

    if (fstring && current == '{') {
      if (m_index + 1 < code_size() && m_source_code[m_index + 1] == '{') {
        // `{{` is an escaped literal brace; keep both for the parser.
        decoded.push_back('{');
        decoded.push_back('{');
        m_index += 2;
        m_current_column += 2;
        continue;
      }
      ++brace_depth;
      decoded.push_back(current);
      ++m_index;
      ++m_current_column;
      continue;
    }

    if ((raw || fstring) && current == '\\') {
      // Raw strings keep the backslash and the following byte verbatim.  For
      // f-strings escapes are decoded later, once literal segments have been
      // separated from embedded expressions.
      decoded.push_back(current);
      if (m_index + 1 < code_size()) {
        decoded.push_back(m_source_code[m_index + 1]);
        m_index += 2;
        m_current_column += 2;
      } else {
        ++m_index;
        ++m_current_column;
      }
      continue;
    }

    if (!raw && !fstring && current == '\\') {
      if (m_index + 1 >= code_size()) {
        decoded.push_back('\\');
        ++m_index;
        ++m_current_column;
        break;
      }

      const char escaped = m_source_code[m_index + 1];
      switch (escaped) {
      case 'n':
        decoded.push_back('\n');
        break;
      case 'r':
        decoded.push_back('\r');
        break;
      case 't':
        decoded.push_back('\t');
        break;
      case '0':
        decoded.push_back('\0');
        break;
      case 'a':
        decoded.push_back('\a');
        break;
      case 'b':
        decoded.push_back('\b');
        break;
      case 'f':
        decoded.push_back('\f');
        break;
      case 'v':
        decoded.push_back('\v');
        break;
      case '\\':
        decoded.push_back('\\');
        break;
      case '"':
        decoded.push_back('"');
        break;
      case '\'':
        decoded.push_back('\'');
        break;
      default:
        // Keep unknown escapes as their escaped character, e.g. \q -> q.
        decoded.push_back(escaped);
        break;
      }

      m_index += 2;
      m_current_column += 2;
      continue;
    }

    decoded.push_back(current);
    ++m_index;
    ++m_current_column;
  }

  if (!closed) {
    const std::string line_text(get_line_text(start_line));

    // Match the shared diagnostic renderer: a three-space gutter followed by
    // the source prefix, preserving tabs so the caret lines up in a terminal.
    std::string  caret_prefix;
    const size_t prefix_size = std::min<size_t>(start_column, line_text.size());
    for (size_t i = 0; i < prefix_size; ++i)
      caret_prefix.push_back(line_text[i] == '\t' ? '\t' : ' ');
    if (start_column > prefix_size)
      caret_prefix.append(start_column - prefix_size, ' ');

    m_error_message += std::format(
        "{}:{}: {}\n   {}\n   {}^\n", m_filename, start_line, error::UnterminatedString, line_text, caret_prefix);
  }

  m_decoded_strings.push_back(std::move(decoded));
  const std::string& stored = m_decoded_strings.back();
  add_token({fstring ? TokenName::AG_FSTRING : TokenName::AG_STRING_LITERAL,
             std::string_view(stored)});
  clear_buffer();
}

void
Lexer::create_char_literal() {
  // On entry m_index points at the first character after the opening quote.
  // Leave m_index on the closing quote; tokenize() advances past it.
  const uint32_t start_line   = m_current_line;
  const uint32_t start_column = m_current_column > 0 ? m_current_column - 1 : 0;

  const auto report = [&](const std::string_view message) {
    const std::string line_text(get_line_text(start_line));
    std::string       caret_prefix;
    const size_t      prefix_size = std::min<size_t>(start_column, line_text.size());
    for (size_t i = 0; i < prefix_size; ++i)
      caret_prefix.push_back(line_text[i] == '\t' ? '\t' : ' ');
    if (start_column > prefix_size)
      caret_prefix.append(start_column - prefix_size, ' ');

    m_error_message += std::format(
        "{}:{}: {}\n   {}\n   {}^\n", m_filename, start_line, message, line_text, caret_prefix);
  };

  if (m_index >= code_size() || m_source_code[m_index] == '\n') {
    report(error::UnterminatedChar);
    m_decoded_strings.push_back("0");
    add_token({TokenName::AG_CHAR_LITERAL, std::string_view(m_decoded_strings.back())});
    return;
  }

  if (m_source_code[m_index] == '\'') {
    report(error::EmptyCharLiteral);
    m_decoded_strings.push_back("0");
    add_token({TokenName::AG_CHAR_LITERAL, std::string_view(m_decoded_strings.back())});
    return;
  }

  char decoded = 0;
  if (m_source_code[m_index] == '\\') {
    if (m_index + 1 >= code_size()) {
      report(error::UnterminatedChar);
      m_decoded_strings.push_back("0");
      add_token({TokenName::AG_CHAR_LITERAL, std::string_view(m_decoded_strings.back())});
      return;
    }

    const char escaped = m_source_code[m_index + 1];
    switch (escaped) {
    case 'n':
      decoded = '\n';
      break;
    case 'r':
      decoded = '\r';
      break;
    case 't':
      decoded = '\t';
      break;
    case '0':
      decoded = '\0';
      break;
    case 'a':
      decoded = '\a';
      break;
    case 'b':
      decoded = '\b';
      break;
    case 'f':
      decoded = '\f';
      break;
    case 'v':
      decoded = '\v';
      break;
    case '\\':
      decoded = '\\';
      break;
    case '"':
      decoded = '"';
      break;
    case '\'':
      decoded = '\'';
      break;
    default:
      report(error::UnrecognizedEscape);
      decoded = escaped;
      break;
    }

    m_index += 2;
    m_current_column += 2;
  } else {
    decoded = m_source_code[m_index];
    ++m_index;
    ++m_current_column;
  }

  if (m_index >= code_size() || m_source_code[m_index] == '\n') {
    report(error::UnterminatedChar);
  } else if (m_source_code[m_index] != '\'') {
    // Consume the rest of the malformed literal so the scanner resumes at a
    // useful place, then report the length error.
    while (m_index < code_size() && m_source_code[m_index] != '\'' &&
           m_source_code[m_index] != '\n') {
      ++m_index;
      ++m_current_column;
    }
    report(error::CharLiteralTooLong);
  }

  // Store the numeric byte value.  PrimaryExpression uses this directly as
  // the literal's IR value.
  m_decoded_strings.push_back(
      std::to_string(static_cast<unsigned char>(decoded)));
  add_token({TokenName::AG_CHAR_LITERAL, std::string_view(m_decoded_strings.back())});
}

void
Lexer::process_word_buffer() {
  const std::string_view word{m_buffer, buffer_size()};

  // `f"..."`, `r"..."`, and the combined `fr"..."`/`rf"..."` forms are
  // single tokens.  The prefix is only recognised when the quote follows
  // immediately, so an ordinary identifier named `f` is unaffected.
  const bool has_f_prefix =
      word == "f" || word == "F" || word == "fr" || word == "rf" ||
      word == "Fr" || word == "fR" || word == "FR" || word == "RF";
  const bool has_r_prefix =
      word == "r" || word == "R" || word == "fr" || word == "rf" ||
      word == "Fr" || word == "fR" || word == "FR" || word == "RF";

  if ((has_f_prefix || has_r_prefix) && peek() == '"') {
    increase(); // move onto the opening quote

    const bool triple = m_index + 2 < code_size() &&
                        m_source_code[m_index + 1] == '"' &&
                        m_source_code[m_index + 2] == '"';

    increase(); // move past the opening quote
    if (triple) {
      increase();
      increase();
    }

    create_string_literal(has_r_prefix, has_f_prefix, triple);
    clear_buffer();
    return;
  }

  if (Token token; contains({m_buffer, buffer_size()}, token)) {
    if (token.token_name == TokenName::AG_BOOL_LITERAL) {
      token.literal_type      = PrimitiveType::BOOL;
      token.literal_type_name = "bool";
    }
    add_token(token);
  } else {
    add_token({TokenName::AG_IDENTIFIER, {m_buffer, buffer_size()}});
  }
  clear_buffer();
}

void
Lexer::process_number_buffer() {
  const std::string raw(m_buffer, buffer_size());

  int    base  = 10;
  size_t start = 0;
  if (raw.size() > 1 && raw[0] == '0') {
    switch (raw[1]) {
    case 'x':
    case 'X':
      base  = 16;
      start = 2;
      break;
    case 'b':
    case 'B':
      base  = 2;
      start = 2;
      break;
    case 'o':
    case 'O':
      base  = 8;
      start = 2;
      break;
    default:
      break;
    }
  }

  const auto digit_value = [](const char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return -1;
  };

  // Split the scanned spelling into a numeric portion and an optional type
  // suffix (`u`, `u8`, `i16`, `u32`, `i64`, ...).  The scanner consumes the
  // suffix so it stays part of this token.
  size_t number_end = start;
  if (base != 10) {
    while (number_end < raw.size()) {
      const char c = raw[number_end];
      if (c == '_') {
        ++number_end;
        continue;
      }
      const int digit = digit_value(c);
      if (digit < 0 || digit >= base)
        break;
      ++number_end;
    }
  } else {
    bool saw_dot = false;
    while (number_end < raw.size()) {
      const char c = raw[number_end];
      if ((c >= '0' && c <= '9') || c == '_') {
        ++number_end;
        continue;
      }
      if (c == '.' && !saw_dot) {
        saw_dot = true;
        ++number_end;
        continue;
      }
      if ((c == 'e' || c == 'E') && number_end + 1 < raw.size()) {
        size_t exponent = number_end + 1;
        if (raw[exponent] == '+' || raw[exponent] == '-')
          ++exponent;
        if (exponent < raw.size() && raw[exponent] >= '0' && raw[exponent] <= '9') {
          number_end = exponent + 1;
          while (number_end < raw.size() &&
                 ((raw[number_end] >= '0' && raw[number_end] <= '9') ||
                  raw[number_end] == '_'))
            ++number_end;
          break;
        }
      }
      break;
    }
  }

  const std::string number(raw.substr(0, number_end));
  std::string       suffix(raw.substr(number_end));

  std::string lower_suffix;
  lower_suffix.reserve(suffix.size());
  for (const char character : suffix)
    lower_suffix.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));

  std::string_view suffix_type_name = "int";
  PrimitiveType    suffix_primitive = PrimitiveType::INT;
  bool             suffix_valid     = true;
  if (lower_suffix.empty() || lower_suffix == "i64") {
    suffix_type_name = "int";
  } else if (lower_suffix == "u") {
    suffix_type_name = "uint";
  } else if (lower_suffix == "u8") {
    suffix_type_name = "u8";
  } else if (lower_suffix == "i8") {
    suffix_type_name = "i8";
  } else if (lower_suffix == "u16") {
    suffix_type_name = "u16";
  } else if (lower_suffix == "i16") {
    suffix_type_name = "i16";
  } else if (lower_suffix == "u32") {
    suffix_type_name = "u32";
  } else if (lower_suffix == "i32") {
    suffix_type_name = "i32";
  } else if (lower_suffix == "u64") {
    suffix_type_name = "u64";
  } else {
    // A trailing `f`/`F` on a decimal float is harmless; everything else is
    // diagnosed below.
    if (!(base == 10 && number.find_first_of(".eE") != std::string::npos &&
          (lower_suffix == "f" || lower_suffix == "d"))) {
      suffix_valid = false;
    }
    if (lower_suffix == "f" || lower_suffix == "d") {
      suffix_type_name = "double";
      suffix_primitive = PrimitiveType::DOUBLE;
    }
  }

  // Decimal literals with a fractional part or exponent are stored as their
  // cleaned source spelling.  The LLVM backend turns them into an IEEE-754
  // value when it emits the operand.
  const bool is_decimal_float =
      base == 10 && number.find_first_of(".eE") != std::string::npos;
  if (is_decimal_float) {
    std::string cleaned;
    cleaned.reserve(number.size());
    for (const char character : number) {
      if (character != '_')
        cleaned.push_back(character);
    }

    char*        end   = nullptr;
    const double value = std::strtod(cleaned.c_str(), &end);
    const bool   valid_float = end != cleaned.c_str() && *end == '\0';
    (void)value;

    if (!valid_float || !suffix_valid) {
      const size_t line_start =
          m_token_start_line > 0 && m_token_start_line <= m_line_starts.size()
              ? m_line_starts[m_token_start_line - 1]
              : 0;
      size_t line_end = m_source_code.find('\n', line_start);
      if (line_end == std::string_view::npos)
        line_end = code_size();
      const std::string line_text(m_source_code.substr(line_start, line_end - line_start));
      std::string       caret_prefix;
      const size_t      prefix_size = std::min<size_t>(m_token_start_column, line_text.size());
      for (size_t i = 0; i < prefix_size; ++i)
        caret_prefix.push_back(line_text[i] == '\t' ? '\t' : ' ');
      if (m_token_start_column > prefix_size)
        caret_prefix.append(m_token_start_column - prefix_size, ' ');

      m_error_message += std::format("{}:{}: {}\n   {}\n   {}^\n",
                                     m_filename,
                                     m_token_start_line,
                                     error::InvalidNumberLiteral,
                                     line_text,
                                     caret_prefix);
      m_decoded_strings.push_back("0.0");
    } else {
      m_decoded_strings.push_back(std::move(cleaned));
    }

    add_token({TokenName::AG_FLOAT_LITERAL,
               std::string_view(m_decoded_strings.back())});
    clear_buffer();
    return;
  }

  bool                valid       = false;
  bool                out_of_range = false;
  unsigned long long  value       = 0;
  const unsigned long long max_value = ~0ULL;
  for (size_t i = start; i < number.size(); ++i) {
    if (number[i] == '_')
      continue;

    const int digit = digit_value(number[i]);
    if (digit < 0 || digit >= base) {
      valid = false;
      break;
    }
    if (value > (max_value - static_cast<unsigned long long>(digit)) /
                    static_cast<unsigned long long>(base)) {
      out_of_range = true;
      break;
    }

    value = value * static_cast<unsigned long long>(base) +
            static_cast<unsigned long long>(digit);
    valid = true;
  }

  if (!valid || out_of_range || !suffix_valid) {
    // Only the line starts seen so far are recorded; derive this token's full
    // source line directly so diagnostics do not spill into following lines.
    const size_t line_start =
        m_token_start_line > 0 && m_token_start_line <= m_line_starts.size()
            ? m_line_starts[m_token_start_line - 1]
            : 0;
    size_t line_end = m_source_code.find('\n', line_start);
    if (line_end == std::string_view::npos)
      line_end = code_size();
    const std::string line_text(m_source_code.substr(line_start, line_end - line_start));
    std::string       caret_prefix;
    const size_t      prefix_size = std::min<size_t>(m_token_start_column, line_text.size());
    for (size_t i = 0; i < prefix_size; ++i)
      caret_prefix.push_back(line_text[i] == '\t' ? '\t' : ' ');
    if (m_token_start_column > prefix_size)
      caret_prefix.append(m_token_start_column - prefix_size, ' ');

    m_error_message += std::format("{}:{}: {}\n   {}\n   {}^\n",
                                   m_filename,
                                   m_token_start_line,
                                   out_of_range ? error::NumberOutOfRange
                                                : error::InvalidNumberLiteral,
                                   line_text,
                                   caret_prefix);
    m_decoded_strings.push_back("0");
  } else {
    m_decoded_strings.push_back(std::to_string(value));
  }

  Token token{TokenName::AG_INT_LITERAL,
              std::string_view(m_decoded_strings.back())};
  token.literal_type      = suffix_primitive;
  token.literal_type_name = suffix_type_name;
  add_token(token);
  clear_buffer();
}

void
Lexer::print_tokens() const {
  for (const auto t : m_tokens) {
    std::cout << static_cast<int>(t.token_name) << ' ';
  }
}

std::ostream&
operator<<(std::ostream& ss,
           const Lexer&  tokenizer) {
  for (const auto t : tokenizer.m_tokens) {
    ss << static_cast<int>(t.token_name) << ' ';
  }
  return ss;
}

} // namespace ag
