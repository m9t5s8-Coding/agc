
#include "IR/DataItem.hpp"
#include <format>

namespace ag {
std::string
DataItem::to_string() {
  return std::format("{}: \"{}\"\n", m_name, m_value);
}
} // namespace ag
