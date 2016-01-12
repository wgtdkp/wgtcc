#ifndef _ERROR_H_
#define _ERROR_H_

class Token;
/*
class Error
{
public:

};
*/

void Error(const char* fileName, int line, int column, const char* fmt, ...);
void Error(const Token* tok, const char* fmt, ...);
void Error(const char* fmt, ...);
void Warning(const char* fileName, int line, int column, const char* fmt, ...);
void Warning(const Token* tok, const char* fmt, ...);
void Warning(const char* fmt, ...);
#endif
