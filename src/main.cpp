#include "Lexer.hpp"
#include "Parser.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>

namespace {

struct EmitTarget {
  bool        enabled   = false;
  bool        to_stdout = false;
  std::string path;
};

void
print_usage(std::ostream& output) {
  output << "Usage: agc <file> [options]\n"
            "\n"
            "Options:\n"
            "  -o <path>        Build an LLVM executable at <path>\n"
            "  --llvm-ir [path] Write textual LLVM IR (stdout when path is omitted)\n"
            "  -ir [path]       Write textual AGC IR (stdout when path is omitted)\n"
            "  -json [path]     Write JSON result (stdout when path is omitted)\n"
            "  -h, --help       Show this help\n"
            "\n"
            "Legacy forms:\n"
            "  agc <file> -o json <path>   Write JSON result\n"
            "\n"
            "Examples:\n"
            "  agc main.ag -o test\n"
            "  agc main.ag --llvm-ir test.ll\n"
            "  agc main.ag -ir test.ir\n"
            "  agc main.ag -json test.json\n";
}

EmitTarget
parse_emit_target(int&  index,
                  int   argc,
                  char* argv[]) {
  EmitTarget target;
  target.enabled = true;

  if (index < argc && argv[index][0] != '-') {
    target.path = argv[index++];
  } else {
    target.to_stdout = true;
  }

  return target;
}

bool
write_text(const EmitTarget&  target,
           const std::string& text) {
  if (!target.enabled)
    return true;

  if (target.to_stdout) {
    std::cout << text;
    if (!text.empty() && text.back() != '\n')
      std::cout << '\n';
    return true;
  }

  std::ofstream output(target.path, std::ios::binary);
  if (!output.is_open()) {
    std::cerr << "Error: cannot open output file '" << target.path << "'\n";
    return false;
  }

  output << text;
  if (!output) {
    std::cerr << "Error: failed to write output file '" << target.path << "'\n";
    return false;
  }

  return true;
}

std::string
shell_quote(const std::string& value) {
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'')
      quoted += "'\\''";
    else
      quoted += character;
  }
  quoted += "'";
  return quoted;
}

bool
build_executable(const std::string& llvm_ir,
                 const std::string& output_path) {
  namespace fs = std::filesystem;

  std::error_code error;
  const auto      temp_directory = fs::temp_directory_path(error);
  if (error) {
    std::cerr << "Error: cannot find a temporary directory: " << error.message() << '\n';
    return false;
  }

  const auto llvm_path = temp_directory / ("agc-" + std::to_string(::getpid()) + ".ll");

  {
    std::ofstream llvm_file(llvm_path, std::ios::binary);
    if (!llvm_file.is_open()) {
      std::cerr << "Error: cannot create temporary LLVM file '" << llvm_path.string() << "'\n";
      return false;
    }
    llvm_file << llvm_ir;
    if (!llvm_file) {
      std::cerr << "Error: failed to write temporary LLVM file '" << llvm_path.string() << "'\n";
      fs::remove(llvm_path, error);
      return false;
    }
  }

  // clang accepts textual LLVM IR and links it against the C runtime, which
  // is where the LLVM backend's __ag_* helpers get calloc/write/read/exit.
  const std::string compile_command = "clang -x ir -O0 -o " + shell_quote(output_path)
                                      + " " + shell_quote(llvm_path.string());

  const bool compiled = std::system(compile_command.c_str()) == 0;

  fs::remove(llvm_path, error);

  if (!compiled) {
    std::cerr << "Error: failed to build LLVM executable '" << output_path
              << "' (is clang installed?)\n";
    return false;
  }

  return true;
}

} // namespace

int
main(int   argc,
     char* argv[]) {
  if (argc < 2) {
    print_usage(std::cerr);
    return EXIT_FAILURE;
  }

  const std::string first_argument = argv[1];
  if (first_argument == "-h" || first_argument == "--help") {
    print_usage(std::cout);
    return EXIT_SUCCESS;
  }

  if (first_argument.empty() || first_argument.front() == '-') {
    print_usage(std::cerr);
    return EXIT_FAILURE;
  }

  const std::string input_file = first_argument;

  EmitTarget  ir_target;
  EmitTarget  json_target;
  EmitTarget  llvm_ir_target;
  std::string executable_path;

  ag::OutputFormat legacy_format = ag::OutputFormat::NONE;
  std::string      legacy_output_file;

  // Keep the old `agc <file> -o json <path>` interface working.  Native
  // assembly output (`-o text`) has been removed while LLVM is the active
  // executable backend.
  const bool legacy_text_mode
      = argc == 5 && std::string(argv[2]) == "-o" && std::string(argv[3]) == "text";
  if (legacy_text_mode) {
    std::cerr << "Error: native assembly output has been removed; use --llvm-ir for LLVM IR\n";
    return EXIT_FAILURE;
  }

  const bool legacy_mode
      = argc == 5 && std::string(argv[2]) == "-o" && std::string(argv[3]) == "json";

  if (legacy_mode) {
    legacy_format      = ag::OutputFormat::JSON;
    legacy_output_file = argv[4];
  } else {
    for (int index = 2; index < argc;) {
      const std::string argument = argv[index++];

      if (argument == "-o") {
        if (index >= argc) {
          std::cerr << "Error: -o requires an output path\n";
          return EXIT_FAILURE;
        }
        executable_path = argv[index++];
      } else if (argument == "--llvm" || argument == "-llvm") {
        // Accepted for backwards compatibility; -o now always uses LLVM.
      } else if (argument == "--llvm-ir" || argument == "-llvm-ir" ||
                 argument == "--emit-llvm") {
        llvm_ir_target = parse_emit_target(index, argc, argv);
      } else if (argument == "-ir" || argument == "--ir" || argument == "--emit-ir") {
        ir_target = parse_emit_target(index, argc, argv);
      } else if (argument == "-json" || argument == "--json") {
        json_target = parse_emit_target(index, argc, argv);
      } else if (argument == "-asm" || argument == "--asm" || argument == "--emit-asm") {
        std::cerr << "Error: native assembly output has been removed; use --llvm-ir for LLVM IR\n";
        return EXIT_FAILURE;
      } else if (argument == "-h" || argument == "--help") {
        print_usage(std::cout);
        return EXIT_SUCCESS;
      } else {
        std::cerr << "Error: unknown option '" << argument << "'\n";
        print_usage(std::cerr);
        return EXIT_FAILURE;
      }
    }
  }

  if (!legacy_mode && !ir_target.enabled && !json_target.enabled
      && !llvm_ir_target.enabled && executable_path.empty()) {
    print_usage(std::cerr);
    return EXIT_FAILURE;
  }

  std::stringstream source;
  {
    std::ifstream file(input_file, std::ios::binary);
    if (!file.is_open()) {
      std::cerr << "Error: could not open source file '" << input_file << "'\n";
      return EXIT_FAILURE;
    }
    source << file.rdbuf();
  }

  ag::Parser parser;
  ag::Lexer  lexer;

  // The lexer keeps a view of the source text, so it must outlive tokenize().
  std::string source_text = source.str();

  lexer.set_filename(input_file);
  lexer.set_source_code(source_text);
  lexer.tokenize();

  parser.set_output_format(legacy_format);
  parser.set_output_file(legacy_output_file);
  parser.set_emit_llvm(!executable_path.empty() || llvm_ir_target.enabled);
  parser.set_emit_native_assembly(json_target.enabled || legacy_format == ag::OutputFormat::JSON);
  parser.set_lexer(lexer);
  parser.parse();

  if (legacy_mode)
    return EXIT_SUCCESS;

  if (!parser.is_syntax_valid()) {
    std::cerr << parser.get_error_message();
  }

  bool success = true;

  if (json_target.enabled)
    success = write_text(json_target, parser.to_json()) && success;

  if (llvm_ir_target.enabled)
    success = write_text(llvm_ir_target, parser.get_llvm_ir()) && success;

  if (ir_target.enabled)
    success = write_text(ir_target, parser.get_ir()) && success;

  if (!executable_path.empty()) {
    if (parser.is_syntax_valid()) {
      success = build_executable(parser.get_llvm_ir(), executable_path) && success;
    } else {
      success = false;
    }
  }

  return success && parser.is_syntax_valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
