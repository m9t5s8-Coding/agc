#include "Statements/for.hpp"

#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/Binary.hpp"
#include "IR/CondJump.hpp"
#include "IR/DerefLoad.hpp"
#include "IR/IR.hpp"
#include "IR/IRTable.hpp"
#include "IR/Jump.hpp"
#include "IR/Label.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <algorithm>

namespace ag {
namespace {
bool
is_word(const Parser& parser,
        std::string_view word) {
  return parser.check(TokenName::AG_IDENTIFIER) &&
         parser.current_token().value == word;
}

DataType
make_int_type() {
  DataType type;
  type.primitive_type = PrimitiveType::INT;
  type.size_in_bytes  = 8;
  type.alignment      = 8;
  return type;
}

bool
insert_loop_binding(Parser& parser,
                    const std::string& name,
                    const DataType& type,
                    const SourceLocation& location) {
  Declaration data;
  data.type     = type;
  data.timing   = ValueTiming::RUNTIME;
  data.location = location;
  if (!get_symbol_table().insert(name, data)) {
    print_error(error::RedefinitionOfSymbol, parser, location);
    return false;
  }
  return true;
}
} // namespace

AG_scope<Statements>
ForStatement::ParseFor(Parser& parser) {
  const Token for_token = parser.current_token();
  parser.advance(); // `for`

  auto for_stmt        = AG_cscope<ForStatement>();
  for_stmt->m_location = {for_token.line, for_token.column};

  // `for i, item in values` and `for (i, item) in values` are equivalent.
  const bool parenthesized = parser.match(TokenName::AG_LEFT_PAREN);
  while (true) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(error::ExpectedIdentifier, parser);
      return nullptr;
    }
    for_stmt->m_names.emplace_back(parser.current_token().value);
    parser.advance();

    if (!parser.match(TokenName::AG_COMMA))
      break;
  }
  if (parenthesized &&
      !parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen))
    return nullptr;

  if (!is_word(parser, "in")) {
    print_error(error::ExpectedIdentifier, parser);
    return nullptr;
  }
  parser.advance(); // `in`

  AG_scope<Expression> iterable;
  if (parser.check(TokenName::AG_DOT_DOT)) {
    iterable = RangeExpression::parse_open_start(parser);
  } else {
    iterable = Expression::parse_expressions(parser);
    if (iterable && parser.check(TokenName::AG_DOT_DOT))
      iterable = RangeExpression::parse_after_start(parser, std::move(iterable));
  }
  if (!iterable)
    return nullptr;
  for_stmt->m_iterable = std::move(iterable);

  if (is_word(parser, "by")) {
    parser.advance();
    for_stmt->m_step = Expression::parse_expressions(parser);
    if (!for_stmt->m_step)
      return nullptr;
  }

  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, for_stmt->m_body, nullptr);

  return for_stmt;
}

bool
ForStatement::analyze_semantics(Parser& parser) {
  if (m_names.empty()) {
    print_error(error::ExpectedIdentifier, parser, m_location);
    return false;
  }

  if (auto* range = dynamic_cast<RangeExpression*>(m_iterable.get())) {
    if (m_names.size() != 1) {
      print_error(error::InvalidArgumentCount, parser, m_location);
      return false;
    }
    if (!range->analyze_semantics(parser, range->source_location(m_location)))
      return false;
    if (!range->end) {
      print_error(error::InvalidOperandTypes, parser, range->source_location(m_location));
      return false;
    }
    if (m_step && !m_step->analyze_semantics(parser, m_step->source_location(m_location)))
      return false;

    m_is_range = true;
    get_symbol_table().enter_scope();
    const bool inserted =
        insert_loop_binding(parser, m_names[0], make_int_type(), m_location);
    parser.enter_loop();
    const bool body_ok = inserted && m_body->analyze_semantics(parser);
    parser.exit_loop();
    get_symbol_table().exit_scope();
    return body_ok;
  }

  if (!m_iterable->analyze_semantics(parser, m_iterable->source_location(m_location)))
    return false;

  auto collection_type = m_iterable->get_data_type();
  if (collection_type.kind == TypeKind::REFERENCE && collection_type.pointee)
    collection_type = *collection_type.pointee;

  m_is_variadic = collection_type.is_variadic;
  m_is_map      = collection_type.is_map && collection_type.map_key_type &&
             collection_type.pointee;
  if (m_is_map) {
    if (m_names.size() > 2) {
      print_error(error::InvalidArgumentCount, parser, m_location);
      return false;
    }
    m_item_type       = *collection_type.pointee;
    m_key_type        = *collection_type.map_key_type;
    m_iteration_count = collection_type.array_length.value_or(0);

    get_symbol_table().enter_scope();
    bool       inserted = true;
    const auto int_type = make_int_type();
    if (m_names.size() == 2)
      inserted = insert_loop_binding(parser, m_names[0], m_key_type, m_location);
    if (inserted)
      inserted = insert_loop_binding(parser, m_names.back(), m_item_type, m_location);
    parser.enter_loop();
    const bool body_ok = inserted && m_body->analyze_semantics(parser);
    parser.exit_loop();
    get_symbol_table().exit_scope();
    return body_ok;
  }

  if (collection_type.kind != TypeKind::POINTER || !collection_type.pointee ||
      (!collection_type.array_length && !m_is_variadic)) {
    print_error(error::InvalidOperandTypes, parser, m_iterable->source_location(m_location));
    return false;
  }

  m_item_type = *collection_type.pointee;
  if (collection_type.array_length)
    m_iteration_count = *collection_type.array_length;

  if (m_names.size() > 2) {
    print_error(error::InvalidArgumentCount, parser, m_location);
    return false;
  }

  get_symbol_table().enter_scope();
  bool       inserted = true;
  const auto int_type = make_int_type();
  if (m_names.size() == 2)
    inserted = insert_loop_binding(parser, m_names[0], int_type, m_location);
  if (inserted)
    inserted = insert_loop_binding(parser, m_names.back(), m_item_type, m_location);
  parser.enter_loop();
  const bool body_ok = inserted && m_body->analyze_semantics(parser);
  parser.exit_loop();
  get_symbol_table().exit_scope();
  return body_ok;
}

void
ForStatement::generate_ir(LabelCounters& counter,
                          Parser&        parser) {
  if (m_is_range) {
    auto* range = dynamic_cast<RangeExpression*>(m_iterable.get());
    if (!range || !range->end)
      return;

    auto variable = get_ir_table().declare_user(
        counter.function_index, m_names[0], get_type_size(make_int_type()));
    const std::string loop_variable = variable ? variable->name : m_names[0];

    const auto start_value = range->start
                                 ? range->start->generate_ir(counter, parser)
                                 : std::string("0");
    parser.add_instructios(
        AG_cscope<Assign>(start_value, loop_variable, counter.function_index));

    const auto end_value = range->end->generate_ir(counter, parser);
    const auto end_slot  = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Assign>(end_value, end_slot, counter.function_index));

    std::string step_value = "1";
    if (m_step)
      step_value = m_step->generate_ir(counter, parser);

    const auto condition_label = "b" + std::to_string(counter.block_label++);
    const auto body_label      = "b" + std::to_string(counter.block_label++);
    const auto continue_label  = "b" + std::to_string(counter.block_label++);
    const auto end_label       = "b" + std::to_string(counter.block_label++);

    parser.add_instructios(AG_cscope<Label>(condition_label));
    const auto comparison = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(
        comparison,
        loop_variable,
        end_slot,
        range->inclusive ? BIN_OPERATOR::LESS_THAN_EQUAL : BIN_OPERATOR::LESS_THAN,
        counter.function_index));
    parser.add_instructios(
        AG_cscope<CondJump>(comparison, end_label, counter.function_index));
    parser.add_instructios(AG_cscope<Label>(body_label));

    parser.push_loop_labels(end_label, continue_label, m_label);
    m_body->generate_ir(counter, parser);
    parser.pop_loop_labels();
    parser.add_instructios(AG_cscope<Label>(continue_label));

    const auto updated = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(updated,
                                             loop_variable,
                                             step_value,
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    parser.add_instructios(
        AG_cscope<Assign>(updated, loop_variable, counter.function_index));
    parser.add_instructios(AG_cscope<Jump>(condition_label));
    parser.add_instructios(AG_cscope<Label>(end_label));
    return;
  }

  // Iterating a map literal walks `[count][key][value]...` pairs.
  if (m_is_map) {
    const auto base_value = m_iterable->generate_ir(counter, parser);
    const auto base_slot  = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Assign>(base_value, base_slot, counter.function_index));

    const auto count_slot = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        count_slot, base_slot, false, counter.function_index));

    const std::string index_slot = make_temp(counter, parser);
    parser.add_instructios(
        AG_cscope<Assign>("0", index_slot, counter.function_index));

    const std::string& key_name = m_names.size() == 2 ? m_names[0] : m_names.back();
    const std::string& value_name = m_names.back();

    const auto bind_value = [&](const std::string& name,
                                const DataType&    type,
                                const std::string& address) {
      auto variable = get_ir_table().declare_user(
          counter.function_index, name, get_type_size(type));
      const std::string slot = variable ? variable->name : name;
      if (type.kind == TypeKind::STRUCT || type.kind == TypeKind::ENUM) {
        parser.add_instructios(
            AG_cscope<Assign>(address, slot, counter.function_index));
      } else {
        const auto loaded = make_temp(counter, parser);
        parser.add_instructios(AG_cscope<DerefLoad>(
            loaded,
            address,
            counter.function_index,
            std::max<size_t>(1, get_type_size(type)),
            type.is_signed));
        parser.add_instructios(
            AG_cscope<Assign>(loaded, slot, counter.function_index));
      }
      return slot;
    };

    const auto condition_label = "b" + std::to_string(counter.block_label++);
    const auto body_label      = "b" + std::to_string(counter.block_label++);
    const auto continue_label  = "b" + std::to_string(counter.block_label++);
    const auto end_label       = "b" + std::to_string(counter.block_label++);

    parser.add_instructios(AG_cscope<Label>(condition_label));
    const auto comparison = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(comparison,
                                             index_slot,
                                             count_slot,
                                             BIN_OPERATOR::LESS_THAN,
                                             counter.function_index));
    parser.add_instructios(
        AG_cscope<CondJump>(comparison, end_label, counter.function_index));
    parser.add_instructios(AG_cscope<Label>(body_label));

    const auto offset = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(offset,
                                             index_slot,
                                             "16",
                                             BIN_OPERATOR::MULTIPLICATION,
                                             counter.function_index));
    const auto entries = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(entries,
                                             base_slot,
                                             "8",
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    const auto entry_address = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(entry_address,
                                             entries,
                                             offset,
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));

    if (m_names.size() == 2)
      bind_value(key_name, m_key_type, entry_address);
    const auto value_address = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(value_address,
                                             entry_address,
                                             "8",
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    bind_value(value_name, m_item_type, value_address);

    parser.push_loop_labels(end_label, continue_label, m_label);
    m_body->generate_ir(counter, parser);
    parser.pop_loop_labels();
    parser.add_instructios(AG_cscope<Label>(continue_label));

    const auto updated_index = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(updated_index,
                                             index_slot,
                                             "1",
                                             BIN_OPERATOR::ADDITION,
                                             counter.function_index));
    parser.add_instructios(
        AG_cscope<Assign>(updated_index, index_slot, counter.function_index));
    parser.add_instructios(AG_cscope<Jump>(condition_label));
    parser.add_instructios(AG_cscope<Label>(end_label));
    return;
  }

  // Iterating a pointer with a statically known array length.
  auto collection_type = m_iterable->get_data_type();
  if (collection_type.kind == TypeKind::REFERENCE && collection_type.pointee)
    collection_type = *collection_type.pointee;
  if (!collection_type.pointee)
    return;

  const auto base_value = m_iterable->generate_ir(counter, parser);
  const auto base_slot  = make_temp(counter, parser);
  parser.add_instructios(
      AG_cscope<Assign>(base_value, base_slot, counter.function_index));

  std::string index_slot;
  if (m_names.size() == 2) {
    auto index_variable = get_ir_table().declare_user(
        counter.function_index, m_names[0], get_type_size(make_int_type()));
    index_slot = index_variable ? index_variable->name : m_names[0];
  } else {
    index_slot = make_temp(counter, parser);
  }
  parser.add_instructios(
      AG_cscope<Assign>("0", index_slot, counter.function_index));

  const std::string& item_name = m_names.back();
  auto item_variable = get_ir_table().declare_user(
      counter.function_index, item_name, get_type_size(m_item_type));
  const std::string item_slot = item_variable ? item_variable->name : item_name;

  const size_t element_scale =
      std::max<size_t>(1, get_type_size(m_item_type));
  const bool aggregate = m_item_type.kind == TypeKind::STRUCT ||
                         m_item_type.kind == TypeKind::ENUM;

  // Variadic arguments are passed as `[count][element 0]...`; the parameter
  // points at element 0, so the count is one slot before it.
  std::string iteration_limit = std::to_string(m_iteration_count);
  if (m_is_variadic) {
    const auto count_address = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<Binary>(count_address,
                                              base_slot,
                                              "8",
                                              BIN_OPERATOR::SUBTRACTION,
                                              counter.function_index));
    const auto count_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        count_value, count_address, false, counter.function_index));
    iteration_limit = count_value;
  }

  const auto condition_label = "b" + std::to_string(counter.block_label++);
  const auto body_label      = "b" + std::to_string(counter.block_label++);
  const auto continue_label  = "b" + std::to_string(counter.block_label++);
  const auto end_label       = "b" + std::to_string(counter.block_label++);

  parser.add_instructios(AG_cscope<Label>(condition_label));
  const auto comparison = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(comparison,
                                            index_slot,
                                            iteration_limit,
                                            BIN_OPERATOR::LESS_THAN,
                                            counter.function_index));
  parser.add_instructios(
      AG_cscope<CondJump>(comparison, end_label, counter.function_index));
  parser.add_instructios(AG_cscope<Label>(body_label));

  const auto offset = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(offset,
                                            index_slot,
                                            std::to_string(element_scale),
                                            BIN_OPERATOR::MULTIPLICATION,
                                            counter.function_index));
  const auto address = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(
      address, base_slot, offset, BIN_OPERATOR::ADDITION, counter.function_index));

  if (aggregate) {
    parser.add_instructios(
        AG_cscope<Assign>(address, item_slot, counter.function_index));
  } else {
    const auto item_value = make_temp(counter, parser);
    parser.add_instructios(AG_cscope<DerefLoad>(
        item_value,
        address,
        counter.function_index,
        std::max<size_t>(1, get_type_size(m_item_type)),
        m_item_type.is_signed));
    parser.add_instructios(
        AG_cscope<Assign>(item_value, item_slot, counter.function_index));
  }

  parser.push_loop_labels(end_label, continue_label, m_label);
  m_body->generate_ir(counter, parser);
  parser.pop_loop_labels();
  parser.add_instructios(AG_cscope<Label>(continue_label));

  const auto updated_index = make_temp(counter, parser);
  parser.add_instructios(AG_cscope<Binary>(updated_index,
                                            index_slot,
                                            "1",
                                            BIN_OPERATOR::ADDITION,
                                            counter.function_index));
  parser.add_instructios(
      AG_cscope<Assign>(updated_index, index_slot, counter.function_index));
  parser.add_instructios(AG_cscope<Jump>(condition_label));
  parser.add_instructios(AG_cscope<Label>(end_label));
}
} // namespace ag
