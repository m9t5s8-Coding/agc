#include "Expressions/Expression.hpp"
#include "IR/AddressOf.hpp"
#include "IR/Assign.hpp"
#include "IR/Binary.hpp"
#include "IR/CondJump.hpp"
#include "IR/DataItem.hpp"
#include "IR/DerefLoad.hpp"
#include "IR/DerefStore.hpp"
#include "IR/FunctionCall.hpp"
#include "IR/IRTable.hpp"
#include "IR/IndexAssign.hpp"
#include "IR/Jump.hpp"
#include "IR/Label.hpp"
#include "IR/MemberLoad.hpp"
#include "IR/MemberStore.hpp"
#include "IR/Narrow.hpp"
#include "IR/Param.hpp"
#include "IR/Return.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Statements/body.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ag {
// Built-in comparisons produce an integer 0/1 regardless of operand type.
// Operator overloads return their declared type and never reach this helper.
static bool
is_comparison_operator(BIN_OPERATOR op) {
  switch (op) {
  case BIN_OPERATOR::EQUAL_TO:
  case BIN_OPERATOR::NOT_EQUAL_TO:
  case BIN_OPERATOR::LESS_THAN:
  case BIN_OPERATOR::LESS_THAN_EQUAL:
  case BIN_OPERATOR::GREATER_THAN:
  case BIN_OPERATOR::GREATER_THAN_EQUAL:
    return true;
  default:
    return false;
  }
}

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
  case TokenName::AG_ADDITION_ASSIGNMENT:
    return BIN_OPERATOR::ADDITION;
  case TokenName::AG_SUBTRACTION_ASSIGNMENT:
    return BIN_OPERATOR::SUBTRACTION;
  case TokenName::AG_MULTIPLY_ASSIGNMENT:
    return BIN_OPERATOR::MULTIPLICATION;
  case TokenName::AG_DIVISION_ASSIGNMENT:
    return BIN_OPERATOR::DIVISION;
  case TokenName::AG_MODULO_ASSIGNMENT:
    return BIN_OPERATOR::MODULUS;
  case TokenName::AG_BITWISE_AND_ASSIGNMENT:
    return BIN_OPERATOR::BITWISE_AND;
  case TokenName::AG_BITWISE_OR_ASSIGNMENT:
    return BIN_OPERATOR::BITWISE_OR;
  case TokenName::AG_BITWISE_XOR_ASSIGNMENT:
    return BIN_OPERATOR::BITWISE_XOR;
  case TokenName::AG_LEFT_SHIFT_ASSIGNMENT:
    return BIN_OPERATOR::LEFT_SHIFT;
  case TokenName::AG_RIGHT_SHIFT_ASSIGNMENT:
    return BIN_OPERATOR::RIGHT_SHIFT;
  default:
    return BIN_OPERATOR::NONE;
  }
}

static bool
is_compound_assignment_token(TokenName token) {
  switch (token) {
  case TokenName::AG_ADDITION_ASSIGNMENT:
  case TokenName::AG_SUBTRACTION_ASSIGNMENT:
  case TokenName::AG_MULTIPLY_ASSIGNMENT:
  case TokenName::AG_DIVISION_ASSIGNMENT:
  case TokenName::AG_MODULO_ASSIGNMENT:
  case TokenName::AG_BITWISE_AND_ASSIGNMENT:
  case TokenName::AG_BITWISE_OR_ASSIGNMENT:
  case TokenName::AG_BITWISE_XOR_ASSIGNMENT:
  case TokenName::AG_LEFT_SHIFT_ASSIGNMENT:
  case TokenName::AG_RIGHT_SHIFT_ASSIGNMENT:
    return true;
  default:
    return false;
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

struct BuiltinParameter {
  PrimitiveType type            = PrimitiveType::INT;
  bool          is_void_pointer = false;
};

struct BuiltinFunction {
  std::string_view              name;
  std::string_view              label;
  PrimitiveType                 return_type          = PrimitiveType::VOID;
  bool                          returns_void_pointer = false;
  std::vector<BuiltinParameter> parameters;
};

// `agc:write` and friends are parsed with a single colon so the built-in
// namespace cannot collide with normal module paths, which use `::`.
std::optional<std::string_view>
builtin_name_from_token(const Token& token) {
  if (token.token_name == TokenName::AG_IDENTIFIER)
    return token.value;
  return std::nullopt;
}

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
      {"alloc", "__ag_alloc", PrimitiveType::STRING, true,
       {{PrimitiveType::INT, false}}},
      {"malloc", "__ag_alloc", PrimitiveType::STRING, true,
       {{PrimitiveType::INT, false}}},
      {"free", "__ag_free", PrimitiveType::VOID, false,
       {{PrimitiveType::INT, true}}},
      {"dealloc", "__ag_free", PrimitiveType::VOID, false,
       {{PrimitiveType::INT, true}}},
      {"write", "__ag_write", PrimitiveType::INT, false,
       {{PrimitiveType::INT, true}, {PrimitiveType::INT, false}}},
      {"write_err", "__ag_write_err", PrimitiveType::INT, false,
       {{PrimitiveType::INT, true}, {PrimitiveType::INT, false}}},
      {"read", "__ag_read", PrimitiveType::INT, false,
       {{PrimitiveType::INT, true}, {PrimitiveType::INT, false}}},
      {"exit", "__ag_exit", PrimitiveType::VOID, false,
       {{PrimitiveType::INT, false}}},
      {"memcpy", "__ag_memcpy", PrimitiveType::STRING, true,
       {{PrimitiveType::INT, true}, {PrimitiveType::INT, true}, {PrimitiveType::INT, false}}},
      {"strcpy", "__ag_strcpy", PrimitiveType::STRING, true,
       {{PrimitiveType::INT, true}, {PrimitiveType::INT, true}}},
  };

  for (const auto& builtin : builtins) {
    if (builtin.name != name)
      continue;

    FunctionEntry entry;
    entry.name        = std::string(builtin.name);
    entry.label       = std::string(builtin.label);
    entry.return_type = std::make_shared<DataType>();
    entry.return_type->primitive_type = builtin.return_type;
    if (builtin.returns_void_pointer)
      *entry.return_type = make_void_pointer();
    entry.is_defined = true;
    entry.is_public  = true;

    for (const auto& parameter_info : builtin.parameters) {
      FunctionParameter parameter;
      if (parameter_info.is_void_pointer)
        parameter.type = make_void_pointer();
      else
        parameter.type.primitive_type = parameter_info.type;
      entry.parameters.push_back(std::move(parameter));
    }

    return entry;
  }

  return std::nullopt;
}

bool
is_function_visible(Parser& parser, const FunctionEntry& entry) {
  return is_visible_from(entry.is_public, entry.module_path, parser.current_module_path());
}

bool
is_member_visible(Parser&                 parser,
                  const StructDefinition& definition,
                  const StructMember&     member) {
  return is_visible_from(
      member.is_public, declaring_module(definition.name), parser.current_module_path());
}

bool
is_addressable_expression(const Expression* expression);

struct OperatorResolution {
  std::optional<FunctionEntry> resolved;
  bool                         ambiguous = false;
};

// Overloads such as `operator<<(char*, char)` and `operator<<(char*, int)`
// are both compatible with a character argument because `char` and `int` are
// interchangeable in arithmetic.  Prefer the candidate whose parameter type
// exactly matches the argument so those overloads stay distinguishable.
bool
same_data_type(const DataType& left,
               const DataType& right) {
  if (left.kind != right.kind)
    return false;

  if (left.kind == TypeKind::POINTER || left.kind == TypeKind::REFERENCE) {
    if (!left.pointee || !right.pointee)
      return !left.pointee && !right.pointee;
    if (left.kind == TypeKind::REFERENCE &&
        left.is_mutable_reference != right.is_mutable_reference)
      return false;
    return same_data_type(*left.pointee, *right.pointee);
  }

  if (left.primitive_type != right.primitive_type)
    return false;
  if (left.primitive_type == PrimitiveType::USER_DEFINED &&
      left.type_name != right.type_name)
    return false;
  return true;
}

int
overload_match_score(const FunctionEntry&         candidate,
                     const std::vector<DataType>& argument_types,
                     bool                         has_receiver) {
  int score = 0;
  for (size_t i = 0; i < argument_types.size() &&
                     i < candidate.parameters.size();
       ++i) {
    const DataType& parameter_type = candidate.parameters[i].type;
    const DataType& argument_type  = argument_types[i];

    bool exact = same_data_type(parameter_type, argument_type);

    // An implicit reference parameter (`&mut T`) receives the address of an
    // lvalue of type `T`.  Score it by the pointee so `&mut int` and
    // `&mut char*` stay distinguishable for a `char*` argument.
    if (!exact && parameter_type.kind == TypeKind::REFERENCE &&
        parameter_type.pointee && argument_type.kind != TypeKind::REFERENCE &&
        same_data_type(*parameter_type.pointee, argument_type))
      exact = true;

    score += exact ? 2 : 1;
  }
  (void)has_receiver;
  return score;
}

OperatorResolution
resolve_operator_overload(Parser&                          parser,
                          std::string_view                name,
                          const std::vector<DataType>&    argument_types,
                          const std::vector<Expression*>&  argument_expressions = {}) {
  OperatorResolution             result;
  std::vector<FunctionEntry>     matches;
  std::vector<FunctionEntry>     candidates;
  std::unordered_set<std::string> seen_labels;

  const auto add_candidates = [&](const std::string& lookup_name) {
    for (const auto& candidate : get_function_table().lookup_all(lookup_name)) {
      if (seen_labels.insert(candidate.label).second)
        candidates.push_back(candidate);
    }
  };

  const std::string resolved_name = parser.resolve_symbol_name(name);
  add_candidates(resolved_name);
  if (resolved_name != name)
    add_candidates(std::string(name));

  // `import module;` (in any of its forms) also imports that module's public
  // operator overloads.  Private operators stay hidden through the normal
  // visibility check below.
  for (const auto& module : get_module_table().imported_modules(parser.current_module_path()))
    add_candidates(module + "::" + std::string(name));

  for (const auto& candidate : candidates) {
    if (!candidate.is_defined && !candidate.is_extern)
      continue;
    if (!is_function_visible(parser, candidate))
      continue;
    if (parser.is_operator_context_active(candidate.label))
      continue;
    if (candidate.parameters.size() != argument_types.size())
      continue;

    bool compatible = true;
    for (size_t i = 0; i < argument_types.size(); ++i) {
      const auto& parameter_type = candidate.parameters[i].type;
      if (is_compatible_type(parameter_type, argument_types[i]))
        continue;

      // Operator parameters may be references.  `cin >> value` should bind
      // `value` by reference just like an ordinary call with a reference
      // parameter, without requiring the caller to write `&mut value`.
      const bool implicit_reference
          = parameter_type.kind == TypeKind::REFERENCE && parameter_type.pointee &&
            i < argument_expressions.size() && argument_expressions[i] &&
            is_addressable_expression(argument_expressions[i]) &&
            is_compatible_type(*parameter_type.pointee, argument_types[i]);
      if (!implicit_reference) {
        compatible = false;
        break;
      }
    }

    if (compatible)
      matches.push_back(candidate);
  }

  if (matches.size() > 1) {
    int    best_score = -1;
    size_t best_count = 0;
    std::optional<FunctionEntry> best;
    for (const auto& match : matches) {
      const int score = overload_match_score(match, argument_types, false);
      if (score > best_score) {
        best_score = score;
        best_count = 1;
        best       = match;
      } else if (score == best_score) {
        ++best_count;
      }
    }

    if (best_count == 1)
      result.resolved = best;
    else
      result.ambiguous = true;
  } else if (matches.size() == 1) {
    result.resolved = matches.front();
  }

  return result;
}

} // namespace

std::string
make_temp(LabelCounters& counter,
          Parser&        parser) {
  std::string temp = "t." + std::to_string(counter.temp_label++);
  get_ir_table().declare_internal(counter.function_index, temp, get_type_size(DataType{}));
  return temp;
}

namespace {

bool
is_identifier_expression(const Expression* expression) {
  const auto* primary = dynamic_cast<const PrimaryExpression*>(expression);
  return primary && primary->primary_type == PrimitiveType::IDENTIFIER;
}

MemberExpression*
as_member_target(Expression* expression) {
  return dynamic_cast<MemberExpression*>(expression);
}

// Struct values are pointer-sized addresses.  Pointer-to-struct types are
// accepted too so `p.x` and `p->x` behave the same.
DataType
unwrap_struct_type(DataType type) {
  if ((type.kind == TypeKind::POINTER || type.kind == TypeKind::REFERENCE) &&
      type.pointee)
    return *type.pointee;
  return type;
}

struct ResolvedEnumVariant {
  const EnumDefinition* definition = nullptr;
  const EnumVariant*    variant    = nullptr;
};

std::optional<ResolvedEnumVariant>
resolve_enum_variant(Parser& parser,
                     std::string_view path) {
  const auto separator = path.rfind("::");
  if (separator == std::string_view::npos)
    return std::nullopt;

  const std::string enum_part(path.substr(0, separator));
  const std::string variant_part(path.substr(separator + 2));

  const std::string resolved_enum = parser.resolve_type_name(enum_part);
  const auto* definition = get_enum_table().lookup(resolved_enum);
  if (!definition)
    definition = get_enum_table().lookup(enum_part);
  if (!definition)
    return std::nullopt;

  const auto* variant = definition->find_variant(variant_part);
  if (!variant)
    return std::nullopt;

  return ResolvedEnumVariant{definition, variant};
}

Expression*
root_assignment_target(Expression* expression) {
  Expression* current = expression;
  while (auto* member = dynamic_cast<MemberExpression*>(current))
    current = member->object.get();
  return current;
}

BinaryExpression*
as_index_target(Expression* expression) {
  auto* index = dynamic_cast<BinaryExpression*>(expression);
  if (!index || index->bin_operator != BIN_OPERATOR::INDEXING || !index->operator_label.empty())
    return nullptr;

  // During semantic analysis the symbol table is available, but it is already
  // unwound while IR is generated.  The indexing node therefore caches whether
  // it is a built-in pointer/reference/byte-address index so code generation
  // does not need to look the name up.  The indexed object may be a plain
  // identifier or a member such as `str.ptr[index]`.
  if (!index->is_string_index && !is_string_like_type(index->left->get_data_type()))
    return nullptr;

  return index;
}

bool
is_pointer_like_type(const DataType& type) {
  return type.kind == TypeKind::POINTER || is_string_like_type(type);
}

// `get_data_type()` exposes the pointee of a reference variable so ordinary
// expressions read the referred value.  Contexts that need the reference
// itself (dereference, address-of, passing it to another reference parameter)
// use this helper instead.
DataType
raw_data_type(Expression* expression) {
  return expression ? expression->get_declared_data_type() : DataType{};
}

// Evaluate a reference-valued expression to the address it holds, bypassing
// the automatic pointee load performed by primary/qualified expressions.
std::string
generate_reference_value(Expression*    expression,
                         LabelCounters& counter,
                         Parser&        parser) {
  return expression ? expression->generate_reference_ir(counter, parser) : "";
}

// Passing `buffer[2]` to a pointer parameter means "the address of element 2",
// not the value stored at that address.  The offset is scaled by the pointee
// size so `values[2]` on an `int*` advances two integers rather than two
// bytes.
std::string
generate_index_address(Expression*    expression,
                       LabelCounters& counter,
                       Parser&        parser) {
  auto* index = as_index_target(expression);
  if (!index)
    return expression->generate_ir(counter, parser);

  const auto object = index->left->generate_ir(counter, parser);
  const auto offset = index->right->generate_ir(counter, parser);

  std::string scaled = offset;
  if (index->index_scale != 1) {
    scaled = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(scaled,
                                             offset,
                                             std::to_string(index->index_scale),
                                             BIN_OPERATOR::MULTIPLICATION,
                                             counter.function_index));
  }

  const auto result = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(
      result, object, scaled, BIN_OPERATOR::ADDITION, counter.function_index));
  return result;
}

UniaryExpression*
as_deref_target(Expression* expression) {
  auto* unary = dynamic_cast<UniaryExpression*>(expression);
  if (!unary || unary->un_operator != UN_OPERATOR::DEREFRENCE)
    return nullptr;
  return unary;
}

bool
is_addressable_expression(const Expression* expression) {
  if (const auto* primary = dynamic_cast<const PrimaryExpression*>(expression))
    return primary->primary_type == PrimitiveType::IDENTIFIER;
  if (const auto* qualified = dynamic_cast<const QualifiedNameExpression*>(expression)) {
    const std::string& name = qualified->resolved_path.empty()
                                  ? qualified->path
                                  : qualified->resolved_path;
    return get_symbol_table().lookup(name).has_value();
  }
  if (dynamic_cast<const MemberExpression*>(expression))
    return true;
  if (const auto* unary = dynamic_cast<const UniaryExpression*>(expression))
    return unary->un_operator == UN_OPERATOR::DEREFRENCE;
  if (const auto* index = dynamic_cast<const BinaryExpression*>(expression))
    return index->bin_operator == BIN_OPERATOR::INDEXING && index->is_string_index;
  return false;
}

// Lower `&lvalue` to an address value.  Named storage needs an explicit
// AddressOf instruction; member and index expressions are ordinary address
// arithmetic because struct values and byte pointers already hold addresses.
std::string
generate_address_of(Expression*    expression,
                    LabelCounters& counter,
                    Parser&        parser) {
  if (auto* primary = dynamic_cast<PrimaryExpression*>(expression);
      primary && primary->primary_type == PrimitiveType::IDENTIFIER) {
    // `&ref` denotes the referenced object, matching C++ references.
    if (raw_data_type(primary).kind == TypeKind::REFERENCE)
      return generate_reference_value(primary, counter, parser);

    const std::string name = primary->resolved_name_known
                                 ? primary->resolved_name
                                 : std::string(primary->get_value());
    const std::string result = make_temp(counter, parser);
    if (auto variable = get_ir_table().lookup_source(counter.function_index, name)) {
      parser.add_instructios(
          AG_cscope<AddressOf>(result, variable->name, counter.function_index));
      return result;
    }
    if (auto global = get_ir_table().lookup_global_source(name)) {
      parser.add_instructios(
          AG_cscope<AddressOf>(result, global->name, counter.function_index));
      return result;
    }
    return "";
  }

  if (auto* qualified = dynamic_cast<QualifiedNameExpression*>(expression)) {
    const std::string& name = qualified->resolved_path.empty() ? qualified->path
                                                               : qualified->resolved_path;
    if (raw_data_type(qualified).kind == TypeKind::REFERENCE)
      return generate_reference_value(qualified, counter, parser);

    const std::string result = make_temp(counter, parser);
    if (auto variable = get_ir_table().lookup_source(counter.function_index, name)) {
      parser.add_instructios(
          AG_cscope<AddressOf>(result, variable->name, counter.function_index));
      return result;
    }
    if (auto global = get_ir_table().lookup_global_source(name)) {
      parser.add_instructios(
          AG_cscope<AddressOf>(result, global->name, counter.function_index));
      return result;
    }
    return "";
  }

  if (auto* deref = as_deref_target(expression))
    return deref->operand ? generate_reference_value(deref->operand.get(), counter, parser)
                          : "";

  if (auto* member = as_member_target(expression)) {
    if (!member->object)
      return "";
    const auto object = member->object->generate_ir(counter, parser);
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(result,
                                             object,
                                             std::to_string(member->byte_offset),
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    return result;
  }

  if (as_index_target(expression))
    return generate_index_address(expression, counter, parser);

  return "";
}

size_t
value_storage_size(const DataType& type) {
  if ((type.kind == TypeKind::STRUCT || type.kind == TypeKind::ENUM) &&
      type.size_in_bytes != 0)
    return type.size_in_bytes;
  return get_type_size(type);
}

size_t
pointer_step(const DataType& type) {
  if (type.kind == TypeKind::POINTER && type.pointee)
    return std::max<size_t>(1, value_storage_size(*type.pointee));
  if (type.kind == TypeKind::REFERENCE && type.pointee)
    return std::max<size_t>(1, value_storage_size(*type.pointee));
  return 1;
}

bool
is_integer_like_type(const DataType& type) {
  return type.primitive_type == PrimitiveType::INT ||
         type.primitive_type == PrimitiveType::CHAR ||
         type.primitive_type == PrimitiveType::BOOL;
}

bool
deref_loads_byte(const DataType& type) {
  return type.primitive_type == PrimitiveType::CHAR;
}

// Width of one value in memory.  Struct/enum values are pointers to their
// storage in AGC, so they keep an eight-byte slot; the fixed-width integer and
// bool spellings use their declared byte size.
size_t
memory_width_of(const DataType& type) {
  return std::max<size_t>(1, get_type_size(type));
}

bool
memory_is_signed(const DataType& type) {
  return type.is_signed;
}

// Aggregate values are represented by a pointer to their fields.  Passing one
// by value therefore needs a fresh copy; references and pointers deliberately
// skip this path and pass the original address.
std::string
generate_aggregate_argument_copy(Expression*    expression,
                                 const DataType& type,
                                 LabelCounters& counter,
                                 Parser&        parser) {
  const auto value = expression->generate_ir(counter, parser);
  const auto size  = value_storage_size(type);

  const auto copy = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Param>(std::to_string(size), counter.function_index));
  parser.add_instructios(
      AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, copy));

  parser.add_instructios(AG_cscope<Param>(copy, counter.function_index));
  parser.add_instructios(AG_cscope<Param>(value, counter.function_index));
  parser.add_instructios(
      AG_cscope<Param>(std::to_string(size), counter.function_index));
  const auto ignored = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<FunctionCall>("__ag_memcpy", counter.function_index, ignored));
  return copy;
}

} // namespace

std::string
narrow_value(const std::string& value,
             const DataType&    type,
             LabelCounters&     counter,
             Parser&            parser) {
  if (type.kind != TypeKind::PRIMITIVE)
    return value;
  const size_t width = memory_width_of(type);
  if (width >= 8)
    return value;

  const auto result = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Narrow>(
      result, value, width, type.is_signed, counter.function_index));
  return result;
}

// Reject writes to declarations marked const (currently `let` variables) and
// writes through shared `&T` references.
static bool
check_assignment_target_is_mutable(Expression*           target,
                                   Parser&               parser,
                                   const SourceLocation& fallback) {
  const auto check_deref = [&](Expression* expression) -> std::optional<bool> {
    auto* deref = as_deref_target(expression);
    if (!deref || !deref->operand)
      return std::nullopt;

    const auto type = raw_data_type(deref->operand.get());
    if (type.kind == TypeKind::REFERENCE && !type.is_mutable_reference) {
      print_error(error::ImmutableAssignment,
                  parser,
                  expression->source_location(fallback));
      return false;
    }
    if (type.kind == TypeKind::REFERENCE || type.kind == TypeKind::POINTER ||
        is_string_like_type(type))
      return true;

    print_error(error::InvalidAssignmentError,
                parser,
                expression->source_location(fallback));
    return false;
  };

  if (const auto result = check_deref(target))
    return *result;

  // Writing through a pointer index mutates the object reached from the base
  // variable, so a `let` binding must reject it just like direct assignment.
  // Unwrap nested indexes (`matrix[0][1] = value`) down to their root base.
  bool through_index = false;
  while (auto* index = as_index_target(target)) {
    target        = index->left.get();
    through_index = true;
  }

  // Field writes mutate the object reached through the base binding.
  bool through_member = false;
  if (as_member_target(target)) {
    target         = root_assignment_target(target);
    through_member = true;
  }

  if (const auto result = check_deref(target))
    return *result;

  auto* identifier = dynamic_cast<PrimaryExpression*>(target);
  if (!identifier || identifier->primary_type != PrimitiveType::IDENTIFIER)
    return true;

  const std::string_view identifier_name
      = identifier->resolved_name_known ? std::string_view(identifier->resolved_name)
                                        : identifier->get_value();
  auto declaration = get_symbol_table().lookup(identifier_name);
  if (!declaration && identifier_name != identifier->get_value())
    declaration = get_symbol_table().lookup(identifier->get_value());
  if (!declaration)
    return true;

  if (through_member) {
    if (declaration->type.kind == TypeKind::REFERENCE) {
      if (declaration->type.is_mutable_reference)
        return true;
      print_error(error::ImmutableAssignment, parser, target->source_location(fallback));
      return false;
    }
    // Raw pointers and byte addresses can mutate their pointee even when the
    // pointer binding itself is a `let`.
    if (declaration->type.kind == TypeKind::POINTER ||
        is_string_like_type(declaration->type))
      return true;
  }

  // Assignment to a reference variable writes through the reference.  A
  // `&mut T` permits that regardless of whether the binding itself is `let`;
  // a shared `&T` rejects it.
  if (declaration->type.kind == TypeKind::REFERENCE) {
    if (declaration->type.is_mutable_reference)
      return true;
    print_error(error::ImmutableAssignment, parser, target->source_location(fallback));
    return false;
  }

  // Index writes through a `let` pointer are rejected for now: the binding is
  // immutable even though the byte buffer is not.
  (void)through_index;

  if (declaration->type.is_const) {
    print_error(error::ImmutableAssignment, parser, target->source_location(fallback));
    return false;
  }
  return true;
}

// A method call with `mut self` needs mutable access to the receiver.  A
// reference/pointer receiver is already indirect access, so the binding being
// a `let` is irrelevant; only the reference's own mutability matters.
static bool
check_method_receiver_is_mutable(Expression*           receiver,
                                 Parser&               parser,
                                 const SourceLocation& fallback) {
  const auto type = receiver->get_data_type();
  if (type.kind == TypeKind::REFERENCE) {
    if (type.is_mutable_reference)
      return true;
    print_error(error::ImmutableAssignment,
                parser,
                receiver->source_location(fallback));
    return false;
  }
  if (type.kind == TypeKind::POINTER || is_string_like_type(type))
    return true;
  return check_assignment_target_is_mutable(receiver, parser, fallback);
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
    TokenName::AG_ADDITION_ASSIGNMENT,    TokenName::AG_SUBTRACTION_ASSIGNMENT,
    TokenName::AG_MULTIPLY_ASSIGNMENT,    TokenName::AG_DIVISION_ASSIGNMENT,
    TokenName::AG_MODULO_ASSIGNMENT,      TokenName::AG_BITWISE_AND_ASSIGNMENT,
    TokenName::AG_BITWISE_OR_ASSIGNMENT,  TokenName::AG_BITWISE_XOR_ASSIGNMENT,
    TokenName::AG_LEFT_SHIFT_ASSIGNMENT,  TokenName::AG_RIGHT_SHIFT_ASSIGNMENT,
};

static bool
is_right_associative(BIN_OPERATOR op) {
  return op == BIN_OPERATOR::ASSIGN_TO;
}

bool
LambdaExpression::is_lambda_ahead(Parser& parser) {
  if (parser.check(TokenName::AG_MOVE))
    return parser.peek_token().token_name == TokenName::AG_LEFT_PAREN;

  if (!parser.check(TokenName::AG_LEFT_PAREN))
    return false;

  int depth = 0;
  for (size_t offset = 0; parser.is_valid(offset); ++offset) {
    switch (parser.peek_token(offset).token_name) {
    case TokenName::AG_LEFT_PAREN:
      ++depth;
      break;
    case TokenName::AG_RIGHT_PAREN:
      if (--depth == 0) {
        const auto next = parser.peek_token(offset + 1).token_name;
        // `(params) -> Type` or `(params) => expr`; a parenthesized value is
        // followed by an ordinary operator instead.
        return next == TokenName::AG_ARROW ||
               next == TokenName::AG_FAT_ARROW;
      }
      break;
    case TokenName::AG_EOF:
      return false;
    default:
      break;
    }
  }
  return false;
}

AG_scope<Expression>
LambdaExpression::parse(Parser& parser) {
  const Token start_token = parser.current_token();

  auto lambda      = AG_cscope<LambdaExpression>();
  lambda->location = {start_token.line, start_token.column};

  parser.match(TokenName::AG_MOVE);

  if (!parser.expect(TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen))
    return nullptr;

  // Parameters are skipped rather than bound: until closures have a runtime
  // representation, the body is not analyzed or generated.  Track nested
  // parentheses so defaults and nested calls do not end the list early.
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
    return nullptr;
  }

  if (parser.match(TokenName::AG_ARROW)) {
    // Skip the return type up to the body.  Function types may contain
    // parentheses and their own `->`, so only stop at a top-level `=>`/`{`.
    int nested_parens   = 0;
    int nested_brackets = 0;
    while (!parser.is_at_end()) {
      const auto token = parser.current_token().token_name;
      if (token == TokenName::AG_LEFT_PAREN) {
        ++nested_parens;
      } else if (token == TokenName::AG_RIGHT_PAREN) {
        if (nested_parens > 0)
          --nested_parens;
        else
          break;
      } else if (token == TokenName::AG_LEFT_BRACKET) {
        ++nested_brackets;
      } else if (token == TokenName::AG_RIGHT_BRACKET) {
        if (nested_brackets > 0)
          --nested_brackets;
      } else if (nested_parens == 0 && nested_brackets == 0 &&
                 (token == TokenName::AG_FAT_ARROW ||
                  token == TokenName::AG_LEFT_BRACE)) {
        break;
      }
      parser.advance();
    }
  }

  const auto skip_block_body = [&]() -> bool {
    int braces = 1;
    while (braces > 0 && !parser.is_at_end()) {
      if (parser.match(TokenName::AG_LEFT_BRACE)) {
        ++braces;
      } else if (parser.match(TokenName::AG_RIGHT_BRACE)) {
        --braces;
      } else {
        parser.advance();
      }
    }
    if (braces != 0) {
      print_error(error::UnexpectedEndOfFile, parser);
      return false;
    }
    return true;
  };

  if (parser.match(TokenName::AG_FAT_ARROW)) {
    // `(x) => { ... }` has a block body rather than an expression body.
    if (parser.match(TokenName::AG_LEFT_BRACE)) {
      if (!skip_block_body())
        return nullptr;
    } else {
      AG_scope<Expression> body = Expression::parse_pratt(parser, 0.0f);
      if (!body)
        return nullptr;
    }
  } else if (parser.match(TokenName::AG_LEFT_BRACE)) {
    if (!skip_block_body())
      return nullptr;
  } else {
    print_error(error::ExpectedFatArrow, parser);
    return nullptr;
  }

  return lambda;
}

PrimitiveType
LambdaExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
LambdaExpression::get_data_type() {
  if (result_type.primitive_type == PrimitiveType::UNEXPECTED) {
    result_type.kind           = TypeKind::FUNCTION;
    result_type.primitive_type = PrimitiveType::USER_DEFINED;
    result_type.size_in_bytes  = 8;
    result_type.alignment      = 8;
  }
  return result_type;
}

bool
LambdaExpression::analyze_semantics(Parser&,
                                    const SourceLocation&) {
  // No closure analysis yet; the parsed body is intentionally not visited so
  // captured names do not produce misleading diagnostics.
  return true;
}

std::string
LambdaExpression::generate_ir(LabelCounters&,
                              Parser&) {
  return "0";
}

// A call may spell explicit generic arguments, e.g. `max<int>(a, b)`.  Generics
// are erased, so the arguments are skipped; the parse only succeeds when the
// matching `>` is followed by `(`, which distinguishes the call from a
// less-than comparison.
static bool
skip_generic_call_arguments(Parser& parser) {
  if (!parser.check(TokenName::AG_LESS))
    return false;

  int    depth = 0;
  size_t close = 0;
  bool   found = false;
  for (size_t offset = 0; parser.is_valid(offset); ++offset) {
    const auto token = parser.peek_token(offset).token_name;
    if (token == TokenName::AG_LESS) {
      ++depth;
    } else if (token == TokenName::AG_GREATER) {
      --depth;
      if (depth == 0) {
        close = offset;
        found = true;
        break;
      }
    } else if (token == TokenName::AG_RIGHT_SHIFT) {
      depth -= 2;
      if (depth <= 0) {
        close = offset;
        found = true;
        break;
      }
    } else if (token == TokenName::AG_EOF ||
               token == TokenName::AG_SEMICOLON ||
               token == TokenName::AG_LEFT_BRACE) {
      return false;
    }
  }

  if (!found)
    return false;

  const auto next = parser.peek_token(close + 1).token_name;
  if (next != TokenName::AG_LEFT_PAREN &&
      next != TokenName::AG_DOUBLE_COLON)
    return false;

  // Consume through the closing `>` (or the `>>` that closes it).
  for (size_t offset = 0; offset <= close; ++offset)
    parser.advance();
  return true;
}

AG_scope<Expression>
Expression::parse_expressions(Parser& parser) {
  return parse_pratt(parser, 0.0f);
}

AG_scope<Expression>
Expression::parse_pratt(Parser& parser,
                        float   min_bp) {
  // `comptime expression` requests compile-time evaluation.  The compiler does
  // not have a constant evaluator yet, so parse the expression normally and
  // let the ordinary runtime path handle it.
  if (parser.match(TokenName::AG_COMPTIME))
    return parse_pratt(parser, min_bp);

  // `await expression` waits for an asynchronous result.  Without an async
  // runtime the operand is evaluated synchronously.
  if (parser.match(TokenName::AG_AWAIT))
    return parse_pratt(parser, min_bp);

  // `<-channel` receives a value.  Receives are synchronous pass-throughs for
  // now, mirroring the `spawn`/`async` lowering.
  if (parser.match(TokenName::AG_CHANNEL_SEND))
    return parse_pratt(parser, min_bp);

  // `box expression` places a value on the heap.  Allocated aggregates already
  // live in heap blocks, so the marker is erased.
  if (parser.match(TokenName::AG_BOX))
    return parse_pratt(parser, 12.0f);

  // `loop { ... }` in expression position produces the value supplied to
  // `break value`.
  if (parser.check(TokenName::AG_LOOP))
    return LoopExpression::parse(parser);

  AG_scope<Expression> left;
  const size_t         left_start = parser.get_current_position();
  if (LambdaExpression::is_lambda_ahead(parser)) {
    left = LambdaExpression::parse(parser);
    if (!left)
      return nullptr;
  } else if (parser.match(TokenName::AG_LEFT_PAREN)) {
    auto first = parse_pratt(parser, 0.0f);
    if (!first)
      return nullptr;

    if (parser.match(TokenName::AG_COMMA)) {
      auto tuple = AG_cscope<TupleExpression>();
      tuple->location = first->source_location(first->location);
      tuple->elements.emplace_back(std::move(first));

      while (!parser.check(TokenName::AG_RIGHT_PAREN)) {
        auto element = parse_pratt(parser, 0.0f);
        if (!element)
          return nullptr;
        tuple->elements.emplace_back(std::move(element));
        if (!parser.match(TokenName::AG_COMMA))
          break;
      }

      if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen))
        return nullptr;
      left = std::move(tuple);
    } else {
      if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen))
        return nullptr;
      left = std::move(first);
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
    // Built-ins use the unique `agc:` namespace.  Normal modules use `::`,
    // so `agc:write(...)` cannot be confused with a user module path.
    if (auto* primary = dynamic_cast<PrimaryExpression*>(left.get());
        primary && primary->primary_type == PrimitiveType::IDENTIFIER &&
        primary->get_value() == "agc" && parser.check(TokenName::AG_COLON)) {
      const SourceLocation path_location = primary->location;
      parser.advance(); // consume ':'

      const auto builtin_name = builtin_name_from_token(parser.current_token());
      if (!builtin_name) {
        print_error(error::ExpectedFunctionName, parser);
        return nullptr;
      }

      auto qualified      = AG_cscope<QualifiedNameExpression>();
      qualified->path     = "agc:" + std::string(*builtin_name);
      qualified->location = path_location;
      parser.advance();

      left = std::move(qualified);
      continue;
    }

    if (parser.check(TokenName::AG_DOUBLE_COLON)) {
      std::string    first_segment;
      SourceLocation path_location;

      if (auto* primary = dynamic_cast<PrimaryExpression*>(left.get());
          primary && primary->primary_type == PrimitiveType::IDENTIFIER) {
        first_segment = std::string(primary->get_value());
        path_location = primary->location;
      } else if (auto* qualified = dynamic_cast<QualifiedNameExpression*>(left.get())) {
        first_segment = qualified->path;
        path_location = qualified->location;
      } else {
        print_error(error::ExpectedModuleName, parser);
        return nullptr;
      }

      left = QualifiedNameExpression::parse(parser, std::move(first_segment), path_location);
      if (!left)
        return nullptr;

      // A qualified struct or enum literal, e.g. `geometry::Point { x: 1 }`.
      // The brace is only consumed when the path actually names a type; this
      // keeps `match value { ... }` from being mistaken for a literal.
      if (parser.check(TokenName::AG_LEFT_BRACE)) {
        auto* qualified = dynamic_cast<QualifiedNameExpression*>(left.get());
        const std::string resolved_type = parser.resolve_type_name(qualified->path);
        if (get_struct_table().lookup(resolved_type) ||
            resolve_enum_variant(parser, qualified->path)) {
          left = StructLiteralExpression::parse(parser, qualified->path, qualified->location);
          if (!left)
            return nullptr;
        }
      }
      continue;
    }

    // `name<T>(...)`: explicit generic arguments are erased before the call.
    if (skip_generic_call_arguments(parser))
      continue;

    if (parser.check(TokenName::AG_LEFT_PAREN)) {
      left = CallExpression::parse(parser, std::move(left));
      if (!left)
        return nullptr;
      continue;
    }

    if (parser.check(TokenName::AG_LEFT_BRACKET)) {
      const Token bracket_token = parser.current_token();
      parser.advance();

      AG_scope<Expression> index;
      if (parser.check(TokenName::AG_DOT_DOT)) {
        // `arr[..end]` and `arr[..]`
        index = RangeExpression::parse_open_start(parser);
      } else {
        index = parse_pratt(parser, 0.0f);
        if (index && parser.check(TokenName::AG_DOT_DOT)) {
          // `arr[start..end]`, `arr[start..=end]`, and `arr[start..]`
          index = RangeExpression::parse_after_start(parser, std::move(index));
        }
      }
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

    // Postfix `?` (try/propagate) and `!` (unwrap) are accepted as no-ops
    // until Result/Option unwrapping exists at runtime.  A `?` that is
    // followed by the start of an expression is the ternary operator instead.
    if (parser.check(TokenName::AG_QUESTION)) {
      const auto next = parser.peek_token().token_name;
      const bool postfix_try =
          next == TokenName::AG_SEMICOLON || next == TokenName::AG_COMMA ||
          next == TokenName::AG_RIGHT_PAREN || next == TokenName::AG_RIGHT_BRACKET ||
          next == TokenName::AG_RIGHT_BRACE || next == TokenName::AG_DOT ||
          next == TokenName::AG_ARROW || next == TokenName::AG_COLON ||
          next == TokenName::AG_QUESTION || next == TokenName::AG_LOGICAL_NOT ||
          next == TokenName::AG_EOF;
      if (postfix_try) {
        parser.advance();
        continue;
      }
    }

    if (parser.check(TokenName::AG_LOGICAL_NOT)) {
      parser.advance();
      continue;
    }

    if (parser.check(TokenName::AG_INCREMENT) || parser.check(TokenName::AG_DECREMENT)) {
      left = UniaryExpression::parse_postfix(parser, std::move(left));
      if (!left)
        return nullptr;
      continue;
    }

    // `value as Type` converts the operand to another primitive type.
    if (parser.check(TokenName::AG_AS)) {
      const Token as_token = parser.current_token();
      parser.advance();

      auto target_type = parse_type(parser);
      if (!target_type)
        return nullptr;

      auto cast          = AG_cscope<CastExpression>();
      cast->operand      = std::move(left);
      cast->target_type  = *target_type;
      cast->location     = {as_token.line, as_token.column};
      left               = std::move(cast);
      continue;
    }

    break;
  }

  // `value |> fn(args)` pipes the left value in as the first argument,
  // becoming `fn(value, args)`.  Pipelines associate to the left.
  while (min_bp <= 2.5f && parser.check(TokenName::AG_PIPE_GT)) {
    parser.advance();

    auto right = parse_pratt(parser, 2.6f);
    if (!right)
      return nullptr;

    auto call = AG_cscope<CallExpression>();
    call->location = left->source_location({});

    if (auto* parsed_call = dynamic_cast<CallExpression*>(right.get())) {
      call->callee = std::move(parsed_call->callee);
      call->arguments.push_back(std::move(left));
      for (auto& argument : parsed_call->arguments)
        call->arguments.push_back(std::move(argument));
    } else {
      call->callee = std::move(right);
      call->arguments.push_back(std::move(left));
    }

    left = std::move(call);
  }

  // `needle in haystack` tests membership in a range or an array literal.
  // `in` is contextual (also used by `for ... in ...`), so it stays an
  // identifier token and is recognised by spelling here.
  while (min_bp <= 7.0f && parser.check(TokenName::AG_IDENTIFIER) &&
         parser.current_token().value == "in") {
    const Token in_token = parser.current_token();
    parser.advance();

    AG_scope<Expression> haystack;
    if (parser.check(TokenName::AG_DOT_DOT)) {
      haystack = RangeExpression::parse_open_start(parser);
    } else {
      haystack = parse_pratt(parser, 7.1f);
      if (haystack && parser.check(TokenName::AG_DOT_DOT))
        haystack = RangeExpression::parse_after_start(parser,
                                                      std::move(haystack));
    }
    if (!haystack)
      return nullptr;

    auto in_expr      = AG_cscope<InExpression>();
    in_expr->needle   = std::move(left);
    in_expr->haystack = std::move(haystack);
    in_expr->location = {in_token.line, in_token.column};
    left              = std::move(in_expr);
  }

  TokenName temp_token;
  while (true) {
    if (!parser.check_one_of(binary_list, &temp_token)) {
      break;
    }
    const bool   compound = is_compound_assignment_token(temp_token);
    BIN_OPERATOR bin_op   = get_binary_operator(temp_token);
    auto [l_bp, r_bp]     = compound ? std::tuple<float, float>{1.1f, 1.0f}
                                     : get_binding_power(bin_op);
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
    binary->is_compound_assignment = compound;
    binary->location = {operator_token.line, operator_token.column};
    left             = std::move(binary);
  }

  // `condition ? then : else` binds looser than the binary operators but
  // tighter than assignment.  Both arms are ordinary expressions.
  const Token question_token = parser.current_token();
  if (min_bp <= 2.0f && parser.match(TokenName::AG_QUESTION)) {
    auto then_expression = parse_pratt(parser, 0.0f);
    if (!then_expression)
      return nullptr;
    if (!parser.expect(TokenName::AG_COLON, error::ExpectedColon))
      return nullptr;
    auto else_expression = parse_pratt(parser, 2.0f);
    if (!else_expression)
      return nullptr;

    auto conditional         = AG_cscope<ConditionalExpression>();
    conditional->condition   = std::move(left);
    conditional->then_branch = std::move(then_expression);
    conditional->else_branch = std::move(else_expression);
    conditional->location    = {question_token.line, question_token.column};
    left                     = std::move(conditional);
  }

  // `channel <- value` sends a value; the value is the expression result.
  while (min_bp <= 1.05f && parser.check(TokenName::AG_CHANNEL_SEND)) {
    const Token send_token = parser.current_token();
    parser.advance();

    auto value = parse_pratt(parser, 1.05f);
    if (!value)
      return nullptr;

    auto send      = AG_cscope<ChannelSendExpression>();
    send->channel  = std::move(left);
    send->value    = std::move(value);
    send->location = {send_token.line, send_token.column};
    left           = std::move(send);
  }

  // `value ??= fallback` computes `value ?? fallback` and stores the result
  // back into the target.  The target is re-parsed from the start of this
  // expression so the same lvalue can appear on both sides.
  if (min_bp <= 1.0f && parser.check(TokenName::AG_COALESCE_ASSIGN)) {
    const size_t after_left = parser.get_current_position();
    parser.set_current_position(left_start);
    auto target = parse_pratt(parser, 2.7f);
    if (!target)
      return nullptr;
    parser.set_current_position(after_left);
    parser.advance(); // `??=`

    auto fallback = parse_pratt(parser, 1.0f);
    if (!fallback)
      return nullptr;

    const SourceLocation location = target->source_location({});

    auto orelse      = AG_cscope<OrelseExpression>();
    orelse->location = location;
    orelse->value    = std::move(left);
    orelse->fallback = std::move(fallback);

    auto assignment = AG_cscope<BinaryExpression>(BIN_OPERATOR::ASSIGN_TO,
                                                  std::move(target),
                                                  std::move(orelse));
    assignment->location = location;
    left                 = std::move(assignment);
  }

  // `value orelse fallback` and `value ?? fallback` bind looser than the
  // ternary operator and associate to the right.  The fallback may be a
  // `return` statement expression.
  while (min_bp <= 1.5f &&
         (parser.check(TokenName::AG_ORELSE) ||
          parser.check(TokenName::AG_COALESCE))) {
    parser.advance();

    AG_scope<Expression> fallback;
    if (parser.check(TokenName::AG_RETURN))
      fallback = ReturnExpression::parse(parser);
    else
      fallback = parse_pratt(parser, 1.5f);
    if (!fallback)
      return nullptr;

    auto orelse       = AG_cscope<OrelseExpression>();
    orelse->value     = std::move(left);
    orelse->fallback  = std::move(fallback);
    orelse->location  = orelse->value ? orelse->value->location : SourceLocation{};
    left              = std::move(orelse);
  }

  return left;
}

AG_scope<Expression>
LoopExpression::parse(Parser& parser) {
  const Token loop_token = parser.current_token();
  parser.advance(); // `loop`

  auto result      = AG_cscope<LoopExpression>();
  result->location = {loop_token.line, loop_token.column};

  if (!parser.check(TokenName::AG_LEFT_BRACE)) {
    print_error(error::ExpectedLeftBrace, parser);
    return nullptr;
  }
  result->body = BodyStatement::ParseBlock(parser);
  if (!result->body)
    return nullptr;
  return result;
}

PrimitiveType
LoopExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
LoopExpression::get_data_type() {
  return result_type;
}

bool
LoopExpression::analyze_semantics(Parser&               parser,
                                  const SourceLocation& location) {
  result_type                 = DataType{};
  result_type.kind            = TypeKind::PRIMITIVE;
  result_type.primitive_type  = PrimitiveType::INT;
  result_type.size_in_bytes   = 8;
  result_type.alignment       = 8;

  parser.enter_loop();
  const bool body_ok = body &&
                       body->analyze_semantics(parser);
  parser.exit_loop();
  return body_ok;
}

std::string
LoopExpression::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  if (!body)
    return "0";

  const auto body_label = "b" + std::to_string(counter.block_label++);
  const auto end_label  = "b" + std::to_string(counter.block_label++);

  const auto result = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Assign>("0", result, counter.function_index));

  parser.add_instructios(AG_cscope<Label>(body_label));
  parser.push_loop_labels(end_label, body_label, label, result);
  body->generate_ir(counter, parser);
  parser.pop_loop_labels();
  parser.add_instructios(AG_cscope<Jump>(body_label));
  parser.add_instructios(AG_cscope<Label>(end_label));
  return result;
}

AG_scope<Expression>
FStringExpression::parse(Parser&               parser,
                        std::string_view      raw,
                        const SourceLocation& location) {
  auto result      = AG_cscope<FStringExpression>();
  result->location = location;

  result->literals.emplace_back("");
  result->empty_literal_index =
      Add_Literals::add_string_literals(result->literals.back());

  const auto decode_literal = [](std::string_view text) {
    std::string decoded;
    decoded.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
      const char current = text[i];
      if (current != '\\' || i + 1 >= text.size()) {
        decoded.push_back(current);
        continue;
      }

      const char escaped = text[++i];
      switch (escaped) {
      case 'n':
        decoded.push_back('\n');
        break;
      case 'r':
        decoded.push_back('\r');
        break;
      case 't':
        decoded.push_back('\t');
        break;
      case '0':
        decoded.push_back('\0');
        break;
      case 'a':
        decoded.push_back('\a');
        break;
      case 'b':
        decoded.push_back('\b');
        break;
      case 'f':
        decoded.push_back('\f');
        break;
      case 'v':
        decoded.push_back('\v');
        break;
      case '\\':
      case '"':
      case '\'':
        decoded.push_back(escaped);
        break;
      default:
        decoded.push_back(escaped);
        break;
      }
    }
    return decoded;
  };

  const auto parse_format = [](std::string_view spec) {
    FStringFormatSpec format;
    size_t            index = 0;

    if (index < spec.size() && spec[index] == '#') {
      format.alternate = true;
      ++index;
    }

    // Zero padding such as `06d` or `0>6`.
    if (index + 1 < spec.size() && spec[index] == '0' &&
        spec[index + 1] >= '0' && spec[index + 1] <= '9') {
      format.fill  = '0';
      format.align = '>';
      ++index;
    }

    const auto is_align = [](char c) { return c == '>' || c == '<' || c == '^'; };

    if (index + 1 < spec.size() && is_align(spec[index + 1])) {
      format.fill  = spec[index];
      format.align = spec[index + 1];
      index += 2;
    } else if (index < spec.size() && is_align(spec[index])) {
      format.align = spec[index];
      ++index;
    }

    while (index < spec.size() && spec[index] >= '0' && spec[index] <= '9') {
      format.width = format.width * 10 + (spec[index] - '0');
      ++index;
    }

    if (index < spec.size() && spec[index] == '.') {
      ++index;
      format.precision = 0;
      while (index < spec.size() && spec[index] >= '0' && spec[index] <= '9') {
        format.precision = format.precision * 10 + (spec[index] - '0');
        ++index;
      }
    }

    if (index < spec.size())
      format.type = spec[index];

    return format;
  };

  std::string literal;
  const auto  flush_literal = [&]() {
    if (literal.empty())
      return;
    FStringPart part;
    part.is_expression = false;
    result->literals.emplace_back(decode_literal(literal));
    part.literal_index =
        Add_Literals::add_string_literals(result->literals.back());
    result->parts.push_back(std::move(part));
    literal.clear();
  };

  const size_t size = raw.size();
  for (size_t i = 0; i < size;) {
    const char current = raw[i];

    // `{{` and `}}` are escaped literal braces.
    if (current == '{' && i + 1 < size && raw[i + 1] == '{') {
      literal.push_back('{');
      i += 2;
      continue;
    }
    if (current == '}' && i + 1 < size && raw[i + 1] == '}') {
      literal.push_back('}');
      i += 2;
      continue;
    }

    if (current != '{') {
      literal.push_back(current);
      ++i;
      continue;
    }

    // Find the matching closing brace, ignoring braces and colons inside
    // nested expressions and quoted strings.
    size_t close = i + 1;
    int    depth = 1;
    char   quote = 0;
    for (; close < size; ++close) {
      const char c = raw[close];
      if (quote != 0) {
        if (c == '\\' && close + 1 < size) {
          ++close;
        } else if (c == quote) {
          quote = 0;
        }
        continue;
      }
      if (c == '"' || c == '\'') {
        quote = c;
      } else if (c == '{') {
        ++depth;
      } else if (c == '}') {
        if (--depth == 0)
          break;
      }
    }

    if (close >= size) {
      // Unterminated expression; treat the brace as literal text.
      literal.push_back(current);
      ++i;
      continue;
    }

    flush_literal();

    std::string_view content = raw.substr(i + 1, close - i - 1);

    // A top-level `:` introduces the format spec.  Skip `::` so module paths
    // such as `ag::io::println` are not mistaken for a separator.
    size_t separator = std::string_view::npos;
    {
      int  nested       = 0;
      char format_quote = 0;
      for (size_t j = 0; j < content.size(); ++j) {
        const char c = content[j];
        if (format_quote != 0) {
          if (c == '\\' && j + 1 < content.size()) {
            ++j;
          } else if (c == format_quote) {
            format_quote = 0;
          }
          continue;
        }
        if (c == '"' || c == '\'') {
          format_quote = c;
        } else if (c == '(' || c == '[' || c == '{') {
          ++nested;
        } else if (c == ')' || c == ']' || c == '}') {
          --nested;
        } else if (c == ':' && nested == 0) {
          if (j + 1 < content.size() && content[j + 1] == ':') {
            ++j;
            continue;
          }
          separator = j;
          break;
        }
      }
    }

    std::string_view expression_text = content;
    std::string_view format_text;
    if (separator != std::string_view::npos) {
      expression_text = content.substr(0, separator);
      format_text     = content.substr(separator + 1);
    }

    const auto trim = [](std::string_view& text) {
      while (!text.empty() &&
             (text.front() == ' ' || text.front() == '\t' || text.front() == '\n'))
        text.remove_prefix(1);
      while (!text.empty() &&
             (text.back() == ' ' || text.back() == '\t' || text.back() == '\n'))
        text.remove_suffix(1);
    };
    trim(expression_text);
    trim(format_text);

    FStringPart part;
    part.is_expression = true;
    part.location      = location;
    part.expression    = parser.parse_inline_expression(expression_text, location);
    if (!part.expression)
      return nullptr;
    part.format = parse_format(format_text);
    result->parts.push_back(std::move(part));

    i = close + 1;
  }

  flush_literal();
  return result;
}

PrimitiveType
FStringExpression::get_type() {
  return PrimitiveType::STRING;
}

DataType
FStringExpression::get_data_type() {
  DataType type;
  type.kind           = TypeKind::PRIMITIVE;
  type.primitive_type = PrimitiveType::STRING;
  type.size_in_bytes  = 8;
  type.alignment      = 8;
  return type;
}

bool
FStringExpression::analyze_semantics(Parser&               parser,
                                     const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);
  for (auto& part : parts) {
    if (!part.is_expression || !part.expression)
      continue;
    if (!part.expression->analyze_semantics(
            parser, part.expression->source_location(diagnostic_location)))
      return false;
  }
  return true;
}

std::string
FStringExpression::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  // Start from the empty string and append each segment.  The runtime
  // concatenation helper allocates a fresh buffer, so the intermediate values
  // are safe to keep on the IR value stack.
  std::string accumulated = "str." + std::to_string(empty_literal_index);

  const auto emit_call = [&](std::string_view name,
                             const std::vector<std::string>& arguments) {
    for (const auto& argument : arguments)
      parser.add_instructios(AG_cscope<Param>(argument, counter.function_index));
    const auto result = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<FunctionCall>(std::string(name), counter.function_index, result));
    return result;
  };

  const auto is_string_like = [](const DataType& type) {
    if (type.primitive_type == PrimitiveType::STRING)
      return true;
    if ((type.kind == TypeKind::POINTER || type.kind == TypeKind::REFERENCE) &&
        type.pointee && type.pointee->kind == TypeKind::PRIMITIVE &&
        type.pointee->primitive_type == PrimitiveType::CHAR)
      return true;
    return false;
  };

  for (auto& part : parts) {
    std::string value;
    bool        numeric = false;

    if (!part.is_expression) {
      value = "str." + std::to_string(part.literal_index);
    } else {
      const DataType type = part.expression->get_data_type();
      value               = part.expression->generate_ir(counter, parser);

      const bool is_float =
          type.kind == TypeKind::PRIMITIVE &&
          (type.primitive_type == PrimitiveType::FLOAT ||
           type.primitive_type == PrimitiveType::DOUBLE);

      if (is_float && part.format.type != 'x' && part.format.type != 'X' &&
          part.format.type != 'd' && part.format.type != 'o' &&
          part.format.type != 'b') {
        const int precision = part.format.precision >= 0 ? part.format.precision : 6;
        value = emit_call("__ag_float_to_string",
                          {value, std::to_string(precision)});
      } else if (is_string_like(type)) {
        // Already a byte address.
      } else if (type.kind == TypeKind::PRIMITIVE &&
                 type.primitive_type == PrimitiveType::CHAR) {
        value = emit_call("__ag_char_to_string", {value});
      } else if (part.format.type == 'x' || part.format.type == 'X') {
        numeric = true;
        value   = emit_call("__ag_int_to_hex",
                          {value,
                           part.format.type == 'X' ? "1" : "0",
                           part.format.alternate ? "1" : "0"});
      } else {
        numeric = true;
        value   = emit_call("__ag_int_to_string", {value});
      }

      if (part.format.width > 0) {
        // Numbers align right by default; strings align left unless the
        // format selects an explicit alignment.
        char align = part.format.align;
        if (align == 0)
          align = numeric ? '>' : '<';
        const char fill = part.format.fill != 0 ? part.format.fill : ' ';
        value = emit_call(align == '<' ? "__ag_pad_right" : "__ag_pad_left",
                          {value,
                           std::to_string(part.format.width),
                           std::to_string(static_cast<unsigned char>(fill))});
      }
    }

    accumulated = emit_call("__ag_str_concat", {accumulated, value});
  }

  return accumulated;
}

AG_scope<Expression>
PrimaryExpression::parse_primary(Parser& parser) {
  // `map { ... }` map literal.  `map` is contextual so it remains usable as an
  // ordinary identifier elsewhere.
  if (parser.check(TokenName::AG_IDENTIFIER) &&
      parser.current_token().value == "map" &&
      parser.peek_token().token_name == TokenName::AG_LEFT_BRACE)
    return MapLiteralExpression::parse(parser);

  if (parser.check(TokenName::AG_LEFT_BRACKET))
    return ArrayLiteralExpression::parse(parser);

  if (parser.check(TokenName::AG_MATCH)) {
    return MatchExpression::parse(parser);
  }

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
  if (!parser.check_one_of({TokenName::AG_STRING_LITERAL,
                            TokenName::AG_FSTRING,
                            TokenName::AG_CHAR_LITERAL,
                            TokenName::AG_INT_LITERAL,
                            TokenName::AG_BOOL_LITERAL,
                            TokenName::AG_FLOAT_LITERAL,
                            TokenName::AG_NULLPTR,
                            TokenName::AG_IDENTIFIER},
                           &temp_token)) {
    return nullptr;
  }

  if (temp_token == TokenName::AG_FSTRING) {
    const Token token = parser.current_token();
    parser.advance();
    return FStringExpression::parse(parser,
                                    token.value,
                                    {token.line, token.column});
  }

  auto primary_exp = AG_cscope<PrimaryExpression>();
  primary_exp->location = {parser.current_token().line, parser.current_token().column};
  if (temp_token == TokenName::AG_STRING_LITERAL) {
    primary_exp->primary_type = PrimitiveType::STRING;
    primary_exp->value        = parser.current_token().value;
    primary_exp->index        = Add_Literals::add_string_literals(primary_exp->value);
    primary_exp->declared_type       = make_builtin_type("string");
    primary_exp->declared_type_known = true;
  } else if (temp_token == TokenName::AG_CHAR_LITERAL) {
    primary_exp->primary_type = PrimitiveType::CHAR;
    primary_exp->value        = parser.current_token().value;
    primary_exp->declared_type       = make_builtin_type("char");
    primary_exp->declared_type_known = true;
  } else if (temp_token == TokenName::AG_INT_LITERAL) {
    const auto& token = parser.current_token();
    const std::string_view spelling =
        token.literal_type_name.empty() ? std::string_view("int")
                                        : token.literal_type_name;
    primary_exp->primary_type        = PrimitiveType::INT;
    primary_exp->value               = token.value;
    primary_exp->declared_type       = make_builtin_type(spelling);
    primary_exp->declared_type_known = true;
  } else if (temp_token == TokenName::AG_BOOL_LITERAL) {
    primary_exp->primary_type        = PrimitiveType::BOOL;
    primary_exp->value               = parser.current_token().value;
    primary_exp->declared_type       = make_builtin_type("bool");
    primary_exp->declared_type_known = true;
  } else if (temp_token == TokenName::AG_FLOAT_LITERAL) {
    primary_exp->primary_type = PrimitiveType::FLOAT;
    primary_exp->value        = parser.current_token().value;
    primary_exp->declared_type       = make_builtin_type("float");
    primary_exp->declared_type_known = true;
  } else if (temp_token == TokenName::AG_NULLPTR) {
    primary_exp->primary_type   = PrimitiveType::VOID;
    primary_exp->value          = "0";
    primary_exp->is_null_pointer = true;
  } else if (temp_token == TokenName::AG_IDENTIFIER) {
    primary_exp->primary_type = PrimitiveType::IDENTIFIER;
    primary_exp->value        = parser.current_token().value;
  }
  parser.advance();

  // `Point { x: 3, y: 4 }` is a struct literal.  The identifier has already
  // been consumed, so the literal parser starts at the opening brace.  Only
  // consume the brace when the name is a known type so a match scrutinee such
  // as `match value { ... }` is parsed as an identifier followed by the match
  // body.
  if (primary_exp->primary_type == PrimitiveType::IDENTIFIER &&
      parser.check(TokenName::AG_LEFT_BRACE)) {
    const std::string resolved_type = parser.resolve_type_name(primary_exp->value);
    if (get_struct_table().lookup(resolved_type) ||
        resolve_enum_variant(parser, primary_exp->value)) {
      return StructLiteralExpression::parse(parser,
                                            std::string(primary_exp->value),
                                            primary_exp->location);
    }
  }

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
  return get_data_type().primitive_type;
}

DataType
MemberExpression::get_data_type() {
  if (member_found)
    return member_type;

  if (object) {
    auto object_type = unwrap_struct_type(object->get_data_type());
    if (object_type.kind == TypeKind::STRUCT) {
      if (const auto* definition = get_struct_table().lookup(object_type.type_name)) {
        if (const auto* member = definition->find_member(field_name))
          return member->type;
      }
    }
  }

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

bool
MemberExpression::analyze_semantics(Parser&               parser,
                                    const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (!object)
    return false;
  if (!object->analyze_semantics(parser, object->source_location(diagnostic_location)))
    return false;

  auto object_type = unwrap_struct_type(object->get_data_type());
  if (object_type.primitive_type == PrimitiveType::UNEXPECTED)
    return false;

  if (object_type.kind != TypeKind::STRUCT) {
    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }

  const auto* definition = get_struct_table().lookup(object_type.type_name);
  if (!definition) {
    print_error(error::UndefinedType, parser, diagnostic_location);
    return false;
  }

  const auto* member = definition->find_member(field_name);
  if (!member) {
    print_error(error::UndefinedMember, parser, diagnostic_location);
    return false;
  }

  if (!is_member_visible(parser, *definition, *member)) {
    print_error(error::PrivateMember, parser, diagnostic_location);
    return false;
  }

  member_type  = member->type;
  byte_offset  = member->byte_offset;
  member_found = true;
  return true;
}

std::string
MemberExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  if (!member_found || !object)
    return "";

  const auto object_value = object->generate_ir(counter, parser);
  const auto result       = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<MemberLoad>(
      result,
      object_value,
      static_cast<uint32_t>(byte_offset),
      counter.function_index,
      memory_width_of(member_type),
      memory_is_signed(member_type)));
  return result;
}

AG_scope<Expression>
RangeExpression::parse_open_start(Parser& parser) {
  const Token dot_token = parser.current_token();
  if (!parser.match(TokenName::AG_DOT_DOT))
    return nullptr;

  auto range      = AG_cscope<RangeExpression>();
  range->location = {dot_token.line, dot_token.column};
  range->inclusive = parser.match(TokenName::AG_ASSIGN);

  const bool at_delimiter =
      parser.check(TokenName::AG_RIGHT_BRACKET) ||
      parser.check(TokenName::AG_LEFT_BRACE) ||
      parser.check(TokenName::AG_COMMA) ||
      parser.check(TokenName::AG_SEMICOLON) ||
      parser.check(TokenName::AG_EOF) ||
      (parser.check(TokenName::AG_IDENTIFIER) &&
       parser.current_token().value == "by");

  if (!at_delimiter) {
    range->end = Expression::parse_pratt(parser, 0.0f);
    if (!range->end)
      return nullptr;
    range->has_end = true;
  }
  return range;
}

AG_scope<Expression>
RangeExpression::parse_after_start(Parser&              parser,
                                   AG_scope<Expression> start) {
  const Token dot_token = parser.current_token();
  if (!parser.match(TokenName::AG_DOT_DOT))
    return nullptr;

  auto range       = AG_cscope<RangeExpression>();
  range->location  = {dot_token.line, dot_token.column};
  range->start     = std::move(start);
  range->has_start = true;
  range->inclusive = parser.match(TokenName::AG_ASSIGN);

  const bool at_delimiter =
      parser.check(TokenName::AG_RIGHT_BRACKET) ||
      parser.check(TokenName::AG_LEFT_BRACE) ||
      parser.check(TokenName::AG_COMMA) ||
      parser.check(TokenName::AG_SEMICOLON) ||
      parser.check(TokenName::AG_EOF) ||
      (parser.check(TokenName::AG_IDENTIFIER) &&
       parser.current_token().value == "by");

  if (!at_delimiter) {
    range->end = Expression::parse_pratt(parser, 0.0f);
    if (!range->end)
      return nullptr;
    range->has_end = true;
  }
  return range;
}

bool
RangeExpression::analyze_semantics(Parser&               parser,
                                   const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (start) {
    if (!start->analyze_semantics(parser, start->source_location(diagnostic_location)))
      return false;
    if (!is_integer_like_type(start->get_data_type())) {
      print_error(error::InvalidOperandTypes, parser,
                  start->source_location(diagnostic_location));
      return false;
    }
  }

  if (end) {
    if (!end->analyze_semantics(parser, end->source_location(diagnostic_location)))
      return false;
    if (!is_integer_like_type(end->get_data_type())) {
      print_error(error::InvalidOperandTypes, parser,
                  end->source_location(diagnostic_location));
      return false;
    }
  }
  return true;
}

PrimitiveType
RangeExpression::get_type() {
  return PrimitiveType::INT;
}

DataType
RangeExpression::get_data_type() {
  DataType type;
  type.primitive_type = PrimitiveType::INT;
  return type;
}

AG_scope<Expression>
ArrayLiteralExpression::parse(Parser& parser) {
  const Token bracket_token = parser.current_token();
  if (!parser.match(TokenName::AG_LEFT_BRACKET))
    return nullptr;

  auto literal      = AG_cscope<ArrayLiteralExpression>();
  literal->location = {bracket_token.line, bracket_token.column};

  if (!parser.check(TokenName::AG_RIGHT_BRACKET)) {
    while (true) {
      auto element = Expression::parse_expressions(parser);
      if (!element)
        return nullptr;
      literal->elements.push_back(std::move(element));

      if (!parser.match(TokenName::AG_COMMA))
        break;
      if (parser.check(TokenName::AG_RIGHT_BRACKET))
        break;
    }
  }

  if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket))
    return nullptr;
  return literal;
}

bool
ArrayLiteralExpression::analyze_semantics(Parser&               parser,
                                          const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (elements.empty()) {
    result_type = make_void_pointer();
    return true;
  }

  DataType element_type;
  bool     have_element_type = false;
  for (const auto& element : elements) {
    if (!element->analyze_semantics(parser, element->source_location(diagnostic_location)))
      return false;

    const auto type = element->get_data_type();
    if (!have_element_type) {
      element_type      = type;
      have_element_type = true;
    } else if (!is_compatible_type(element_type, type) &&
               !is_compatible_type(type, element_type)) {
      print_error(error::IncompatibleTypes,
                  parser,
                  element->source_location(diagnostic_location));
      return false;
    }
  }

  result_type.kind           = TypeKind::POINTER;
  result_type.primitive_type = PrimitiveType::USER_DEFINED;
  result_type.pointee        = std::make_shared<DataType>(element_type);
  result_type.array_length   = elements.size();
  result_type.size_in_bytes  = 8;
  result_type.alignment      = 8;
  return true;
}

PrimitiveType
ArrayLiteralExpression::get_type() {
  return result_type.primitive_type;
}

DataType
ArrayLiteralExpression::get_data_type() {
  return result_type;
}

std::string
ArrayLiteralExpression::generate_ir(LabelCounters& counter,
                                    Parser&        parser) {
  if (elements.empty())
    return "0";

  // Elements are packed using their declared storage width.  Most values are
  // eight-byte slots, while `bool`/`u8`/`i8` occupy one byte and the wider
  // fixed-width integers occupy two or four.
  const size_t element_width =
      result_type.pointee ? memory_width_of(*result_type.pointee) : 8;
  parser.add_instructios(AG_cscope<Param>(
      std::to_string(elements.size() * element_width),
      counter.function_index));
  const auto result = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));

  for (size_t i = 0; i < elements.size(); ++i) {
    const auto value = elements[i]->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<MemberStore>(
        result,
        static_cast<uint32_t>(i * element_width),
        value,
        counter.function_index,
        element_width,
        result_type.pointee ? result_type.pointee->is_signed : true));
  }
  return result;
}

AG_scope<Expression>
MapLiteralExpression::parse(Parser& parser) {
  const Token map_token = parser.current_token();
  parser.advance(); // `map`

  auto literal      = AG_cscope<MapLiteralExpression>();
  literal->location = {map_token.line, map_token.column};

  if (!parser.expect(TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace))
    return nullptr;

  if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    while (true) {
      Entry entry;
      entry.location = {parser.current_token().line,
                        parser.current_token().column};

      entry.key = Expression::parse_pratt(parser, 0.0f);
      if (!entry.key)
        return nullptr;
      if (!parser.expect(TokenName::AG_COLON, error::ExpectedColon))
        return nullptr;
      entry.value = Expression::parse_pratt(parser, 0.0f);
      if (!entry.value)
        return nullptr;

      literal->entries.push_back(std::move(entry));
      if (!parser.match(TokenName::AG_COMMA))
        break;
      if (parser.check(TokenName::AG_RIGHT_BRACE))
        break;
    }
  }

  if (!parser.expect(TokenName::AG_RIGHT_BRACE, error::ExpectedRightBrace))
    return nullptr;
  return literal;
}

std::string
MapLiteralExpression::generate_ir(LabelCounters& counter,
                                  Parser&        parser) {
  if (entries.empty())
    return "0";

  // Layout: [count][key 0][value 0][key 1][value 1]...
  parser.add_instructios(
      AG_cscope<Param>(std::to_string(8 + entries.size() * 16),
                       counter.function_index));
  const auto result = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));
  parser.add_instructios(AG_cscope<MemberStore>(
      result, 0, std::to_string(entries.size()), counter.function_index));

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto key   = entries[i].key->generate_ir(counter, parser);
    const auto value = entries[i].value->generate_ir(counter, parser);
    const auto base  = static_cast<uint32_t>(8 + i * 16);
    parser.add_instructios(AG_cscope<MemberStore>(
        result, base, key, counter.function_index));
    parser.add_instructios(AG_cscope<MemberStore>(
        result, base + 8, value, counter.function_index));
  }
  return result;
}

bool
MapLiteralExpression::analyze_semantics(Parser&               parser,
                                        const SourceLocation& location) {
  if (entries.empty())
    return true;

  for (auto& entry : entries) {
    if (!entry.key->analyze_semantics(parser,
                                      entry.key->source_location(entry.location)))
      return false;
    if (!entry.value->analyze_semantics(parser,
                                        entry.value->source_location(entry.location)))
      return false;
  }

  key_type   = entries.front().key->get_data_type();
  value_type = entries.front().value->get_data_type();
  return true;
}

PrimitiveType
MapLiteralExpression::get_type() {
  return PrimitiveType::USER_DEFINED;
}

DataType
MapLiteralExpression::get_data_type() {
  auto pointee = std::make_shared<DataType>(value_type);

  DataType type;
  type.kind           = TypeKind::POINTER;
  type.primitive_type = PrimitiveType::USER_DEFINED;
  type.pointee        = std::move(pointee);
  type.size_in_bytes  = 8;
  type.alignment      = 8;
  type.is_map         = true;
  type.map_key_type   = std::make_shared<DataType>(key_type);
  type.array_length   = entries.size();
  return type;
}

PrimitiveType
TupleExpression::get_type() {
  return result_type.primitive_type;
}

DataType
TupleExpression::get_data_type() {
  return result_type;
}

bool
TupleExpression::analyze_semantics(Parser&               parser,
                                   const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  element_types.clear();
  element_types.reserve(elements.size());
  for (const auto& element : elements) {
    if (!element->analyze_semantics(parser,
                                    element->source_location(diagnostic_location)))
      return false;
    element_types.push_back(element->get_data_type());
  }

  result_type.kind           = TypeKind::STRUCT;
  result_type.primitive_type = PrimitiveType::USER_DEFINED;
  result_type.size_in_bytes  = 8;
  result_type.alignment      = 8;
  result_type.tuple_elements.clear();
  for (const auto& type : element_types)
    result_type.tuple_elements.push_back(std::make_shared<DataType>(type));
  return true;
}

std::string
TupleExpression::generate_ir(LabelCounters& counter,
                             Parser&        parser) {
  if (elements.empty())
    return "0";

  parser.add_instructios(AG_cscope<Param>(std::to_string(elements.size() * 8),
                                          counter.function_index));
  const auto result = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));

  for (size_t i = 0; i < elements.size(); ++i) {
    const auto value = elements[i]->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<MemberStore>(result,
                                                  static_cast<uint32_t>(i * 8),
                                                  value,
                                                  counter.function_index));
  }
  return result;
}

PrimitiveType
StructLiteralExpression::get_type() {
  return result_type.primitive_type;
}

DataType
StructLiteralExpression::get_data_type() {
  return result_type;
}

bool
StructLiteralExpression::analyze_semantics(Parser&               parser,
                                           const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  type_name = parser.resolve_type_name(type_name);

  if (const auto* definition = get_struct_table().lookup(type_name)) {
    type_name   = definition->name;
    result_type = make_struct_type(type_name);

    // A struct literal outside the declaring module must not be able to
    // construct a value while skipping private fields.  Such types must be
    // built through a public function provided by their module.
    if (!is_visible_from(false,
                         declaring_module(definition->name),
                         parser.current_module_path())) {
      for (const auto& member : definition->members) {
        if (!member.is_public) {
          print_error(error::PrivateMember, parser, diagnostic_location);
          return false;
        }
      }
    }

    if (spread) {
      if (!spread->analyze_semantics(parser, spread->source_location(diagnostic_location)))
        return false;

      const auto spread_type = unwrap_struct_type(spread->get_data_type());
      if (spread_type.kind != TypeKind::STRUCT || spread_type.type_name != type_name) {
        print_error(error::IncompatibleTypes, parser, spread->source_location(diagnostic_location));
        return false;
      }
    }

    std::unordered_set<std::string> initialized_fields;

    for (const auto& initializer : fields) {
      const auto* member = definition->find_member(initializer.name);
      if (!member) {
        print_error(error::UndefinedMember, parser, initializer.location);
        return false;
      }

      if (!is_member_visible(parser, *definition, *member)) {
        print_error(error::PrivateMember, parser, initializer.location);
        return false;
      }

      if (!initialized_fields.insert(initializer.name).second) {
        print_error(error::DuplicateFieldName, parser, initializer.location);
        return false;
      }

      if (!initializer.value ||
          !initializer.value->analyze_semantics(
              parser, initializer.value->source_location(initializer.location)))
        return false;

      if (!is_compatible_type(member->type, initializer.value->get_data_type())) {
        print_error(error::IncompatibleTypes,
                    parser,
                    initializer.value->source_location(initializer.location));
        return false;
      }
    }

    return true;
  }

  // `Status::Moved { x: 1, y: 2 }` uses the same brace syntax as a struct
  // literal, but the path names an enum variant rather than a struct.
  const auto resolved = resolve_enum_variant(parser, type_name);
  if (!resolved) {
    print_error(error::UndefinedType, parser, diagnostic_location);
    return false;
  }

  if (spread) {
    print_error(error::IncompatibleTypes, parser, spread->source_location(diagnostic_location));
    return false;
  }

  m_is_enum_literal = true;
  m_enum_name       = resolved->definition->name;
  m_variant_index   = resolved->variant->index;
  result_type       = make_enum_type(m_enum_name);

  std::unordered_set<std::string> initialized_fields;
  for (const auto& initializer : fields) {
    const auto* field = resolved->variant->find_field(initializer.name);
    if (!field) {
      print_error(error::UndefinedMember, parser, initializer.location);
      return false;
    }

    if (!initialized_fields.insert(initializer.name).second) {
      print_error(error::DuplicateFieldName, parser, initializer.location);
      return false;
    }

    if (!initializer.value ||
        !initializer.value->analyze_semantics(
            parser, initializer.value->source_location(initializer.location)))
      return false;

    if (!is_compatible_type(field->type, initializer.value->get_data_type())) {
      print_error(error::IncompatibleTypes,
                  parser,
                  initializer.value->source_location(initializer.location));
      return false;
    }
  }

  return true;
}

std::string
StructLiteralExpression::generate_ir(LabelCounters& counter,
                                     Parser&        parser) {
  if (m_is_enum_literal) {
    const auto* definition = get_enum_table().lookup(m_enum_name);
    if (!definition)
      return "0";

    parser.add_instructios(AG_cscope<Param>(std::to_string(definition->size_in_bytes),
                                            counter.function_index));
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));
    parser.add_instructios(AG_cscope<MemberStore>(result,
                                                  0,
                                                  std::to_string(m_variant_index),
                                                  counter.function_index));

    if (m_variant_index >= definition->variants.size())
      return result;
    const auto& variant = definition->variants[m_variant_index];

    for (const auto& initializer : fields) {
      const auto* field = variant.find_field(initializer.name);
      if (!field)
        continue;

      const auto value = initializer.value->generate_ir(counter, parser);
      parser.add_instructios(AG_cscope<MemberStore>(result,
                                                    static_cast<uint32_t>(field->byte_offset),
                                                    value,
                                                    counter.function_index));
    }
    return result;
  }

  const auto* definition = get_struct_table().lookup(type_name);
  if (!definition)
    return "0";

  parser.add_instructios(AG_cscope<Param>(std::to_string(definition->size_in_bytes),
                                          counter.function_index));
  const auto result = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));

  std::unordered_set<std::string> initialized_fields;
  for (const auto& initializer : fields)
    initialized_fields.insert(initializer.name);

  if (spread) {
    const auto spread_value = spread->generate_ir(counter, parser);
    for (const auto& member : definition->members) {
      if (initialized_fields.contains(member.name))
        continue;

      const auto field_value = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<MemberLoad>(
          field_value,
          spread_value,
          static_cast<uint32_t>(member.byte_offset),
          counter.function_index,
          memory_width_of(member.type),
          memory_is_signed(member.type)));
      parser.add_instructios(AG_cscope<MemberStore>(
          result,
          static_cast<uint32_t>(member.byte_offset),
          field_value,
          counter.function_index,
          memory_width_of(member.type),
          memory_is_signed(member.type)));
    }
  }

  for (const auto& initializer : fields) {
    const auto* member = definition->find_member(initializer.name);
    if (!member)
      continue;

    const auto value = initializer.value->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<MemberStore>(
        result,
        static_cast<uint32_t>(member->byte_offset),
        value,
        counter.function_index,
        memory_width_of(member->type),
        memory_is_signed(member->type)));
  }

  return result;
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

  m_is_method = false;
  m_address_arguments.clear();
  m_reference_arguments.clear();
  std::vector<FunctionEntry> candidates;
  std::optional<DataType>    receiver_argument_type;

  if (auto* qualified = dynamic_cast<QualifiedNameExpression*>(callee.get());
      qualified && qualified->path.rfind("agc:", 0) == 0) {
    const std::string builtin_name = qualified->path.substr(4);
    if (auto builtin = make_builtin_entry(builtin_name))
      candidates.push_back(std::move(*builtin));
    else {
      print_error(error::UndefinedFunction, parser, diagnostic_location);
      return false;
    }
  } else if (auto* member_callee = dynamic_cast<MemberExpression*>(callee.get())) {
    // `value.method(...)` resolves against methods registered for the
    // receiver's struct type.  The receiver is passed as the implicit first
    // argument.
    if (!member_callee->object ||
        !member_callee->object->analyze_semantics(
            parser, member_callee->object->source_location(diagnostic_location)))
      return false;

    const auto receiver_type = unwrap_struct_type(member_callee->object->get_data_type());
    if (receiver_type.primitive_type == PrimitiveType::UNEXPECTED)
      return false;
    if (receiver_type.kind != TypeKind::STRUCT &&
        receiver_type.kind != TypeKind::ENUM) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    const auto method_candidates = get_function_table().lookup_all(
        std::string(receiver_type.type_name) + "." + member_callee->field_name);
    bool saw_private_method = false;
    for (const auto& candidate : method_candidates) {
      // Associated functions have no receiver and are only callable through
      // the owner type, not on an instance.
      if (candidate.is_static)
        continue;
      if (!is_function_visible(parser, candidate)) {
        saw_private_method = true;
        continue;
      }
      candidates.push_back(candidate);
    }
    if (candidates.empty()) {
      print_error(saw_private_method ? error::PrivateFunction : error::UndefinedFunction,
                  parser,
                  diagnostic_location);
      return false;
    }

    m_is_method            = true;
    receiver_argument_type = receiver_type;
  } else {
    std::string function_name;

    if (auto* primary = dynamic_cast<PrimaryExpression*>(callee.get());
        primary && primary->primary_type == PrimitiveType::IDENTIFIER) {
      function_name = std::string(primary->get_value());
    } else if (auto* qualified = dynamic_cast<QualifiedNameExpression*>(callee.get())) {
      function_name = qualified->path;
    } else {
      print_error(error::ExpectedFunctionName, parser, diagnostic_location);
      return false;
    }

    const std::string resolved_name = parser.resolve_symbol_name(function_name);
    bool              saw_private_function = false;
    auto collect_visible_candidates = [&](const std::string& lookup_name) {
      std::vector<FunctionEntry> visible;
      for (const auto& candidate : get_function_table().lookup_all(lookup_name)) {
        if (!is_function_visible(parser, candidate)) {
          saw_private_function = true;
          continue;
        }
        visible.push_back(candidate);
      }
      return visible;
    };

    candidates = collect_visible_candidates(resolved_name);
    if (candidates.empty() && resolved_name != function_name)
      candidates = collect_visible_candidates(function_name);

    // Associated functions are registered as `Owner.method`, but are called
    // with type-qualified syntax: `Point::origin(...)`.  Fall back to that
    // spelling when ordinary module/function resolution found nothing.
    if (candidates.empty()) {
      const auto separator = function_name.rfind("::");
      if (separator != std::string::npos) {
        const std::string owner_name = function_name.substr(0, separator);
        const std::string method_name = function_name.substr(separator + 2);
        const std::string owner_type = parser.resolve_type_name(owner_name);

        if (get_struct_table().lookup(owner_type) ||
            get_enum_table().lookup(owner_type)) {
          for (const auto& candidate :
               get_function_table().lookup_all(owner_type + "." + method_name)) {
            if (!candidate.is_static)
              continue;
            if (!is_function_visible(parser, candidate)) {
              saw_private_function = true;
              continue;
            }
            candidates.push_back(candidate);
          }
        }
      }
    }

    if (candidates.empty() && saw_private_function) {
      print_error(error::PrivateFunction, parser, diagnostic_location);
      return false;
    }

    if (candidates.empty()) {
      if (const auto enum_variant = resolve_enum_variant(parser, function_name)) {
        m_is_enum_constructor = true;
        m_enum_name           = enum_variant->definition->name;
        m_variant_index       = enum_variant->variant->index;
        m_result_type         = make_enum_type(m_enum_name);

        if (arguments.size() != enum_variant->variant->fields.size()) {
          print_error(error::InvalidArgumentCount, parser, diagnostic_location);
          return false;
        }

        for (size_t i = 0; i < arguments.size(); ++i) {
          if (!arguments[i]->analyze_semantics(
                  parser, arguments[i]->source_location(diagnostic_location)))
            return false;
          if (!is_compatible_type(enum_variant->variant->fields[i].type,
                                  arguments[i]->get_data_type())) {
            print_error(error::InvalidArgumentType,
                        parser,
                        arguments[i]->source_location(diagnostic_location));
            return false;
          }
        }

        return true;
      }

      std::string builtin_name = function_name;
      if (const auto separator = builtin_name.rfind("::"); separator != std::string::npos)
        builtin_name = builtin_name.substr(separator + 2);

      if (auto builtin = make_builtin_entry(builtin_name))
        candidates.push_back(std::move(*builtin));
      else {
        print_error(error::UndefinedFunction, parser, diagnostic_location);
        return false;
      }
    }
  }

  std::vector<DataType> argument_types;
  argument_types.reserve(arguments.size() + (m_is_method ? 1 : 0));
  if (m_is_method && receiver_argument_type)
    argument_types.push_back(*receiver_argument_type);
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
    // External declarations have no AGC body, but the linker provides their
    // symbol.  Treat them as callable candidates just like defined functions.
    if (!candidate.is_defined && !candidate.is_extern)
      continue;
    saw_defined_candidate = true;

    // A variadic function keeps its variadic parameter last.  Fixed arguments
    // are checked against every parameter except that one; any remaining
    // arguments are checked against its element type.
    const bool   variadic = candidate.is_variadic && !candidate.parameters.empty();
    const size_t fixed_parameters =
        variadic ? candidate.parameters.size() - 1 : candidate.parameters.size();

    if (!variadic) {
      size_t required_parameters = candidate.parameters.size();
      for (size_t i = 0; i < candidate.parameters.size(); ++i) {
        if (candidate.parameters[i].default_value) {
          required_parameters = i;
          break;
        }
      }

      if (argument_types.size() < required_parameters ||
          argument_types.size() > candidate.parameters.size())
        continue;
    } else {
      size_t required_parameters = fixed_parameters;
      for (size_t i = 0; i < fixed_parameters; ++i) {
        if (candidate.parameters[i].default_value) {
          required_parameters = i;
          break;
        }
      }
      if (argument_types.size() < required_parameters)
        continue;
    }
    saw_matching_arity = true;

    bool compatible = true;
    const size_t compared_arguments =
        std::min(argument_types.size(), fixed_parameters);
    for (size_t i = 0; i < compared_arguments; ++i) {
      Expression* argument_expression = nullptr;
      if (m_is_method) {
        if (i > 0 && i - 1 < arguments.size())
          argument_expression = arguments[i - 1].get();
      } else if (i < arguments.size()) {
        argument_expression = arguments[i].get();
      }

      const auto& parameter_type = candidate.parameters[i].type;
      const bool address_argument
          = argument_expression && as_index_target(argument_expression) != nullptr;
      const bool pointer_parameter = is_pointer_like_type(parameter_type);
      const bool implicit_reference_argument
          = argument_expression && parameter_type.kind == TypeKind::REFERENCE &&
            parameter_type.pointee &&
            !is_compatible_type(parameter_type, argument_types[i]) &&
            is_addressable_expression(argument_expression) &&
            is_compatible_type(*parameter_type.pointee, argument_types[i]);
      const bool compatible_argument
          = argument_types[i].primitive_type != PrimitiveType::UNEXPECTED &&
            (is_compatible_type(parameter_type, argument_types[i]) ||
             implicit_reference_argument ||
             (address_argument && pointer_parameter));

      if (!compatible_argument) {
        compatible = false;
        if (!mismatching_argument)
          mismatching_argument = i;
        break;
      }
    }

    if (compatible && variadic) {
      const auto& variadic_type = candidate.parameters.back().type;
      const auto* element_type  = variadic_type.pointee.get();
      for (size_t i = fixed_parameters; i < argument_types.size(); ++i) {
        const size_t argument_index = m_is_method ? i - 1 : i;
        const bool   spread =
            argument_index < m_spread_arguments.size() &&
            m_spread_arguments[argument_index];

        bool compatible_argument = false;
        if (element_type) {
          if (spread) {
            // `...array` passes a pointer to the element type.
            compatible_argument =
                (argument_types[i].kind == TypeKind::POINTER ||
                 argument_types[i].kind == TypeKind::REFERENCE) &&
                argument_types[i].pointee &&
                is_compatible_type(*element_type, *argument_types[i].pointee);
          } else {
            compatible_argument =
                is_compatible_type(*element_type, argument_types[i]);
          }
        }

        if (!compatible_argument) {
          compatible = false;
          if (!mismatching_argument)
            mismatching_argument = i;
          break;
        }
      }
    }

    if (compatible)
      matches.push_back(candidate);
  }

  if (matches.size() > 1) {
    int                         best_score = -1;
    size_t                      best_count = 0;
    std::optional<FunctionEntry> best;
    for (const auto& match : matches) {
      const int score = overload_match_score(match, argument_types, m_is_method);
      if (score > best_score) {
        best_score = score;
        best_count = 1;
        best       = match;
      } else if (score == best_score) {
        ++best_count;
      }
    }

    if (best_count != 1) {
      print_error(error::AmbiguousOverload, parser, diagnostic_location);
      return false;
    }
    resolved = best;
  } else if (matches.size() == 1) {
    resolved = matches.front();
  }

  if (!resolved) {
    if (!saw_defined_candidate) {
      print_error(error::UndefinedFunction, parser, diagnostic_location);
    } else if (!saw_matching_arity) {
      print_error(error::InvalidArgumentCount, parser, diagnostic_location);
    } else {
      size_t argument_index = mismatching_argument.value_or(0);
      if (m_is_method && argument_index > 0)
        --argument_index;

      const SourceLocation& argument_location
          = mismatching_argument && argument_index < arguments.size()
                ? arguments[argument_index]->source_location(diagnostic_location)
                : diagnostic_location;
      print_error(error::InvalidArgumentType, parser, argument_location);
    }
    return false;
  }

  if (m_is_method && !resolved->parameters.empty() &&
      !resolved->parameters.front().type.is_const) {
    auto* member_callee = dynamic_cast<MemberExpression*>(callee.get());
    if (member_callee && member_callee->object &&
        !check_method_receiver_is_mutable(
            member_callee->object.get(), parser, diagnostic_location))
      return false;
  }

  // Apply named arguments by reordering them into parameter order.  This keeps
  // the rest of analysis and IR generation positional.  Reordering is only
  // attempted when every parameter receives an argument.
  {
    const size_t receiver_offset = m_is_method ? 1 : 0;
    const bool   has_named =
        std::any_of(argument_names.begin(), argument_names.end(),
                    [](const std::string& name) { return !name.empty(); });
    if (has_named &&
        resolved->parameters.size() >= receiver_offset &&
        arguments.size() == resolved->parameters.size() - receiver_offset) {
      std::vector<size_t> slots(arguments.size(), 0);
      std::vector<bool>   used(resolved->parameters.size(), false);
      bool                reorderable = true;

      for (size_t i = 0; i < arguments.size(); ++i) {
        size_t parameter_index = receiver_offset + i;
        if (i < argument_names.size() && !argument_names[i].empty()) {
          parameter_index = resolved->parameters.size();
          for (size_t p = receiver_offset; p < resolved->parameters.size(); ++p) {
            if (!used[p] && resolved->parameters[p].name == argument_names[i]) {
              parameter_index = p;
              break;
            }
          }
        }
        if (parameter_index >= resolved->parameters.size() ||
            used[parameter_index]) {
          reorderable = false;
          break;
        }
        used[parameter_index] = true;
        slots[i]              = parameter_index - receiver_offset;
      }

      bool needs_reorder = false;
      for (size_t i = 0; i < slots.size(); ++i) {
        if (slots[i] != i)
          needs_reorder = true;
      }

      if (reorderable && needs_reorder) {
        std::vector<AG_scope<Expression>> reordered(arguments.size());
        std::vector<std::string>          reordered_names(arguments.size());
        std::vector<bool>                 reordered_spreads(arguments.size(), false);
        for (size_t i = 0; i < arguments.size(); ++i) {
          const size_t slot   = slots[i];
          reordered[slot]       = std::move(arguments[i]);
          reordered_names[slot] = i < argument_names.size() ? argument_names[i] : "";
          reordered_spreads[slot] =
              i < m_spread_arguments.size() ? m_spread_arguments[i] : false;
        }
        arguments          = std::move(reordered);
        argument_names     = std::move(reordered_names);
        m_spread_arguments = std::move(reordered_spreads);
      }
    }
  }

  m_address_arguments.assign(arguments.size(), false);
  m_reference_arguments.assign(arguments.size(), false);
  const size_t first_explicit = m_is_method ? 1 : 0;
  for (size_t i = 0; i < arguments.size(); ++i) {
    const size_t parameter_index = first_explicit + i;
    if (parameter_index >= resolved->parameters.size())
      continue;

    const auto& parameter_type = resolved->parameters[parameter_index].type;
    const auto  argument_type  = arguments[i]->get_data_type();
    const bool implicit_reference =
        parameter_type.kind == TypeKind::REFERENCE && parameter_type.pointee &&
        !is_compatible_type(parameter_type, argument_type) &&
        is_addressable_expression(arguments[i].get()) &&
        is_compatible_type(*parameter_type.pointee, argument_type);

    if (implicit_reference) {
      m_reference_arguments[i] = true;
      if (parameter_type.is_mutable_reference &&
          !check_assignment_target_is_mutable(
              arguments[i].get(), parser, diagnostic_location))
        return false;
    } else if (is_pointer_like_type(parameter_type) &&
               as_index_target(arguments[i].get())) {
      m_address_arguments[i] = true;
    }
  }

  m_label               = resolved->label;
  m_result_type         = resolved->return_type ? *resolved->return_type : DataType{};
  m_resolved_parameters = resolved->parameters;
  m_is_variadic         = resolved->is_variadic;
  return true;
}

std::string
CallExpression::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  if (m_is_enum_constructor) {
    std::vector<std::string> argument_values;
    argument_values.reserve(arguments.size());
    for (const auto& argument : arguments)
      argument_values.push_back(argument->generate_ir(counter, parser));

    const auto* definition = get_enum_table().lookup(m_enum_name);
    if (!definition)
      return "0";

    parser.add_instructios(AG_cscope<Param>(std::to_string(definition->size_in_bytes),
                                            counter.function_index));
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));
    parser.add_instructios(AG_cscope<MemberStore>(result,
                                                  0,
                                                  std::to_string(m_variant_index),
                                                  counter.function_index));

    if (m_variant_index < definition->variants.size()) {
      const auto& variant = definition->variants[m_variant_index];
      for (size_t i = 0; i < argument_values.size() && i < variant.fields.size(); ++i) {
        parser.add_instructios(AG_cscope<MemberStore>(
            result,
            static_cast<uint32_t>(variant.fields[i].byte_offset),
            argument_values[i],
            counter.function_index));
      }
    }

    return result;
  }

  // Evaluate the receiver (for methods) and every argument first.  A nested
  // call may clobber the argument registers, so the outer call's Param
  // instructions must be emitted only after all evaluation has finished.
  std::string receiver_value;
  if (m_is_method) {
    auto* member_callee = dynamic_cast<MemberExpression*>(callee.get());
    if (!member_callee || !member_callee->object)
      return "";
    receiver_value = member_callee->object->generate_ir(counter, parser);
  }

  const size_t first_explicit = m_is_method ? 1 : 0;
  const size_t fixed_parameters =
      m_is_variadic && !m_resolved_parameters.empty()
          ? m_resolved_parameters.size() - 1
          : m_resolved_parameters.size();
  const size_t fixed_argument_count =
      m_is_variadic ? std::min(arguments.size(),
                               fixed_parameters > first_explicit
                                   ? fixed_parameters - first_explicit
                                   : size_t{0})
                    : arguments.size();

  std::vector<std::string> argument_values;
  argument_values.reserve(arguments.size() + m_resolved_parameters.size());
  for (size_t i = 0; i < fixed_argument_count; ++i) {
    if (i < m_reference_arguments.size() && m_reference_arguments[i]) {
      const size_t parameter_index = first_explicit + i;
      const auto&  parameter_type  = m_resolved_parameters[parameter_index].type;
      if (parameter_type.pointee &&
          (parameter_type.pointee->kind == TypeKind::STRUCT ||
           parameter_type.pointee->kind == TypeKind::ENUM))
        argument_values.push_back(arguments[i]->generate_ir(counter, parser));
      else
        argument_values.push_back(
            generate_address_of(arguments[i].get(), counter, parser));
    } else if (i < m_address_arguments.size() && m_address_arguments[i]) {
      argument_values.push_back(
          generate_index_address(arguments[i].get(), counter, parser));
    } else {
      const size_t parameter_index = first_explicit + i;
      const auto&  parameter_type  = m_resolved_parameters[parameter_index].type;
      if (parameter_type.kind == TypeKind::STRUCT ||
          parameter_type.kind == TypeKind::ENUM)
        argument_values.push_back(generate_aggregate_argument_copy(
            arguments[i].get(), parameter_type, counter, parser));
      else
        argument_values.push_back(narrow_value(
            arguments[i]->generate_ir(counter, parser),
            parameter_type,
            counter,
            parser));
    }
  }

  // Fill omitted trailing arguments with their declared defaults.  The
  // defaults are analyzed once at the declaration site, but evaluated here in
  // the caller so they can produce normal IR values.
  for (size_t i = first_explicit + fixed_argument_count; i < fixed_parameters; ++i) {
    const auto& parameter = m_resolved_parameters[i];
    if (!parameter.default_value)
      return "";
    if (parameter.type.kind == TypeKind::STRUCT ||
        parameter.type.kind == TypeKind::ENUM)
      argument_values.push_back(generate_aggregate_argument_copy(
          parameter.default_value.get(), parameter.type, counter, parser));
    else
      argument_values.push_back(narrow_value(
          parameter.default_value->generate_ir(counter, parser),
          parameter.type,
          counter,
          parser));
  }

  // Pack the variadic arguments into a block:
  //   [count][element 0][element 1]...
  // The callee receives a pointer to `element 0`; the count lives one slot
  // before it, so iterating a variadic parameter can recover the length.
  if (m_is_variadic) {
    const size_t variadic_start =
        fixed_parameters > first_explicit ? fixed_parameters - first_explicit
                                          : size_t{0};
    size_t variadic_argument_count =
        arguments.size() > variadic_start ? arguments.size() - variadic_start : 0;

    const bool single_spread =
        variadic_argument_count == 1 &&
        variadic_start < m_spread_arguments.size() &&
        m_spread_arguments[variadic_start];
    if (single_spread) {
      const auto spread_type = arguments[variadic_start]->get_data_type();
      variadic_argument_count = spread_type.array_length.value_or(0);
    }

    parser.add_instructios(AG_cscope<Param>(
        std::to_string((variadic_argument_count + 1) * 8),
        counter.function_index));
    const auto block = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, block));
    parser.add_instructios(AG_cscope<MemberStore>(
        block, 0, std::to_string(variadic_argument_count), counter.function_index));

    if (single_spread) {
      const auto source = arguments[variadic_start]->generate_ir(counter, parser);
      for (size_t i = 0; i < variadic_argument_count; ++i) {
        const auto offset = std::to_string(i * 8);
        std::string address = source;
        if (i != 0) {
          address = make_temp(counter, parser);
          parser.add_instructios(AG_cscope<Binary>(address,
                                                   source,
                                                   offset,
                                                   BIN_OPERATOR::ADDITION,
                                                   counter.function_index));
        }
        const auto element = make_temp(counter, parser);
        parser.add_instructios(AG_cscope<DerefLoad>(
            element, address, false, counter.function_index));
        parser.add_instructios(AG_cscope<MemberStore>(
            block,
            static_cast<uint32_t>((i + 1) * 8),
            element,
            counter.function_index));
      }
    } else {
      for (size_t i = 0; i < variadic_argument_count; ++i) {
        const auto value =
            arguments[variadic_start + i]->generate_ir(counter, parser);
        parser.add_instructios(AG_cscope<MemberStore>(
            block,
            static_cast<uint32_t>((i + 1) * 8),
            value,
            counter.function_index));
      }
    }

    const auto first_element = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(first_element,
                                             block,
                                             "8",
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    argument_values.push_back(first_element);
  }

  if (m_is_method)
    parser.add_instructios(AG_cscope<Param>(receiver_value, counter.function_index));

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
PrimaryExpression::get_declared_data_type() {
  if (is_null_pointer) {
    DataType type;
    type.kind           = TypeKind::POINTER;
    type.primitive_type = PrimitiveType::VOID;
    type.pointee        = std::make_shared<DataType>();
    type.pointee->kind  = TypeKind::PRIMITIVE;
    type.pointee->primitive_type = PrimitiveType::VOID;
    type.size_in_bytes  = 8;
    type.alignment      = 8;
    return type;
  }

  // Literal nodes record the type inferred from their token during parsing.
  if (primary_type != PrimitiveType::IDENTIFIER && declared_type_known)
    return declared_type;

  if (primary_type == PrimitiveType::IDENTIFIER) {
    if (declared_type_known)
      return declared_type;

    const std::string_view name = resolved_name_known ? std::string_view(resolved_name)
                                                      : value;
    auto                   data = get_symbol_table().lookup(name);
    if (!data && name != value)
      data = get_symbol_table().lookup(value);
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

DataType
PrimaryExpression::get_data_type() {
  auto declared = get_declared_data_type();
  if (declared.kind == TypeKind::REFERENCE && declared.pointee)
    return *declared.pointee;
  return declared;
}

AG_scope<Expression>
QualifiedNameExpression::parse(Parser&               parser,
                               std::string           first_segment,
                               const SourceLocation& location) {
  auto expression      = AG_cscope<QualifiedNameExpression>();
  expression->path     = std::move(first_segment);
  expression->location = location;

  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedModuleName,
                  parser);
      return nullptr;
    }
    expression->path += "::";
    expression->path += parser.current_token().value;
    parser.advance();
  }

  return expression;
}

PrimitiveType
QualifiedNameExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
QualifiedNameExpression::get_declared_data_type() {
  if (m_is_enum_value)
    return m_result_type;

  if (m_declared_type_known)
    return m_declared_type;

  const std::string& name = resolved_path.empty() ? path : resolved_path;
  if (auto data = get_symbol_table().lookup(name))
    return data->type;
  if (name != path) {
    if (auto data = get_symbol_table().lookup(path))
      return data->type;
  }

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

DataType
QualifiedNameExpression::get_data_type() {
  auto declared = get_declared_data_type();
  if (!m_is_enum_value && declared.kind == TypeKind::REFERENCE && declared.pointee)
    return *declared.pointee;
  return declared;
}

bool
QualifiedNameExpression::analyze_semantics(Parser&               parser,
                                           const SourceLocation& location) {
  resolved_path = parser.resolve_symbol_name(path);

  if (auto data = get_symbol_table().lookup(resolved_path)) {
    m_declared_type       = data->type;
    m_declared_type_known = true;
    return true;
  }
  if (resolved_path != path) {
    if (auto data = get_symbol_table().lookup(path)) {
      m_declared_type       = data->type;
      m_declared_type_known = true;
      return true;
    }
  }

  if (const auto resolved = resolve_enum_variant(parser, path)) {
    if (!resolved->variant->fields.empty()) {
      print_error(error::InvalidArgumentCount, parser, this->source_location(location));
      return false;
    }

    m_is_enum_value  = true;
    m_enum_name      = resolved->definition->name;
    m_variant_index  = resolved->variant->index;
    m_result_type    = make_enum_type(m_enum_name);
    return true;
  }

  print_error(error::UseOfUndefined, parser, this->source_location(location));
  return false;
}

std::string
QualifiedNameExpression::generate_ir(LabelCounters& counter,
                                     Parser&        parser) {
  if (m_is_enum_value) {
    const auto* definition = get_enum_table().lookup(m_enum_name);
    if (!definition)
      return "0";

    parser.add_instructios(AG_cscope<Param>(std::to_string(definition->size_in_bytes),
                                            counter.function_index));
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<FunctionCall>("__ag_alloc", counter.function_index, result));
    parser.add_instructios(AG_cscope<MemberStore>(result,
                                                  0,
                                                  std::to_string(m_variant_index),
                                                  counter.function_index));
    return result;
  }

  const std::string& name = resolved_path.empty() ? path : resolved_path;
  std::string source;
  if (auto variable = get_ir_table().lookup_source(counter.function_index, name))
    source = variable->name;
  else if (auto global = get_ir_table().lookup_global_source(name))
    source = global->name;
  else
    source = name;

  const auto declared = get_declared_data_type();
  if (declared.kind == TypeKind::REFERENCE && declared.pointee) {
    if (declared.pointee->kind == TypeKind::STRUCT ||
        declared.pointee->kind == TypeKind::ENUM)
      return source;

    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        result, source, deref_loads_byte(*declared.pointee), counter.function_index));
    return result;
  }

  return source;
}

std::string
QualifiedNameExpression::generate_reference_ir(LabelCounters& counter,
                                               Parser&         parser) {
  if (m_is_enum_value)
    return generate_ir(counter, parser);

  const std::string& name = resolved_path.empty() ? path : resolved_path;
  if (auto variable = get_ir_table().lookup_source(counter.function_index, name))
    return variable->name;
  if (auto global = get_ir_table().lookup_global_source(name))
    return global->name;
  return name;
}

AG_scope<Expression>
MemberExpression::parse(Parser&              parser,
                        AG_scope<Expression> object) {
  const Token op_token = parser.current_token();
  parser.advance(); // consumes '.' or '->'

  // Method/field names may collide with contextual keywords such as `with`
  // (`lock.with(...)`), so accept those spellings as member names.
  if (!parser.check_one_of(
          {TokenName::AG_IDENTIFIER, TokenName::AG_KEYWORD, TokenName::AG_WITH})) {
    print_error(error::ExpectedMemberName, parser);
    return nullptr;
  }

  const Token member_token = parser.current_token();
  parser.advance();

  auto member        = AG_cscope<MemberExpression>();
  member->object     = std::move(object);
  member->field_name = member_token.value;
  member->is_arrow   = (op_token.token_name == TokenName::AG_ARROW);
  return member;
}

AG_scope<Expression>
StructLiteralExpression::parse(Parser&        parser,
                               std::string    type_name,
                               SourceLocation location) {
  auto literal       = AG_cscope<StructLiteralExpression>();
  literal->type_name = std::move(type_name);
  literal->location  = location;

  if (!parser.expect(TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace))
    return nullptr;

  if (parser.match(TokenName::AG_RIGHT_BRACE))
    return literal;

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    if (parser.match(TokenName::AG_DOT_DOT)) {
      auto spread = Expression::parse_pratt(parser, 0);
      if (!spread)
        return nullptr;
      literal->spread = std::move(spread);
      parser.match(TokenName::AG_COMMA);
      break;
    }

    FieldInitializer initializer;
    initializer.location = {parser.current_token().line, parser.current_token().column};

    AG_EXPECT_GET_ID_OR_RETURN(
        parser, error::ExpectedFieldName, initializer.name, nullptr, TokenName::AG_IDENTIFIER);

    AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

    initializer.value = Expression::parse_pratt(parser, 0);
    if (!initializer.value)
      return nullptr;

    literal->fields.push_back(std::move(initializer));

    if (!parser.match(TokenName::AG_COMMA))
      break;
  }

  if (!parser.expect(TokenName::AG_RIGHT_BRACE, error::ExpectedRightBrace))
    return nullptr;

  return literal;
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
      // `...values` spreads an existing array into the remaining arguments.
      const bool spread = parser.match(TokenName::AG_ELLIPSIS);

      // `greet(name: "ag")` binds an argument by parameter name.  A bare
      // identifier followed by `:` is unambiguous here because ternary
      // conditions start with an expression, not `identifier :`.
      std::string argument_name;
      if (parser.check(TokenName::AG_IDENTIFIER) &&
          parser.peek_token().token_name == TokenName::AG_COLON) {
        argument_name = std::string(parser.current_token().value);
        parser.advance(); // name
        parser.advance(); // ':'
      }

      auto       arg    = Expression::parse_pratt(parser, 0);
      if (!arg) {
        return nullptr;
      }
      call->arguments.push_back(std::move(arg));
      call->argument_names.push_back(std::move(argument_name));
      call->m_spread_arguments.push_back(spread);
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

    // `&mut value` is a mutable borrow.  The `mut` belongs to the prefix
    // operator, not to the operand expression.
    if (temp_token == TokenName::AG_REFERENCE && parser.match(TokenName::AG_MUT))
      prefix_expr->is_mutable_reference = true;

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

  std::string resolved(value);
  auto        data = get_symbol_table().lookup(value);

  // A module-level value declared in the active module is stored under its
  // qualified name.  Local variables and parameters are still plain names and
  // win over module storage because they are looked up first.
  if (!data) {
    const std::string qualified = parser.qualify_name(value);
    if (qualified != resolved) {
      data = get_symbol_table().lookup(qualified);
      if (data)
        resolved = qualified;
    }
  }

  // Imported values arrive as symbol aliases (`import ag::io { cout }`).
  if (!data) {
    const std::string imported = parser.resolve_symbol_name(value);
    if (imported != resolved) {
      data = get_symbol_table().lookup(imported);
      if (data)
        resolved = imported;
    }
  }

  auto type = data ? data->type.primitive_type : PrimitiveType::UNEXPECTED;
  if (type == PrimitiveType::UNEXPECTED) {
    print_error(error::UseOfUndefined, parser, this->source_location(location));
    return false;
  }
  declared_type       = data->type;
  declared_type_known = true;
  resolved_name       = std::move(resolved);
  resolved_name_known = true;
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

  if (un_operator == UN_OPERATOR::REFRENCE) {
    if (!is_addressable_expression(operand.get())) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }
    if (is_mutable_reference &&
        !check_assignment_target_is_mutable(operand.get(), parser, diagnostic_location))
      return false;

    result_type                        = DataType{};
    result_type.kind                   = TypeKind::REFERENCE;
    result_type.primitive_type         = PrimitiveType::USER_DEFINED;
    result_type.pointee                = std::make_shared<DataType>(operand_type);
    result_type.is_mutable_reference   = is_mutable_reference;
    result_type_known                  = true;
    return true;
  }

  if (un_operator == UN_OPERATOR::DEREFRENCE) {
    const auto deref_type = raw_data_type(operand.get());
    DataType pointee;
    if ((deref_type.kind == TypeKind::POINTER ||
         deref_type.kind == TypeKind::REFERENCE) &&
        deref_type.pointee) {
      pointee = *deref_type.pointee;
    } else if (is_string_like_type(deref_type)) {
      pointee.primitive_type = PrimitiveType::CHAR;
    } else {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    if (pointee.primitive_type == PrimitiveType::VOID) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    result_type       = pointee;
    result_type_known = true;
    return true;
  }

  switch (un_operator) {
  case UN_OPERATOR::PRE_INCREMENT:
  case UN_OPERATOR::PRE_DECREMENT:
  case UN_OPERATOR::POST_INCREMENT:
  case UN_OPERATOR::POST_DECREMENT: {
    if (!is_identifier_expression(operand.get()) && !as_index_target(operand.get()) &&
        !as_member_target(operand.get()) && !as_deref_target(operand.get())) {
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
    auto resolution = resolve_operator_overload(
        parser, function_name, {operand_type}, {operand.get()});
    if (resolution.ambiguous) {
      print_error(error::AmbiguousOverload, parser, diagnostic_location);
      return false;
    }
    if (resolution.resolved) {
      const auto& parameters = resolution.resolved->parameters;
      if (!parameters.empty() && operand) {
        const auto& parameter_type = parameters.front().type;
        operand_is_operator_reference =
            parameter_type.kind == TypeKind::REFERENCE && parameter_type.pointee &&
            is_addressable_expression(operand.get()) &&
            !is_compatible_type(parameter_type, operand_type) &&
            is_compatible_type(*parameter_type.pointee, operand_type);
        if (operand_is_operator_reference &&
            parameter_type.is_mutable_reference &&
            !check_assignment_target_is_mutable(
                operand.get(), parser, diagnostic_location))
          return false;
      }
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

  // A direct reference variable is written through rather than rebound.
  // `get_data_type()` already exposes the pointee, so `left_type` is the
  // value type used for normal operator resolution.
  const auto left_declared = raw_data_type(left.get());
  is_reference_target = left_declared.kind == TypeKind::REFERENCE &&
                        left_declared.pointee &&
                        !as_deref_target(left.get()) &&
                        !as_member_target(left.get()) &&
                        !as_index_target(left.get());

  if (bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    auto* identifier = dynamic_cast<PrimaryExpression*>(left.get());
    if (identifier && identifier->primary_type == PrimitiveType::IDENTIFIER) {
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    } else if (as_member_target(left.get()) || as_deref_target(left.get())) {
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    } else {
      if (!as_index_target(left.get())) {
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

  if (is_compound_assignment) {
    auto* identifier = dynamic_cast<PrimaryExpression*>(left.get());
    if (identifier && identifier->primary_type == PrimitiveType::IDENTIFIER) {
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    } else if (as_member_target(left.get()) || as_deref_target(left.get())) {
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    } else {
      if (!as_index_target(left.get())) {
        print_error(error::InvalidAssignmentError, parser, diagnostic_location);
        return false;
      }
      if (!check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
        return false;
    }

    if (left_type.primitive_type == PrimitiveType::UNEXPECTED ||
        right_type.primitive_type == PrimitiveType::UNEXPECTED ||
        left_type.primitive_type == PrimitiveType::VOID ||
        right_type.primitive_type == PrimitiveType::VOID) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    // Prefer a dedicated `operator+=`-style overload, then fall back to the
    // normal binary operator and assign its result back to the target.
    const std::string compound_name = "operator" + std::string(to_symbol(bin_operator)) + "=";
    auto compound_resolution
        = resolve_operator_overload(parser, compound_name, {left_type, right_type});
    if (compound_resolution.ambiguous) {
      print_error(error::AmbiguousOverload, parser, diagnostic_location);
      return false;
    }
    if (compound_resolution.resolved) {
      const auto return_type = compound_resolution.resolved->return_type;
      if (!return_type || return_type->primitive_type == PrimitiveType::VOID ||
          !is_compatible_type(left_type, *return_type)) {
        print_error(error::InvalidOperandTypes, parser, diagnostic_location);
        return false;
      }
      operator_label    = compound_resolution.resolved->label;
      result_type       = left_type;
      result_type_known = true;
      return true;
    }

    const std::string base_name = "operator" + std::string(to_symbol(bin_operator));
    auto base_resolution = resolve_operator_overload(parser, base_name, {left_type, right_type});
    if (base_resolution.ambiguous) {
      print_error(error::AmbiguousOverload, parser, diagnostic_location);
      return false;
    }
    if (base_resolution.resolved) {
      const auto return_type = base_resolution.resolved->return_type;
      if (!return_type || return_type->primitive_type == PrimitiveType::VOID ||
          !is_compatible_type(left_type, *return_type)) {
        print_error(error::InvalidOperandTypes, parser, diagnostic_location);
        return false;
      }
      operator_label    = base_resolution.resolved->label;
      result_type       = left_type;
      result_type_known = true;
      return true;
    }

    if (!is_compatible_type(left_type, right_type)) {
      print_error(error::InvalidOperandTypes, parser, diagnostic_location);
      return false;
    }

    result_type       = left_type;
    result_type_known = true;
    return true;
  }

  // Raw pointers and references support the usual `ptr + int`, `int + ptr`,
  // and `ptr - int` arithmetic.  The integer is scaled by the pointee size so
  // `*(ptr + 1)` advances by one element rather than one byte.
  if ((bin_operator == BIN_OPERATOR::ADDITION ||
       bin_operator == BIN_OPERATOR::SUBTRACTION) &&
      is_indirect_type(left_type) && is_integer_like_type(right_type)) {
    is_pointer_arithmetic = true;
    pointer_is_left       = true;
    pointer_scale         = pointer_step(left_type);
    result_type           = left_type;
    result_type_known     = true;
    return true;
  }
  if (bin_operator == BIN_OPERATOR::ADDITION && is_integer_like_type(left_type) &&
      is_indirect_type(right_type)) {
    is_pointer_arithmetic = true;
    pointer_is_left       = false;
    pointer_scale         = pointer_step(right_type);
    result_type           = right_type;
    result_type_known     = true;
    return true;
  }

  const std::string operator_name = "operator" + std::string(to_symbol(bin_operator));
  auto              resolution = resolve_operator_overload(
      parser, operator_name, {left_type, right_type}, {left.get(), right.get()});
  if (resolution.ambiguous) {
    print_error(error::AmbiguousOverload, parser, diagnostic_location);
    return false;
  }
  if (resolution.resolved) {
    const auto& parameters = resolution.resolved->parameters;
    const auto is_implicit_reference = [&](size_t index,
                                           Expression* expression,
                                           const DataType& argument_type) {
      if (index >= parameters.size() || !expression)
        return false;
      const auto& parameter_type = parameters[index].type;
      return parameter_type.kind == TypeKind::REFERENCE && parameter_type.pointee &&
             is_addressable_expression(expression) &&
             !is_compatible_type(parameter_type, argument_type) &&
             is_compatible_type(*parameter_type.pointee, argument_type);
    };

    left_is_operator_reference =
        is_implicit_reference(0, left.get(), left_type);
    right_is_operator_reference =
        is_implicit_reference(1, right.get(), right_type);

    if (left_is_operator_reference && parameters[0].type.is_mutable_reference &&
        !check_assignment_target_is_mutable(left.get(), parser, diagnostic_location))
      return false;
    if (right_is_operator_reference && parameters.size() > 1 &&
        parameters[1].type.is_mutable_reference &&
        !check_assignment_target_is_mutable(right.get(), parser, diagnostic_location))
      return false;

    operator_label    = resolution.resolved->label;
    result_type       = resolution.resolved->return_type ? *resolution.resolved->return_type : DataType{};
    result_type_known = true;
    return true;
  }

  if (bin_operator == BIN_OPERATOR::INDEXING &&
      dynamic_cast<RangeExpression*>(right.get())) {
    // Slicing currently produces a pointer to the first element.  The end
    // bound is parsed and checked but slice lengths are not stored yet.
    if ((left_type.kind == TypeKind::POINTER ||
         left_type.kind == TypeKind::REFERENCE) &&
        left_type.pointee && !is_void_pointer_type(left_type)) {
      result_type       = left_type;
      result_type_known = true;
      is_slice_index    = true;
      index_scale       = pointer_step(left_type);
      return true;
    }

    if (left_type.primitive_type == PrimitiveType::STRING) {
      result_type       = left_type;
      result_type_known = true;
      is_slice_index    = true;
      index_scale       = 1;
      return true;
    }

    print_error(error::InvalidOperandTypes, parser, diagnostic_location);
    return false;
  }

  if (bin_operator == BIN_OPERATOR::INDEXING) {
    const bool integer_index =
        right_type.primitive_type != PrimitiveType::UNEXPECTED &&
        is_compatible_datatype(PrimitiveType::INT, right_type.primitive_type);

    if (integer_index) {
      // Any pointer or reference can be indexed; the result is one element of
      // its pointee type.  The legacy `string` spelling stays a byte address
      // whose index reads an integer-sized byte value.
      if ((left_type.kind == TypeKind::POINTER ||
           left_type.kind == TypeKind::REFERENCE) &&
          left_type.pointee && !is_void_pointer_type(left_type)) {
        result_type        = *left_type.pointee;
        result_type_known  = true;
        is_string_index    = true;
        index_scale        = pointer_step(left_type);
        index_width        = memory_width_of(result_type);
        index_signed       = memory_is_signed(result_type);
        index_loads_byte   = index_width == 1;
        return true;
      }

      if (left_type.primitive_type == PrimitiveType::STRING) {
        result_type                = DataType{};
        result_type.primitive_type = PrimitiveType::INT;
        result_type_known          = true;
        is_string_index            = true;
        index_scale                = 1;
        index_width                = 1;
        index_signed               = true;
        index_loads_byte           = true;
        return true;
      }
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

  if (is_comparison_operator(bin_operator)) {
    // Comparisons yield an AGC `int` 0/1 rather than the left operand type, so
    // `!(ptr == other)` and arithmetic on comparison results work as expected.
    result_type                = DataType{};
    result_type.primitive_type = PrimitiveType::INT;
    result_type.size_in_bytes  = 8;
    result_type.alignment      = 8;
    result_type_known          = true;
    return true;
  }

  result_type       = left_type;
  result_type_known = true;
  return true;
}

std::string
BinaryExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  if (is_compound_assignment) {
    auto emit_operation = [&](const std::string& current,
                              const std::string& right_value) -> std::string {
      if (!operator_label.empty()) {
        parser.add_instructios(AG_cscope<Param>(current, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(right_value, counter.function_index));
        const auto result = make_temp(counter, parser);
        parser.add_instructios(
            AG_cscope<FunctionCall>(operator_label, counter.function_index, result));
        return result;
      }

      const auto result = make_temp(counter, parser);
      parser.add_instructios(
          AG_cscope<Binary>(result, current, right_value, bin_operator, counter.function_index));
      return result;
    };

    if (is_reference_target) {
      const auto address  = generate_reference_value(left.get(), counter, parser);
      const auto pointee  = left->get_data_type();
      const auto width    = memory_width_of(pointee);
      const auto signedness = memory_is_signed(pointee);
      const bool aggregate = pointee.kind == TypeKind::STRUCT ||
                             pointee.kind == TypeKind::ENUM;

      std::string current = address;
      if (!aggregate) {
        current = make_temp(counter, parser);
        parser.add_instructios(AG_cscope<DerefLoad>(
            current, address, counter.function_index, width, signedness));
      }

      const auto right_value = right->generate_ir(counter, parser);
      const auto updated     = emit_operation(current, right_value);
      if (aggregate) {
        parser.add_instructios(AG_cscope<Param>(address, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(updated, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(
            std::to_string(value_storage_size(pointee)), counter.function_index));
        const auto ignored = make_temp(counter, parser);
        parser.add_instructios(
            AG_cscope<FunctionCall>("__ag_memcpy", counter.function_index, ignored));
      } else {
        parser.add_instructios(AG_cscope<DerefStore>(
            address, updated, counter.function_index, width, signedness));
      }
      return updated;
    }

    if (auto* index = as_index_target(left.get())) {
      const auto address = generate_index_address(left.get(), counter, parser);
      const auto current = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<DerefLoad>(
          current, address, counter.function_index, index->index_width, index->index_signed));

      const auto right_value = right->generate_ir(counter, parser);
      const auto updated     = emit_operation(current, right_value);
      parser.add_instructios(AG_cscope<DerefStore>(
          address, updated, counter.function_index, index->index_width, index->index_signed));
      return updated;
    }

    if (auto* member = as_member_target(left.get())) {
      const auto object  = member->object->generate_ir(counter, parser);
      const auto member_type = member->get_data_type();
      const auto current = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<MemberLoad>(
          current,
          object,
          static_cast<uint32_t>(member->byte_offset),
          counter.function_index,
          memory_width_of(member_type),
          memory_is_signed(member_type)));

      const auto right_value = right->generate_ir(counter, parser);
      const auto updated     = emit_operation(current, right_value);
      parser.add_instructios(AG_cscope<MemberStore>(
          object,
          static_cast<uint32_t>(member->byte_offset),
          updated,
          counter.function_index,
          memory_width_of(member_type),
          memory_is_signed(member_type)));
      return updated;
    }

    if (auto* deref = as_deref_target(left.get())) {
      const auto address = generate_reference_value(deref->operand.get(), counter, parser);
      const auto deref_type = deref->get_data_type();
      const auto width = memory_width_of(deref_type);
      const auto signedness = memory_is_signed(deref_type);
      const auto current = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<DerefLoad>(
          current, address, counter.function_index, width, signedness));

      const auto right_value = right->generate_ir(counter, parser);
      const auto updated     = emit_operation(current, right_value);
      parser.add_instructios(AG_cscope<DerefStore>(
          address, updated, counter.function_index, width, signedness));
      return updated;
    }

    const auto target      = left->generate_ir(counter, parser);
    const auto right_value = right->generate_ir(counter, parser);
    const auto updated     = emit_operation(target, right_value);
    const auto narrowed
        = narrow_value(updated, left->get_data_type(), counter, parser);
    parser.add_instructios(AG_cscope<Assign>(narrowed, target, counter.function_index));
    return narrowed;
  }

  if (bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    if (is_reference_target) {
      const auto value    = right->generate_ir(counter, parser);
      const auto address  = generate_reference_value(left.get(), counter, parser);
      const auto pointee  = left->get_data_type();
      if (pointee.kind == TypeKind::STRUCT || pointee.kind == TypeKind::ENUM) {
        parser.add_instructios(AG_cscope<Param>(address, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(value, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(
            std::to_string(value_storage_size(pointee)), counter.function_index));
        const auto ignored = make_temp(counter, parser);
        parser.add_instructios(
            AG_cscope<FunctionCall>("__ag_memcpy", counter.function_index, ignored));
        return value;
      }
      parser.add_instructios(AG_cscope<DerefStore>(
          address,
          value,
          counter.function_index,
          memory_width_of(pointee),
          memory_is_signed(pointee)));
      return value;
    }

    if (auto* index = as_index_target(left.get())) {
      const auto value   = right->generate_ir(counter, parser);
      const auto address = generate_index_address(left.get(), counter, parser);
      const auto element = left->get_data_type();
      if (element.kind == TypeKind::STRUCT || element.kind == TypeKind::ENUM) {
        parser.add_instructios(AG_cscope<Param>(address, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(value, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(
            std::to_string(value_storage_size(element)), counter.function_index));
        const auto result = make_temp(counter, parser);
        parser.add_instructios(
            AG_cscope<FunctionCall>("__ag_memcpy", counter.function_index, result));
        return value;
      }
      parser.add_instructios(AG_cscope<DerefStore>(
          address,
          value,
          counter.function_index,
          index->index_width,
          index->index_signed));
      return value;
    }

    if (auto* member = as_member_target(left.get())) {
      const auto value  = right->generate_ir(counter, parser);
      const auto object = member->object->generate_ir(counter, parser);
      parser.add_instructios(AG_cscope<MemberStore>(
          object,
          static_cast<uint32_t>(member->byte_offset),
          value,
          counter.function_index,
          memory_width_of(member->get_data_type()),
          memory_is_signed(member->get_data_type())));
      return value;
    }

    if (auto* deref = as_deref_target(left.get())) {
      const auto value   = right->generate_ir(counter, parser);
      const auto address = generate_reference_value(deref->operand.get(), counter, parser);
      const auto pointee = deref->get_data_type();
      if (pointee.kind == TypeKind::STRUCT || pointee.kind == TypeKind::ENUM) {
        parser.add_instructios(AG_cscope<Param>(address, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(value, counter.function_index));
        parser.add_instructios(AG_cscope<Param>(
            std::to_string(value_storage_size(pointee)), counter.function_index));
        const auto result = make_temp(counter, parser);
        parser.add_instructios(
            AG_cscope<FunctionCall>("__ag_memcpy", counter.function_index, result));
        return value;
      }
      parser.add_instructios(AG_cscope<DerefStore>(
          address,
          value,
          counter.function_index,
          memory_width_of(pointee),
          memory_is_signed(pointee)));
      return value;
    }

    const auto r          = right->generate_ir(counter, parser);
    const auto left_value = left->generate_ir(counter, parser);
    const auto narrowed
        = narrow_value(r, left->get_data_type(), counter, parser);
    parser.add_instructios(AG_cscope<Assign>(narrowed, left_value, counter.function_index));
    return narrowed;
  }

  if (is_pointer_arithmetic) {
    const auto left_value  = left->generate_ir(counter, parser);
    const auto right_value = right->generate_ir(counter, parser);

    const std::string offset_source = pointer_is_left ? right_value : left_value;
    std::string       offset        = offset_source;
    if (pointer_scale != 1) {
      offset = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<Binary>(offset,
                                               offset_source,
                                               std::to_string(pointer_scale),
                                               BIN_OPERATOR::MULTIPLICATION,
                                               counter.function_index));
    }

    const auto result = make_temp(counter, parser);
    if (pointer_is_left) {
      parser.add_instructios(AG_cscope<Binary>(
          result, left_value, offset, bin_operator, counter.function_index));
    } else {
      parser.add_instructios(AG_cscope<Binary>(
          result, offset, right_value, BIN_OPERATOR::ADDITION, counter.function_index));
    }
    return result;
  }

  if (bin_operator == BIN_OPERATOR::INDEXING && is_slice_index) {
    const auto object = left->generate_ir(counter, parser);
    auto*      range  = dynamic_cast<RangeExpression*>(right.get());
    std::string offset = (range && range->start)
                             ? range->start->generate_ir(counter, parser)
                             : std::string("0");

    if (index_scale != 1) {
      const auto scaled = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<Binary>(scaled,
                                               offset,
                                               std::to_string(index_scale),
                                               BIN_OPERATOR::MULTIPLICATION,
                                               counter.function_index));
      offset = scaled;
    }

    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(
        result, object, offset, BIN_OPERATOR::ADDITION, counter.function_index));
    return result;
  }

  if (bin_operator == BIN_OPERATOR::INDEXING && is_string_index) {
    const auto address = generate_index_address(this, counter, parser);
    // A pointer to a struct/enum yields the element's address directly; an
    // aggregate value is represented by the address of its fields.
    if (result_type.kind == TypeKind::STRUCT || result_type.kind == TypeKind::ENUM)
      return address;

    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        result,
        address,
        counter.function_index,
        index_width,
        index_signed));
    return result;
  }

  if (!operator_label.empty()) {
    const auto left_value = left_is_operator_reference
                                ? generate_address_of(left.get(), counter, parser)
                                : left->generate_ir(counter, parser);
    const auto right_value = right_is_operator_reference
                                 ? generate_address_of(right.get(), counter, parser)
                                 : right->generate_ir(counter, parser);
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
    std::string value = operand_is_operator_reference
                            ? generate_address_of(operand.get(), counter, parser)
                            : operand->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<Param>(value, counter.function_index));

    std::string result;
    if (result_type.primitive_type != PrimitiveType::VOID)
      result = make_temp(counter, parser);

    parser.add_instructios(AG_cscope<FunctionCall>(operator_label, counter.function_index, result));
    return result;
  }

  // Only `++`/`--` read-modify-write their target.  Other prefix operators
  // (`!`, `~`, unary `-`) must evaluate the target normally, otherwise
  // `!arr[i]` would be mistaken for `arr[i]--`.
  const bool is_incdec = un_operator == UN_OPERATOR::PRE_INCREMENT ||
                         un_operator == UN_OPERATOR::POST_INCREMENT ||
                         un_operator == UN_OPERATOR::PRE_DECREMENT ||
                         un_operator == UN_OPERATOR::POST_DECREMENT;

  if (un_operator == UN_OPERATOR::REFRENCE) {
    // Aggregate values are already represented by their address.  Taking the
    // address of a struct/enum therefore yields that pointer instead of the
    // address of the stack slot that stores the pointer.
    if (result_type.pointee &&
        (result_type.pointee->kind == TypeKind::STRUCT ||
         result_type.pointee->kind == TypeKind::ENUM))
      return operand->generate_ir(counter, parser);

    return generate_address_of(operand.get(), counter, parser);
  }

  if (un_operator == UN_OPERATOR::DEREFRENCE) {
    const auto address = generate_reference_value(operand.get(), counter, parser);
    // Dereferencing a pointer/reference to an aggregate also stays an address:
    // the aggregate value is the pointer to its fields.
    if (result_type.kind == TypeKind::STRUCT || result_type.kind == TypeKind::ENUM)
      return address;

    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        result,
        address,
        counter.function_index,
        memory_width_of(result_type),
        memory_is_signed(result_type)));
    return result;
  }

  if (auto* index = as_index_target(operand.get()); is_incdec && index) {
    const auto address = generate_index_address(operand.get(), counter, parser);

    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        old_value,
        address,
        counter.function_index,
        index->index_width,
        index->index_signed));

    const auto updated = make_temp(counter, parser);
    const auto op      = (un_operator == UN_OPERATOR::PRE_INCREMENT ||
                          un_operator == UN_OPERATOR::POST_INCREMENT)
                             ? BIN_OPERATOR::ADDITION
                             : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(
        AG_cscope<Binary>(updated, old_value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<DerefStore>(
        address,
        updated,
        counter.function_index,
        index->index_width,
        index->index_signed));

    return (un_operator == UN_OPERATOR::PRE_INCREMENT ||
            un_operator == UN_OPERATOR::PRE_DECREMENT)
               ? updated
               : old_value;
  }

  if (auto* member = as_member_target(operand.get()); is_incdec && member) {
    const auto object = member->object->generate_ir(counter, parser);
    const auto member_type = member->get_data_type();
    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<MemberLoad>(
        old_value,
        object,
        static_cast<uint32_t>(member->byte_offset),
        counter.function_index,
        memory_width_of(member_type),
        memory_is_signed(member_type)));

    const auto updated = make_temp(counter, parser);
    const auto op      = (un_operator == UN_OPERATOR::PRE_INCREMENT ||
                          un_operator == UN_OPERATOR::POST_INCREMENT)
                             ? BIN_OPERATOR::ADDITION
                             : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(
        AG_cscope<Binary>(updated, old_value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<MemberStore>(
        object,
        static_cast<uint32_t>(member->byte_offset),
        updated,
        counter.function_index,
        memory_width_of(member_type),
        memory_is_signed(member_type)));

    return (un_operator == UN_OPERATOR::PRE_INCREMENT ||
            un_operator == UN_OPERATOR::PRE_DECREMENT)
               ? updated
               : old_value;
  }

  if (auto* deref = as_deref_target(operand.get()); is_incdec && deref) {
    const auto address = generate_reference_value(deref->operand.get(), counter, parser);
    const auto deref_type = deref->get_data_type();
    const auto width = memory_width_of(deref_type);
    const auto signedness = memory_is_signed(deref_type);
    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        old_value, address, counter.function_index, width, signedness));

    const auto updated = make_temp(counter, parser);
    const auto op      = (un_operator == UN_OPERATOR::PRE_INCREMENT ||
                          un_operator == UN_OPERATOR::POST_INCREMENT)
                             ? BIN_OPERATOR::ADDITION
                             : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(
        AG_cscope<Binary>(updated, old_value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<DerefStore>(
        address, updated, counter.function_index, width, signedness));

    return (un_operator == UN_OPERATOR::PRE_INCREMENT ||
            un_operator == UN_OPERATOR::PRE_DECREMENT)
               ? updated
               : old_value;
  }

  const auto operand_declared = raw_data_type(operand.get());
  if (is_incdec && operand_declared.kind == TypeKind::REFERENCE &&
      operand_declared.pointee) {
    const auto address = generate_reference_value(operand.get(), counter, parser);
    const auto width = memory_width_of(*operand_declared.pointee);
    const auto signedness = memory_is_signed(*operand_declared.pointee);
    const auto old_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        old_value, address, counter.function_index, width, signedness));

    const auto updated = make_temp(counter, parser);
    const auto op      = (un_operator == UN_OPERATOR::PRE_INCREMENT ||
                          un_operator == UN_OPERATOR::POST_INCREMENT)
                             ? BIN_OPERATOR::ADDITION
                             : BIN_OPERATOR::SUBTRACTION;
    parser.add_instructios(
        AG_cscope<Binary>(updated, old_value, "1", op, counter.function_index));
    parser.add_instructios(AG_cscope<DerefStore>(
        address, updated, counter.function_index, width, signedness));

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

AG_scope<Expression>
ReturnExpression::parse(Parser& parser) {
  const Token return_token = parser.current_token();
  parser.match(TokenName::AG_RETURN);

  auto expression      = AG_cscope<ReturnExpression>();
  expression->location = {return_token.line, return_token.column};

  if (!parser.check(TokenName::AG_SEMICOLON) &&
      !parser.check(TokenName::AG_RIGHT_PAREN) &&
      !parser.check(TokenName::AG_RIGHT_BRACKET) &&
      !parser.check(TokenName::AG_COMMA)) {
    expression->value = Expression::parse_pratt(parser, 0.0f);
    if (!expression->value)
      return nullptr;
  }
  return expression;
}

PrimitiveType
ReturnExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
ReturnExpression::get_data_type() {
  return result_type;
}

bool
ReturnExpression::analyze_semantics(Parser&               parser,
                                    const SourceLocation& location) {
  const auto* return_type = parser.current_function_return_type();
  if (!return_type) {
    print_error(error::ReturnOutsideFunction, parser,
                this->source_location(location));
    return false;
  }

  if (!value) {
    result_type = *return_type;
    return return_type->primitive_type == PrimitiveType::VOID;
  }

  if (!value->analyze_semantics(parser, value->source_location(location)))
    return false;

  result_type = value->get_data_type();
  if (return_type->primitive_type != PrimitiveType::VOID &&
      !is_compatible_type(*return_type, result_type)) {
    print_error(error::IncompatibleTypes, parser,
                value->source_location(location));
    return false;
  }
  return true;
}

std::string
ReturnExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  const std::string expression =
      value ? value->generate_ir(counter, parser) : "";
  parser.add_instructios(AG_cscope<Return>(expression, counter.function_index));
  return expression;
}

PrimitiveType
OrelseExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
OrelseExpression::get_data_type() {
  if (result_type_known)
    return result_type;
  if (value)
    return value->get_data_type();
  if (fallback)
    return fallback->get_data_type();

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

bool
OrelseExpression::analyze_semantics(Parser&               parser,
                                    const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);
  if (!value ||
      !value->analyze_semantics(parser, value->source_location(diagnostic_location)) ||
      !fallback ||
      !fallback->analyze_semantics(parser,
                                   fallback->source_location(diagnostic_location)))
    return false;

  result_type       = value->get_data_type();
  result_type_known = true;
  return true;
}

std::string
OrelseExpression::generate_ir(LabelCounters& counter,
                              Parser&        parser) {
  const auto value_expression = value->generate_ir(counter, parser);
  const auto value_slot       = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Assign>(value_expression, value_slot, counter.function_index));

  const auto condition = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(condition,
                                           value_slot,
                                           "0",
                                           BIN_OPERATOR::NOT_EQUAL_TO,
                                           counter.function_index));

  const auto fallback_label = "b" + std::to_string(counter.block_label++);
  const auto then_label     = "b" + std::to_string(counter.block_label++);
  const auto end_label      = "b" + std::to_string(counter.block_label++);

  parser.add_instructios(
      AG_cscope<CondJump>(condition, fallback_label, counter.function_index));
  parser.add_instructios(AG_cscope<Label>(then_label));

  const auto result = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Assign>(value_slot, result, counter.function_index));
  parser.add_instructios(AG_cscope<Jump>(end_label));

  parser.add_instructios(AG_cscope<Label>(fallback_label));
  parser.add_instructios(AG_cscope<Assign>(
      fallback->generate_ir(counter, parser), result, counter.function_index));
  parser.add_instructios(AG_cscope<Label>(end_label));
  return result;
}

std::string
ConditionalExpression::generate_ir(LabelCounters& counter,
                                   Parser&        parser) {
  const auto result = make_temp(counter, parser);

  const auto condition_value = condition->generate_ir(counter, parser);
  const auto else_label      = "b" + std::to_string(counter.block_label++);
  const auto then_label      = "b" + std::to_string(counter.block_label++);
  const auto end_label       = "b" + std::to_string(counter.block_label++);

  parser.add_instructios(
      AG_cscope<CondJump>(condition_value, else_label, counter.function_index));

  parser.add_instructios(AG_cscope<Label>(then_label));
  parser.add_instructios(AG_cscope<Assign>(
      then_branch->generate_ir(counter, parser), result, counter.function_index));
  parser.add_instructios(AG_cscope<Jump>(end_label));

  parser.add_instructios(AG_cscope<Label>(else_label));
  parser.add_instructios(AG_cscope<Assign>(
      else_branch->generate_ir(counter, parser), result, counter.function_index));
  parser.add_instructios(AG_cscope<Jump>(end_label));

  parser.add_instructios(AG_cscope<Label>(end_label));
  return result;
}

PrimitiveType
ConditionalExpression::get_type() {
  return get_data_type().primitive_type;
}

DataType
ConditionalExpression::get_data_type() {
  if (result_type_known)
    return result_type;
  if (then_branch)
    return then_branch->get_data_type();

  DataType unknown;
  unknown.primitive_type = PrimitiveType::UNEXPECTED;
  return unknown;
}

bool
ConditionalExpression::analyze_semantics(Parser&               parser,
                                         const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (!condition ||
      !condition->analyze_semantics(parser,
                                    condition->source_location(diagnostic_location)))
    return false;
  if (!then_branch ||
      !then_branch->analyze_semantics(parser,
                                      then_branch->source_location(diagnostic_location)))
    return false;
  if (!else_branch ||
      !else_branch->analyze_semantics(parser,
                                      else_branch->source_location(diagnostic_location)))
    return false;

  const auto then_type = then_branch->get_data_type();
  const auto else_type = else_branch->get_data_type();
  if (!is_compatible_type(then_type, else_type) &&
      !is_compatible_type(else_type, then_type)) {
    print_error(error::IncompatibleTypes, parser, diagnostic_location);
    return false;
  }

  result_type       = then_type.primitive_type != PrimitiveType::UNEXPECTED
                          ? then_type
                          : else_type;
  result_type_known = true;
  return true;
}

std::string
CastExpression::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  if (!operand)
    return "0";

  const auto value = operand->generate_ir(counter, parser);

  // Character/byte casts keep only the low 8 bits.
  if (target_type.primitive_type == PrimitiveType::CHAR) {
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(result,
                                             value,
                                             "255",
                                             BIN_OPERATOR::BITWISE_AND,
                                             counter.function_index));
    return result;
  }

  return value;
}

PrimitiveType
CastExpression::get_type() {
  return target_type.primitive_type;
}

DataType
CastExpression::get_data_type() {
  return target_type;
}

bool
CastExpression::analyze_semantics(Parser&               parser,
                                  const SourceLocation& location) {
  if (!operand)
    return false;
  return operand->analyze_semantics(parser,
                                    operand->source_location(
                                        source_location(location)));
}

std::string
InExpression::generate_ir(LabelCounters& counter,
                          Parser&        parser) {
  if (!needle || !haystack)
    return "0";

  const auto needle_value = needle->generate_ir(counter, parser);

  const auto emit_compare = [&](const std::string& right,
                                BIN_OPERATOR       op) {
    const auto result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(
        result, needle_value, right, op, counter.function_index));
    return result;
  };

  if (auto* range = dynamic_cast<RangeExpression*>(haystack.get())) {
    std::string condition;
    if (range->start) {
      const auto lower = emit_compare(range->start->generate_ir(counter, parser),
                                      BIN_OPERATOR::GREATER_THAN_EQUAL);
      if (condition.empty()) {
        condition = lower;
      } else {
        const auto combined = make_temp(counter, parser);
        parser.add_instructios(AG_cscope<Binary>(combined, condition, lower,
                                                 BIN_OPERATOR::LOGICAL_AND,
                                                 counter.function_index));
        condition = combined;
      }
    }
    if (range->end) {
      const auto upper = emit_compare(range->end->generate_ir(counter, parser),
                                      range->inclusive
                                          ? BIN_OPERATOR::LESS_THAN_EQUAL
                                          : BIN_OPERATOR::LESS_THAN);
      if (condition.empty()) {
        condition = upper;
      } else {
        const auto combined = make_temp(counter, parser);
        parser.add_instructios(AG_cscope<Binary>(combined, condition, upper,
                                                 BIN_OPERATOR::LOGICAL_AND,
                                                 counter.function_index));
        condition = combined;
      }
    }
    return condition.empty() ? "1" : condition;
  }

  if (auto* array = dynamic_cast<ArrayLiteralExpression*>(haystack.get())) {
    std::string condition = "0";
    for (auto& element : array->elements) {
      const auto comparison =
          emit_compare(element->generate_ir(counter, parser),
                       BIN_OPERATOR::EQUAL_TO);
      const auto combined = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<Binary>(combined, condition,
                                               comparison,
                                               BIN_OPERATOR::LOGICAL_OR,
                                               counter.function_index));
      condition = combined;
    }
    return condition;
  }

  // Fall back to the haystack's truthiness for non-collection operands.
  const auto value = haystack->generate_ir(counter, parser);
  return emit_compare(value, BIN_OPERATOR::NOT_EQUAL_TO);
}

PrimitiveType
InExpression::get_type() {
  return PrimitiveType::INT;
}

DataType
InExpression::get_data_type() {
  DataType type;
  type.kind           = TypeKind::PRIMITIVE;
  type.primitive_type = PrimitiveType::INT;
  type.size_in_bytes  = 8;
  type.alignment      = 8;
  return type;
}

bool
InExpression::analyze_semantics(Parser&               parser,
                                const SourceLocation& location) {
  if (!needle || !haystack)
    return false;
  if (!needle->analyze_semantics(parser,
                                 needle->source_location(location)))
    return false;
  return haystack->analyze_semantics(parser,
                                     haystack->source_location(location));
}

std::string
ChannelSendExpression::generate_ir(LabelCounters& counter,
                                   Parser&        parser) {
  if (!value)
    return "0";
  if (channel)
    channel->generate_ir(counter, parser);
  return value->generate_ir(counter, parser);
}

PrimitiveType
ChannelSendExpression::get_type() {
  return value ? value->get_type() : PrimitiveType::VOID;
}

DataType
ChannelSendExpression::get_data_type() {
  if (value)
    return value->get_data_type();

  DataType type;
  type.primitive_type = PrimitiveType::VOID;
  return type;
}

bool
ChannelSendExpression::analyze_semantics(Parser&               parser,
                                         const SourceLocation& location) {
  if (!channel || !value)
    return false;
  if (!channel->analyze_semantics(parser,
                                  channel->source_location(location)))
    return false;
  return value->analyze_semantics(parser,
                                  value->source_location(location));
}

std::string
PrimaryExpression::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  if (primary_type == PrimitiveType::STRING) {
    return "str." + std::to_string(index);
  }

  // Float literals are stored in the same i64 slots as every other value.
  // Materialize the IEEE-754 double bit pattern so the operand is a valid
  // integer constant for the current backend.
  if (primary_type == PrimitiveType::FLOAT ||
      primary_type == PrimitiveType::DOUBLE) {
    const std::string text(value);
    char*             end   = nullptr;
    const double      parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0')
      return "0";
    return std::to_string(std::bit_cast<uint64_t>(parsed));
  }

  if (primary_type == PrimitiveType::IDENTIFIER) {
    const std::string name = resolved_name_known ? resolved_name : std::string(value);
    std::string       source;
    if (auto variable = get_ir_table().lookup_source(counter.function_index, name))
      source = variable->name;
    else if (auto global = get_ir_table().lookup_global_source(name))
      source = global->name;
    else
      source = name;

    const auto declared = get_declared_data_type();
    if (declared.kind == TypeKind::REFERENCE && declared.pointee) {
      // References read as their referred value.  Aggregates are already
      // represented by an address, so they need no load.
      if (declared.pointee->kind == TypeKind::STRUCT ||
          declared.pointee->kind == TypeKind::ENUM)
        return source;

      const auto result = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<DerefLoad>(
          result,
          source,
          counter.function_index,
          memory_width_of(*declared.pointee),
          memory_is_signed(*declared.pointee)));
      return result;
    }

    return source;
  }

  return std::string(value);
}

std::string
PrimaryExpression::generate_reference_ir(LabelCounters& counter,
                                        Parser&         parser) {
  if (primary_type == PrimitiveType::IDENTIFIER) {
    const std::string name = resolved_name_known ? resolved_name : std::string(value);
    if (auto variable = get_ir_table().lookup_source(counter.function_index, name))
      return variable->name;
    if (auto global = get_ir_table().lookup_global_source(name))
      return global->name;
    return name;
  }
  return generate_ir(counter, parser);
}

// Parse one numeric match alternative: either a plain integer literal or a
// range such as `1..=9` / `10..20`.  `..=` is lexed as `..` followed by `=`.
static bool
parse_match_numeric_alternative(Parser&                  parser,
                                MatchPatternAlternative& alternative) {
  if (!parser.check(TokenName::AG_INT_LITERAL)) {
    print_error(error::ExpectedPattern, parser);
    return false;
  }

  alternative.literal = std::string(parser.current_token().value);
  parser.advance();

  if (parser.check(TokenName::AG_DOT_DOT)) {
    parser.advance();
    alternative.is_range  = true;
    alternative.inclusive = parser.match(TokenName::AG_ASSIGN);

    if (!parser.check(TokenName::AG_INT_LITERAL)) {
      print_error(error::ExpectedPattern, parser);
      return false;
    }
    alternative.range_start = alternative.literal;
    alternative.range_end   = std::string(parser.current_token().value);
    parser.advance();
  }
  return true;
}

bool
parse_match_pattern(Parser&               parser,
                    MatchPattern&         pattern,
                    const SourceLocation& location) {
  pattern          = MatchPattern{};
  pattern.location = location;

  const auto parse_parenthesized_bindings = [&]() -> bool {
    if (!parser.match(TokenName::AG_LEFT_PAREN))
      return true;

    if (!parser.check(TokenName::AG_RIGHT_PAREN)) {
      do {
        if (!parser.check(TokenName::AG_IDENTIFIER)) {
          print_error(error::ExpectedIdentifier, parser);
          return false;
        }
        MatchBinding binding;
        binding.name     = std::string(parser.current_token().value);
        binding.location = {parser.current_token().line,
                            parser.current_token().column};
        parser.advance();
        pattern.bindings.push_back(std::move(binding));
      } while (parser.match(TokenName::AG_COMMA));
    }
    return parser.expect(TokenName::AG_RIGHT_PAREN,
                         error::ExpectedRightParen);
  };

  const auto parse_struct_bindings = [&]() -> bool {
    while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      if (parser.is_at_end()) {
        print_error(error::UnexpectedEndOfFile, parser);
        return false;
      }

      if (!parser.check(TokenName::AG_IDENTIFIER)) {
        print_error(error::ExpectedFieldName, parser);
        return false;
      }

      MatchBinding binding;
      binding.field_name = std::string(parser.current_token().value);
      binding.name       = binding.field_name;
      binding.location   = {parser.current_token().line,
                            parser.current_token().column};
      parser.advance();

      if (parser.match(TokenName::AG_COLON)) {
        if (!parser.check(TokenName::AG_IDENTIFIER)) {
          print_error(error::ExpectedIdentifier, parser);
          return false;
        }
        binding.name     = std::string(parser.current_token().value);
        binding.location = {parser.current_token().line,
                            parser.current_token().column};
        parser.advance();
      }

      pattern.bindings.push_back(std::move(binding));

      if (parser.match(TokenName::AG_COMMA))
        continue;
      if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
        print_error(error::ExpectedComma, parser);
        return false;
      }
    }
    parser.advance(); // `}`
    return true;
  };

  if (parser.check(TokenName::AG_IDENTIFIER) &&
      parser.current_token().value == "_") {
    pattern.kind = MatchPatternKind::WILDCARD;
    parser.advance();
    return true;
  }

  if (parser.check(TokenName::AG_BOOL_LITERAL)) {
    // `true`/`false` are their own token kind, but they compare exactly like
    // the integer literals `1`/`0` that they represent.
    pattern.kind    = MatchPatternKind::LITERAL;
    pattern.literal = std::string(parser.current_token().value);
    parser.advance();
    return true;
  }

  if (parser.check(TokenName::AG_INT_LITERAL)) {
    MatchPatternAlternative first;
    if (!parse_match_numeric_alternative(parser, first))
      return false;
    pattern.alternatives.push_back(first);

    // `100 | 200 | 300` matches any of the listed alternatives.
    while (parser.match(TokenName::AG_BITWISE_OR)) {
      MatchPatternAlternative next;
      if (!parse_match_numeric_alternative(parser, next))
        return false;
      pattern.alternatives.push_back(next);
    }

    if (pattern.alternatives.size() > 1) {
      pattern.kind = MatchPatternKind::ALTERNATION;
    } else if (first.is_range) {
      pattern.kind = MatchPatternKind::RANGE;
    } else {
      pattern.kind    = MatchPatternKind::LITERAL;
      pattern.literal = first.literal;
    }
    return true;
  }

  if (parser.check(TokenName::AG_IDENTIFIER) &&
      parser.peek_token().token_name == TokenName::AG_AT) {
    // `n @ 10..=99` binds the scrutinee value when the pattern matches.
    pattern.binding_name = std::string(parser.current_token().value);
    parser.advance(); // binding name
    parser.advance(); // '@'

    MatchPatternAlternative alternative;
    if (!parse_match_numeric_alternative(parser, alternative))
      return false;
    pattern.alternatives.push_back(alternative);

    if (alternative.is_range) {
      pattern.kind = MatchPatternKind::RANGE;
    } else {
      pattern.kind    = MatchPatternKind::LITERAL;
      pattern.literal = alternative.literal;
    }
    return true;
  }

  if (!parser.check(TokenName::AG_IDENTIFIER)) {
    print_error(error::ExpectedPattern, parser);
    return false;
  }

  std::string path(parser.current_token().value);
  parser.advance();
  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile
                                     : error::ExpectedEnumMemberName,
                  parser);
      return false;
    }
    path += "::";
    path += parser.current_token().value;
    parser.advance();
  }

  const auto separator = path.rfind("::");
  if (separator == std::string::npos) {
    // `Some(value)` / `Ok(value)` on pointer-shaped optionals and results.
    pattern.kind         = MatchPatternKind::OPAQUE_VARIANT;
    pattern.variant_name = path;
    return parse_parenthesized_bindings();
  }

  pattern.kind         = MatchPatternKind::ENUM_VARIANT;
  pattern.enum_name    = path.substr(0, separator);
  pattern.variant_name = path.substr(separator + 2);

  if (parser.check(TokenName::AG_LEFT_PAREN))
    return parse_parenthesized_bindings();
  if (parser.match(TokenName::AG_LEFT_BRACE))
    return parse_struct_bindings();
  return true;
}

bool
analyze_match_pattern(Parser&               parser,
                      MatchPattern&         pattern,
                      const DataType&       scrutinee_type,
                      const SourceLocation&) {
  const auto declare_binding = [&](const std::string&    name,
                                   const DataType&       type,
                                   const SourceLocation& location) {
    Declaration declaration;
    declaration.type     = type;
    declaration.timing   = ValueTiming::RUNTIME;
    declaration.location = location;
    if (!get_symbol_table().insert(name, declaration)) {
      print_error(error::RedefinitionOfSymbol, parser, location);
      return false;
    }
    return true;
  };

  if (pattern.kind == MatchPatternKind::ENUM_VARIANT) {
    if (scrutinee_type.kind != TypeKind::ENUM) {
      print_error(error::IncompatibleTypes, parser, pattern.location);
      return false;
    }

    const auto resolved =
        resolve_enum_variant(parser,
                             pattern.enum_name + "::" + pattern.variant_name);
    if (!resolved) {
      print_error(error::UndefinedMember, parser, pattern.location);
      return false;
    }
    if (resolved->definition->name != scrutinee_type.type_name) {
      print_error(error::IncompatibleTypes, parser, pattern.location);
      return false;
    }

    pattern.enum_name     = resolved->definition->name;
    pattern.variant_index = resolved->variant->index;

    if (pattern.bindings.size() != resolved->variant->fields.size()) {
      print_error(error::InvalidArgumentCount, parser, pattern.location);
      return false;
    }

    for (size_t i = 0; i < pattern.bindings.size(); ++i) {
      auto&       binding = pattern.bindings[i];
      const auto* field   = binding.field_name.empty()
                                ? &resolved->variant->fields[i]
                                : resolved->variant->find_field(binding.field_name);
      if (!field) {
        print_error(error::UndefinedMember, parser, binding.location);
        return false;
      }

      binding.field_index =
          static_cast<size_t>(field - resolved->variant->fields.data());
      binding.type = field->type;
      if (!declare_binding(binding.name, field->type, binding.location))
        return false;
    }
    return true;
  }

  if (pattern.kind == MatchPatternKind::OPAQUE_VARIANT) {
    DataType binding_type = scrutinee_type;
    if (binding_type.primitive_type == PrimitiveType::UNEXPECTED) {
      binding_type.kind           = TypeKind::PRIMITIVE;
      binding_type.primitive_type = PrimitiveType::INT;
      binding_type.size_in_bytes  = 8;
      binding_type.alignment      = 8;
    }
    for (auto& binding : pattern.bindings)
      if (!declare_binding(binding.name, binding_type, binding.location))
        return false;
    return true;
  }

  if (pattern.kind == MatchPatternKind::LITERAL ||
      pattern.kind == MatchPatternKind::RANGE ||
      pattern.kind == MatchPatternKind::ALTERNATION) {
    if (scrutinee_type.primitive_type == PrimitiveType::UNEXPECTED ||
        !is_compatible_datatype(PrimitiveType::INT,
                                scrutinee_type.primitive_type)) {
      print_error(error::InvalidOperandTypes, parser, pattern.location);
      return false;
    }

    if (!pattern.binding_name.empty()) {
      DataType binding_type;
      binding_type.kind           = TypeKind::PRIMITIVE;
      binding_type.primitive_type = PrimitiveType::INT;
      binding_type.size_in_bytes  = 8;
      binding_type.alignment      = 8;
      if (!declare_binding(pattern.binding_name, binding_type, pattern.location))
        return false;
    }
  }

  return true;
}

std::string
generate_match_pattern_condition(LabelCounters&      counter,
                                 Parser&             parser,
                                 const MatchPattern& pattern,
                                 const std::string&  scrutinee_value) {
  if (pattern.kind == MatchPatternKind::WILDCARD)
    return {};

  const auto emit_compare = [&](const std::string& left,
                                const std::string& right,
                                BIN_OPERATOR       op) {
    auto value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(
        value, left, right, op, counter.function_index));
    return value;
  };

  if (pattern.kind == MatchPatternKind::ENUM_VARIANT) {
    const auto tag = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<MemberLoad>(tag, scrutinee_value, 0, counter.function_index));
    return emit_compare(tag,
                        std::to_string(pattern.variant_index),
                        BIN_OPERATOR::EQUAL_TO);
  }

  if (pattern.kind == MatchPatternKind::OPAQUE_VARIANT)
    return emit_compare(scrutinee_value, "0", BIN_OPERATOR::NOT_EQUAL_TO);

  if (pattern.kind == MatchPatternKind::LITERAL)
    return emit_compare(scrutinee_value, pattern.literal, BIN_OPERATOR::EQUAL_TO);

  const auto emit_alternative = [&](const MatchPatternAlternative& alternative) {
    if (!alternative.is_range)
      return emit_compare(scrutinee_value, alternative.literal,
                          BIN_OPERATOR::EQUAL_TO);

    const auto lower = emit_compare(scrutinee_value,
                                    alternative.range_start,
                                    BIN_OPERATOR::GREATER_THAN_EQUAL);
    const auto upper = emit_compare(
        scrutinee_value, alternative.range_end,
        alternative.inclusive ? BIN_OPERATOR::LESS_THAN_EQUAL
                              : BIN_OPERATOR::LESS_THAN);
    return emit_compare(lower, upper, BIN_OPERATOR::LOGICAL_AND);
  };

  std::string condition;
  for (const auto& alternative : pattern.alternatives) {
    const auto alternative_condition = emit_alternative(alternative);
    condition = condition.empty()
                    ? alternative_condition
                    : emit_compare(condition, alternative_condition,
                                   BIN_OPERATOR::LOGICAL_OR);
  }
  return condition;
}

void
generate_match_pattern_bindings(LabelCounters&      counter,
                                Parser&             parser,
                                const MatchPattern& pattern,
                                const std::string&  scrutinee_value) {
  const auto assign_binding = [&](const std::string& name, const DataType& type) {
    auto variable = get_ir_table().declare_user(counter.function_index,
                                                name,
                                                get_type_size(type));
    const std::string destination = variable ? variable->name : name;
    parser.add_instructios(
        AG_cscope<Assign>(scrutinee_value, destination, counter.function_index));
  };

  if (pattern.kind == MatchPatternKind::ENUM_VARIANT) {
    const auto* definition = get_enum_table().lookup(pattern.enum_name);
    if (!definition || pattern.variant_index >= definition->variants.size())
      return;

    const auto& variant = definition->variants[pattern.variant_index];
    for (const auto& binding : pattern.bindings) {
      if (binding.field_index >= variant.fields.size())
        continue;

      const auto value = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<MemberLoad>(
          value, scrutinee_value,
          static_cast<uint32_t>(variant.fields[binding.field_index].byte_offset),
          counter.function_index));

      auto variable = get_ir_table().declare_user(counter.function_index,
                                                  binding.name,
                                                  get_type_size(binding.type));
      const std::string destination = variable ? variable->name : binding.name;
      parser.add_instructios(
          AG_cscope<Assign>(value, destination, counter.function_index));
    }
    return;
  }

  DataType binding_type;
  binding_type.kind           = TypeKind::PRIMITIVE;
  binding_type.primitive_type = PrimitiveType::INT;
  binding_type.size_in_bytes  = 8;
  binding_type.alignment      = 8;

  if (pattern.kind == MatchPatternKind::OPAQUE_VARIANT) {
    for (const auto& binding : pattern.bindings)
      assign_binding(binding.name, binding_type);
    return;
  }

  if (!pattern.binding_name.empty())
    assign_binding(pattern.binding_name, binding_type);
}

AG_scope<Expression>
MatchExpression::parse(Parser& parser) {
  const Token match_token = parser.current_token();
  parser.advance(); // `match`

  auto match_expr        = AG_cscope<MatchExpression>();
  match_expr->location   = {match_token.line, match_token.column};

  const bool parenthesized = parser.match(TokenName::AG_LEFT_PAREN);
  match_expr->scrutinee = Expression::parse_pratt(parser, 0);
  if (!match_expr->scrutinee)
    return nullptr;
  if (parenthesized &&
      !parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen))
    return nullptr;

  if (!parser.expect(TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace))
    return nullptr;

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    MatchArm arm;
    arm.location = {parser.current_token().line, parser.current_token().column};

    if (!parse_match_pattern(parser, arm.pattern, arm.location))
      return nullptr;

    if (parser.match(TokenName::AG_IF)) {
      arm.guard = Expression::parse_pratt(parser, 0);
      if (!arm.guard)
        return nullptr;
    }

    if (!parser.expect(TokenName::AG_FAT_ARROW, error::ExpectedFatArrow))
      return nullptr;

    arm.body = Expression::parse_pratt(parser, 0);
    if (!arm.body)
      return nullptr;

    match_expr->arms.push_back(std::move(arm));

    if (parser.match(TokenName::AG_COMMA))
      continue;
    if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      print_error(error::ExpectedComma, parser);
      return nullptr;
    }
  }

  parser.advance(); // `}`
  return match_expr;
}

PrimitiveType
MatchExpression::get_type() {
  return result_type.primitive_type;
}

DataType
MatchExpression::get_data_type() {
  return result_type;
}

bool
MatchExpression::analyze_semantics(Parser&               parser,
                                   const SourceLocation& location) {
  const auto& diagnostic_location = this->source_location(location);

  if (!scrutinee ||
      !scrutinee->analyze_semantics(parser, scrutinee->source_location(diagnostic_location)))
    return false;

  const auto scrutinee_type = scrutinee->get_data_type();
  bool       first_result   = true;

  for (auto& arm : arms) {
    get_symbol_table().enter_scope();

    auto fail = [&]() {
      get_symbol_table().exit_scope();
      return false;
    };

    if (!analyze_match_pattern(parser,
                               arm.pattern,
                               scrutinee_type,
                               arm.pattern.location))
      return fail();

    if (arm.guard &&
        !arm.guard->analyze_semantics(parser,
                                      arm.guard->source_location(diagnostic_location)))
      return fail();

    if (!arm.body ||
        !arm.body->analyze_semantics(parser,
                                     arm.body->source_location(diagnostic_location)))
      return fail();

    const auto body_type = arm.body->get_data_type();
    if (first_result) {
      result_type  = body_type;
      first_result = false;
    } else if (!is_compatible_type(result_type, body_type)) {
      print_error(error::IncompatibleTypes, parser, arm.body->source_location(diagnostic_location));
      return fail();
    }

    get_symbol_table().exit_scope();
  }

  if (first_result) {
    print_error(error::ExpectedPattern, parser, diagnostic_location);
    return false;
  }

  return true;
}

std::string
MatchExpression::generate_ir(LabelCounters& counter,
                             Parser&        parser) {
  if (!scrutinee)
    return "0";

  const auto scrutinee_value = scrutinee->generate_ir(counter, parser);

  std::string result;
  if (result_type.primitive_type != PrimitiveType::VOID) {
    result = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Assign>("0", result, counter.function_index));
  }

  const auto end_label = counter.block_label++;

  for (const auto& arm : arms) {
    const auto next_label = counter.block_label++;

    const std::string condition = generate_match_pattern_condition(
        counter, parser, arm.pattern, scrutinee_value);

    if (!condition.empty())
      parser.add_instructios(
          AG_cscope<CondJump>(condition, "b" + std::to_string(next_label), counter.function_index));

    generate_match_pattern_bindings(counter, parser, arm.pattern, scrutinee_value);

    if (arm.guard) {
      const auto guard_value = arm.guard->generate_ir(counter, parser);
      parser.add_instructios(
          AG_cscope<CondJump>(guard_value, "b" + std::to_string(next_label), counter.function_index));
    }

    const auto body_value = arm.body->generate_ir(counter, parser);
    if (!result.empty())
      parser.add_instructios(AG_cscope<Assign>(body_value, result, counter.function_index));

    parser.add_instructios(AG_cscope<Jump>("b" + std::to_string(end_label), counter.function_index));
    parser.add_instructios(AG_cscope<Label>("b" + std::to_string(next_label)));
  }

  parser.add_instructios(AG_cscope<Label>("b" + std::to_string(end_label)));
  return result;
}

} // namespace ag
