#include "Lexer.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
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
  ag::Lexer lexer;
  {
    std::string file_data = buffer.str();
    lexer.set_source_code(file_data);
  }
  lexer.tokenize();
  // std::cout << lexer << std::endl;

  return EXIT_SUCCESS;
}
