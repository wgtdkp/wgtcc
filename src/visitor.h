#ifndef _VISITOR_
#define _VISITOR_

#include "ast.h"

class Expr;
class Stmt;

class Visitor
{
public:
	void VisitExpr(Expr* expr);
};





#endif

