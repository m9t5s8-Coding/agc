#include "IR/FunctionCall.hpp"
#include <format>

namespace ag {
std::string
FunctionCall::to_string() {
  return std::format("call {}\n", m_name);
}
} // namespace ag
