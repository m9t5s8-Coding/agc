#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class Return : public Instructions {
public:
  virtual std::string
  to_string() override;
};
} // namespace ag
