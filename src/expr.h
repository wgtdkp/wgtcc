#ifndef _EXPR_H_
#define _EXPR_H_

#include <cassert>
#include "ast.h"

/********** Expr ************/

class Expr : public ASTNode
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
		: Expr(type), _offset(offset) {}
	
	//do nothing
	virtual Variable* TypeChecking(void) { return this; }

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


class AssignExpr : public ASTNode
{
public:
	~AssignExpr(void) {}
};


class BinaryOp : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~BinaryOp(void) {}

	virtual bool IsLVal(void) const { return false; }

protected:
	BinaryOp(Expr* lhs, Expr* rhs, Type* type=nullptr)
		: Expr(type), _lhs(lhs), _rhs(rhs) {}

	//TODO: 1.type checking; 2. evalute the type;
	virtual BinaryOp* TypeChecking(void) { return this; }

	Expr* _lhs;
	Expr* _rhs;
};

class CommaOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~CommaOp(void) {}

	//default IsLVal() return false

protected:
	CommaOp(Expr* lhs, Expr* rhs)
		: BinaryOp(lhs, rhs) {}

	virtual CommaOp* TypeChecking(void) { 
		//the type of comma expr is the type of rhs
		_ty = _rhs->Ty();
		return this;
	}
};

class SubScriptingOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~SubScriptingOp(void) {}
	virtual bool IsLVal(void) const { return true; }
protected:
	SubScriptingOp(Expr* lhs, Expr* rhs)
		: BinaryOp(lhs, rhs) {}

	virtual SubScriptingOp* TypeChecking(void);
};

class MemberRefOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~MemberRefOp(void) {}
	virtual bool IsLVal(void) const { return true; }
protected:
	MemberRefOp(Expr* lhs, const char* memberName, bool isPtrOp)
		: BinaryOp(lhs, nullptr), _memberName(_memberName), _isPtrOp(isPtrOp) {}
	virtual MemberRefOp* TypeChecking(void);
private:
	
	const char* _memberName;
	bool _isPtrOp;
};

class MultiplicativeOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~MultiplicativeOp(void) {}
	//virtual bool IsLVal(void) const { return false; }
protected:
	MultiplicativeOp(Expr* lhs, Expr* rhs, int op)
		: BinaryOp(lhs, rhs), _op(op) {}
	virtual MultiplicativeOp* TypeChecking(void);
private:
	int _op;
};

class AdditiveOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~AdditiveOp(void) {}
	//virtual bool IsLVal(void) const { return false; }
protected:
	AdditiveOp(Expr* lhs, Expr* rhs, bool isAdd)
		: BinaryOp(lhs, rhs), _isAdd(isAdd) {}
	virtual AdditiveOp* TypeChecking(void);
private:
	bool _isAdd;
};

class ShiftOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~ShiftOp(void) {}
protected:
	ShiftOp(Expr* lhs, Expr* rhs, bool isLeft)
		: BinaryOp(lhs, rhs), _isLeft(isLeft) {}
	virtual ShiftOp* TypeChecking(void);
private:
	bool _isLeft;
};


// including euqality operators: '==', '!=' 
class RelationalOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~RelationalOp(void) {}
protected:
	RelationalOp(Expr* lhs, Expr* rhs, int op) 
		: BinaryOp(lhs, rhs), _op(op) {
		assert('<' == op || '>' == op 
			|| Token::LE_OP == op || Token::GE_OP == op
			|| Token::EQ_OP == op || Token::NE_OP == op);
	}
	virtual RelationalOp* TypeChecking(void);
private:
	int _op;
};

class BinaryBitwiseOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~BinaryBitwiseOp(void) {}
protected:
	BinaryBitwiseOp(Expr* lhs, Expr* rhs, int op)
		: BinaryOp(lhs, rhs), _op(op) {
		assert('&' == op || '|' == op || '^' == op);
	}
	virtual BinaryBitwiseOp* TypeChecking(void);
private:
	int _op;
};

class BinaryLogicalOp : public BinaryOp
{
	friend class TranslationUnit;
public:
	virtual ~BinaryLogicalOp(void) {}
protected:
	BinaryLogicalOp(Expr* lhs, Expr* rhs, bool isAnd)
		: BinaryOp(lhs, rhs), _isAnd(isAnd) {}
	virtual BinaryLogicalOp* TypeChecking(void);
private:
	bool _isAnd;
};


/************* Unary Operator ****************/

class UnaryOp : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~UnaryOp(void) {}

	//TODO: like '++i' is lvalue, but '~i' is not lvalue
	virtual bool IsLVal(void) const {
		/*only deref('*') op is lvalue;
		  so it's only deref with override this func*/
		return false;
	}

protected:
	UnaryOp(Expr* operand, Type* type)
		: _operand(operand), Expr(type) {}

	virtual UnaryOp* TypeChecking(void) { return this; }

	Expr* _operand;
};

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
