#include "scanner.h"
#include "token.h"

#include <iostream>
#include <string>
#include <list>

std::string program;

static void PrintToken(const Token& tok) {
  std::cout << tok.str_ << "\t"
            << tok.loc_.line_ << "\t"
            << tok.loc_.column_ << "\t"
            << tok.ws_ << std::endl; 
}

static void test_list() {
  std::list<int> li;
  li.push_back();  
}


int main(int argc, char* argv[]) {
  if (argc < 2)
    return -1;
  
  program = argv[0];
  std::string fileName = argv[1];
  auto text = ReadFile(fileName);
  Scanner scanner(text, &fileName, 1);
  while (true) {
    const auto& tok = scanner.Scan();
    if (tok.tag_ == Token::END)
      break;
    PrintToken(tok);
  }

  return 0;
}