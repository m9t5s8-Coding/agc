#include "IR/Param.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Param::to_string() {
  return std::format("param {}\n", m_name);
}

void
Param::generate_code(CodeGenContext&) {}

} // namespace ag
