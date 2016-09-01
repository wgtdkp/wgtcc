#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "scanner.h"
#include "parser.h"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>


std::string program;
std::string inFileName;
std::string outFileName;


void Usage()
{
  printf("Usage: wgtcc [options] file...\n"
       "Options: \n"
       "  --help    show this information\n"
       "  -D        define macro\n"
       "  -I        add search path\n");
  
  exit(0);
}

int main(int argc, char* argv[])
{
  bool printPreProcessed = false;
  bool printAssembly = false;

  if (argc < 2) {
    Usage();
  }
  program = std::string(argv[0]);

  for (auto i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      inFileName = std::string(argv[i]);
      continue;
    }

    switch (argv[i][1]) {
    case 'I':
      assert(0);
      //cpp.AddSearchPath(std::string(&argv[i][2]));
      break;
    case 'D':
      assert(0);
      //ParseDef(cpp, &argv[i][2]);
      break;

    case 'P':
      switch (argv[i][2]) {
      case 'P': printPreProcessed = true; break;
      case 'A': printAssembly = true; break;
      default:
        Error("unrecognized command line option '%s'", argv[i]);
      }
      break;
    case '-': // --
      switch (argv[i][2]) {
      case 'h': Usage(); break;
      default:
        Error("unrecognized command line option '%s'", argv[i]);
      }
      break;
    case '\0':
    default:
      Error("unrecognized command line option '%s'", argv[i]);
    }
  }

  if (inFileName.size() == 0) {
    Usage();
  }

  clock_t begin = clock();
  // change current directory

  // Preprocessing
  Preprocessor cpp(&inFileName);
  
  outFileName = inFileName;
  std::string dir = "./";  
  auto pos = inFileName.rfind('/');
  
  if (pos != std::string::npos) {
    dir = inFileName.substr(0, pos + 1);
    outFileName = inFileName.substr(pos + 1);
  }
  outFileName.back() = 's';
  cpp.AddSearchPath(dir);

  TokenSequence ts;
  cpp.Process(ts);

  if (printPreProcessed) {
    std::cout << std::endl << "###### Preprocessed ######" << std::endl;
    ts.Print();
  }

  // Parsing
  Parser parser(ts);
  parser.Parse();
  
  // CodeGen
  
  auto outFile = fopen(outFileName.c_str(), "w");
  assert(outFile);

  Generator::SetInOut(&parser, outFile);
  Generator g;
  g.Gen();

  clock_t end = clock(); 

  fclose(outFile);

  if (printAssembly) {
    auto str = ReadFile(outFileName);
    std::cout << *str << std::endl;
  }
  
  std::string sys = "gcc -std=c11 -Wall " + outFileName;
  system(sys.c_str());

  std::cout << "time: " << (end - begin) * 1.0f / CLOCKS_PER_SEC << std::endl;

  return 0;
}
