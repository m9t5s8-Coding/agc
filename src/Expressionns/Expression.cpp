#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/Binary.hpp"
#include "IR/DataItem.hpp"
#include "IR/FunctionCall.hpp"
#include "IR/IRTable.hpp"
#include "IR/IndexAssign.hpp"
#include "IR/Param.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ag {
static UN_OPERATOR
get_prefix_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_INCREMENT:
    return UN_OPERATOR::PRE_INCREMENT;
  case TokenName::AG_DECREMENT:
    return UN_OPERATOR::PRE_DECREMENT;
  case TokenName::AG_BITWISE_NOT:
    return UN_OPERATOR::BITWISE_NOT;
  case TokenName::AG_LOGICAL_NOT:
    return UN_OPERATOR::LOGICAl_NOT;
  case TokenName::AG_REFERENCE:
    return UN_OPERATOR::REFRENCE;
  case TokenName::AG_ASTERISK:
    return UN_OPERATOR::DEREFRENCE;
  case TokenName::AG_PLUS:
    return UN_OPERATOR::POSITIVE;
  case TokenName::AG_MINUS:
    return UN_OPERATOR::NEGATIVE;
  default:
    return UN_OPERATOR::NONE;
  }

  return UN_OPERATOR::NONE;
}

static UN_OPERATOR
get_postfix_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_INCREMENT:
    return UN_OPERATOR::POST_INCREMENT;
  case TokenName::AG_DECREMENT:
    return UN_OPERATOR::POST_DECREMENT;
  default:
    return UN_OPERATOR::NONE;
  }
  return UN_OPERATOR::NONE;
}

static BIN_OPERATOR
get_binary_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_ASSIGN:
    return BIN_OPERATOR::ASSIGN_TO;
  case TokenName::AG_LOGICAL_OR:
    return BIN_OPERATOR::LOGICAL_OR;
  case TokenName::AG_LOGICAL_AND:
    return BIN_OPERATOR::LOGICAL_AND;
  case TokenName::AG_BITWISE_OR:
    return BIN_OPERATOR::BITWISE_OR;
  case TokenName::AG_BITWISE_XOR:
    return BIN_OPERATOR::BITWISE_XOR;
  case TokenName::AG_REFERENCE:
    return BIN_OPERATOR::BITWISE_AND;
  case TokenName::AG_EQUAL_TO:
    return BIN_OPERATOR::EQUAL_TO;
  case TokenName::AG_NOT_EQUAL_TO:
    return BIN_OPERATOR::NOT_EQUAL_TO;
  case TokenName::AG_LESS:
    return BIN_OPERATOR::LESS_THAN;
  case TokenName::AG_GREATER:
    return BIN_OPERATOR::GREATER_THAN;
  case TokenName::AG_LESS_EQUAL:
    return BIN_OPERATOR::LESS_THAN_EQUAL;
  case TokenName::AG_GREATER_EQUAL:
    return BIN_OPERATOR::GREATER_THAN_EQUAL;
  case TokenName::AG_LEFT_SHIFT:
    return BIN_OPERATOR::LEFT_SHIFT;
  case TokenName::AG_RIGHT_SHIFT:
    return BIN_OPERATOR::RIGHT_SHIFT;
  case TokenName::AG_PLUS:
    return BIN_OPERATOR::ADDITION;
  case TokenName::AG_MINUS:
    return BIN_OPERATOR::SUBTRACTION;
  case TokenName::AG_ASTERISK:
    return BIN_OPERATOR::MULTIPLICATION;
  case TokenName::AG_SLASH:
    return BIN_OPERATOR::DIVISION;
  case TokenName::AG_MODULO:
    return BIN_OPERATOR::MODULUS;
  default:
    return BIN_OPERATOR::NONE;
  }
}

static std::string_view
get_unary_operator_symbol(UN_OPERATOR op) {
  switch (op) {
  case UN_OPERATOR::PRE_INCREMENT:
  case UN_OPERATOR::POST_INCREMENT:
    return "++";
  case UN_OPERATOR::PRE_DECREMENT:
  case UN_OPERATOR::POST_DECREMENT:
    return "--";
  case UN_OPERATOR::LOGICAl_NOT:
    return "!";
  case UN_OPERATOR::BITWISE_NOT:
    return "~";
  case UN_OPERATOR::DEREFRENCE:
    return "*";
  case UN_OPERATOR::REFRENCE:
    return "&";
  case UN_OPERATOR::POSITIVE:
    return "+";
  case UN_OPERATOR::NEGATIVE:
    return "-";
  case UN_OPERATOR::NONE:
    return "";
  }
  return "";
}

static constexpr int POSTFIX_BP = 100;

namespace {

struct BuiltinFunction {
  std::string_view           name;
  std::string_view           label;
  PrimitiveType              return_type;
  std::vector<PrimitiveType> parameter_types;
};

// Built-ins are not declared in the source file, so synthesize a normal
// function entry for them.  User functions with the same name take precedence
// because they are already present in the function table.
DataType
make_void_pointer() {
  DataType pointee;
  pointee.primitive_type = PrimitiveType::VOID;
  pointee.type_name      = "void";

  DataType pointer;
  pointer.kind           = TypeKind::POINTER;
  pointer.primitive_type = PrimitiveType::USER_DEFINED;
  pointer.pointee        = std::make_shared<DataType>(std::move(pointee));
  return pointer;
}

std::optional<FunctionEntry>
make_builtin_entry(std::string_view name) {
  static const std::vector<BuiltinFunction> builtins = {
      {"alloc", "__ag_alloc", PrimitiveType::STRING, {PrimitiveType::INT}},
      {"malloc", "__ag_alloc", PrimitiveType::STRING, {PrimitiveType::INT}},
      {"free", "__ag_free", PrimitiveType::VOID, {PrimitiveType::STRING}},
      {"dealloc", "__ag_free", PrimitiveType::VOID, {PrimitiveType::STRING}},
  };

  for (const auto& builtin : builtins) {
    if (builtin.name != name)
      continue;

    const bool is_allocation = builtin.name == "alloc" || builtin.name == "malloc";

    FunctionEntry entry;
    entry.name        = std::string(builtin.name);
    entry.label       = std::string(builtin.label);
    entry.return_type = std::make_shared<DataType>();
    entry.return_type->primitive_type = builtin.return_type;
    if (is_allocation)
      *entry.return_type = make_void_pointer();
    entry.is_defined = true;

    for (const auto parameter_type : builtin.parameter_types) {
      FunctionParameter parameter;
      parameter.type.primitive_type = parameter_type;
      if (!is_allocation)
        parameter.type = make_void_pointer();
      entry.parameters.push_back(std::move(parameter));
    }

    return entry;
  }

  return std::nullopt;
}

struct OperatorResolution {
  std::optional<FunctionEntry> resolved;
  bool                         ambiguous = false;
};

OperatorResolution
resolve_operator_overload(Parser&                          parser,
                          std::string_view                name,
                          const std::vector<DataType>&    argument_types) {
  OperatorResolution             result;
  std::vector<FunctionEntry>     matches;

  for (const auto& candidate : get_function_table().lookup_all(name)) {
    if (!candidate.is_defined)
      continue;
    if (parser.is_operator_context_active(candidate.label))
      continue;
    if (candidate.parameters.size() != argument_types.size())
      continue;

    bool compatible = true;
    for (size_t i = 0; i < argument_types.size(); ++i) {
      if (!is_compatible_type(candidate.parameters[i].type, argument_types[i])) {
        compatible = false;
        break;
      }
    }

    if (compatible)
      matches.push_back(candidate);
  }

  if (matches.size() > 1)
    result.ambiguous = true;
  else if (matches.size() == 1)
    result.resolved = matches.front();

  return result;
}

std::string
make_temp(LabelCounters& counter,
          Parser&        parser) {
  std::string temp = "t." + std::to_string(counter.temp_label++);
  get_ir_table().declare_internal(counter.function_index, temp, get_type_size(DataType{}));
  return temp;
}

bool
is_identifier_expression(const Expression* expression) {
  const auto* primary = dynamic_cast<const PrimaryExpression*>(expression);
  return primary && primary->primary_type == PrimitiveType::IDENTIFIER;
}

BinaryExpression*
as_string_index_target(Expression* expression) {
  auto* index = dynamic_cast<BinaryExpression*>(expression);
  if (!index || index->bin_operator != BIN_OPERATOR::INDEXING || !index->operator_label.empty())
    return nullptr;

  auto* object = dynamic_cast<PrimaryExpression*>(index->left.get());
  if (!object || object->primary_type != PrimitiveType::IDENTIFIER)
    return nullptr;

  // During semantic analysis the symbol table is available, but it is already
  // unwound while IR is generated.  The indexing node therefore caches whether
  // it targets a string so code generation does not need to look the name up.
  if (!index->is_string_index &&
      index->left->get_data_type().primitive_type != PrimitiveType::STRING)
    return nullptr;

  return index;
}

} // namespace

// Reject writes to declarations marked const (currently `let` variables).
static bool
check_assignment_target_is_mutable(Expression*           target,
                                   Parser&               parser,
                                   const SourceLocation& fallback) {
  // Writing through a string index mutates the buffer owned by the base
  // variable, so a `let` binding must reject it just like direct assignment.
  if (auto* index = as_string_index_target(target))
    target = index->left.get();

  auto* identifier = dynamic_cast<PrimaryExpression*>(target);
  if (!identifier || identifier->primary_type != PrimitiveType::IDENTIFIER)
    return true;

  auto declaration = get_symbol_table().lookup(identifier->get_value());
  if (declaration && declaration->type.is_const) {
    print_error(error::ImmutableAssignment, parser, target->source_location(fallback));
    return false;
  }
  return true;
}

// static int
// led_binding_power(TokenName token) {
//   switch (token) {
//   case TokenName::AG_DOT:
//   case TokenName::AG_ARROW:
//   case TokenName::AG_LEFT_PAREN:
//   case TokenName::AG_LEFT_BRACE:
//   case TokenName::AG_INCREMENT:
//   case TokenName::AG_DECREMENT:
//     return POSTFIX_BP;
//   default:
//     break;
//   }

static std::tuple<float,
                  float>
get_binding_power(BIN_OPERATOR bin_op) {

  switch (bin_op) {
  case BIN_OPERATOR::ASSIGN_TO:
    return {1.1f, 1.0f};
  case BIN_OPERATOR::LOGICAL_OR:
    return {2.0f, 2.1f};
  case BIN_OPERATOR::LOGICAL_AND:
    return {3.0f, 3.1f};
  case BIN_OPERATOR::BITWISE_OR:
    return {4.0f, 4.1f};
  case BIN_OPERATOR::BITWISE_XOR:
    return {5.0f, 5.1f};
  case BIN_OPERATOR::BITWISE_AND:
    return {6.0f, 6.1f};
  case BIN_OPERATOR::EQUAL_TO:
  case BIN_OPERATOR::NOT_EQUAL_TO:
    return {7.0f, 7.1f};
  case BIN_OPERATOR::LESS_THAN:
  case BIN_OPERATOR::GREATER_THAN:
  case BIN_OPERATOR::LESS_THAN_EQUAL:
  case BIN_OPERATOR::GREATER_THAN_EQUAL:
    return {8.0f, 8.1f};
  case BIN_OPERATOR::LEFT_SHIFT:
  case BIN_OPERATOR::RIGHT_SHIFT:
    return {9.0f, 9.1f};
  case BIN_OPERATOR::ADDITION:
  case BIN_OPERATOR::SUBTRACTION:
    return {10.0f, 10.1f};
  case BIN_OPERATOR::MULTIPLICATION:
  case BIN_OPERATOR::DIVISION:
  case BIN_OPERATOR::MODULUS:
    return {11.0f, 11.1f};
  default:
    return {0.0f, 0.1f};
  }
}

static const std::initializer_list<TokenName> binary_list = {
    TokenName::AG_ASSIGN,      TokenName::AG_LOGICAL_OR,  TokenName::AG_LOGICAL_AND, TokenName::AG_BITWISE_OR,
    TokenName::AG_BITWISE_XOR, TokenName::AG_REFERENCE,   TokenName::AG_EQUAL_TO,    TokenName::AG_NOT_EQUAL_TO,
    TokenName::AG_LESS,        TokenName::AG_GREATER,     TokenName::AG_LESS_EQUAL,  TokenName::AG_GREATER_EQUAL,
    TokenName::AG_LEFT_SHIFT,  TokenName::AG_RIGHT_SHIFT, TokenName::AG_PLUS,        TokenName::AG_MINUS,
    TokenName::AG_ASTERISK,    TokenName::AG_SLASH,       TokenName::AG_MODULO,
};

static bool
is_right_associative(BIN_OPERATOR op) {
  return op == BIN_OPERATOR::ASSIGN_TO;
}

AG_scope<Expression>
Expression::parse_expressions(Parser& parser) {
  return parse_pratt(parser, 0.0f);
}

AG_scope<Expression>
Expression::parse_pratt(Parser& parser,
                        float   min_bp) {
  AG_scope<Expression> left;
  if (parser.match(TokenName::AG_LEFT_PAREN)) {
    left = parse_pratt(parser, 0.0f);
    if (!left)
      return nullptr;
    if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen)) {
      return nullptr;
    }
  } else {
    left = UniaryExpression::parse_prefix(parser);
    if (!left) {
      return nullptr;
    }
  }

  // Postfix operators bind tighter than every binary operator.  Calls,
  // indexing, and ++/-- are all handled here so they can be chained.
  while (true) {
    if (parser.check(TokenName::AG_LEFT_PAREN)) {
      left = CallExpression::parse(parser, std::move(left));
      if (!left)
        return nullptr;
      continue;
    }

    if (parser.check(TokenName::AG_LEFT_BRACKET)) {
      const Token bracket_token = parser.current_token();
      parser.advance();

      auto index = parse_pratt(parser, 0.0f);
      if (!index)
        return nullptr;
      if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket))
        return nullptr;

      auto binary      = AG_cscope<BinaryExpression>(BIN_OPERATOR::INDEXING,
                                                     std::move(left),
                                                     std::move(index));
      binary->location = {bracket_token.line, bracket_token.column};
      left             = std::move(binary);
      continue;
    }

    if (parser.check(TokenName::AG_DOT) || parser.check(TokenName::AG_ARROW)) {
      left = MemberExpression::parse(parser, std::move(left));
      if (!left)
        return nullptr;
      continue;
    }

    if (parser.check(TokenName::AG_INCREMENT) || parser.check(TokenName::AG_DECREMENT)) {
      left = UniaryExpression::parse_postfix(parser, std::move(left));
      if (!left)
        return nullptr;
      continue;
    }

    break;
  }

  TokenName temp_token;
  while (true) {
    if (!parser.check_one_of(binary_list, &temp_token)) {
      break;
    }
    BIN_OPERATOR bin_op = get_binary_operator(temp_token);
    auto [l_bp, r_bp]   = get_binding_power(bin_op);
    if (l_bp < min_bp) {
      break;
    }
    const Token operator_token = parser.current_token();
    parser.advance();
    auto right = parse_pratt(parser, r_bp);
    if (!right) {
      return nullptr;
    }
    auto binary = AG_cscope<BinaryExpression>(bin_op, std::move(left), std::move(right));
    binary->location = {operator_token.line, operator_token.column};
    left             = std::move(binary);
  }
  return left;
}

AG_scope<Expression>
PrimaryExpression::parse_primary(Parser& parser) {
  if (parser.check(TokenName::AG_OPERATOR)) {
    const Token operator_token = parser.current_token();
    parser.advance();

    auto symbol = parse_operator_symbol(parser);
    if (!symbol)
      return nullptr;

    auto primary_exp          = AG_cscope<PrimaryExpression>();
    primary_exp->location     = {operator_token.line, operator_token.column};
    primary_exp->primary_type = PrimitiveType::IDENTIFIER;
    primary_exp->value        = parser.intern_operator_name("operator" + *symbol);
    return primary_exp;
  }

  TokenName temp_token;
  if (!parser.check_one_of({TokenName::AG_STRING_LITERAL, TokenName::AG_INT_LITERAL, TokenName::AG_IDENTIFIER},
                           &temp_token)) {
    return nullptr;
  }
  auto primary_exp = AG_cscope<PrimaryExpression>();
  primary_exp->location = {parser.current_token().line, parser.current_token().column};
  if (temp_token == TokenName::AG_STRING_LITERAL) {
    primary_exp->primary_type = PrimitiveType::STRING;
    primary_exp->value        = parser.current_token().value;
    primary_exp->index        = Add_Literals::add_string_literals(primary_exp->value);
  } else if (temp_token == TokenName::AG_INT_LITERAL) {
    primary_exp->primary_type = PrimitiveType::INT;
    primary_exp->value        = parser.current_token().value;
  } else if (temp_token == TokenName::AG_IDENTIFIER) {
    primary_exp->primary_type = PrimitiveType::IDENTIFIER;
    primary_exp->value        = parser.current_token().value;
  }
  parser.advance();
  return primary_exp;
}

PrimitiveType
Expression::get_type() {
  return PrimitiveType::VOID;
}

DataType
Expression::get_data_type() {
  DataType type;
  type.primitive_type = get_type();
  return type;
}

PrimitiveType
BinaryExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
BinaryExpression::get_data_type() {
  if (result_type_known)
    return result_type;
  if (left)
    return left->get_data_type();

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

PrimitiveType
UniaryExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
UniaryExpression::get_data_type() {
  if (result_type_known)
    return result_type;
  if (operand)
    return operand->get_data_type();

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

PrimitiveType
MemberExpression::get_type() {
  return PrimitiveType::VOID;
}

PrimitiveType
IndexExpression::get_type() {
  return PrimitiveType::VOID;
}

PrimitiveType
CallExpression::get_type() {
  return m_result_type.primitive_type;
}

DataType
CallExpression::get_data_type() {
  return m_result_type;
}

bool
CallExpression::analyze_semantics(Parser&               parser,
                                  const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  auto* function_name = dynamic_cast<PrimaryExpression*>(callee.get());
  if (!function_name || function_name->primary_type != PrimitiveType::IDENTIFIER) {
    print_error(error::ExpectedFunctionName, parser, diagnostic_location);
    return false;
  }

  auto candidates = get_function_table().lookup_all(function_name->get_value());
  if (candidates.empty()) {
    if (auto builtin = make_builtin_entry(function_name->get_value()))
      candidates.push_back(std::move(*builtin));
    else {
      print_error(error::UndefinedFunction, parser, diagnostic_location);
      return false;
    }
  }

  std::vector<DataType> argument_types;
  argument_types.reserve(arguments.size());
  for (const auto& argument : arguments) {
    if (!argument->analyze_semantics(parser, argument->source_location(diagnostic_location)))
      return false;
    argument_types.push_back(argument->get_data_type());
  }

  std::optional<FunctionEntry>   resolved;
  std::vector<FunctionEntry>     matches;
  bool                           saw_defined_candidate = false;
  bool                           saw_matching_arity    = false;
  std::optional<size_t>          mismatching_argument;

  for (const auto& candidate : candidates) {
    if (!candidate.is_defined)
      continue;
    saw_defined_candidate = true;

    if (!candidate.is_variadic && arguments.size() != candidate.parameters.size())
      continue;
    saw_matching_arity = true;

    bool compatible = true;
    for (size_t i = 0; i < arguments.size() && i < candidate.parameters.size(); ++i) {
      if (argument_types[i].primitive_type == PrimitiveType::UNEXPECTED ||
          !is_compatible_type(candidate.parameters[i].type, argument_types[i])) {
        compatible = false;
        if (!mismatching_argument)
          mismatching_argument = i;
        break;
      }
    }

    if (compatible)
      matches.push_back(candidate);
  }

  if (matches.size() > 1) {
    print_error(error::AmbiguousOverload, parser, diagnostic_location);
    return false;
  }
  if (matches.size() == 1)
    resolved = matches.front();

  if (!resolved) {
    if (!saw_defined_candidate) {
      print_error(error::UndefinedFunction, parser, diagnostic_location);
    } else if (!saw_matching_arity) {
      print_error(error::InvalidArgumentCount, parser, diagnostic_location);
    } else {
      const SourceLocation& argument_location
          = mismatching_argument && *mismatching_argument < arguments.size()
                ? arguments[*mismatching_argument]->source_location(diagnostic_location)
                : diagnostic_location;
      print_error(error::InvalidArgumentType, parser, argument_location);
    }
    return false;
  }

  m_label       = resolved->label;
  m_result_type = resolved->return_type ? *resolved->return_type : DataType{};
  return true;
}

std::string
CallExpression::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  // Evaluate every argument first.  A nested call may clobber the argument
  // registers, so the outer call's Param instructions must be emitted only
  // after all argument evaluation has finished.
  std::vector<std::string> argument_values;
  argument_values.reserve(arguments.size());
  for (const auto& argument : arguments)
    argument_values.push_back(argument->generate_ir(counter, parser));

  for (const auto& value : argument_values)
    parser.add_instructios(AG_cscope<Param>(value, counter.function_index));

  std::string result;
  if (m_result_type.primitive_type != PrimitiveType::VOID)
    result = make_temp(counter, parser);

  parser.add_instructios(AG_cscope<FunctionCall>(m_label, counter.function_index, result));
  return result;
}

PrimitiveType
PrimaryExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
PrimaryExpression::get_data_type() {
  if (primary_type == PrimitiveType::IDENTIFIER) {
    auto data = get_symbol_table().lookup(value);
    if (!data) {
      DataType unknown;
      unknown.primitive_type = PrimitiveType::UNEXPECTED;
      return unknown;
    }
    return data->type;
  }

  DataType type;
  type.primitive_type = primary_type;
  return type;
}

AG_scope<Expression>
MemberExpression::parse(Parser&              parser,
                        AG_scope<Expression> object) {
  TokenName op_tok;
  parser.peek_token_name(&op_tok);
  parser.advance(); // consumes '.' or '->'

  if (!parser.expect(TokenName::AG_IDENTIFIER, error::ExpectedMemberName)) {
    return nullptr;
  }

  auto member        = AG_cscope<MemberExpression>();
  member->object     = std::move(object);
  member->field_name = parser.current_token().value;
  member->is_arrow   = (op_tok == TokenName::AG_ARROW);
  return member;
}

AG_scope<Expression>
CallExpression::parse(Parser&              parser,
                      AG_scope<Expression> callee) {
  const SourceLocation callee_location = callee ? callee->location : SourceLocation{};
  parser.advance(); // consume '('

  auto call       = AG_cscope<CallExpression>();
  call->location = callee_location;
  call->callee   = std::move(callee);

  if (!parser.check(TokenName::AG_RIGHT_PAREN)) {
    do {
      auto arg = Expression::parse_pratt(parser, 0);
      if (!arg) {
        return nullptr;
      }
      call->arguments.push_back(std::move(arg));
    } while (parser.match(TokenName::AG_COMMA));
  }

  if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen)) {
    return nullptr;
  }
  return call;
}

AG_scope<Expression>
IndexExpression::parse(Parser&              parser,
                       AG_scope<Expression> object) {
  parser.advance(); // consume '['

  auto idx_expr    = AG_cscope<IndexExpression>();
  idx_expr->object = std::move(object);
  idx_expr->index  = Expression::parse_pratt(parser, 0);
  if (!idx_expr->index) {
    return nullptr;
  }

  if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket)) {
    return nullptr;
  }
  return idx_expr;
}

AG_scope<Expression>
UniaryExpression::parse_prefix(Parser& parser) {
  static const std::initializer_list<TokenName> uniary_list = {
      TokenName::AG_INCREMENT,
      TokenName::AG_DECREMENT,
      TokenName::AG_BITWISE_NOT,
      TokenName::AG_LOGICAL_NOT,
      TokenName::AG_REFERENCE,
      TokenName::AG_ASTERISK,
      TokenName::AG_PLUS,
      TokenName::AG_MINUS,
  };
  TokenName temp_token;
  if (parser.check_one_of(uniary_list, &temp_token)) {
    const Token operator_token = parser.current_token();
    auto        prefix_expr    = AG_cscope<UniaryExpression>();
    prefix_expr->un_operator   = get_prefix_operator(temp_token);
    prefix_expr->location      = {operator_token.line, operator_token.column};
    parser.advance();
    prefix_expr->operand = Expression::parse_pratt(parser, POSTFIX_BP - 1);
    if (!prefix_expr->operand) {
      return nullptr;
    }
    return prefix_expr;
  }

  return PrimaryExpression::parse_primary(parser);
}

AG_scope<Expression>
UniaryExpression::parse_postfix(Parser&              parser,
                                AG_scope<Expression> operand) {
  static const std::initializer_list<TokenName> uniary_list = {
      TokenName::AG_INCREMENT,
      TokenName::AG_DECREMENT,
  };
  TokenName temp_token;
  const Token operator_token = parser.current_token();
  if (!parser.match_one_of(uniary_list, &temp_token)) {
    return nullptr;
  }
  auto postfix_expr          = AG_cscope<UniaryExpression>();
  postfix_expr->un_operator  = get_postfix_operator(temp_token);
  postfix_expr->location     = {operator_token.line, operator_token.column};
  postfix_expr->operand      = std::move(operand);

  return postfix_expr;
}

bool
PrimaryExpression::analyze_semantics(Parser&               parser,
                                     const SourceLocation& location) {
  if (primary_type != PrimitiveType::IDENTIFIER)
    return true;
  auto data = get_symbol_table().lookup(value);
  auto type = data ? data->type.primitive_type : PrimitiveType::UNEXPECTED;
  if (type == PrimitiveType::UNEXPECTED) {
    print_error(error::UseOfUndefined, parser, this->source_location(location));
    return false;
  }
  return true;
}

bool
UniaryExpression::analyze_semantics(Parser&               parser,
                                    const SourceLocation& location) {
  if (!operand)
    return true;

  const auto& diagnostic_location = this->source_location(location);
  if (!operand->analyze_semantics(parser, operand->source_location(diagnostic_location)))
    return false;

  const auto operand_type = operand->get_data_type();
  if (operand_type.primitive_type == PrimitiveType::UNEXPECTED ||
      operand_type.primitive_type == PrimitiveType::VOID) {
    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }

  switch (un_operator) {
  case UN_OPERATOR::PRE_INCREMENT:
  case UN_OPERATOR::PRE_DECREMENT:
  case UN_OPERATOR::POST_INCREMENT:
  case UN_OPERATOR::POST_DECREMENT: {
    if (!is_identifier_expression(operand.get()) && !as_string_index_target(operand.get())) {
      print_error(error::InvalidAssignmentError, parser, diagnostic_location);
      return false;
    }
    if (!check_assignment_target_is_mutable(operand.get(), parser, diagnostic_location))
      return false;
    if (!is_compatible_datatype(PrimitiveType::INT, operand_type.primitive_type)) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }
    result_type       = operand_type;
    result_type_known = true;
    return true;
  }
  default:
    break;
  }

  const auto symbol = get_unary_operator_symbol(un_operator);
  if (!symbol.empty()) {
    const std::string function_name = "operator" + std::string(symbol);
    auto resolution = resolve_operator_overload(parser, function_name, {operand_type});
    if (resolution.ambiguous) {
      print_error(error::AmbiguousOverload, parser, diagnostic_location);
      return false;
    }
    if (resolution.resolved) {
      operator_label    = resolution.resolved->label;
      result_type       = resolution.resolved->return_type ? *resolution.resolved->return_type : DataType{};
      result_type_known = true;
      return true;
    }
  }

  switch (un_operator) {
  case UN_OPERATOR::POSITIVE:
  case UN_OPERATOR::NEGATIVE:
  case UN_OPERATOR::BITWISE_NOT:
    if (!is_compatible_datatype(PrimitiveType::INT, operand_type.primitive_type)) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }
    result_type       = operand_type;
    result_type_known = true;
    return true;
  case UN_OPERATOR::LOGICAl_NOT:
    result_type           = DataType{};
    result_type.primitive_type = PrimitiveType::INT;
    result_type_known     = true;
    return true;
  default:
    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }
}
bool
BinaryExpression::analyze_semantics(Parser&               parser,
                                    const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (!left->analyze_semantics(parser, left->source_location(diagnostic_location)))
    return false;
  if (!right->analyze_semantics(parser, right->source_location(diagnostic_location)))
    return false;

  const auto left_type  = left->get_data_type();
  const auto right_type = right->get_data_type();

  if (bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    auto* identifier = dynamic_cast<PrimaryExpression*>(left.get());
    if (identifier && identifier->primary_type == PrimitiveType::IDENTIFIER) {
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    } else {
      if (!as_string_index_target(left.get())) {
        print_error(error::InvalidAssignmentError, parser, diagnostic_location);
        return false;
      }
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    }

    if (left_type.primitive_type == PrimitiveType::UNEXPECTED ||
        right_type.primitive_type == PrimitiveType::UNEXPECTED ||
        !is_compatible_type(left_type, right_type)) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    result_type       = left_type;
    result_type_known = true;
    return true;
  }

  const std::string operator_name = "operator" + std::string(to_symbol(bin_operator));
  auto              resolution
      = resolve_operator_overload(parser, operator_name, {left_type, right_type});
  if (resolution.ambiguous) {
    print_error(error::AmbiguousOverload, parser, diagnostic_location);
    return false;
  }
  if (resolution.resolved) {
    operator_label    = resolution.resolved->label;
    result_type       = resolution.resolved->return_type ? *resolution.resolved->return_type : DataType{};
    result_type_known = true;
    return true;
  }

  if (bin_operator == BIN_OPERATOR::INDEXING) {
    if (left_type.primitive_type == PrimitiveType::STRING &&
        right_type.primitive_type != PrimitiveType::UNEXPECTED &&
        is_compatible_datatype(PrimitiveType::INT, right_type.primitive_type)) {
      result_type                 = DataType{};
      result_type.primitive_type  = PrimitiveType::INT;
      result_type_known           = true;
      is_string_index             = true;
      return true;
    }
    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }

  if (left_type.primitive_type == PrimitiveType::UNEXPECTED ||
      right_type.primitive_type == PrimitiveType::UNEXPECTED ||
      !is_compatible_type(left_type, right_type)) {
    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }

  result_type       = left_type;
  result_type_known = true;
  return true;
}

std::string
BinaryExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  if (bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    auto* index = dynamic_cast<BinaryExpression*>(left.get());
    if (index && index->bin_operator == BIN_OPERATOR::INDEXING) {
      const auto value  = right->generate_ir(counter, parser);
      const auto object = index->left->generate_ir(counter, parser);
      const auto offset = index->right->generate_ir(counter, parser);
      parser.add_instructios(
          AG_cscope<IndexAssign>(object, offset, value, counter.function_index));
      return value;
    }

    const auto r          = right->generate_ir(counter, parser);
    const auto left_value = left->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<Assign>(r, left_value, counter.function_index));
    return left_value;
  }

  if (!operator_label.empty()) {
    const auto left_value  = left->generate_ir(counter, parser);
    const auto right_value = right->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<Param>(left_value, counter.function_index));
    parser.add_instructios(AG_cscope<Param>(right_value, counter.function_index));

    std::string result;
    if (result_type.primitive_type != PrimitiveType::VOID)
      result = make_temp(counter, parser);

    parser.add_instructios(AG_cscope<FunctionCall>(operator_label, counter.function_index, result));
    return result;
  }

  const auto r    = right->generate_ir(counter, parser);
  const auto l    = left->generate_ir(counter, parser);
  const auto temp = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Binary>(temp, l, r, bin_operator, counter.function_index));
  return temp;
}

std::string
UniaryExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  if (!operand)
    return "";

  if (!operator_label.empty()) {
    std::string value = operand->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<Param>(value, counter.function_index));

    std::string result;
    if (result_type.primitive_type != PrimitiveType::VOID)
      result = make_temp(counter, parser);

    parser.add_instructios(AG_cscope<FunctionCall>(operator_label, counter.function_index, result));
    return result;
  }

  if (auto* index = as_string_index_target(operand.get())) {
    const auto object = index->left->generate_ir(counter, parser);
    const auto offset = index->right->generate_ir(counter, parser);

    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(old_value,
                                             object,
                                             offset,
                                             BIN_OPERATOR::INDEXING,
                                             counter.function_index));

    const auto updated = make_temp(counter, parser);
    const auto op      = (un_operator == UN_OPERATOR::PRE_INCREMENT ||
                          un_operator == UN_OPERATOR::POST_INCREMENT)
                             ? BIN_OPERATOR::ADDITION
                             : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(
        AG_cscope<Binary>(updated, old_value, "1", op, counter.function_index));
    parser.add_instructios(
        AG_cscope<IndexAssign>(object, offset, updated, counter.function_index));

    return (un_operator == UN_OPERATOR::PRE_INCREMENT ||
            un_operator == UN_OPERATOR::PRE_DECREMENT)
               ? updated
               : old_value;
  }

  const std::string value = operand->generate_ir(counter, parser);

  switch (un_operator) {
  case UN_OPERATOR::POSITIVE:
    return value;
  case UN_OPERATOR::NEGATIVE: {
    const auto temp = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Binary>(temp, "0", value, BIN_OPERATOR::SUBTRACTION, counter.function_index));
    return temp;
  }
  case UN_OPERATOR::LOGICAl_NOT: {
    const auto temp = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Binary>(temp, value, "0", BIN_OPERATOR::EQUAL_TO, counter.function_index));
    return temp;
  }
  case UN_OPERATOR::BITWISE_NOT: {
    const auto temp = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Binary>(temp, value, "-1", BIN_OPERATOR::BITWISE_XOR, counter.function_index));
    return temp;
  }
  case UN_OPERATOR::PRE_INCREMENT:
  case UN_OPERATOR::PRE_DECREMENT: {
    const auto temp = make_temp(counter, parser);
    const auto op   = un_operator == UN_OPERATOR::PRE_INCREMENT ? BIN_OPERATOR::ADDITION
                                                                : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(AG_cscope<Binary>(temp, value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<Assign>(temp, value, counter.function_index));
    return value;
  }
  case UN_OPERATOR::POST_INCREMENT:
  case UN_OPERATOR::POST_DECREMENT: {
    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Assign>(value, old_value, counter.function_index));

    const auto updated = make_temp(counter, parser);
    const auto op      = un_operator == UN_OPERATOR::POST_INCREMENT ? BIN_OPERATOR::ADDITION
                                                                    : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(AG_cscope<Binary>(updated, value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<Assign>(updated, value, counter.function_index));
    return old_value;
  }
  default:
    return "";
  }
}

std::string
PrimaryExpression::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  if (primary_type == PrimitiveType::STRING) {
    return "str." + std::to_string(index);
  }

  if (primary_type == PrimitiveType::IDENTIFIER) {
    if (auto variable = get_ir_table().lookup_source(counter.function_index, value))
      return variable->name;
  }

  return std::string(value);
}

} // namespace ag
