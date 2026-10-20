#include "IR/Param.hpp"
#include <format>

namespace ag {
std::string
Param::to_string() {
  return std::format("param {}\n", m_name);
}
} // namespace ag
