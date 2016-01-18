#ifndef _DECL_H_
#define _DECL_H_

#include "ast.h"

/********** Declaration ***********/

class FuncDef : public ExtDecl
{
	friend class TranslationUnit;
public:
	virtual ~FuncDef(void) {}
protected:
	FuncDef(FuncType* type, CompoundStmt* stmt)
		: _type(type), _stmt(stmt) {}
private:
	FuncType* _type;
	CompoundStmt* _stmt;
};

class Decl : public ExtDecl
{

};

#endif
