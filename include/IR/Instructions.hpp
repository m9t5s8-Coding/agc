#pragma once
#include <string>

namespace ag {
class Instructions {
public:
  virtual ~Instructions() = default;

  virtual std::string
  to_string() = 0;
};
} // namespace ag
