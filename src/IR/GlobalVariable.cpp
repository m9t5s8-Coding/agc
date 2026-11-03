#include "IR/GlobalVariable.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
GlobalVariable::to_string() {
  return std::format("global {} = {}\n", m_name, m_initializer);
}

void
GlobalVariable::generate_code(CodeGenContext& context) {
  context.data << '\t' << m_name << ": dq " << m_initializer << '\n';
}
} // namespace ag
