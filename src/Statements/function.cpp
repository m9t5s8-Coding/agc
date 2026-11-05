#include "Statements/function.hpp"

#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"
#include "Statements/return.hpp"
#include "IR/IR.hpp"
#include "IR/IRTable.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <cctype>
#include <optional>
#include <string_view>

namespace ag {

static std::optional<std::vector<FunctionParameter>>
parse_function_parameter(Parser& parser) {
  std::vector<FunctionParameter> l_parameters;

  if (parser.match(TokenName::AG_RIGHT_PAREN))
    return l_parameters;

  bool saw_default = false;
  while (parser.is_valid()) {
    FunctionParameter param;
    param.location = {parser.current_token().line, parser.current_token().column};

    const bool is_mut = parser.match(TokenName::AG_MUT);

    // Methods may write `self` without an explicit type.  The enclosing
    // `impl` block supplies the struct type.
    if (parser.check(TokenName::AG_IDENTIFIER) && parser.current_token().value == "self") {
      if (parser.current_impl_type().empty()) {
        print_error(ag::error::ExpectedParameterName, parser, param.location);
        return std::nullopt;
      }
      if (!l_parameters.empty()) {
        print_error(ag::error::InvalidArgumentCount, parser, param.location);
        return std::nullopt;
      }

      param.name = "self";
      param.type = make_named_type(parser.current_impl_type());
      param.type.is_const = !is_mut;
      parser.advance();
    } else {
      if (is_mut) {
        print_error(ag::error::ExpectedParameterName, parser, param.location);
        return std::nullopt;
      }

      // Parameters may be written as `name: Type` or as just `Type`.  The
      // latter is an unnamed parameter that is still part of the ABI but
      // cannot be referenced from the function body.
      if (parser.check(TokenName::AG_IDENTIFIER) &&
          parser.peek_token().token_name == TokenName::AG_COLON) {
        param.name = parser.current_token().value;
        parser.advance(); // name
        parser.advance(); // ':'
      }

      auto type = parse_type(parser);
      if (!type)
        return std::nullopt;

      param.type = std::move(*type);
    }

    if (parser.match(TokenName::AG_ASSIGN)) {
      AG_scope<Expression> default_expression;
      AG_PARSE_EXPRESSION_OR_RETURN(parser, default_expression, std::nullopt);
      param.default_value = std::shared_ptr<Expression>(std::move(default_expression));
      saw_default         = true;
    } else if (saw_default) {
      print_error(error::DefaultArgumentNotTrailing, parser, param.location);
      return std::nullopt;
    }

    l_parameters.push_back(std::move(param));
    if (!parser.match(TokenName::AG_COMMA))
      break;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, std::nullopt);
  return l_parameters;
}

static std::optional<std::vector<FunctionParameter>>
parse_parameter_group(Parser& parser) {
  if (!parser.expect(TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen))
    return std::nullopt;
  return parse_function_parameter(parser);
}

static std::optional<DataType>
parse_return_type(Parser& parser) {
  auto type = parse_type(parser);
  if (!type)
    return std::nullopt;

  // `Self` inside an impl block is the enclosing struct type.
  if (type->primitive_type == PrimitiveType::USER_DEFINED && type->type_name == "Self" &&
      !parser.current_impl_type().empty()) {
    *type = make_named_type(parser.current_impl_type());
  }

  return type;
}

static std::string
mangle_operator_symbol(const std::string_view symbol) {
  if (symbol == "+")
    return "add";
  if (symbol == "-")
    return "sub";
  if (symbol == "*")
    return "mul";
  if (symbol == "/")
    return "div";
  if (symbol == "%")
    return "mod";
  if (symbol == "<<")
    return "shl";
  if (symbol == ">>")
    return "shr";
  if (symbol == "<")
    return "lt";
  if (symbol == "<=")
    return "le";
  if (symbol == ">")
    return "gt";
  if (symbol == ">=")
    return "ge";
  if (symbol == "==")
    return "eq";
  if (symbol == "!=")
    return "ne";
  if (symbol == "&")
    return "band";
  if (symbol == "|")
    return "bor";
  if (symbol == "^")
    return "bxor";
  if (symbol == "&&")
    return "land";
  if (symbol == "||")
    return "lor";
  if (symbol == "!")
    return "lnot";
  if (symbol == "~")
    return "bnot";
  if (symbol == "++")
    return "inc";
  if (symbol == "--")
    return "dec";
  if (symbol == "[]")
    return "index";
  if (symbol == "=")
    return "assign";
  if (symbol == "+=")
    return "add_assign";
  if (symbol == "-=")
    return "sub_assign";
  if (symbol == "*=")
    return "mul_assign";
  if (symbol == "/=")
    return "div_assign";
  if (symbol == "%=")
    return "mod_assign";

  std::string mangled = "op";
  for (const char c : symbol) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
      mangled.push_back(c);
    else
      mangled += std::format("_{:02x}", static_cast<unsigned char>(c));
  }
  return mangled;
}

namespace {
std::string
mangle_label_component(std::string_view name) {
  std::string result;
  result.reserve(name.size());
  for (size_t i = 0; i < name.size(); ++i) {
    if (name[i] == ':' && i + 1 < name.size() && name[i + 1] == ':') {
      result.push_back('$');
      ++i;
    } else {
      result.push_back(name[i]);
    }
  }
  return result;
}
} // namespace

std::string
FunctionStatement::source_name() const {
  if (!m_owner_type.empty())
    return m_owner_type + "." + m_function_name;
  if (!m_module_path.empty())
    return m_module_path + "::" + m_function_name;
  return m_function_name;
}

std::string
FunctionStatement::function_label() const {
  std::string label;
  if (!m_is_operator) {
    label = generate_function_label(m_function_name, m_parameters);
  } else {
    const std::string_view symbol =
        m_function_name.size() > 8 ? std::string_view(m_function_name).substr(8) : std::string_view{};
    label = generate_function_label("__agop_" + mangle_operator_symbol(symbol), m_parameters);
  }

  // Methods live in the struct's namespace so two types may define methods
  // with the same source name and signature.  The owner type is already
  // module-qualified, so it also carries the module label prefix.
  if (!m_owner_type.empty())
    label = mangle_label_component(m_owner_type) + "$" + label;
  else if (!m_module_path.empty())
    label = mangle_label_component(m_module_path) + "$" + label;
  return label;
}

AG_scope<Statements>
FunctionStatement::ParseFunction(Parser& parser) {
  parser.advance();

  auto        function_stmt = AG_cscope<FunctionStatement>();
  function_stmt->m_is_public = parser.take_pending_public();
  TokenName   temp_token;
  const Token function_token = parser.current_token();

  function_stmt->m_owner_type  = parser.current_impl_type();
  function_stmt->m_module_path = parser.current_module_path();

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedFunctionName, function_stmt->m_function_name, nullptr, TokenName::AG_IDENTIFIER);
  function_stmt->m_location = {function_token.line, function_token.column};

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
                                TokenName::AG_SEMICOLON,
                                TokenName::AG_FAT_ARROW);
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
    } else if (temp_token == TokenName::AG_FAT_ARROW) {
      parser.advance();
      AG_scope<Expression> expression;
      AG_PARSE_EXPRESSION_OR_RETURN(parser, expression, nullptr);
      AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);
      function_stmt->m_function_body
          = ReturnStatement::Create(std::move(expression), function_stmt->m_location);
    }
  }
  return function_stmt;
}

AG_scope<Statements>
FunctionStatement::ParseOperator(Parser& parser) {
  const Token operator_token = parser.current_token();
  parser.advance(); // `operator`

  auto function_stmt           = AG_cscope<FunctionStatement>();
  function_stmt->m_location    = {operator_token.line, operator_token.column};
  function_stmt->m_is_operator = true;
  function_stmt->m_is_public   = parser.take_pending_public();
  function_stmt->m_module_path = parser.current_module_path();

  std::string                   symbol;
  std::vector<FunctionParameter> parameters;
  TokenName                     temp_token;

  if (parser.check(TokenName::AG_LEFT_PAREN)) {
    // Infix declaration form:
    //   operator (left: Type) << (right: Type) -> ReturnType { ... }
    auto left_parameters = parse_parameter_group(parser);
    if (!left_parameters)
      return nullptr;
    if (left_parameters->size() != 1) {
      print_error(error::InvalidArgumentCount, parser, function_stmt->m_location);
      return nullptr;
    }

    auto parsed_symbol = parse_operator_symbol(parser);
    if (!parsed_symbol)
      return nullptr;
    symbol = std::move(*parsed_symbol);

    auto right_parameters = parse_parameter_group(parser);
    if (!right_parameters)
      return nullptr;
    if (right_parameters->size() != 1) {
      print_error(error::InvalidArgumentCount, parser, function_stmt->m_location);
      return nullptr;
    }

    parameters.push_back(std::move((*left_parameters)[0]));
    parameters.push_back(std::move((*right_parameters)[0]));
  } else {
    // Conventional declaration form:
    //   operator+(left: Type, right: Type) -> ReturnType { ... }
    auto parsed_symbol = parse_operator_symbol(parser);
    if (!parsed_symbol)
      return nullptr;
    symbol = std::move(*parsed_symbol);

    auto parsed_parameters = parse_parameter_group(parser);
    if (!parsed_parameters)
      return nullptr;
    parameters = std::move(*parsed_parameters);
  }

  if (parameters.empty() || parameters.size() > 2) {
    print_error(error::InvalidArgumentCount, parser, function_stmt->m_location);
    return nullptr;
  }

  function_stmt->m_function_name = "operator" + symbol;
  function_stmt->m_parameters    = std::move(parameters);

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
                              TokenName::AG_SEMICOLON,
                              TokenName::AG_FAT_ARROW);
  }

  function_stmt->is_declaration = (temp_token == TokenName::AG_SEMICOLON);

  if (!function_stmt->register_function(parser))
    return nullptr;

  if (function_stmt->is_declaration) {
    parser.advance();
    return function_stmt;
  }

  if (temp_token == TokenName::AG_LEFT_BRACE) {
    AG_PARSE_SCOPE(parser, function_stmt->m_function_body, nullptr);
  } else if (temp_token == TokenName::AG_FAT_ARROW) {
    parser.advance();
    AG_scope<Expression> expression;
    AG_PARSE_EXPRESSION_OR_RETURN(parser, expression, nullptr);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);
    function_stmt->m_function_body
        = ReturnStatement::Create(std::move(expression), function_stmt->m_location);
  }

  return function_stmt;
}

bool
FunctionStatement::register_function(Parser& parser) {
  FunctionEntry entry;
  entry.name        = source_name();
  entry.label       = function_label();
  entry.module_path = m_module_path;
  entry.return_type = std::make_shared<DataType>(m_return_type);
  entry.parameters  = m_parameters;
  entry.is_defined  = !is_declaration;
  entry.is_public   = m_is_public;
  entry.location    = m_location;

  if (!get_function_table().declare(entry.label, entry)) {
    print_error(error::RedefinitionOfFunction, parser, m_location);
    return false;
  }
  return true;
}

bool
FunctionStatement::analyze_semantics(Parser& parser) {
  // Default arguments are evaluated at the call site.  Analyze them before
  // entering the parameter scope so they cannot accidentally refer to a
  // parameter that does not exist in the caller's context.
  for (const auto& param : m_parameters) {
    if (!param.default_value)
      continue;

    const auto& default_location = param.default_value->source_location(param.location);
    if (!param.default_value->analyze_semantics(parser, default_location))
      return false;
    if (!is_compatible_type(param.type, param.default_value->get_data_type())) {
      print_error(error::IncompatibleTypes, parser, default_location);
      return false;
    }
  }

  get_symbol_table().enter_scope();
  Declaration data;
  for (const auto& param : m_parameters) {
    if (param.name.empty())
      continue;

    data.type     = param.type;
    data.timing   = ValueTiming::RUNTIME;
    data.location = param.location;
    if (!get_symbol_table().insert(param.name, data)) {
      print_error(error::RedefinitionOfSymbol, parser, param.location);
      get_symbol_table().exit_scope();
      return false;
    }
  }

  const bool track_operator = m_is_operator && !is_declaration && m_function_body;
  if (track_operator)
    parser.push_operator_context(function_label());

  parser.push_function_return_type(m_return_type);
  auto return_val = !m_function_body || m_function_body->analyze_semantics(parser);
  parser.pop_function_return_type();

  if (track_operator)
    parser.pop_operator_context();

  get_symbol_table().exit_scope();
  return return_val;
}

void
FunctionStatement::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  if (is_declaration || !m_function_body)
    return;

  const std::string function_label = this->function_label();

  counter.function_index = get_ir_table().insert(source_name(), function_label);

  // Parameters are passed in registers, but the function body treats every
  // name uniformly: give each parameter a stack slot and let the prologue
  // spill the incoming register into it.
  for (size_t i = 0; i < m_parameters.size(); ++i) {
    if (m_parameters[i].name.empty())
      continue;

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
