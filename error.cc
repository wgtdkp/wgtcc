#include "error.h"

#include "ast.h"
#include "token.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


extern std::string program;


void Error(const char* format, ...) {
  fprintf(stderr,
          "%s: " ANSI_COLOR_RED "error: " ANSI_COLOR_RESET,
          program.c_str());
  
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  
  fprintf(stderr, "\n");

  exit(-1);
}


static void VError(const SourceLocation& loc,
                   const char* format,
                   va_list args) {
  assert(loc.filename);
  fprintf(stderr,
          "%s:%d:%d: " ANSI_COLOR_RED "error: " ANSI_COLOR_RESET,
          loc.filename->c_str(),
          loc.line,
          loc.column);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n    ");

  bool saw_no_space = false;
  int nspaces = 0;
  for (auto p = loc.line_begin; *p != '\n' && *p != 0; ++p) {
    if (!saw_no_space && (*p == ' ' || *p == '\t')) {
      ++nspaces;
    } else {
      saw_no_space = true;
      fputc(*p, stderr);
    }
  }
  
  fprintf(stderr, "\n    ");
  for (unsigned i = 1; i + nspaces < loc.column; ++i)
    fputc(' ', stderr);
  fprintf(stderr, ANSI_COLOR_GREEN "^\n");
  exit(-1);	
}


void Error(const SourceLocation& loc, const char* format, ...) {
  va_list args;
  va_start(args, format);
  VError(loc, format, args);
  va_end(args);
}


void Error(const Token* tok, const char* format, ...) {
  va_list args;
  va_start(args, format);
  VError(tok->loc(), format, args);
  va_end(args);
}


void Error(const Expr* expr, const char* format, ...) {
  va_list args;
  va_start(args, format);
  VError(expr->tok()->loc(), format, args);
  va_end(args);
}
