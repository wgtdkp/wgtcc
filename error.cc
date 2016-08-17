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


void Error(const char* format, ...)
{
    fprintf(stderr,  "%s: " ANSI_COLOR_RED "error: " ANSI_COLOR_RESET,
            program.c_str());
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");

    exit(0);
}


static void VError(const Token* tok, const char* format, va_list args)
{
    assert(tok->_fileName);

    int column = tok->_begin - tok->_lineBegin + 1;

    fprintf(stderr,  "%s:%d:%d: " ANSI_COLOR_RED "error: " ANSI_COLOR_RESET,
            tok->_fileName->c_str(), tok->_line, column);
 
    vfprintf(stderr, format, args);
    
    fprintf(stderr, "\n    ");

    bool sawNoSpace = false;
    int nspaces = 0;
    for (auto p = tok->_lineBegin; *p != '\n' && *p != 0; p++) {
        if (!sawNoSpace && (*p == ' ' || *p == '\t'))
            nspaces++;
        else {
            sawNoSpace = true;
            fputc(*p, stderr);
        }
    }
    
    fprintf(stderr, "\n    ");

    for (int i = 1; i + nspaces < column; i++)
        fputc(' ', stderr);
    
    fprintf(stderr, ANSI_COLOR_GREEN "^\n");

    exit(0);
}


void Error(const Token* tok, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    VError(tok, format, args);
    va_end(args);
}


void Error(const Expr* expr, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    VError(expr->Tok(), format, args);
    va_end(args);
}
