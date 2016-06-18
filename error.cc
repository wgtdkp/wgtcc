#include "error.h"

#include "token.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>


static void Error(
    const char* fileName, 
    int line, int column, 
    const char* label,
    const char* fmt,
    va_list args)
{
    //fprintf(stderr, "[ %s ]: [ %d ]: ", __FILE__, __LINE__);
    fprintf(stderr, "%s: %d: %d: ", fileName, line, column);
    //Error(label, fmt, args);
    fprintf(stderr, "%s: ", label);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    
    if (strcmp(label, "error") == 0)
        exit(0);
}

static void Error(const char* label, const char* fmt, va_list args)
{
    //fprintf(stderr, "[ %s ]: [ %d ]: ", __FILE__, __LINE__);
    fprintf(stderr, "%s: ", label);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    
    if (strcmp(label, "error") == 0)
        exit(0);
}


void Error(const char* fileName, int line, int column, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error(fileName, line, column, "error", fmt, args);
    va_end(args);
}

void Error(const Token* tok, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error(tok->FileName(), tok->Line(), tok->Column(), "error", fmt, args);
    va_end(args);
}

void Error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error("error", fmt, args);
    va_end(args);
}

void Warning(const char* fileName, int line, int column, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error(fileName, line, column, "waring", fmt, args);
    va_end(args);
}

void Warning(const Token* tok, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error(tok->FileName(), tok->Line(), tok->Column(), "warning", fmt, args);
    va_end(args);
}

void Warning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Error("warning", fmt, args);
    va_end(args);
}
