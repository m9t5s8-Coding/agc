#pragma once

#include <string>

namespace ag {

class Parser;

// Translate the linear AGC IR into textual LLVM IR.  The result is compiled
// with clang by the driver and is the default executable backend.
std::string
generate_llvm_ir(Parser& parser);

} // namespace ag
