
#include "IR/DataItem.hpp"
#include "Statements/Statements.hpp"
#include <format>
#include <string>

namespace ag {
namespace {

std::string
escape_for_ir(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());

  for (const unsigned char character : value) {
    switch (character) {
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    case '\\':
      escaped += "\\\\";
      break;
    case '"':
      escaped += "\\\"";
      break;
    case '\0':
      escaped += "\\0";
      break;
    default:
      if (character >= 32 && character <= 126)
        escaped.push_back(static_cast<char>(character));
      else
        escaped += std::format("\\x{:02x}", character);
      break;
    }
  }

  return escaped;
}

void
emit_data_bytes(std::ostream&      output,
                const std::string& value) {
  bool        first = true;
  std::string run;

  auto flush_run = [&] {
    if (run.empty())
      return;

    if (!first)
      output << ", ";
    output << '"' << run << '"';
    first = false;
    run.clear();
  };

  for (const unsigned char character : value) {
    // Keep printable text readable, but emit control characters, quotes and
    // backslashes as explicit bytes so NASM never sees a raw newline.
    if (character >= 32 && character <= 126 && character != '"' && character != '\\') {
      run.push_back(static_cast<char>(character));
      continue;
    }

    flush_run();
    if (!first)
      output << ", ";
    output << static_cast<unsigned int>(character);
    first = false;
  }

  flush_run();
  if (!first)
    output << ", ";
  output << '0';
}

} // namespace

std::string
DataItem::to_string() {
  return std::format("{}: \"{}\"\n", m_name, escape_for_ir(m_value));
}

void
DataItem::generate_code(CodeGenContext& context) {
  context.data << '\t' << m_name << ": db ";
  emit_data_bytes(context.data, m_value);
  context.data << '\n';
}
} // namespace ag
