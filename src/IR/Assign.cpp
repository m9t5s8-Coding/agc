#include "IR/Assign.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Assign::to_string() {
  return std::format("{} = {}\n", m_destination, m_source);
}

void
Assign::generate_code(CodeGenContext&) {}

} // namespace ag
