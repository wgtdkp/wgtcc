#ifndef _AST_H_
#define _AST_H_

#include <list>
#include <memory>
#include "token.h"
#include "symbol.h"

class ASTVisitor;
class ASTNode;
class Expr;
class BinaryOp;
class ConditionalOp;
class UnaryOp;
class FuncCall;
class Variable;
class Constant;
class AssignExpr;

/************ AST Node *************/

class ASTNode
{
public:
	virtual ~ASTNode(void) {}

protected:
	ASTNode(void) {}

private:
    //ASTNode(void);  //禁止直接创建Node

    //Coordinate _coord;
};

typedef ASTNode ExtDecl;

class TranslationUnit : public ASTNode
{
public:
	virtual ~TranslationUnit(void) {
		auto iter = _extDecls.begin();
		for (; iter != _extDecls.end(); iter++)
			delete *iter;
	}

	void Add(ExtDecl* extDecl) {
		_extDecls.push_back(extDecl);
	}

	static TranslationUnit* NewTranslationUnit(void) {
		return new TranslationUnit();
	}
	/************** Binary Operator ****************/
	static BinaryOp* NewBinaryOp(int op, Expr* lhs, Expr* rhs);
	static BinaryOp* NewMemberRefOp(int op, Expr* lhs, const char* rhsName);


	static ConditionalOp* NewConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse);
	static FuncCall* NewFuncCall(Expr* designator, const std::list<Expr*>& args);
	static Variable* NewVariable(Type* type, int offset = 0);
	static Constant* NewConstant(ArithmType* type, size_t val);
	static Constant* NewConstant(PointerType* type);

	static UnaryOp* NewUnaryOp(int op, Expr* operand, Type* type=nullptr);
private:
	TranslationUnit(void) {}

	std::list<ExtDecl*> _extDecls;
};

/*
class ASTVisitor
{
public:
	virtual void Visit(ASTNode* node) = 0;
};
*/

#endif _AST_H_
