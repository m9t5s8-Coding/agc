
#include "IR/DataItem.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
DataItem::to_string() {
  return std::format("{}: \"{}\"\n", m_name, m_value);
}

void
DataItem::generate_code(CodeGenContext& context) {
  context.data << std::format("{}: db \"{}\", 0\n", m_name, m_value);
}
} // namespace ag
