#include "Statements/function.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Statements/body.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <optional>

namespace ag {

static std::optional<std::vector<FunctionParam>>
parse_function_parameter(Parser& parser) {
  std::vector<FunctionParam> l_parameters;

  if (parser.match(TokenName::AG_RIGHT_PAREN))
    return l_parameters;

  FunctionParam param;
  while (parser.is_valid()) {

    AG_EXPECT_GET_ID_OR_RETURN(
        parser, ag::error::ExpectedFunctionName, param.variable_name, std::nullopt, TokenName::AG_IDENTIFIER);

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, ag::error::ExpectedColon, std::nullopt);

    AG_EXPECT_GET_VALUE_OR_RETURN(parser,
                                  ag::error::ExpectedKeyword,
                                  param.data_type,
                                  std::nullopt,
                                  TokenName::AG_KEYWORD,
                                  TokenName::AG_IDENTIFIER);

    l_parameters.push_back(param);
    if (!parser.match(TokenName::AG_COMMA))
      break;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, std::nullopt);
  return l_parameters;
}

static std::optional<DataType_Data>
parse_return_type(Parser& parser) {
  DataType_Data out_value;
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

void
FunctionStatement::generate(CodeGenContext& context) {}
} // namespace ag
