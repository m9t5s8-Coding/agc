#include "Statements/Utils.hpp"
#include "Parser.hpp"
#include <format>
#include <iostream>

namespace ag {
void
print_error(const std::string_view error,
            Parser&                parser,
            const SourceLocation&  location) {
  const auto line_code = parser.get_lexer().get_line_text(location.line);

  std::string error_msg = std::format("{}:{}: {}\n", parser.get_filename(), location.line, error);

  parser.append_error_message(error_msg);
  error_msg.clear();
  error_msg += std::format("   {}\n", parser.get_lexer().get_line_text(location.line));
  parser.append_error_message(error_msg);
}

void
print_error(const std::string_view error,
            Parser&                parser) {
  const auto& token     = parser.peek_token();
  const auto  line_code = parser.get_lexer().get_line_text(token.line);

  std::string error_msg = std::format("{}:{}: {}\n", parser.get_filename(), token.line, error);

  parser.append_error_message(error_msg);
  error_msg.clear();
  error_msg += std::format("   {}\n", parser.get_lexer().get_line_text(token.line));
  parser.append_error_message(error_msg);
  error_msg.clear();
  error_msg += "    ";
  error_msg.append(token.column, ' ');
  error_msg += "^\n";
  parser.append_error_message(error_msg);
}
} // namespace ag
