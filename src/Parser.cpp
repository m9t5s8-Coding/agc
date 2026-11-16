#include "Parser.hpp"

#include "IR/IRTable.hpp"
#include "LLVM/LLVMBackend.hpp"
#include "IR/Instructions.hpp"
#include "Expressions/Expression.hpp"
#include "Statements/ExpressionStatement.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Statements/body.hpp"
#include "Statements/enum.hpp"
#include "Statements/break.hpp"
#include "Statements/continue.hpp"
#include "Statements/defer.hpp"
#include "Statements/for.hpp"
#include "Statements/function.hpp"
#include "Statements/guard.hpp"
#include "Statements/loop.hpp"
#include "Statements/with.hpp"
#include "Statements/if.hpp"
#include "Statements/impl.hpp"
#include "Statements/import.hpp"
#include "Statements/let.hpp"
#include "Statements/module.hpp"
#include "Statements/return.hpp"
#include "Statements/struct.hpp"
#include "Statements/test.hpp"
#include "Statements/type_alias.hpp"
#include "Statements/var.hpp"
#include "Statements/while.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "json.hpp"

#include <fstream>
#include <utility>

namespace ag {
namespace {
bool
looks_like_operator_declaration(const Parser& parser) {
  size_t offset = 1;
  if (!parser.is_valid(offset))
    return false;

  const auto first = parser.peek_token(offset).token_name;
  if (first == TokenName::AG_LEFT_PAREN)
    return true;

  if (first == TokenName::AG_LEFT_BRACKET) {
    if (!parser.is_valid(offset + 1) ||
        parser.peek_token(offset + 1).token_name != TokenName::AG_RIGHT_BRACKET)
      return false;
    offset += 2;
  } else {
    ++offset;
  }

  if (!parser.is_valid(offset) ||
      parser.peek_token(offset).token_name != TokenName::AG_LEFT_PAREN)
    return false;
  ++offset;

  if (!parser.is_valid(offset))
    return false;

  // Zero-parameter operators are invalid, but treating them as declarations
  // gives a useful arity diagnostic instead of parsing them as member access.
  if (parser.peek_token(offset).token_name == TokenName::AG_RIGHT_PAREN)
    return true;

  if (parser.peek_token(offset).token_name != TokenName::AG_IDENTIFIER)
    return false;
  ++offset;

  return parser.is_valid(offset) &&
         parser.peek_token(offset).token_name == TokenName::AG_COLON;
}

// File modules such as `ag/io.ag` are loaded with `mod ag::io;`, but their
// declarations belong to the parent namespace (`ag`).  Rewrite the longest
// registered module-part prefix so `ag::io::println` and `io::println` both
// find `ag::println`.
std::string
rewrite_module_part_prefix(const std::string& path) {
  size_t position = path.rfind("::");
  while (position != std::string::npos) {
    const std::string prefix = path.substr(0, position);
    if (const auto* part = get_module_table().find_module_part(prefix))
      return *part + path.substr(position);
    if (position == 0)
      break;
    position = path.rfind("::", position - 1);
  }
  return path;
}
} // namespace

std::string
Parser::resolve_symbol_name(std::string_view name) const {
  const std::string raw(name);
  const auto        separator = raw.find("::");
  const std::string first     = raw.substr(0, separator);
  const std::string& scope    = current_module_path();

  const auto find_symbol_alias = [&](std::string_view search_scope) -> const std::string* {
    return get_module_table().find_symbol_alias(search_scope, raw);
  };
  const auto find_module_alias = [&](std::string_view search_scope) -> const std::string* {
    return get_module_table().find_module_alias(search_scope, first);
  };

  if (separator == std::string::npos) {
    if (const auto* target = find_symbol_alias(scope))
      return *target;
  }

  if (const auto* target = find_module_alias(scope))
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));

  // Names that are already qualified are first tried relative to the active
  // module, which makes `inner::helper()` work inside `mod outer { ... }`.
  if (!scope.empty()) {
    const std::string candidate = scope + "::" + raw;
    if (!get_function_table().lookup_all(candidate).empty() ||
        get_struct_table().lookup(candidate) != nullptr ||
        get_enum_table().lookup(candidate) != nullptr)
      return candidate;
  }

  if (separator == std::string::npos) {
    if (const auto* target = find_symbol_alias({}))
      return *target;
  }

  if (const auto* target = find_module_alias({}))
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));

  return rewrite_module_part_prefix(raw);
}

std::string
Parser::resolve_type_name(std::string_view name) const {
  const std::string raw(name);
  const auto        separator = raw.find("::");
  const std::string first     = raw.substr(0, separator);
  const std::string& scope    = current_module_path();

  if (separator == std::string::npos) {
    if (const auto* target = get_module_table().find_symbol_alias(scope, raw))
      return *target;
  }

  if (const auto* target = get_module_table().find_module_alias(scope, first)) {
    // `import ag::String;` aliases the file part's spelling.  For a bare
    // type name (`String`) the actual declaration is `ag::String`, not the
    // owning namespace `ag`.
    if (separator == std::string::npos) {
      const std::string candidate = *target + "::" + raw;
      if (get_struct_table().lookup(candidate) != nullptr ||
          get_enum_table().lookup(candidate) != nullptr ||
          get_type_alias_table().lookup(candidate) != nullptr)
        return candidate;
    }
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));
  }

  if (!scope.empty()) {
    const std::string candidate = scope + "::" + raw;
    if (get_struct_table().lookup(candidate) != nullptr ||
        get_enum_table().lookup(candidate) != nullptr ||
        get_type_alias_table().lookup(candidate) != nullptr)
      return candidate;
  }

  if (separator == std::string::npos) {
    if (const auto* target = get_module_table().find_symbol_alias({}, raw))
      return *target;
  }

  if (const auto* target = get_module_table().find_module_alias({}, first))
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));

  return rewrite_module_part_prefix(raw);
}

void
Parser::parse_module_tokens(Lexer&                             lexer,
                            std::string                        module_path,
                            std::vector<AG_scope<Statements>>& statements) {
  Lexer*       saved_lexer  = m_lexer;
  TokensVector saved_tokens = std::move(m_tokens);
  const size_t saved_index  = m_index;
  const Token  saved_eot    = m_EOToken;

  set_lexer(lexer);
  m_index = 0;
  push_module_path(std::move(module_path));

  while (!is_at_end() && !m_invalid_syntax) {
    const size_t position_before_statement = m_index;
    auto         statement                 = parse_statement();
    if (!statement) {
      m_invalid_syntax = true;
      if (m_error_message.empty())
        print_error(is_at_end() ? error::UnexpectedEndOfFile : error::UnexpectedToken, *this);
      break;
    }

    statements.emplace_back(std::move(statement));

    if (m_index == position_before_statement) {
      m_invalid_syntax = true;
      if (m_error_message.empty())
        print_error(is_at_end() ? error::UnexpectedEndOfFile : error::UnexpectedToken, *this);
      break;
    }
  }

  pop_module_path();

  m_lexer   = saved_lexer;
  m_tokens  = std::move(saved_tokens);
  m_index   = saved_index;
  m_EOToken = saved_eot;
}

void
Parser::parse() {
  LabelCounters counter;

  m_ir_text.clear();
  m_assembly_text.clear();
  m_llvm_text.clear();

  if (!m_invalid_syntax) {
    while (!is_at_end()) {
      const size_t position_before_statement = m_index;
      auto         statement                 = parse_statement();

      if (!add_statements(std::move(statement))) {
        m_invalid_syntax = true;
        if (m_error_message.empty()) {
          print_error(is_at_end() ? error::UnexpectedEndOfFile : error::UnexpectedToken, *this);
        }
        break;
      }

      // A successful parse must consume at least one token.  This guards
      // against an accidental infinite loop at EOF or on an empty statement.
      if (m_index == position_before_statement) {
        m_invalid_syntax = true;
        if (m_error_message.empty()) {
          print_error(is_at_end() ? error::UnexpectedEndOfFile : error::UnexpectedToken, *this);
        }
        break;
      }
    }
  }

  if (!m_invalid_syntax && !m_parse_only) {
    for (const auto& ptr : m_statements) {
      if (!ptr->analyze_semantics(*this)) {
        m_invalid_syntax = true;
        break;
      }
    }
  }

  if (!m_invalid_syntax && !m_parse_only) {
    Add_Literals::add_to_instruction(counter, *this);
    for (const auto& ptr : m_statements) {
      ptr->generate_ir(counter, *this);
    }

    // The generated entry point always calls main, so a translation unit
    // without it cannot produce a runnable program.  Report this at EOF
    // instead of letting the linker fail later with an unresolved symbol.
    if (get_ir_table().find_function("main") == nullptr) {
      m_invalid_syntax = true;
      print_error(error::MissingMainFunction, *this);
    }
  }

  if (!m_invalid_syntax) {
    // Keep the textual IR around so the driver can write it with -ir.
    for (const auto& instr : m_instructions) {
      m_ir_text += instr->to_string();
    }

    // LLVM is the executable backend.  It translates the same linear IR the
    // native backend used to consume, so structs, members, and methods keep
    // their existing lowering.
    if (m_emit_llvm)
      m_llvm_text = generate_llvm_ir(*this);

    // Native NASM generation is kept only for the JSON debug output.  It is
    // not part of executable builds anymore.
    if (m_emit_native_assembly) {
      CodeGenContext context;
      for (const auto& instr : m_instructions) {
        instr->generate_code(context);
      }

      add_system_call_function(context);

      std::stringstream ss;
      ss << "section .data\n"
         << context.data.str() << "\n\n"
         << "section .text\n"
         << "global _start\n\n"
         << context.syscall.str() << context.text.str();
      m_assembly_text = ss.str();
    }
  }

  // Backwards-compatible `agc file.ag -o json <file>` mode.
  if (!m_output_file.empty() && m_output_format != OutputFormat::NONE) {
    std::ofstream outfile(m_output_file);
    if (!outfile.is_open()) {
      return;
    }

    if (m_output_format == OutputFormat::JSON)
      outfile << to_json();
    else if (m_output_format == OutputFormat::TEXT)
      outfile << m_assembly_text;

    outfile.close();
  }
}

std::string
Parser::to_json() const {
  nlohmann::json j;
  j["error"]    = m_error_message;
  j["status"]   = !m_invalid_syntax;
  j["ir"]       = m_ir_text;
  j["assembly"] = m_assembly_text;
  // Preserve the old JSON field name used by existing clients.
  j["output"] = m_assembly_text;
  return j.dump(4);
}

AG_scope<Statements>
Parser::parse_statement() {
  const auto& token = current_token();

  // `outer: for ...` and the other loop forms attach a label to the loop so
  // `break outer`/`continue outer` can target it.
  if (token.token_name == TokenName::AG_IDENTIFIER &&
      peek_token().token_name == TokenName::AG_COLON) {
    const auto next = peek_token(2).token_name;
    if (next == TokenName::AG_FOR || next == TokenName::AG_WHILE ||
        next == TokenName::AG_UNTIL || next == TokenName::AG_LOOP) {
      const std::string label(token.value);
      advance(); // label
      advance(); // ':'
      push_loop_label_name(label);
      auto statement = parse_statement();
      pop_loop_label_name();
      if (statement)
        statement->set_loop_label(label);
      return statement;
    }
  }

  // `spawn { ... }` starts a concurrent task.  With no scheduler yet the block
  // runs synchronously as a normal block.
  if (token.token_name == TokenName::AG_IDENTIFIER && token.value == "spawn" &&
      peek_token().token_name == TokenName::AG_LEFT_BRACE) {
    advance(); // `spawn`
    return BodyStatement::ParseBlock(*this);
  }

  switch (token.token_name) {
  case TokenName::AG_LET: {
    return LetStatement::ParseLet(*this);
  }
  case TokenName::AG_VAR: {
    return VarStatement::ParseVar(*this);
  }
  case TokenName::AG_CONST: {
    return LetStatement::ParseConst(*this);
  }
  case TokenName::AG_FUNC: {
    return FunctionStatement::ParseFunction(*this);
  }
  case TokenName::AG_STRUCT: {
    return StructStatement::ParseStruct(*this);
  }
  case TokenName::AG_ENUM: {
    return EnumStatement::ParseEnum(*this);
  }
  case TokenName::AG_IMPL: {
    return ImplStatement::ParseImpl(*this);
  }
  case TokenName::AG_TYPE: {
    return TypeAliasStatement::ParseType(*this);
  }
  case TokenName::AG_EXTERN: {
    // `extern "c" { ... }` groups declarations that come from another
    // language.  The ABI string is informational; the declarations inside are
    // ordinary bodyless functions that keep their exact C symbol names.
    advance();
    if (check(TokenName::AG_STRING_LITERAL))
      advance();
    if (!check(TokenName::AG_LEFT_BRACE)) {
      print_error(error::ExpectedLeftBrace, *this);
      return nullptr;
    }
    const bool previous_extern = in_extern_block();
    set_in_extern_block(true);
    auto block = BodyStatement::ParseBlock(*this);
    set_in_extern_block(previous_extern);
    return block;
  }
  case TokenName::AG_AT: {
    // Attributes such as `@inline` and `@deprecated("...")` are accepted and
    // ignored for now; the decorated declaration is parsed normally.
    advance(); // `@`
    if (!check_one_of({TokenName::AG_IDENTIFIER, TokenName::AG_KEYWORD})) {
      print_error(is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedIdentifier,
                  *this);
      return nullptr;
    }
    advance(); // attribute name

    if (match(TokenName::AG_LEFT_PAREN)) {
      int depth = 1;
      while (depth > 0 && !is_at_end()) {
        if (check(TokenName::AG_LEFT_PAREN))
          ++depth;
        else if (check(TokenName::AG_RIGHT_PAREN))
          --depth;
        advance();
      }
      if (depth != 0) {
        print_error(error::UnexpectedEndOfFile, *this);
        return nullptr;
      }
    }

    return parse_statement();
  }
  case TokenName::AG_COMPTIME: {
    // Compile-time functions are ordinary functions for now; the marker is
    // accepted and the declaration is parsed normally.
    advance();
    return parse_statement();
  }
  case TokenName::AG_TRAIT: {
    // Traits are not dispatched yet.  Consume the declaration so files using
    // `trait Name { ... }` keep parsing; `impl Trait for Type` is handled by
    // the normal impl parser.
    advance();
    take_pending_public();
    if (!check(TokenName::AG_IDENTIFIER)) {
      print_error(is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedInterfaceName,
                  *this);
      return nullptr;
    }
    advance();

    if (match(TokenName::AG_LESS)) {
      int depth = 1;
      while (depth > 0 && !is_at_end()) {
        if (check(TokenName::AG_LESS))
          ++depth;
        else if (check(TokenName::AG_GREATER))
          --depth;
        advance();
      }
      if (depth != 0) {
        print_error(error::UnexpectedEndOfFile, *this);
        return nullptr;
      }
    }

    if (!check(TokenName::AG_LEFT_BRACE)) {
      print_error(error::ExpectedLeftBrace, *this);
      return nullptr;
    }

    int brace_depth = 0;
    do {
      if (is_at_end()) {
        print_error(error::UnexpectedEndOfFile, *this);
        return nullptr;
      }
      if (check(TokenName::AG_LEFT_BRACE))
        ++brace_depth;
      else if (check(TokenName::AG_RIGHT_BRACE))
        --brace_depth;
      advance();
    } while (brace_depth > 0);

    auto empty = AG_cscope<BodyStatement>();
    return empty;
  }
  case TokenName::AG_IMPORT: {
    return ImportStatement::ParseImport(*this);
  }
  case TokenName::AG_MOD: {
    return ModuleStatement::ParseModule(*this);
  }
  case TokenName::AG_PUB: {
    advance();
    const bool previous_public = m_pending_public;
    set_pending_public(true);
    auto statement = parse_statement();
    set_pending_public(previous_public);
    return statement;
  }
  case TokenName::AG_PRIV: {
    advance();
    const bool previous_public = m_pending_public;
    set_pending_public(false);
    auto statement = parse_statement();
    set_pending_public(previous_public);
    return statement;
  }
  case TokenName::AG_STATIC: {
    advance();
    const bool previous_static = m_pending_static;
    set_pending_static(true);
    auto statement = parse_statement();
    set_pending_static(previous_static);
    return statement;
  }
  case TokenName::AG_OPERATOR: {
    if (looks_like_operator_declaration(*this))
      return FunctionStatement::ParseOperator(*this);
    return ExpressionStatement::parse_expression_statement(*this);
  }
  case TokenName::AG_LEFT_BRACE: {
    return BodyStatement::ParseBlock(*this);
  }
  case TokenName::AG_RETURN: {
    return ReturnStatement::ParseReturn(*this);
  }
  case TokenName::AG_IF: {
    return IfStatement::ParseIf(*this);
  }
  case TokenName::AG_WHILE: {
    return WhileStatement::ParseWhile(*this);
  }
  case TokenName::AG_UNTIL: {
    return WhileStatement::ParseUntil(*this);
  }
  case TokenName::AG_GUARD: {
    return GuardStatement::ParseGuard(*this);
  }
  case TokenName::AG_FOR: {
    return ForStatement::ParseFor(*this);
  }
  case TokenName::AG_LOOP: {
    return LoopStatement::ParseLoop(*this);
  }
  case TokenName::AG_BREAK: {
    return BreakStatement::ParseBreak(*this);
  }
  case TokenName::AG_CONTINUE: {
    return ContinueStatement::ParseContinue(*this);
  }
  case TokenName::AG_WITH: {
    return WithStatement::ParseWith(*this);
  }
  case TokenName::AG_DEFER: {
    return DeferStatement::ParseDefer(*this);
  }
  case TokenName::AG_ASYNC: {
    // `async func ...` is parsed as an ordinary function; asynchronous
    // execution is not implemented yet.
    advance();
    return parse_statement();
  }
  case TokenName::AG_UNSAFE: {
    // `unsafe { ... }` is a normal block; the type system already permits
    // raw pointer operations where they are used.
    advance();
    if (!check(TokenName::AG_LEFT_BRACE)) {
      print_error(is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedLeftBrace,
                  *this);
      return nullptr;
    }
    return BodyStatement::ParseBlock(*this);
  }
  case TokenName::AG_TEST: {
    return TestStatement::ParseTest(*this);
  }
  default:
    return ExpressionStatement::parse_expression_statement(*this);
  }
  return nullptr;
}

AG_scope<Expression>
Parser::parse_inline_expression(std::string_view      source,
                                const SourceLocation& location) {
  m_embedded_sources.emplace_back(source);
  const std::string& stored = m_embedded_sources.back();

  Lexer nested;
  nested.set_filename(get_filename());
  nested.set_source_code(stored);
  nested.tokenize();

  TokensVector tokens = nested.get_tokens();
  Token        eof;
  eof.token_name = TokenName::AG_EOF;
  eof.value      = {};
  eof.line       = location.line;
  eof.column     = location.column;
  tokens.push_back(eof);

  // Keep the lexer (and the decoded literal storage it owns) alive for the
  // lifetime of the parser, then parse the fragment through the normal Pratt
  // parser by temporarily swapping the active token stream.
  m_embedded_lexers.push_back(std::move(nested));

  TokensVector saved_tokens = std::move(m_tokens);
  const size_t saved_index  = m_index;
  const Token  saved_eof    = m_EOToken;

  m_tokens   = std::move(tokens);
  m_index    = 0;
  m_EOToken  = eof;

  AG_scope<Expression> expression = Expression::parse_pratt(*this, 0.0f);

  m_tokens  = std::move(saved_tokens);
  m_index   = saved_index;
  m_EOToken = saved_eof;
  return expression;
}

void
Parser::add_instructios(AG_scope<Instructions> instruction) {
  if (instruction) {
    m_instructions.emplace_back(std::move(instruction));
  }
}
} // namespace ag
