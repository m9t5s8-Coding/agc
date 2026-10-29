#include "Statements/Utils.hpp"
#include "IR/DataItem.hpp"
#include "IR/IRTable.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include <format>

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
  const auto& token     = parser.current_token();
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

void
Add_Literals::add_to_instruction(LabelCounters& counter,
                                 Parser&        parser) {
  std::string str_name;
  for (const auto l : s_literals) {
    str_name = "str." + std ::to_string(counter.string_label++);
    parser.add_instructios(AG_cscope<DataItem>(str_name, std::string(l)));
  }
}

void
add_system_call_function(CodeGenContext& context) {
  // Function names are mangled (`main` becomes `main$`), so ask the IR table
  // for the actual label instead of assuming the raw source name.
  const auto* main_function = get_ir_table().find_function("main");
  const auto  main_label    = main_function && !main_function->label.empty() ? main_function->label : "main";

  context.syscall << std::format("_start:\n\tand rsp, -16\n\tcall {}\n\tmov rdi, rax\n\tcall __ag_exit\n\n",
                                 main_label);
  context.syscall << std::format("__ag_exit:\n\tmov rax, 60\n\tsyscall\n\tret\n\n");
  context.syscall << std::format(
      "__ag_write:\n\tmov rdx, rsi\n\tmov rsi, rdi\n\tmov rdi, 1\n\tmov rax, 1\n\tsyscall\n\tret\n\n");

  context.syscall << std::format(
      "__ag_read:\n\tmov rdx, rsi\n\tmov rsi, rdi\n\tmov rdi, 0\n\tmov rax, 0\n\tsyscall\n\tret\n\n");
}
} // namespace ag
