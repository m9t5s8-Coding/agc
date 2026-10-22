#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class Return : public Instructions {
public:
  Return(const std::string& name) : m_name(name) {}

  virtual std::string
  to_string() override;

  std::string m_name;
};
} // namespace ag
