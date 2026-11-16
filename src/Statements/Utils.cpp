#include "Statements/Utils.hpp"
#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/CondJump.hpp"
#include "IR/DataItem.hpp"
#include "IR/GlobalVariable.hpp"
#include "IR/IRTable.hpp"
#include "IR/Jump.hpp"
#include "IR/Label.hpp"
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
    if (primary->primary_type == PrimitiveType::INT ||
        primary->primary_type == PrimitiveType::CHAR ||
        primary->primary_type == PrimitiveType::BOOL)
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

namespace {
// Only literal initializers (optionally wrapped in unary +/-) can be placed
// directly in a global's data section.  Anything else needs the once-only
// initialization guard.
bool
is_constant_initializer(Expression* expression) {
  if (!expression)
    return false;

  if (auto* primary = dynamic_cast<PrimaryExpression*>(expression)) {
    return primary->primary_type == PrimitiveType::INT ||
           primary->primary_type == PrimitiveType::CHAR ||
           primary->primary_type == PrimitiveType::BOOL ||
           primary->primary_type == PrimitiveType::STRING;
  }

  if (auto* unary = dynamic_cast<UniaryExpression*>(expression)) {
    return (unary->un_operator == UN_OPERATOR::NEGATIVE ||
            unary->un_operator == UN_OPERATOR::POSITIVE) &&
           is_constant_initializer(unary->operand.get());
  }

  return false;
}
} // namespace

void
generate_static_local(LabelCounters&   counter,
                      Parser&          parser,
                      std::string_view name,
                      const DataType&  type,
                      Expression*      initializer,
                      bool             is_const) {
  if (counter.function_index < 0)
    return;

  // The source name used by `declare_global` must be unique, while the local
  // binding below maps the plain identifier to the resulting global slot.  The
  // temp counter distinguishes same-named statics in sibling blocks.
  const std::string key = "static." + std::to_string(counter.function_index) + "." +
                          std::to_string(counter.temp_label++) + "." +
                          std::string(name);

  const bool constant_initializer = is_constant_initializer(initializer);
  const std::string initial_value =
      constant_initializer ? global_initializer(initializer) : "0";

  auto global = get_ir_table().declare_global(key,
                                              get_type_size(type),
                                              is_const,
                                              initial_value);
  if (!global)
    return;

  parser.add_instructios(
      AG_cscope<GlobalVariable>(global->name, global->initializer, counter.function_index));

  auto binding = get_ir_table().declare_static_local(counter.function_index,
                                                     std::string(name),
                                                     global->name);
  if (!binding)
    return;

  if (!initializer || constant_initializer)
    return;

  // Run the initializer once, on the first execution of the declaration.
  auto guard = get_ir_table().declare_global(key + ".guard", 8, false, "0");
  if (!guard)
    return;
  parser.add_instructios(
      AG_cscope<GlobalVariable>(guard->name, guard->initializer, counter.function_index));

  const std::string skip_label = "b" + std::to_string(counter.block_label++);
  const std::string init_label = "b" + std::to_string(counter.block_label++);
  const std::string end_label  = "b" + std::to_string(counter.block_label++);

  // A non-zero guard means the value is already initialized.  The next
  // instruction is the skip label, so `CondJump` branches there when true.
  parser.add_instructios(
      AG_cscope<CondJump>(guard->name, init_label, counter.function_index));
  parser.add_instructios(AG_cscope<Label>(skip_label));
  parser.add_instructios(AG_cscope<Jump>(end_label));

  parser.add_instructios(AG_cscope<Label>(init_label));
  const auto value = type.kind == TypeKind::REFERENCE
                         ? initializer->generate_reference_ir(counter, parser)
                         : initializer->generate_ir(counter, parser);
  parser.add_instructios(AG_cscope<Assign>(value, global->name, counter.function_index));
  parser.add_instructios(AG_cscope<Assign>("1", guard->name, counter.function_index));

  parser.add_instructios(AG_cscope<Label>(end_label));
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

std::optional<std::vector<std::string>>
parse_generic_parameter_list(Parser& parser) {
  std::vector<std::string> names;
  if (!parser.check(TokenName::AG_LESS))
    return names;

  parser.advance(); // `<`

  int  angle_depth  = 1;
  int  paren_depth  = 0;
  bool at_parameter = true;

  while (!parser.is_at_end() && angle_depth > 0) {
    if (at_parameter && angle_depth == 1) {
      // `const N: int` and `type T` prefix a parameter with a keyword.  The
      // first identifier is still the parameter name in every form.
      if (parser.check(TokenName::AG_CONST) || parser.check(TokenName::AG_TYPE))
        parser.advance();
      if (parser.check(TokenName::AG_IDENTIFIER))
        names.emplace_back(parser.current_token().value);
      at_parameter = false;
    }

    switch (parser.current_token().token_name) {
    case TokenName::AG_LESS:
      ++angle_depth;
      break;
    case TokenName::AG_GREATER:
      --angle_depth;
      break;
    case TokenName::AG_RIGHT_SHIFT:
      angle_depth -= 2;
      break;
    case TokenName::AG_LEFT_PAREN:
      ++paren_depth;
      break;
    case TokenName::AG_RIGHT_PAREN:
      if (paren_depth > 0)
        --paren_depth;
      break;
    case TokenName::AG_COMMA:
      if (angle_depth == 1 && paren_depth == 0)
        at_parameter = true;
      break;
    default:
      break;
    }
    parser.advance();
  }

  if (angle_depth > 0) {
    print_error(error::UnexpectedEndOfFile, parser);
    return std::nullopt;
  }
  return names;
}

bool
skip_generic_arguments(Parser& parser) {
  if (!parser.match(TokenName::AG_LESS))
    return true;

  int angle_depth = 1;
  while (!parser.is_at_end() && angle_depth > 0) {
    switch (parser.current_token().token_name) {
    case TokenName::AG_LESS:
      ++angle_depth;
      break;
    case TokenName::AG_GREATER:
      --angle_depth;
      break;
    case TokenName::AG_RIGHT_SHIFT:
      angle_depth -= 2;
      break;
    default:
      break;
    }
    parser.advance();
  }

  if (angle_depth > 0) {
    print_error(error::UnexpectedEndOfFile, parser);
    return false;
  }
  return true;
}

bool
skip_where_clause(Parser& parser) {
  if (!(parser.check(TokenName::AG_IDENTIFIER) && parser.current_token().value == "where"))
    return true;

  parser.advance(); // `where`

  int angle_depth = 0;
  int paren_depth = 0;
  while (!parser.is_at_end()) {
    const auto token = parser.current_token().token_name;
    if (angle_depth == 0 && paren_depth == 0 &&
        (token == TokenName::AG_LEFT_BRACE ||
         token == TokenName::AG_SEMICOLON ||
         token == TokenName::AG_FAT_ARROW))
      return true;

    switch (token) {
    case TokenName::AG_LESS:
      ++angle_depth;
      break;
    case TokenName::AG_GREATER:
      if (angle_depth > 0)
        --angle_depth;
      break;
    case TokenName::AG_RIGHT_SHIFT:
      angle_depth = std::max(0, angle_depth - 2);
      break;
    case TokenName::AG_LEFT_PAREN:
      ++paren_depth;
      break;
    case TokenName::AG_RIGHT_PAREN:
      if (paren_depth > 0)
        --paren_depth;
      break;
    default:
      break;
    }
    parser.advance();
  }

  print_error(error::UnexpectedEndOfFile, parser);
  return false;
}

std::optional<DataType>
parse_type(Parser& parser) {
  // Reference types are written `&T` and `&mut T`.  The `mut` qualifier is
  // part of the type, not a separate parameter modifier, so it is accepted in
  // every type position (parameters, locals, fields, and return types).
  bool is_reference         = false;
  bool is_mutable_reference = false;
  if (parser.match(TokenName::AG_REFERENCE)) {
    is_reference         = true;
    is_mutable_reference = parser.match(TokenName::AG_MUT);
  }

  // Function types such as `func(int) -> int`.  Parameter types are skipped
  // for now; the result is an opaque callable value while first-class
  // functions are unimplemented.
  if (parser.check(TokenName::AG_FUNC)) {
    parser.advance();
    if (!parser.expect(TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen))
      return std::nullopt;

    int depth = 1;
    while (depth > 0 && !parser.is_at_end()) {
      if (parser.match(TokenName::AG_LEFT_PAREN)) {
        ++depth;
      } else if (parser.match(TokenName::AG_RIGHT_PAREN)) {
        --depth;
      } else {
        parser.advance();
      }
    }
    if (depth != 0) {
      print_error(error::UnexpectedEndOfFile, parser);
      return std::nullopt;
    }

    DataType function_type;
    function_type.kind           = TypeKind::FUNCTION;
    function_type.primitive_type = PrimitiveType::USER_DEFINED;
    function_type.size_in_bytes  = 8;
    function_type.alignment      = 8;

    if (parser.match(TokenName::AG_ARROW)) {
      auto result = parse_type(parser);
      if (!result)
        return std::nullopt;
      function_type.return_type = std::make_shared<DataType>(std::move(*result));
    }

    if (is_reference) {
      DataType reference;
      reference.kind                 = TypeKind::REFERENCE;
      reference.primitive_type       = PrimitiveType::USER_DEFINED;
      reference.pointee              = std::make_shared<DataType>(std::move(function_type));
      reference.is_mutable_reference = is_mutable_reference;
      reference.size_in_bytes        = 8;
      reference.alignment            = 8;
      return reference;
    }

    return function_type;
  }

  // Tuple types such as `(int, string)`.  Tuple values are opaque pointer-sized
  // aggregates until tuple layout and indexing are implemented.
  if (parser.match(TokenName::AG_LEFT_PAREN)) {
    DataType tuple_type;
    if (!parser.check(TokenName::AG_RIGHT_PAREN)) {
      do {
        auto element = parse_type(parser);
        if (!element)
          return std::nullopt;
        tuple_type.tuple_elements.push_back(
            std::make_shared<DataType>(std::move(*element)));
      } while (parser.match(TokenName::AG_COMMA));
    }

    if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen))
      return std::nullopt;

    tuple_type.kind           = TypeKind::STRUCT;
    tuple_type.primitive_type = PrimitiveType::USER_DEFINED;
    tuple_type.size_in_bytes  = 8;
    tuple_type.alignment      = 8;

    if (is_reference) {
      DataType reference;
      reference.kind                 = TypeKind::REFERENCE;
      reference.primitive_type       = PrimitiveType::USER_DEFINED;
      reference.pointee              = std::make_shared<DataType>(std::move(tuple_type));
      reference.is_mutable_reference = is_mutable_reference;
      reference.size_in_bytes        = 8;
      reference.alignment            = 8;
      return reference;
    }

    return tuple_type;
  }

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

  const bool is_generic_parameter = parser.is_generic_parameter(raw_name);
  const bool has_generic_arguments = parser.check(TokenName::AG_LESS);
  if (has_generic_arguments && !skip_generic_arguments(parser))
    return std::nullopt;

  DataType type;
  bool     unresolved_user_type = false;
  if (is_generic_parameter) {
    // Generic parameters are opaque, pointer-sized values until
    // monomorphization exists.  Treating `T` as a user-defined type keeps
    // signatures such as `func max<T>(a: T, b: T) -> T` parseable.
    type.kind                 = TypeKind::PRIMITIVE;
    type.primitive_type       = PrimitiveType::USER_DEFINED;
    type.type_name            = parser.intern_name(raw_name);
    type.is_generic_parameter = true;
    type.size_in_bytes        = 8;
    type.alignment            = 8;
  } else {
    const std::string resolved_name = parser.resolve_type_name(raw_name);
    type.type_name                  = parser.intern_name(resolved_name);
    type.primitive_type             = get_datatypes(type.type_name);
    if (type.primitive_type != PrimitiveType::USER_DEFINED)
      apply_builtin_type_properties(type);

    if (const auto* alias = get_type_alias_table().lookup(type.type_name)) {
      type = *alias;
    } else if (type.primitive_type == PrimitiveType::USER_DEFINED) {
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
      } else if (has_generic_arguments) {
        // `Result<int, string>` and similar library types are accepted as
        // opaque values while generic instantiations are unimplemented.
        type.kind          = TypeKind::STRUCT;
        type.size_in_bytes = 8;
        type.alignment     = 8;
      } else {
        // Pointers to incomplete user-defined types are useful for opaque
        // handles (for example the `ostream*` stream type in the examples).
        // A non-pointer declaration, however, must name a real type.
        unresolved_user_type = true;
      }
    }

    if (unresolved_user_type) {
      // Pointers/references to incomplete types are resolved below; keep the
      // flag alive until the suffix operators have been parsed.
      type.size_in_bytes = 8;
      type.alignment     = 8;
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

  // Array suffixes (`T[]`, `int[5]`, `T[N]`) are accepted as pointer-sized
  // storage for now.  Actual array values and slices are not implemented yet.
  while (parser.match(TokenName::AG_LEFT_BRACKET)) {
    std::optional<size_t> array_length;
    if (!parser.check(TokenName::AG_RIGHT_BRACKET)) {
      if (parser.check(TokenName::AG_INT_LITERAL)) {
        try {
          array_length = static_cast<size_t>(std::stoull(std::string(parser.current_token().value)));
        } catch (...) {
          array_length = std::nullopt;
        }
        parser.advance();
      } else {
        // Constant generic lengths such as `T[N]` are not evaluated yet.
        int bracket_depth = 1;
        while (!parser.is_at_end() && bracket_depth > 0) {
          if (parser.check(TokenName::AG_LEFT_BRACKET))
            ++bracket_depth;
          else if (parser.check(TokenName::AG_RIGHT_BRACKET))
            --bracket_depth;
          if (bracket_depth > 0)
            parser.advance();
        }
      }
    }

    if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket))
      return std::nullopt;

    auto element = std::make_shared<DataType>(type);

    DataType array;
    // Arrays currently use the same address representation as pointers so
    // existing indexing and pointer arithmetic work on them.  `array_length`
    // records the declared size for later slice/array support.
    array.kind           = TypeKind::POINTER;
    array.primitive_type = PrimitiveType::USER_DEFINED;
    array.pointee        = std::move(element);
    array.array_length   = array_length;
    array.size_in_bytes  = 8;
    array.alignment      = 8;
    type                 = std::move(array);
  }

  // Optional types (`T?`) are accepted syntactically; the runtime value keeps
  // its underlying representation while optional handling is unimplemented.
  // An optional of an incomplete type is pointer-shaped, so it is allowed even
  // without a definition (for example `User?` returned by an opaque API).
  const bool has_optional = parser.match(TokenName::AG_QUESTION);

  if (is_reference) {
    if (type.kind == TypeKind::PRIMITIVE && type.primitive_type == PrimitiveType::VOID) {
      print_error(error::InvalidOperandTypes, parser, type_location);
      return std::nullopt;
    }

    auto pointee = std::make_shared<DataType>(type);

    DataType reference;
    reference.kind                 = TypeKind::REFERENCE;
    reference.primitive_type       = PrimitiveType::USER_DEFINED;
    reference.pointee              = std::move(pointee);
    reference.is_mutable_reference = is_mutable_reference;
    type                           = std::move(reference);
  }

  if (unresolved_user_type && !has_pointer && !is_reference && !has_optional) {
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

  // Copy in 8-byte chunks and handle the trailing remainder byte-by-byte.
  context.syscall << "__ag_memcpy:\n"
                     "\tmov rax, rdi\n"
                     "\tmov rcx, rdx\n"
                     "\tshr rcx, 3\n"
                     "\trep movsq\n"
                     "\tmov rcx, rdx\n"
                     "\tand rcx, 7\n"
                     "\trep movsb\n"
                     "\tret\n\n";

  // Find the terminator first, then reuse the 8-byte copy path.
  context.syscall << "__ag_strcpy:\n"
                     "\tmov rax, rdi\n"
                     "\tmov rdx, rsi\n"
                     ".strcpy_find_end:\n"
                     "\tcmp byte [rdx], 0\n"
                     "\tje .strcpy_copy\n"
                     "\tinc rdx\n"
                     "\tjmp .strcpy_find_end\n"
                     ".strcpy_copy:\n"
                     "\tsub rdx, rsi\n"
                     "\tinc rdx\n"
                     "\tmov rcx, rdx\n"
                     "\tshr rcx, 3\n"
                     "\trep movsq\n"
                     "\tmov rcx, rdx\n"
                     "\tand rcx, 7\n"
                     "\trep movsb\n"
                     "\tret\n\n";

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

TypeAliasTable&
get_type_alias_table() {
  static TypeAliasTable table;
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
