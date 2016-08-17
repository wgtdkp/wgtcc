#ifndef _WGTCC_ERROR_H_
#define _WGTCC_ERROR_H_

class ASTNode;
class Token;
class Expr;

void Error(const char* format, ...);
//void Error(const Token* tok, const char* format, ...);
void Error(const ASTNode* node, const char* format, ...);

#endif
