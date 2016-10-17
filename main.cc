#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "scanner.h"
#include "parser.h"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

std::string program;
std::string inFileName;
std::string outFileName;
bool debug = false;
static bool onlyPreprocess = false;
static bool onlyCompile = false;
static std::list<std::string> gccArgs;
static std::vector<std::string> defines;
static std::vector<std::string> includePaths;
static std::vector<std::string> gccFileNames;
static std::vector<std::string> wgtccFileNames;


static void Usage()
{
  printf("Usage: wgtcc [options] file...\n"
       "Options: \n"
       "  --help    Display this information\n"
       "  -D        Define object like macro\n"
       "  -I        Add search path\n"
       "  -E        Preprocess only; do not compile, assemble or link\n"
       "  -S        Compile only; do not assemble or link\n"
       "  -o        specify output file\n");
  
  exit(-2);
}


static void ValidateFileName(const std::string& fileName) {
  auto pos = fileName.rfind('.');
  if (pos == std::string::npos || "c" != fileName.substr(pos + 1))
    Error("bad file name format:'%s'", fileName.c_str());
}


static void DefineMacro(Preprocessor& cpp, const std::string& def)
{
  auto pos = def.find('=');
  std::string macro;
  std::string* replace;
  if (pos == std::string::npos) {
    macro = def;
    replace = new std::string();
  } else {
    macro = def.substr(0, pos);
    replace = new std::string(def.substr(pos + 1));
  }
  cpp.AddMacro(macro, replace); 
}


static std::string GetDir(const std::string& path)
{
  auto pos = path.rfind('/');
  if (pos == std::string::npos)
    return "./";
  return path.substr(0, pos + 1);
}


static std::string GetName(const std::string& path)
{
  auto pos = path.rfind('/');
  if (pos == std::string::npos)
    return path;
  return path.substr(pos + 1);
}


static int RunWgtcc()
{
  Preprocessor cpp(&inFileName);
  for (auto& def: defines)
    DefineMacro(cpp, def);
  for (auto& path: includePaths)
    cpp.AddSearchPath(path);
  cpp.AddSearchPath(GetDir(inFileName));
  
  FILE* fp = nullptr;
  if (outFileName.size())
    fp = fopen(outFileName.c_str(), "w");
  TokenSequence ts;
  cpp.Process(ts);
  if (onlyPreprocess) {
    ts.Print(fp);
    return 0;
  }

  if (!onlyCompile || outFileName.size() == 0) {
    outFileName = GetName(inFileName);
    outFileName.back() = 's';
  }
  fp = fopen(outFileName.c_str(), "w");
  
  Parser parser(ts);
  parser.Parse();
  Generator::SetInOut(&parser, fp);
  Generator().Gen();
  fclose(fp);
  return 0;
}


static int RunGcc()
{
  // Froce C11
  bool specStd = false;
  for (auto& arg: gccArgs) {
    if (arg.substr(0, 4) == "-std") {
      arg = "-std=c11";
      specStd = true;
    }
  }
  if (!specStd)
    gccArgs.push_front("-std=c11");
  for (const auto& fileName: gccFileNames)
    gccArgs.push_back(fileName);

  std::string systemArg = "gcc";
  for (const auto& arg: gccArgs)
    systemArg += " " + arg;
  std::cout << systemArg << std::endl;
  auto ret = system(systemArg.c_str());
  //for (const auto& fileName: gccFileNames) {
  //  auto rm = "rm -f " + fileName;
  //  if(system(rm.c_str())) {}
  //}
  return ret;
}


static void ParseInclude(int argc, char* argv[], int& i)
{
  if (argv[i][2]) {
    includePaths.push_back(&argv[i][2]);
    return;
  }

  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  includePaths.push_back(argv[++i]);
  gccArgs.push_back(argv[i]);
}

static void ParseDefine(int argc, char* argv[], int& i)
{
  if (argv[i][2]) {
    defines.push_back(&argv[i][2]);
    return;
  }

  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  defines.push_back(argv[++i]);
  gccArgs.push_back(argv[i]);
}

static void ParseOut(int argc, char* argv[], int& i)
{
  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  outFileName = argv[++i];
  gccArgs.push_back(argv[i]);
}

/* Use:
 *   wgtcc: compile
 *   gcc: assemble and link
 * Allowing multi file may not be a good idea... 
 */
int main(int argc, char* argv[])
{
  if (argc < 2)
    Usage();

  program = std::string(argv[0]);
  // Preprocessing
  //Preprocessor cpp(&inFileName);
  
  for (auto i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      auto fileName = std::string(argv[i]);
      ValidateFileName(fileName);
      wgtccFileNames.push_back(fileName);
      fileName = GetName(fileName);
      fileName.back() = 's';
      gccFileNames.push_back(fileName);
      continue;
    }

    gccArgs.push_back(argv[i]);

    switch (argv[i][1]) {
    case 'E': onlyPreprocess = true; break;
    case 'S': onlyCompile = true; break;
    case 'I': ParseInclude(argc, argv, i); break;
    case 'D': ParseDefine(argc, argv, i); break;
    case 'o': ParseOut(argc, argv, i); break;
    case 'g': gccArgs.pop_back(); debug = true; break;
    default:;
    }
  }

  if ((onlyPreprocess || onlyCompile) && wgtccFileNames.size() > 1) {
    Error("cannot specify option '-E', '-S' or '-o' with multi file");
  }

#ifdef DEBUG
  inFileName = wgtccFileNames[0];
  RunWgtcc();
  return RunGcc();
#else
  bool hasError = false;
  for (auto& fileName: wgtccFileNames) {
    pid_t pid = fork();
    if (pid < 0) {
      Error("fork error");
    } else if (pid == 0) {
      inFileName = fileName;
      return RunWgtcc();
    } else {
      int stat;
      wait(&stat);
      hasError = hasError || !WIFEXITED(stat);
    }
  }

  if (hasError || onlyPreprocess || onlyCompile)
    return 0;
  return RunGcc();
#endif
}
