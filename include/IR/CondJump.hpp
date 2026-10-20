#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class CondJump : public Instructions {
public:
  CondJump(const std::string& cond,
           const std::string& false_block)
      : m_cond(cond),
        m_false_block(false_block) {}

  virtual std::string
  to_string() override;

  std::string m_cond;
  std::string m_false_block;
};
} // namespace ag
