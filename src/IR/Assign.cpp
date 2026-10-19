#include "IR/Assign.hpp"
#include <format>

namespace ag {
std::string
Assign::to_string() {
  return std::format("{} = {}\n", m_destination, m_source);
}
} // namespace ag
