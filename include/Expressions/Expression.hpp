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

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) {
    return "";
  }

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) {
    return true;
  }
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

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) override;

  BIN_OPERATOR         bin_operator = BIN_OPERATOR::NONE;
  AG_scope<Expression> left;
  AG_scope<Expression> right;
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

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) override {
    return "";
  }

  AG_scope<Expression> operand;
  UN_OPERATOR          un_operator;
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

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override;

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) override {
    return std::string(value);
  }

  std::string_view value;
  PrimitiveType    primary_type;
};

class MemberExpression : public Expression {
public:
  AG_scope<Expression> object;
  std::string          field_name;
  bool                 is_arrow = false;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> object);

  virtual bool
  analyze_semantics(Parser&,
                    const SourceLocation&) override {
    return true;
  }

  virtual PrimitiveType
  get_type() override;

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) override {
    return "";
  }
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
                    const SourceLocation&) override {
    return true;
  }

  virtual PrimitiveType
  get_type() override;

  virtual std::string
  generate_ir(CodeGenContext& context,
              Parser&         parser) override {
    return "";
  }
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
  generate_ir(CodeGenContext& context,
              Parser&         parser) override {
    return "";
  }
};

} // namespace ag
