#include "Statements/function.hpp"

#include "Statements/Statements.hpp"
#include "IR/IR.hpp"
#include "IR/IRTable.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <optional>

namespace ag {

static std::optional<std::vector<FunctionParameter>>
parse_function_parameter(Parser& parser) {
  std::vector<FunctionParameter> l_parameters;

  if (parser.match(TokenName::AG_RIGHT_PAREN))
    return l_parameters;

  FunctionParameter param;
  while (parser.is_valid()) {

    param.location.line = parser.current_token().line;
    AG_EXPECT_GET_ID_OR_RETURN(
        parser, ag::error::ExpectedFunctionName, param.name, std::nullopt, TokenName::AG_IDENTIFIER);

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, ag::error::ExpectedColon, std::nullopt);

    AG_EXPECT_GET_VALUE_OR_RETURN(
        parser, ag::error::ExpectedKeyword, param.type, std::nullopt, TokenName::AG_KEYWORD, TokenName::AG_IDENTIFIER);

    l_parameters.push_back(param);
    if (!parser.match(TokenName::AG_COMMA))
      break;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, std::nullopt);
  return l_parameters;
}

static std::optional<DataType>
parse_return_type(Parser& parser) {
  DataType out_value;
  AG_EXPECT_GET_VALUE_OR_RETURN(
      parser, ag::error::ExpectedKeyword, out_value, std::nullopt, TokenName::AG_KEYWORD, TokenName::AG_IDENTIFIER);
  return out_value;
}

AG_scope<Statements>
FunctionStatement::ParseFunction(Parser& parser) {
  parser.advance();

  auto         function_stmt = AG_cscope<FunctionStatement>();
  TokenName    temp_token;
  const auto   function_line = parser.current_token().line;

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedFunctionName, function_stmt->m_function_name, nullptr, TokenName::AG_IDENTIFIER);
  function_stmt->m_location.line = function_line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen, nullptr);

  AG_PARSE_OPT_OR_RETURN(parser, parse_function_parameter, function_stmt->m_parameters, nullptr);

  {
    AG_CHECK_ONE_OF_OR_RETURN(parser,
                              ag::error::ExpectedAfterParameters,
                              &temp_token,
                              nullptr,
                              TokenName::AG_ARROW,
                              TokenName::AG_LEFT_BRACE,
                              TokenName::AG_SEMICOLON);

    if (temp_token == TokenName::AG_ARROW) {
      parser.advance();
      AG_PARSE_OPT_OR_RETURN(parser, parse_return_type, function_stmt->m_return_type, nullptr);

      AG_CHECK_ONE_OF_OR_RETURN(parser,
                                ag::error::ExpectedAfterReturnType,
                                &temp_token,
                                nullptr,
                                TokenName::AG_LEFT_BRACE,
                                TokenName::AG_SEMICOLON);
    }

    function_stmt->is_declaration = (temp_token == TokenName::AG_SEMICOLON);

    // Register the signature before the body is parsed.  All top-level
    // statements are parsed before semantic analysis, so calls can refer to
    // functions that are defined later in the file.
    if (!function_stmt->register_function(parser))
      return nullptr;

    if (function_stmt->is_declaration) {
      parser.advance();
      return function_stmt;
    } else if (temp_token == TokenName::AG_LEFT_BRACE) {
      AG_PARSE_SCOPE(parser, function_stmt->m_function_body, nullptr);
    }
  }
  return function_stmt;
}

bool
FunctionStatement::register_function(Parser& parser) {
  FunctionEntry entry;
  entry.name        = m_function_name;
  entry.label       = generate_function_label(m_function_name, m_parameters);
  entry.return_type = std::make_shared<DataType>(m_return_type);
  entry.parameters  = m_parameters;
  entry.is_defined  = !is_declaration;
  entry.location    = m_location;

  if (!get_function_table().declare(entry.label, entry)) {
    print_error(error::RedefinitionOfFunction, parser, m_location);
    return false;
  }
  return true;
}

bool
FunctionStatement::analyze_semantics(Parser& parser) {
  get_symbol_table().enter_scope();
  Declaration data;
  for (const auto& param : m_parameters) {
    data.type     = param.type;
    data.timing   = ValueTiming::RUNTIME;
    data.location = param.location;
    if (!get_symbol_table().insert(param.name, data)) {
      print_error(error::RedefinitionOfSymbol, parser, param.location);
      get_symbol_table().exit_scope();
      return false;
    }
  }

  parser.push_function_return_type(m_return_type);
  auto return_val = !m_function_body || m_function_body->analyze_semantics(parser);
  parser.pop_function_return_type();

  get_symbol_table().exit_scope();
  return return_val;
}

void
FunctionStatement::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  if (is_declaration || !m_function_body)
    return;

  std::string function_label = generate_function_label(m_function_name, m_parameters);

  counter.function_index = get_ir_table().insert(m_function_name, function_label);

  // Parameters are passed in registers, but the function body treats every
  // name uniformly: give each parameter a stack slot and let the prologue
  // spill the incoming register into it.
  for (size_t i = 0; i < m_parameters.size(); ++i) {
    get_ir_table().declare_user(counter.function_index,
                                m_parameters[i].name,
                                get_type_size(m_parameters[i].type),
                                true,
                                static_cast<uint32_t>(i));
  }

  parser.add_instructios(AG_cscope<FunctionLabel>(function_label, counter.function_index));
  m_function_body->generate_ir(counter, parser);
  counter.function_index = -1;
}

} // namespace ag
