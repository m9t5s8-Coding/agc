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
#include "Statements/function.hpp"
#include "Statements/if.hpp"
#include "Statements/impl.hpp"
#include "Statements/import.hpp"
#include "Statements/let.hpp"
#include "Statements/module.hpp"
#include "Statements/return.hpp"
#include "Statements/struct.hpp"
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

  return raw;
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

  if (const auto* target = get_module_table().find_module_alias(scope, first))
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));

  if (!scope.empty()) {
    const std::string candidate = scope + "::" + raw;
    if (get_struct_table().lookup(candidate) != nullptr ||
        get_enum_table().lookup(candidate) != nullptr)
      return candidate;
  }

  if (separator == std::string::npos) {
    if (const auto* target = get_module_table().find_symbol_alias({}, raw))
      return *target;
  }

  if (const auto* target = get_module_table().find_module_alias({}, first))
    return *target + (separator == std::string::npos ? std::string{} : raw.substr(separator));

  return raw;
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

  if (!m_invalid_syntax) {
    for (const auto& ptr : m_statements) {
      if (!ptr->analyze_semantics(*this)) {
        m_invalid_syntax = true;
        break;
      }
    }
  }

  if (!m_invalid_syntax) {
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

  switch (token.token_name) {
  case TokenName::AG_LET: {
    return LetStatement::ParseLet(*this);
  }
  case TokenName::AG_VAR: {
    return VarStatement::ParseVar(*this);
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
  default:
    return ExpressionStatement::parse_expression_statement(*this);
  }
  return nullptr;
}

void
Parser::add_instructios(AG_scope<Instructions> instruction) {
  if (instruction) {
    m_instructions.emplace_back(std::move(instruction));
  }
}
} // namespace ag
