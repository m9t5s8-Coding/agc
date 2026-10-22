#include "IR/Label.hpp"
#include <format>

namespace ag {
std::string
Label::to_string() {
  return std::format("\n{}:\n", m_name);
}
} // namespace ag
