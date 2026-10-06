#pragma once
#include "Statements/Statements.hpp"
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

enum class PRIMARY_TYPE {
  INT_LITERAL,
  STRING_LITERAL,
  IDENTIFIER,
  VOID,
};

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

  virtual PRIMARY_TYPE
  get_type() {
    return PRIMARY_TYPE::VOID;
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

  virtual PRIMARY_TYPE
  get_type() override {
    return PRIMARY_TYPE::VOID;
  }

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

  virtual PRIMARY_TYPE
  get_type() override {
    return PRIMARY_TYPE::VOID;
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

  virtual PRIMARY_TYPE
  get_type() override {
    return primary_type;
  }

private:
  std::string_view value;
  PRIMARY_TYPE     primary_type = PRIMARY_TYPE::VOID;
};

class MemberExpression : public Expression {
public:
  AG_scope<Expression> object;
  std::string          field_name;
  bool                 is_arrow = false;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> object);

  virtual PRIMARY_TYPE
  get_type() override {
    return PRIMARY_TYPE::VOID;
  }
};

class CallExpression : public Expression {
public:
  AG_scope<Expression>              callee;
  std::vector<AG_scope<Expression>> arguments;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> callee);

  virtual PRIMARY_TYPE
  get_type() override {
    return PRIMARY_TYPE::VOID;
  }
};

class IndexExpression : public Expression {
public:
  AG_scope<Expression> object;
  AG_scope<Expression> index;

  static AG_scope<Expression>
  parse(Parser&              parser,
        AG_scope<Expression> object);

  virtual PRIMARY_TYPE
  get_type() {
    return PRIMARY_TYPE::VOID;
  }
};

} // namespace ag
