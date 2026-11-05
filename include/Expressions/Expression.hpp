#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
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

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) {
    return "";
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
  bool                 is_string_index       = false;
  bool                 is_compound_assignment = false;
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

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

  std::string_view value;
  PrimitiveType    primary_type = PrimitiveType::UNEXPECTED;
  size_t           index = 0;
};

// `math::sqrt`, `geometry::Point`, or `ag::io::println`.  The path is kept in
// source spelling until semantic analysis resolves import aliases.
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

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(LabelCounters& counter,
              Parser&         parser) override;

private:
  bool        m_is_enum_value = false;
  std::string m_enum_name;
  size_t      m_variant_index = 0;
  DataType    m_result_type;
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
};

struct MatchBinding {
  std::string    name;
  std::string    field_name;
  size_t         field_index = 0;
  DataType       type;
  SourceLocation location;
};

struct MatchPattern {
  MatchPatternKind           kind = MatchPatternKind::WILDCARD;
  std::string                enum_name;
  std::string                variant_name;
  size_t                     variant_index = 0;
  std::string                literal;
  std::vector<MatchBinding>  bindings;
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

} // namespace ag
