#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class WriteStatement : public Statements {
public:
  WriteStatement()
      : m_buffer(""),
        m_buffer_size("") {}

  virtual ~WriteStatement() {}

  static AG_scope<Statements>
  ParseWrite(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

private:
  std::string_view m_buffer;
  std::string_view m_buffer_size;
  // AG_scope<Expression> m_buffer;
  // AG_scope<Expression> m_buffer_size;
};
} // namespace ag
