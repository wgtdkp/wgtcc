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


/********** Expr ************/

class Expr : public ASTNode
{
    friend class TranslationUnit;
	friend class Parser;
public:
    virtual ~Expr(void) {}

	Type* Ty(void) {
		return _ty;
	}

	const Type* Ty(void) const {
		assert(nullptr != _ty);
		return _ty;
	}


	virtual bool IsLVal(void) const = 0;

protected:
   explicit Expr(Type* type) : _ty(type) {}

   Type* _ty;
};


/********* Identifier *************/
class Variable : public Expr
{
	friend class TranslationUnit;
	friend class Parser;
public:
	static const int TYPE = -1;
	static const int VAR = 0;
	~Variable(void) {}

	bool IsVar(void) const {
		return _offset >= 0;
	}

	//ofcouse a variable is a lvalue expression
	virtual bool IsLVal(void) const {
		return true;
	}

	bool operator==(const Variable& other) const {
		return _offset == other._offset
			&& *_ty == *other._ty;
	}

	bool operator!=(const Variable& other) const {
		return !(*this == other);
	}

protected:
	Variable(Type* type, int offset= VAR)
		: Expr(type), _offset(offset) {}

private:

	//the relative address
	int _offset;
};


//integer or floating
class Constant : public Expr
{
	friend class TranslationUnit;
public:
	~Constant(void) {}
	
	virtual bool IsLVal(void) const {
		return false;
	}

protected:
	Constant(ArithmType* type, size_t val) 
		: Expr(type), _val(val) {}

	//the null constant pointer
	explicit Constant(PointerType* type) 
		: Expr(type), _val(0) {}

	size_t _val;
};


class AssignExpr : public ASTNode
{
public:
	~AssignExpr(void) {}
};


class BinaryOp : public Expr
{
	friend class TranslationUnit;
public:
	~BinaryOp(void) {}

	virtual bool IsLVal(void) const {
		//some operator like subscipting is lvaue
		//but, typically, arithemetic expression is not lvalue
		//TODO: switch(op) {}
		return false;
	}

protected:
	BinaryOp(Type* type, int op, Expr* lhs, Expr* rhs)
		: Expr(type), _op(op), _lhs(lhs), _rhs(rhs) {}

	int _op;
	Expr* _lhs;
	Expr* _rhs;
};

class UnaryOp : public Expr
{
	friend class TranslationUnit;
public: 
	~UnaryOp(void) {}

	//TODO: like '++i' is lvalue, but '~i' is not lvalue
	virtual bool IsLVal(void) const {
		//only deref('*') op is lvalue
		return (Token::DEREF == _op);
	}

protected:
	UnaryOp(Type* type, int op, Expr* expr) 
		: Expr(type), _expr(expr) {
		switch (op) {
		case Token::PREFIX_INC: case Token::PREFIX_DEC:
		case Token::ADDR: case Token::DEREF: case Token::PLUS: 
		case Token::MINUS: case '~': case '!':
			break;
		default: assert(false);
		}
	}

	int _op;
	Expr* _expr;
};

class FuncCall : public Expr
{
	friend class TranslationUnit;
public:
	~FuncCall(void) {}

	//a function call is ofcouse not lvalue
	virtual bool IsLVal(void) const {
		return false;
	}

protected:
	FuncCall(Type* type, Expr* caller, std::list<Expr*> args) 
		: Expr(type), _caller(caller), _args(args) {}

	Expr* _caller;
	std::list<Expr*> _args;
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

/********** no constructor for epxression *********
	static Expr* NewExpr(Type* type=nullptr) {
		return new Expr(type);
	}
*/
	static BinaryOp* NewBinaryOp(Type* type, int op, Expr* lhs, Expr* rhs) {
		return new BinaryOp(type, op, lhs, rhs);
	}

	static UnaryOp* NewUnaryOp(Type* type, int op, Expr* expr) {
		return new UnaryOp(type, op, expr);
	}

	static FuncCall* NewFuncCall(Type* type, Expr* caller, const std::list<Expr*>& args) {
		return new FuncCall(type, caller, args);
	}

	static Variable* NewVariable(Type* type, int offset=0) {
		return new Variable(type, offset);
	}

	static Constant* NewConstant(ArithmType* type, size_t val) {
		return new Constant(type, val);
	}

	static Constant* NewConstant(PointerType* type) {
		return new Constant(type);
	}

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
