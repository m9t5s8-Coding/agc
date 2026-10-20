#include "Lexer.hpp"
#include "Parser.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int
main(int   argc,
     char* argv[]) {
  if (argc < 2) {
    std::cout << "Usuage: agc <file> ...\n";
    return EXIT_FAILURE;
  }
  std::string      output_file;
  ag::OutputFormat format = ag::OutputFormat::NONE;
  if (argc == 5) {
    if (!strcmp(argv[2], "-o")) {
      if (!strcmp(argv[3], "json")) {
        format = ag::OutputFormat::JSON;
      } else if (!strcmp(argv[3], "text")) {
        format = ag::OutputFormat::TEXT;
      }
      output_file = argv[4];
    }
  }

  std::stringstream buffer;
  {
    std::filesystem::path path(argv[1]);
    if (path.extension() != ".ag") {
      std::cerr << "Error: Invalid file extension. Expected a '.ag' file.\n";
      return EXIT_FAILURE;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
      std::cerr << "File not opened!\n";
      return EXIT_FAILURE;
    }
    buffer << file.rdbuf();
  }
  ag::Parser  parser;
  ag::Lexer   lexer;
  std::string file_data = buffer.str();
  lexer.set_filename(argv[1]);
  lexer.set_source_code(file_data);
  lexer.tokenize();
  parser.set_output_format(format);
  parser.set_output_file(output_file);
  parser.set_lexer(lexer);
  parser.parse();
  if (!parser.is_syntax_valid()) {
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
