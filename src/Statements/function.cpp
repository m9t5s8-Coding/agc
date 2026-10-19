#include "Statements/function.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
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

  auto      function_stmt = AG_cscope<FunctionStatement>();
  TokenName temp_token;

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedFunctionName, function_stmt->m_function_name, nullptr, TokenName::AG_IDENTIFIER);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen, nullptr);

  AG_PARSE_OPT_OR_RETURN(parser, parse_function_parameter, function_stmt->m_parameters, nullptr);

  {
    AG_CHECK_ONE_OF_OR_RETURN(
        parser, "", &temp_token, nullptr, TokenName::AG_ARROW, TokenName::AG_LEFT_BRACE, TokenName::AG_SEMICOLON);

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

    if (temp_token == TokenName::AG_SEMICOLON) {
      parser.advance();
      function_stmt->is_declaration = true;
      return function_stmt;
    } else if (temp_token == TokenName::AG_LEFT_BRACE) {
      AG_PARSE_SCOPE(parser, function_stmt->m_function_body, nullptr);
    }
  }
  return function_stmt;
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
      return false;
    }
  }
  auto return_val = m_function_body->analyze_semantics(parser);
  get_symbol_table().exit_scope();
  return return_val;
}

void
FunctionStatement::generate_ir(CodeGenContext& context,
                               Parser&         parser) {
  m_function_body->generate_ir(context, parser);
}

} // namespace ag
