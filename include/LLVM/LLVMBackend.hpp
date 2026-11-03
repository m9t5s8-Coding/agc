#pragma once

#include <string>

namespace ag {

class Parser;

// Translate the linear AGC IR into textual LLVM IR.  The result can be
// compiled with clang/llc without changing the native NASM backend.
std::string
generate_llvm_ir(Parser& parser);

} // namespace ag
