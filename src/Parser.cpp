#include "Parser.hpp"

#include "IR/IRTable.hpp"
#include "IR/Instructions.hpp"
#include "Statements/ExpressionStatement.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Statements/body.hpp"
#include "Statements/exit.hpp"
#include "Statements/function.hpp"
#include "Statements/if.hpp"
#include "Statements/let.hpp"
#include "Statements/read.hpp"
#include "Statements/return.hpp"
#include "Statements/var.hpp"
#include "Statements/while.hpp"
#include "Statements/write.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "json.hpp"

#include <fstream>
#include <utility>

namespace ag {
void
Parser::parse() {
  LabelCounters  counter;
  CodeGenContext context;

  m_ir_text.clear();
  m_assembly_text.clear();

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

    // The generated `_start` always calls main, so a translation unit without
    // it cannot produce a runnable program.  Report this at EOF instead of
    // letting NASM/ld fail later with an unresolved symbol.
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

    for (const auto& instr : m_instructions) {
      instr->generate_code(context);
    }
  }

  add_system_call_function(context);

  std::stringstream ss;
  {
    ss << "section .data\n"
       << context.data.str() << "\n\n"
       << "section .text\n"
       << "global _start\n\n"
       << context.syscall.str() << context.text.str();
  }
  m_assembly_text = ss.str();

  // Backwards-compatible `agc file.ag -o text|json <file>` mode.
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
  case ag::TokenName::AG_EXIT: {
    return ExitStatement::ParseExit(*this);
  }
  case TokenName::AG_WRITE: {
    return WriteStatement::ParseWrite(*this);
  }
  case TokenName::AG_READ: {
    return ReadStatement::ParseRead(*this);
  }
  case TokenName::AG_LET: {
    return LetStatement::ParseLet(*this);
  }
  case TokenName::AG_VAR: {
    return VarStatement::ParseVar(*this);
  }
  case TokenName::AG_FUNC: {
    return FunctionStatement::ParseFunction(*this);
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
