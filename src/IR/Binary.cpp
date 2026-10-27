#include "IR/Binary.hpp"
#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"
#include <format>

namespace ag {
std::string
Binary::to_string() {
  return std ::format("{} = {} {} {}\n", m_store_to, m_left, to_symbol(m_operator), m_right);
}

void
Binary::generate_code(CodeGenContext&) {}

} // namespace ag
