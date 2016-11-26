#include "cpp.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"

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
std::string in_filename;
std::string out_filename;
bool debug = false;
static bool only_preprocess = false;
static bool only_compile = false;
static std::string gcc_In_filename;
static std::list<std::string> gcc_args;
static std::vector<std::string> defines;
static std::list<std::string> include_paths;


static void Usage() {
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


static void ValidateFileName(const std::string& filename) {
  auto ext = filename.substr(std::max(0UL, filename.size() - 2));
  if (ext != ".c" && ext != ".s" && ext != ".o")
    Error("bad file name format:'%s'", filename.c_str());
}


static void DefineMacro(Preprocessor& cpp, const std::string& def) {
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


static std::string GetName(const std::string& path) {
  auto pos = path.rfind('/');
  if (pos == std::string::npos)
    return path;
  return path.substr(pos + 1);
}


static int RunWgtcc() {
  if (in_filename.back() == 's')
    return 0;

  Preprocessor cpp(&in_filename);
  for (auto& def: defines)
    DefineMacro(cpp, def);
  for (auto& path: include_paths)
    cpp.AddSearchPath(path);

  FILE* fp = stdout;
  if (out_filename.size())
    fp = fopen(out_filename.c_str(), "w");
  TokenSequence ts;
  cpp.Process(ts);
  if (only_preprocess) {
    ts.Print(fp);
    return 0;
  }

  if (!only_compile || out_filename.size() == 0) {
    out_filename = GetName(in_filename);
    out_filename.back() = 's';
  }
  fp = fopen(out_filename.c_str(), "w");
  
  Parser parser(ts);
  parser.Parse();
  //Generator::SetInOut(&parser, fp);
  //Generator().Gen();
  fclose(fp);
  return 0;
}


static int RunGcc() {
  // Froce C11
  bool spec_std = false;
  for (auto& arg: gcc_args) {
    if (arg.substr(0, 4) == "-std") {
      arg = "-std=c11";
      spec_std = true;
    }
  }
  if (!spec_std)
    gcc_args.push_front("-std=c11");

  std::string system_arg = "gcc";
  for (const auto& arg: gcc_args)
    system_arg += " " + arg;
  auto ret = system(system_arg.c_str());
  return ret;
}


static void ParseInclude(int argc, char* argv[], int& i) {
  if (argv[i][2]) {
    include_paths.push_front(&argv[i][2]);
    return;
  }

  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  include_paths.push_front(argv[++i]);
  gcc_args.push_back(argv[i]);
}


static void ParseDefine(int argc, char* argv[], int& i) {
  if (argv[i][2]) {
    defines.push_back(&argv[i][2]);
    return;
  }

  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  defines.push_back(argv[++i]);
  gcc_args.push_back(argv[i]);
}


static void ParseOut(int argc, char* argv[], int& i) {
  if (i == argc - 1)
    Error("missing argument to '%s'", argv[i]);
  out_filename = argv[++i];
  gcc_args.push_back(argv[i]);
}


/* Use:
 *   wgtcc: compile
 *   gcc: assemble and link
 * Allowing multi file may not be a good idea... 
 */
int main(int argc, char* argv[]) {
  if (argc < 2)
    Usage();

  program = std::string(argv[0]);
  // Preprocessing
  //Preprocessor cpp(&in_filename);
  
  for (auto i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {
      in_filename = std::string(argv[i]);
      ValidateFileName(in_filename);
      continue;
    }

    gcc_args.push_back(argv[i]);

    switch (argv[i][1]) {
    case 'E': only_preprocess = true; break;
    case 'S': only_compile = true; break;
    case 'I': ParseInclude(argc, argv, i); break;
    case 'D': ParseDefine(argc, argv, i); break;
    case 'o': ParseOut(argc, argv, i); break;
    case 'g': gcc_args.pop_back(); debug = true; break;
    default:;
    }
  }

#ifdef DEBUG
  RunWgtcc();
#else
  bool has_error = false;
  pid_t pid = fork();
  if (pid < 0) {
    Error("fork error");
  } else if (pid == 0) {
    return RunWgtcc();
  } else {
    int stat;
    wait(&stat);
    has_error = has_error || !WIFEXITED(stat);
  }

  if (has_error)
    return 0;
#endif

  if (only_preprocess || only_compile)
    return 0;

  if (out_filename.size() == 0) {
    out_filename = GetName(in_filename);
    out_filename.back() = 's';
  }
  gcc_In_filename = out_filename;
  gcc_In_filename.back() = 's';
  gcc_args.push_back(gcc_In_filename);
  auto ret = RunGcc();
  auto cmd = "rm -f " + out_filename;
  if (system(cmd.c_str())) {}
  return ret;
}
