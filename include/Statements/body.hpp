#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class BodyStatement : public Statements {
public:
  BodyStatement() {}

  virtual ~BodyStatement() {}

  static AG_scope<Statements>
  ParseBlock(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

private:
  std::vector<AG_scope<Statements>> m_statements;
};
} // namespace ag
