#include "IR/Return.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Return::to_string() {
  return std::format("return {}\n", m_name);
}

void
Return::generate_code(CodeGenContext& context) {
  context.text << "\tret\n";
}

} // namespace ag
