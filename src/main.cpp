#include "Lexer.hpp"
#include "Parser.hpp"

#include <cstdlib>
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
  std::stringstream buffer;
  {
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
  parser.set_lexer(lexer);
  parser.parse();
  if (!parser.is_syntax_valid()) {
    std::cout << "Invalid Syntax!" << std::endl;
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
