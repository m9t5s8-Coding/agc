#pragma once

#include "Statements/Statements.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ag {
class Lexer;
class Parser;

// mod geometry { ... }
// mod geometry;
class ModuleStatement : public Statements {
public:
  virtual ~ModuleStatement();

  static AG_scope<Statements>
  ParseModule(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  std::string                       m_path;
  std::vector<AG_scope<Statements>> m_statements;

  // File modules own their source and lexer so token views and diagnostics
  // remain valid after parsing returns to the including file.
  std::string            m_source;
  std::unique_ptr<Lexer> m_lexer;
};
} // namespace ag
