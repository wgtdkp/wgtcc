#ifndef _AST_H_
#define _AST_H_

#include <list>
#include <memory>
#include "token.h"
#include "symbol.h"

class ASTVisitor;
class ASTNode;
typedef ASTNode ExtDecl;
class Expr;
class BinaryOp;
class CommaOp;
class SubScriptingOp;
class MemberRefOp;
class MultiplicativeOp;
class AdditiveOp;

class UnaryOp;
class PostfixIncDecOp;
class PrefixIncDecOp;
class AddrOp;
class CastOp;

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


/********** Declaration ***********/

class FuncDef : public ExtDecl
{

};

class Decl : public ExtDecl
{

};

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
	//static BinaryOp* NewBinaryOp(Type* type, int op, Expr* lhs, Expr* rhs);
	static CommaOp* NewCommaOp(Expr* lhs, Expr* rhs);
	static SubScriptingOp* NewSubScriptingOp(Expr* lhs, Expr* rhs);
	static MemberRefOp* NewMemberRefOp(Expr* lhs, const char* memberName, bool isPtrOp);
	static MultiplicativeOp* NewMultiplicativeOp(Expr* lhs, Expr* rhs, int op);
	static AdditiveOp* NewAdditiveOp(Expr* lhs, Expr* rhs, bool isAdd);

	static FuncCall* NewFuncCall(Expr* designator, const std::list<Expr*>& args);
	static Variable* NewVariable(Type* type, int offset = 0);
	static Constant* NewConstant(ArithmType* type, size_t val);
	static Constant* NewConstant(PointerType* type);

	static PostfixIncDecOp* NewPostfixIncDecOp(Expr* operand, bool isInc);
	static PrefixIncDecOp* NewPrefixIncDecOp(Expr* operand, bool isInc);
	static AddrOp* NewAddrOp(Expr* operand);
	static CastOp* NewCastOp(Expr* operand, Type* desType);
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
