#include "Statements/Utils.hpp"
#include "Parser.hpp"
#include <format>
#include <iostream>

namespace ag {
void
print_error(const std::string_view error,
            const Parser&          parser) {
  const auto& token     = parser.peek_token();
  const auto  line_code = parser.get_lexer().get_line_text(token.line);

  std::string error_msg =
      std::format("{}:{}: {}", parser.get_filename(), token.line, error);

  std::cout << error_msg << '\n';
  error_msg.clear();
  error_msg +=
      std::format("   {}", parser.get_lexer().get_line_text(token.line));
  std::cout << error_msg << '\n';
  error_msg.clear();
  error_msg += "    ";
  error_msg.append(token.column, ' ');
  error_msg += '^';
  std::cout << error_msg << '\n';
}
} // namespace ag
