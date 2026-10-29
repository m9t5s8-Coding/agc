#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class CondJump : public Instructions {
public:
  CondJump(const std::string& cond,
           const std::string& false_block,
           int                index = -1)
      : Instructions(index),
        m_cond(cond),
        m_false_block(false_block) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_cond;
  std::string m_false_block;
};
} // namespace ag
