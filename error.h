#ifndef _WGTCC_ERROR_H_
#define _WGTCC_ERROR_H_

class Token;

void Error(const Token* tok, const char* format, ...);

#endif
