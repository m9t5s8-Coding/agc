#pragma once
#include "Expressions/Expression.hpp"
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class Binary : public Instructions {
public:
  Binary(const std::string& store_to,
         const std::string& left,
         const std::string& right,
         BIN_OPERATOR       bn_op)
      : m_store_to(store_to),
        m_left(left),
        m_right(right),
        m_operator(bn_op) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string  m_store_to;
  std::string  m_left, m_right;
  BIN_OPERATOR m_operator = BIN_OPERATOR::NONE;
};
} // namespace ag
