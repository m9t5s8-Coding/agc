#include "Statements/Utils.hpp"
#include "Expressions/Expression.hpp"
#include "IR/DataItem.hpp"
#include "IR/IRTable.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include <format>
#include <memory>
#include <optional>
#include <string>

namespace ag {
namespace {
// Shared diagnostic renderer used by every syntax and semantic error so the
// source line and caret always use the same gutter and tab handling.
void
append_diagnostic(Parser&               parser,
                  const std::string_view error,
                  uint32_t               line,
                  uint32_t               column) {
  if (line == 0) {
    const auto& token = parser.current_token();
    line              = token.line;
    column            = token.column;
  }

  const std::string_view line_text = parser.get_lexer().get_line_text(line);

  parser.append_error_message(std::format("{}:{}: {}\n", parser.get_filename(), line, error));
  parser.append_error_message(std::format("   {}\n", line_text));

  // Reproduce the source prefix exactly (tabs included) so the caret sits
  // under the reported token regardless of indentation.
  std::string caret = "   ";
  const size_t prefix_size = std::min<size_t>(column, line_text.size());
  for (size_t i = 0; i < prefix_size; ++i)
    caret.push_back(line_text[i] == '\t' ? '\t' : ' ');
  if (column > prefix_size)
    caret.append(column - prefix_size, ' ');
  caret += "^\n";
  parser.append_error_message(caret);
}
} // namespace

std::string
global_initializer(Expression* expression) {
  if (!expression)
    return "0";

  if (auto* primary = dynamic_cast<PrimaryExpression*>(expression)) {
    if (primary->primary_type == PrimitiveType::INT)
      return std::string(primary->value);
    if (primary->primary_type == PrimitiveType::STRING)
      return "str." + std::to_string(primary->index);
    return "0";
  }

  if (auto* unary = dynamic_cast<UniaryExpression*>(expression)) {
    const std::string inner = global_initializer(unary->operand.get());
    if (unary->un_operator == UN_OPERATOR::NEGATIVE) {
      // Only negate numeric literals; string labels are addresses and should
      // not be turned into nonsensical negative values.
      if (!inner.empty() && inner[0] != 's' && inner[0] != 'g')
        return "-" + inner;
    } else if (unary->un_operator == UN_OPERATOR::POSITIVE) {
      return inner;
    }
  }

  return "0";
}

void
print_error(const std::string_view error,
            Parser&                parser,
            const SourceLocation&  location) {
  append_diagnostic(parser, error, location.line, location.column);
}

void
print_error(const std::string_view error,
            Parser&                parser) {
  const auto& token = parser.current_token();
  append_diagnostic(parser, error, token.line, token.column);
}

std::optional<DataType>
parse_type(Parser& parser) {
  if (!parser.check_one_of({TokenName::AG_KEYWORD, TokenName::AG_IDENTIFIER})) {
    print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedTypeName,
                parser);
    return std::nullopt;
  }

  const SourceLocation type_location = {parser.current_token().line,
                                        parser.current_token().column};

  std::string raw_name(parser.current_token().value);
  parser.advance();

  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedTypeName,
                  parser);
      return std::nullopt;
    }
    raw_name += "::";
    raw_name += parser.current_token().value;
    parser.advance();
  }

  DataType type;
  const std::string resolved_name = parser.resolve_type_name(raw_name);
  type.type_name                  = parser.intern_name(resolved_name);
  type.primitive_type             = get_datatypes(type.type_name);

  bool unresolved_user_type = false;

  if (type.primitive_type == PrimitiveType::USER_DEFINED) {
    if (type.type_name == "Self") {
      if (parser.current_impl_type().empty()) {
        print_error(error::UndefinedType, parser, type_location);
        return std::nullopt;
      }
      type = make_named_type(parser.current_impl_type());
    } else if (const auto* definition = get_struct_table().lookup(type.type_name)) {
      type.kind          = TypeKind::STRUCT;
      type.type_name     = definition->name;
      type.size_in_bytes = definition->size_in_bytes;
      type.alignment     = definition->alignment;
    } else if (const auto* definition = get_enum_table().lookup(type.type_name)) {
      type.kind          = TypeKind::ENUM;
      type.type_name     = definition->name;
      type.size_in_bytes = definition->size_in_bytes;
      type.alignment     = definition->alignment;
    } else {
      // Pointers to incomplete user-defined types are useful for opaque
      // handles (for example the `ostream*` stream type in the examples).
      // A non-pointer declaration, however, must name a real type.
      unresolved_user_type = true;
    }
  }

  bool has_pointer = false;
  while (parser.match(TokenName::AG_ASTERISK)) {
    has_pointer = true;

    auto pointee = std::make_shared<DataType>(type);

    DataType pointer;
    pointer.kind           = TypeKind::POINTER;
    pointer.primitive_type = PrimitiveType::USER_DEFINED;
    pointer.pointee        = std::move(pointee);
    type                   = std::move(pointer);
  }

  if (unresolved_user_type && !has_pointer) {
    print_error(error::UndefinedType, parser, type_location);
    return std::nullopt;
  }

  return type;
}

std::optional<std::string>
parse_operator_symbol(Parser& parser) {
  const Token token = parser.current_token();

  auto consume = [&](const std::string_view symbol) -> std::optional<std::string> {
    parser.advance();
    return std::string(symbol);
  };

  switch (token.token_name) {
  case TokenName::AG_PLUS:
    return consume("+");
  case TokenName::AG_MINUS:
    return consume("-");
  case TokenName::AG_ASTERISK:
    return consume("*");
  case TokenName::AG_SLASH:
    return consume("/");
  case TokenName::AG_MODULO:
    return consume("%");
  case TokenName::AG_LEFT_SHIFT:
    return consume("<<");
  case TokenName::AG_RIGHT_SHIFT:
    return consume(">>");
  case TokenName::AG_LESS:
    return consume("<");
  case TokenName::AG_LESS_EQUAL:
    return consume("<=");
  case TokenName::AG_GREATER:
    return consume(">");
  case TokenName::AG_GREATER_EQUAL:
    return consume(">=");
  case TokenName::AG_EQUAL_TO:
    return consume("==");
  case TokenName::AG_NOT_EQUAL_TO:
    return consume("!=");
  case TokenName::AG_REFERENCE:
    return consume("&");
  case TokenName::AG_BITWISE_OR:
    return consume("|");
  case TokenName::AG_BITWISE_XOR:
    return consume("^");
  case TokenName::AG_LOGICAL_AND:
    return consume("&&");
  case TokenName::AG_LOGICAL_OR:
    return consume("||");
  case TokenName::AG_LOGICAL_NOT:
    return consume("!");
  case TokenName::AG_BITWISE_NOT:
    return consume("~");
  case TokenName::AG_INCREMENT:
    return consume("++");
  case TokenName::AG_DECREMENT:
    return consume("--");
  case TokenName::AG_ASSIGN:
    return consume("=");
  case TokenName::AG_ADDITION_ASSIGNMENT:
    return consume("+=");
  case TokenName::AG_SUBTRACTION_ASSIGNMENT:
    return consume("-=");
  case TokenName::AG_MULTIPLY_ASSIGNMENT:
    return consume("*=");
  case TokenName::AG_DIVISION_ASSIGNMENT:
    return consume("/=");
  case TokenName::AG_MODULO_ASSIGNMENT:
    return consume("%=");
  case TokenName::AG_BITWISE_AND_ASSIGNMENT:
    return consume("&=");
  case TokenName::AG_BITWISE_OR_ASSIGNMENT:
    return consume("|=");
  case TokenName::AG_BITWISE_XOR_ASSIGNMENT:
    return consume("^=");
  case TokenName::AG_LEFT_SHIFT_ASSIGNMENT:
    return consume("<<=");
  case TokenName::AG_RIGHT_SHIFT_ASSIGNMENT:
    return consume(">>=");
  case TokenName::AG_LEFT_BRACKET:
    parser.advance();
    if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket))
      return std::nullopt;
    return std::string("[]");
  default:
    print_error(error::ExpectedOperator, parser);
    return std::nullopt;
  }
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

  // Heap allocation is backed by anonymous private mappings.  A 16-byte
  // header stores the requested size so free() does not need a second
  // argument; the returned pointer starts immediately after the header.
  context.syscall << "__ag_alloc:\n"
                     "\tpush rbx\n"
                     "\tmov rbx, rdi\n"
                     "\tlea rsi, [rbx + 16]\n"
                     "\txor edi, edi\n"
                     "\tmov edx, 3\n"
                     "\tmov r10d, 0x22\n"
                     "\tmov r8, -1\n"
                     "\txor r9d, r9d\n"
                     "\tmov eax, 9\n"
                     "\tsyscall\n"
                     "\ttest rax, rax\n"
                     "\tjs .alloc_failed\n"
                     "\tmov [rax], rbx\n"
                     "\tadd rax, 16\n"
                     "\tpop rbx\n"
                     "\tret\n"
                     ".alloc_failed:\n"
                     "\txor eax, eax\n"
                     "\tpop rbx\n"
                     "\tret\n\n";

  context.syscall << "__ag_free:\n"
                     "\ttest rdi, rdi\n"
                     "\tjz .free_done\n"
                     "\tsub rdi, 16\n"
                     "\tmov rsi, [rdi]\n"
                     "\tadd rsi, 16\n"
                     "\tmov eax, 11\n"
                     "\tsyscall\n"
                     ".free_done:\n"
                     "\tret\n\n";
}

ModuleTable&
get_module_table() {
  static ModuleTable table;
  return table;
}

StructTable&
get_struct_table() {
  static StructTable table;
  return table;
}

EnumTable&
get_enum_table() {
  static EnumTable table;
  return table;
}

DataType
make_struct_type(std::string_view name) {
  DataType type;
  type.kind           = TypeKind::STRUCT;
  type.primitive_type = PrimitiveType::USER_DEFINED;
  type.type_name      = name;

  if (const auto* definition = get_struct_table().lookup(name)) {
    // Point at the registry-owned name so types captured by AST nodes remain
    // valid after parser-local strings are popped.
    type.type_name     = definition->name;
    type.size_in_bytes = definition->size_in_bytes;
    type.alignment     = definition->alignment;
  }

  return type;
}

DataType
make_enum_type(std::string_view name) {
  DataType type;
  type.kind           = TypeKind::ENUM;
  type.primitive_type = PrimitiveType::USER_DEFINED;
  type.type_name      = name;

  if (const auto* definition = get_enum_table().lookup(name)) {
    type.type_name     = definition->name;
    type.size_in_bytes = definition->size_in_bytes;
    type.alignment     = definition->alignment;
  }

  return type;
}

DataType
make_named_type(std::string_view name) {
  if (const auto* definition = get_struct_table().lookup(name))
    return make_struct_type(definition->name);
  if (const auto* definition = get_enum_table().lookup(name))
    return make_enum_type(definition->name);
  return make_struct_type(name);
}
} // namespace ag
