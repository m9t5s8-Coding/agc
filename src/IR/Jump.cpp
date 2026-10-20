#include "IR/Jump.hpp"
#include <format>

namespace ag {
std::string
Jump::to_string() {
  return std::format("jump {}\n", m_name);
}
} // namespace ag
