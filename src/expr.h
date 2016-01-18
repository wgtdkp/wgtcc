#ifndef _EXPR_H_
#define _EXPR_H_

#include <cassert>
#include "ast.h"
#include "stmt.h"

/********** Expr ************/

class Expr : public Stmt
{
	friend class TranslationUnit;
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
	/*you can construct a expression without specifying a type, 
	  then the type should be evaluated in TypeChecking() */
	explicit Expr(Type* type) : _ty(type) {}
	/*do type checking and evaluating the expression type;
	  called after construction*/
	virtual Expr* TypeChecking(void) = 0;
	
	Type* _ty;
};


/********* Identifier *************/
class Variable : public Expr
{
	friend class TranslationUnit;
public:
	static const int TYPE = -1;
	static const int VAR = 0;
	~Variable(void) {}

	bool IsVar(void) const {
		return _offset >= 0;
	}

	int Offset(void) const { return _offset; }
	int Storage(void) const { return _storage; }
	void SetOffset(int offset) { _offset = offset; }
	void SetStorage(int storage) { _storage = storage; }

	//of course a variable is a lvalue expression
	virtual bool IsLVal(void) const { return true; }

	bool operator==(const Variable& other) const {
		return _offset == other._offset
			&& *_ty == *other._ty;
	}

	bool operator!=(const Variable& other) const {
		return !(*this == other);
	}

protected:
	Variable(Type* type, int offset = VAR)
		: Expr(type), _offset(offset), _storage(0) {}
	
	//do nothing
	virtual Variable* TypeChecking(void) { return this; }

private:

	//the relative address
	int _offset;
	int _storage;
};


//integer or floating
class Constant : public Expr
{
	friend class TranslationUnit;
public:
	~Constant(void) {}

	virtual bool IsLVal(void) const { return false; }

protected:
	Constant(ArithmType* type, size_t val)
		: Expr(type), _val(val) {}

	//the null constant pointer
	explicit Constant(PointerType* type)
		: Expr(type), _val(0) {}
	
	virtual Constant* TypeChecking(void) { return this; }

	size_t _val;
};


class TempVar : public Expr
{
	friend class TempVar;
public:
	virtual ~TempVar(void) {}
	virtual bool IsLVal(void) const { return true; }
protected:
	explicit TempVar(Type* type) 
		: Expr(type), _tag(GenTag()) {}
private:
	static int GenTag(void) {
		static int tag = 0;
		return ++tag;
	}
	int _tag;
};

class AssignExpr : public ASTNode
{
public:
	~AssignExpr(void) {}
};

class ConditionalOp : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~ConditionalOp(void) {}
	virtual bool IsLVal(void) const { return false; }
protected:
	ConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
		: Expr(nullptr), _cond(cond), _exprTrue(exprTrue), _exprFalse(exprFalse) {}
	virtual ConditionalOp* TypeChecking(void);
private:
	Expr* _cond;
	Expr* _exprTrue;
	Expr* _exprFalse;
};

class BinaryOp : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~BinaryOp(void) {}

	//like member ref operator is a lvalue
	virtual bool IsLVal(void) const { return false; }

protected:
	BinaryOp(int op, Expr* lhs, Expr* rhs)
		:Expr(nullptr), _op(op), _lhs(lhs), _rhs(rhs) {}

	//TODO: 1.type checking; 2. evalute the type;
	virtual BinaryOp* TypeChecking(void);// { return this; }
	BinaryOp* SubScriptingOpTypeChecking(void);
	BinaryOp* MemberRefOpTypeChecking(const char* rhsName);
	BinaryOp* MultiOpTypeChecking(void);
	BinaryOp* AdditiveOpTypeChecking(void);
	BinaryOp* ShiftOpTypeChecking(void);
	BinaryOp* RelationalOpTypeChecking(void);
	BinaryOp* EqualityOpTypeChecking(void);
	BinaryOp* BitwiseOpTypeChecking(void);
	BinaryOp* LogicalOpTypeChecking(void);
	BinaryOp* AssignOpTypeChecking(void);

	int _op;
	Expr* _lhs;
	Expr* _rhs;
};

/************* Unary Operator ****************/

class UnaryOp : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~UnaryOp(void) {}

	//TODO: like '*p' is lvalue, but '~i' is not lvalue
	virtual bool IsLVal(void) const {
		/*only deref('*') op is lvalue;
		  so it's only deref with override this func*/
		return (Token::DEREF == _op);
	}

protected:
	UnaryOp(int op, Expr* operand, Type* type=nullptr)
		: Expr(type), _op(op), _operand(operand) {}

	virtual UnaryOp* TypeChecking(void);
	UnaryOp* IncDecOpTypeChecking(void);
	UnaryOp* AddrOpTypeChecking(void);
	UnaryOp* DerefOpTypeChecking(void);
	UnaryOp* UnaryArithmOpTypeChecking(void);
	UnaryOp* CastOpTypeChecking(void);

	int _op;
	Expr* _operand;
};

/*
class PostfixIncDecOp : public UnaryOp
{
	friend class TranslationUnit;
public:
	virtual ~PostfixIncDecOp(void) {}
	//virtual bool IsLVal(void) const { return false; }
protected:
	PostfixIncDecOp(Expr* operand, bool isInc) 
		: UnaryOp(operand, operand->Ty()), _isInc(isInc) {}
	virtual PostfixIncDecOp* TypeChecking(void);
private:
	bool _isInc;
};

class PrefixIncDecOp : public UnaryOp
{
	friend class TranslationUnit;
public:
	virtual ~PrefixIncDecOp(void) {}
	//virtual bool IsLVal(void) const { return false; }
protected:
	PrefixIncDecOp(Expr* operand, bool isInc)
		: UnaryOp(operand, operand->Ty()), _isInc(isInc) {}
	virtual PrefixIncDecOp* TypeChecking(void);
private:
	bool _isInc;
};

class AddrOp : public UnaryOp
{
	friend class TranslationUnit;
public:
	virtual ~AddrOp(void) {}
	//virtual bool IsLVal(void) const { return false; }
protected:
	//the type will be evaluated in TypeChecking()
	AddrOp(Expr* operand) : UnaryOp(operand, nullptr) {}
	virtual AddrOp* TypeChecking(void);
};

class CastOp : public UnaryOp
{
	friend class TranslationUnit;
public:
	virtual ~CastOp(void) {}
protected:
	CastOp(Expr* operand, Type* desType) 
		: UnaryOp(operand, desType) {}
	virtual CastOp* TypeChecking(void);
};
*/

class FuncCall : public Expr
{
	friend class TranslationUnit;
public:
	~FuncCall(void) {}

	//a function call is ofcourse not lvalue
	virtual bool IsLVal(void) const { return false; }

protected:
	FuncCall(Expr* designator, std::list<Expr*> args)
		:_designator(designator), _args(args), Expr(nullptr) {}

	virtual FuncCall* TypeChecking(void);

	Expr* _designator;
	std::list<Expr*> _args;
};

#endif
