#ifndef _WGTCC_ERROR_H_
#define _WGTCC_ERROR_H_


class ASTNode;

void Error(const ASTNode* node, const char* format, ...);

#endif
