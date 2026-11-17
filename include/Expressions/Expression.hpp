#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include <deque>
#include <memory>
#include <vector>

namespace ag {
enum class BIN_OPERATOR {
  NONE,
  ADDITION,
  SUBTRACTION,
  MULTIPLICATION,
  DIVISION,
  MODULUS,
  BITWISE_XOR,
  BITWISE_AND,
  BITWISE_OR,
  LESS_THAN,
  LESS_THAN_EQUAL,
  GREATER_THAN,
  GREATER_THAN_EQUAL,
  EQUAL_TO,
  ASSIGN_TO,
  NOT_EQUAL_TO,
  LOGICAL_AND,
  LOGICAL_OR,
  LEFT_SHIFT,
  RIGHT_SHIFT,
  INDEXING,
  ARROW
};

inline std::string_view
to_symbol(BIN_OPERATOR o) {
  switch (o) {
  case BIN_OPERATOR::ADDITION:
    return "+";
  case BIN_OPERATOR::SUBTRACTION:
    return "-";
  case BIN_OPERATOR::MULTIPLICATION:
    return "*";
  case BIN_OPERATOR::DIVISION:
    return "/";
  case BIN_OPERATOR::MODULUS:
    return "%";

  case BIN_OPERATOR::BITWISE_XOR:
    return "^";
  case BIN_OPERATOR::BITWISE_AND:
    return "&";
  case BIN_OPERATOR::BITWISE_OR:
    return "|";
  case BIN_OPERATOR::LEFT_SHIFT:
    return "<<";
  case BIN_OPERATOR::RIGHT_SHIFT:
    return ">>";

  case BIN_OPERATOR::LESS_THAN:
    return "<";
  case BIN_OPERATOR::LESS_THAN_EQUAL:
    return "<=";
  case BIN_OPERATOR::GREATER_THAN:
    return ">";
  case BIN_OPERATOR::GREATER_THAN_EQUAL:
    return ">=";
  case BIN_OPERATOR::EQUAL_TO:
    return "==";
  case BIN_OPERATOR::NOT_EQUAL_TO:
    return "!=";

  case BIN_OPERATOR::ASSIGN_TO:
    return "=";
  case BIN_OPERATOR::LOGICAL_AND:
    return "&&";
  case BIN_OPERATOR::LOGICAL_OR:
    return "||";

  case BIN_OPERATOR::INDEXING:
    return "[]";
  case BIN_OPERATOR::ARROW:
    return "->";

  case BIN_OPERATOR::NONE:
    return "<none>";
  }
  return "<unknown>";
}

enum class UN_OPERATOR {
  PRE_INCREMENT,
  POST_INCREMENT,
  PRE_DECREMENT,
  POST_DECREMENT,
  LOGICAl_NOT,
  BITWISE_NOT,
  DEREFRENCE,
  REFRENCE,
  POSITIVE,
  NEGATIVE,
  NONE,
};

enum class PrimitiveType;

class Parser;

// Allocate a compiler-generated temporary slot in the current function.
std::string
make_temp(LabelCounters& counter,
          Parser&        parser);

// Wrap `value` to the storage width of `type` and return the resulting i64
// expression.  Used when assigning into a local whose declared type is one of
// the fixed-width integers or `bool`; wider and aggregate types pass through.
std::string
narrow_value(const std::string& value,
             const DataType&    type,
             LabelCounters&     counter,
             Parser&            parser);

class Expression {
public:
  Expression() {}
  virtual ~Expression() {}

  static AG_scope<Expression>
  parse_expressions(Parser& parser);

  static AG_scope<Expression>
  parse_pratt(Parser&,
              float min_bp);

  virtual PrimitiveType
  get_type();

  virtual DataType
  get_data_type();

  // Raw storage type.  Reference variables expose their pointee through
  // `get_data_type()` so ordinary expressions transparently read the referred
  // value; this hook lets dereference/address-of code recover the reference
  // itself when an address is needed.
  virtual DataType
  get_declared_data_type() {
    return get_data_type();
  }

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) {
    return "";
  }

  // Produce the expression's address/reference value without applying the
  // implicit dereference used for reading reference variables.
  virtual std::string
  generate_reference_ir(LabelCounters& counter,
                        Parser&         parser) {
    return generate_ir(counter, parser);
  }

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) {
    return true;
  }

  // Prefer the node's own token location, falling back to the statement
  // location supplied by the caller when it was never populated.
  const SourceLocation&
  source_location(const SourceLocation& fallback) const {
    return location.line != 0 ? location : fallback;
  }

  SourceLocation location;
};

class BinaryExpression : public Expression {
public:
  BinaryExpression()
      : bin_operator{BIN_OPERATOR::NONE},
        left{nullptr},
        right{nullptr} {}

  BinaryExpression(BIN_OPERATOR         bin_op,
                   AG_scope<Expression> p_left,
                   AG_scope<Expression> p_right)
      : bin_operator(bin_op),
        left(std::move(p_left)),
        right(std::move(p_right)) {}

  virtual ~BinaryExpression() {}

  static AG_scope<Expression>
  parse_binary(Parser&);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  BIN_OPERATOR         bin_operator = BIN_OPERATOR::NONE;
  AG_scope<Expression> left;
  AG_scope<Expression> right;
  std::string          operator_label;
  DataType             result_type;
  bool                 result_type_known     = false;
  // Set for the built-in `base[index]` form on pointers, references, and byte
  // addresses.  The scale and load width are cached during semantic analysis
  // so IR generation does not need to re-resolve the base type.
  bool                 is_string_index       = false;
  bool                 is_slice_index        = false;
  size_t               index_scale           = 1;
  bool                 index_loads_byte      = false;
  // Storage width/signedness of one indexed element.  `char`/`bool`/`u8`
  // use one byte, `i16`/`u16` two, and so on.  The default keeps the old
  // pointer-sized behavior for loop and legacy call sites.
  size_t               index_width           = 8;
  bool                 index_signed          = true;
  bool                 is_compound_assignment = false;

  // Division, modulo, right shift, and ordered comparisons select unsigned
  // instructions when an operand is unsigned.
  bool                 is_unsigned_operation = false;

  // `ptr + n` and `n + ptr` scale the integer by the pointee size.  The flag
  // and scale are resolved during semantic analysis so IR generation does not
  // need the symbol table again.
  bool                 is_pointer_arithmetic = false;
  bool                 pointer_is_left       = false;
  size_t               pointer_scale         = 1;

  // `ref = value` and `ref += value` write through a reference variable
  // instead of rebinding the reference itself.
  bool                 is_reference_target = false;

  // Operator parameters may be references.  These flags record which operand
  // must be passed as an address because the corresponding parameter is
  // reference-typed (for example `cin >> value`).
  bool                 left_is_operator_reference  = false;
  bool                 right_is_operator_reference = false;
};

// `condition ? then_expression : else_expression`
class ConditionalExpression : public Expression {
public:
  ConditionalExpression() {}
  virtual ~ConditionalExpression() {}

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> condition;
  AG_scope<Expression> then_branch;
  AG_scope<Expression> else_branch;
  DataType             result_type;
  bool                 result_type_known = false;
};

// `value as Type` converts between primitive representations.  Integer and
// character casts truncate to their storage width; float conversions are
// accepted but currently keep the operand's bit pattern.
class CastExpression : public Expression {
public:
  CastExpression() {}
  virtual ~CastExpression() {}

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> operand;
  DataType             target_type;
};

// `channel <- value` sends a value.  Without a concurrent runtime the channel
// operand is evaluated for its side effects and the value is passed through.
class ChannelSendExpression : public Expression {
public:
  ChannelSendExpression() {}
  virtual ~ChannelSendExpression() {}

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> channel;
  AG_scope<Expression> value;
};

// `needle in haystack` tests membership in a range or array literal.
class InExpression : public Expression {
public:
  InExpression() {}
  virtual ~InExpression() {}

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> needle;
  AG_scope<Expression> haystack;
};

// `(a, b, ...)` tuple literal.  Elements are stored in pointer-sized slots in
// a heap block, matching the representation used by `let (a, b) = tuple;`.
class TupleExpression : public Expression {
public:
  TupleExpression() {}
  virtual ~TupleExpression() {}

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  std::vector<AG_scope<Expression>> elements;
  std::vector<DataType>             element_types;
  DataType                          result_type;
};

// Anonymous functions.  The syntax is parsed (including `move` closures and
// both `=> expr` and `{ ... }` bodies), but lambdas do not have a callable
// representation yet: the value is an opaque zero until first-class function
// pointers and closure environments exist.
class LambdaExpression : public Expression {
public:
  LambdaExpression() {}
  virtual ~LambdaExpression() {}

  // True when the current token starts a lambda rather than a parenthesized
  // expression: `(params) -> ...` or `(params) => ...`.
  static bool
  is_lambda_ahead(Parser& parser);

  static AG_scope<Expression>
  parse(Parser& parser);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  DataType result_type;
};

// `return expression` used as the fallback of `value orelse return ...`.
// The expression's value is returned from the enclosing function; the node
// itself yields the same type so surrounding expressions still type-check.
class ReturnExpression : public Expression {
public:
  ReturnExpression() {}
  virtual ~ReturnExpression() {}

  static AG_scope<Expression>
  parse(Parser& parser);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> value;
  DataType             result_type;
};

// `value orelse fallback` / `value ?? fallback`.  Until Option/Result have a
// real representation, the zero value is treated as absent.
class OrelseExpression : public Expression {
public:
  OrelseExpression() {}
  virtual ~OrelseExpression() {}

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> value;
  AG_scope<Expression> fallback;
  DataType             result_type;
  bool                 result_type_known = false;
};

class UniaryExpression : public Expression {
public:
  UniaryExpression() {}
  virtual ~UniaryExpression() {}

  static AG_scope<Expression>
  parse_prefix(Parser&);

  static AG_scope<Expression>
  parse_postfix(Parser&,
                AG_scope<Expression>);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Expression> operand;
  UN_OPERATOR          un_operator = UN_OPERATOR::NONE;
  std::string          operator_label;
  DataType             result_type;
  bool                 result_type_known = false;

  // Set for `&mut value`; `&value` leaves it false.
  bool                 is_mutable_reference = false;

  // True when the operator parameter is a reference and the operand must be
  // passed as its address.
  bool                 operand_is_operator_reference = false;
};

class PrimaryExpression : public Expression {
public:
  static AG_scope<Expression>
  parse_primary(Parser& parser);

  std::string_view
  get_value() {
    return value;
  }

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual DataType
  get_declared_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  virtual std::string
  generate_reference_ir(LabelCounters& counter,
                        Parser&         parser) override;

  std::string_view value;
  PrimitiveType    primary_type = PrimitiveType::UNEXPECTED;
  size_t           index = 0;

  DataType declared_type;
  bool     declared_type_known = false;

  // `null` / `nullptr`.  The literal is represented as `void*`: it converts
  // to any pointer or reference type but not to an integer, so it cannot be
  // confused with the integer literal `0`.
  bool is_null_pointer = false;

  // Module-level values are stored under their module-qualified name.  Local
  // values keep their source spelling.  Semantic analysis records the name
  // that actually resolved here so code generation can find the right slot.
  std::string resolved_name;
  bool        resolved_name_known = false;
};

// `math::sqrt`, `geometry::Point`, or `ag::io::println`.  The path is kept in
// source spelling until semantic analysis resolves import aliases.
// `f"..."` strings are split into literal and `{expression:format}`
// segments.  Each embedded expression is parsed as an ordinary expression so
// member calls, indexing, and operators work without a separate grammar.
struct FStringFormatSpec {
  char fill      = ' ';
  char align     = 0; // '>', '<', or '^'
  int  width     = 0;
  int  precision = -1;
  char type      = 0; // 'x', 'X', 'o', 'b', 'd', 's', 'f', ...
  bool alternate = false;
};

struct FStringPart {
  bool                 is_expression = false;
  std::size_t          literal_index = 0;
  AG_scope<Expression> expression;
  FStringFormatSpec    format;
  SourceLocation       location;
};

class FStringExpression : public Expression {
public:
  static AG_scope<Expression>
  parse(Parser&               parser,
        std::string_view      raw,
        const SourceLocation& location);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  std::vector<FStringPart> parts;

  // Literal segments are registered with Add_Literals during parsing so the
  // data items exist before IR generation.  A deque keeps element addresses
  // stable for the string_views stored by Add_Literals.
  std::deque<std::string> literals;
  std::size_t             empty_literal_index = 0;
};

// `loop { ... }` used as an expression.  `break value` stores the value in a
// hidden result slot and jumps to the loop's end label.
class LoopExpression : public Expression {
public:
  static AG_scope<Expression>
  parse(Parser& parser);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  AG_scope<Statements> body;
  std::string          label;
  DataType             result_type;
};

class QualifiedNameExpression : public Expression {
public:
  std::string path;
  std::string resolved_path;

  static AG_scope<Expression>
  parse(Parser&              parser,
        std::string          first_segment,
        const SourceLocation& location);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual DataType
  get_declared_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  virtual std::string
  generate_reference_ir(LabelCounters& counter,
                        Parser&         parser) override;

private:
  bool        m_is_enum_value = false;
  std::string m_enum_name;
  size_t      m_variant_index = 0;
  DataType    m_result_type;
  DataType    m_declared_type;
  bool        m_declared_type_known = false;
};

class MemberExpression : public Expression {
public:
  AG_scope<Expression> object;
  std::string          field_name;
  bool                 is_arrow = false;

  // Resolved during semantic analysis so IR generation does not need to walk
  // the symbol/struct tables again.
  DataType member_type;
  size_t   byte_offset   = 0;
  bool     member_found  = false;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> object);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;
};

// `Point { x: 3, y: 4 }` and the spread form `Point { x: 5, ..other }`.
// `start..end`, `start..=end`, `start..`, `..end`, and `..`.  Ranges are used
// by slicing and by `for` loops.  A range currently has an integer value type;
// slice length tracking is not implemented yet.
class RangeExpression : public Expression {
public:
  AG_scope<Expression> start;
  AG_scope<Expression> end;
  bool                 inclusive = false;
  bool                 has_start = false;
  bool                 has_end   = false;

  // Parse `..end` / `..` where the start is omitted.
  static AG_scope<Expression>
  parse_open_start(Parser& parser);

  // Parse the remainder after `start`, starting at the `..` token.
  static AG_scope<Expression>
  parse_after_start(Parser& parser, AG_scope<Expression> start);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;
};

// `[1, 2, 3]`  — array literals are represented as a heap-allocated block and
// a pointer to the element type.  Empty literals produce a `void*` null value.
class ArrayLiteralExpression : public Expression {
public:
  std::vector<AG_scope<Expression>> elements;
  DataType                          result_type;

  static AG_scope<Expression>
  parse(Parser& parser);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;
};

// `map { "key": value, ... }` literal.  Entries are stored as consecutive
// key/value pairs in a heap block; `for (key, value) in map` walks the pairs.
class MapLiteralExpression : public Expression {
public:
  struct Entry {
    AG_scope<Expression> key;
    AG_scope<Expression> value;
    SourceLocation       location;
  };

  std::vector<Entry> entries;
  DataType           key_type;
  DataType           value_type;

  static AG_scope<Expression>
  parse(Parser& parser);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;
};

class StructLiteralExpression : public Expression {
public:
  struct FieldInitializer {
    std::string          name;
    AG_scope<Expression> value;
    SourceLocation       location;
  };

  std::string                    type_name;
  std::vector<FieldInitializer>  fields;
  AG_scope<Expression>           spread;

  static AG_scope<Expression>
  parse(Parser&            parser,
        std::string        type_name,
        SourceLocation     location);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  DataType result_type;

private:
  bool        m_is_enum_literal = false;
  std::string m_enum_name;
  size_t      m_variant_index = 0;
};

class CallExpression : public Expression {
public:
  AG_scope<Expression>              callee;
  std::vector<AG_scope<Expression>> arguments;
  // Parallel to `arguments`: non-empty for `f(name: value)` calls.
  std::vector<std::string>          argument_names;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> callee);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

private:
  std::string                    m_label;
  DataType                       m_result_type;
  std::vector<FunctionParameter> m_resolved_parameters;
  // String-index arguments passed to pointer parameters use the address of
  // the indexed byte (`buffer[2]` becomes `buffer + 2`) instead of loading the
  // byte value.  This lets builtins such as memcpy/strcpy copy from an offset.
  std::vector<bool>              m_address_arguments;
  // A reference parameter may be called with either `f(&value)` or, for
  // convenience, `f(value)`.  This records the implicit `&value` case so IR
  // generation can take the address instead of loading the value.
  std::vector<bool>              m_reference_arguments;
  // `sum(...values)` expands an existing array into a variadic parameter.
  std::vector<bool>              m_spread_arguments;
  bool                           m_is_variadic        = false;
  bool                           m_is_method          = false;
  bool                           m_is_enum_constructor = false;
  std::string                    m_enum_name;
  size_t                         m_variant_index = 0;
};

class IndexExpression : public Expression {
public:
  AG_scope<Expression> object;
  AG_scope<Expression> index;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> object);

  virtual PrimitiveType
  get_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override {
    return true;
  }

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override {
    return "";
  }
};

enum class MatchPatternKind {
  WILDCARD,
  LITERAL,
  ENUM_VARIANT,
  RANGE,
  ALTERNATION,
  // Bare `Some(value)`/`Ok(value)` kind pattern.  The payload binding receives
  // the scrutinee value itself, which works for pointer-shaped optionals.
  OPAQUE_VARIANT,
};

struct MatchBinding {
  std::string    name;
  std::string    field_name;
  size_t         field_index = 0;
  DataType       type;
  SourceLocation location;
};

// One numeric alternative inside a range or `|` pattern.  A single range
// pattern stores one alternative with `is_range` set; an alternation may mix
// plain literals and ranges.
struct MatchPatternAlternative {
  bool        is_range = false;
  bool        inclusive = false;
  std::string literal;
  std::string range_start;
  std::string range_end;
};

struct MatchPattern {
  MatchPatternKind           kind = MatchPatternKind::WILDCARD;
  std::string                enum_name;
  std::string                variant_name;
  size_t                     variant_index = 0;
  std::string                literal;
  std::vector<MatchBinding>  bindings;
  std::vector<MatchPatternAlternative> alternatives;
  // `n @ pattern` binds the matched value to `n`.
  std::string                binding_name;
  SourceLocation             location;
};

struct MatchArm {
  MatchPattern         pattern;
  AG_scope<Expression> guard;
  AG_scope<Expression> body;
  SourceLocation       location;
};

// match value {
//   Status::Ok => true,
//   Status::Error(message) => false,
//   _ => false,
// }
class MatchExpression : public Expression {
public:
  AG_scope<Expression>    scrutinee;
  std::vector<MatchArm>   arms;
  DataType                result_type;

  static AG_scope<Expression>
  parse(Parser& parser);

  virtual PrimitiveType
  get_type() override;

  virtual DataType
  get_data_type() override;

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;
};

// Pattern parsing and lowering shared by `match`, `if let`, and `while let`.
bool
parse_match_pattern(Parser&               parser,
                    MatchPattern&         pattern,
                    const SourceLocation& location);

// Declare pattern bindings in the current symbol-table scope.
bool
analyze_match_pattern(Parser&               parser,
                      MatchPattern&         pattern,
                      const DataType&       scrutinee_type,
                      const SourceLocation& location);

// Emit the match test.  Returns an empty string for an unconditional pattern.
std::string
generate_match_pattern_condition(LabelCounters&       counter,
                                 Parser&              parser,
                                 const MatchPattern&  pattern,
                                 const std::string&   scrutinee_value);

// Emit assignments for pattern bindings.
void
generate_match_pattern_bindings(LabelCounters&       counter,
                                Parser&              parser,
                                const MatchPattern&  pattern,
                                const std::string&   scrutinee_value);

} // namespace ag
