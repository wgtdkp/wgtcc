#ifndef _AST_H_
#define _AST_H_

#include <list>
#include <memory>
#include "token.h"
#include "type.h"

class ASTVisitor;
class ASTNode;

//Expression
class Expr;
class BinaryOp;
class ConditionalOp;
class UnaryOp;
class FuncCall;
class Variable;
class Constant;
class TempVar;

//statement
class Stmt;
class IfStmt;
class JumpStmt;
class LabelStmt;
class EmptyStmt;


class FuncDef;

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


/*********** Statement *************/

class Stmt : public ASTNode
{

};

class EmptyStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~EmptyStmt(void) {}
protected:
	EmptyStmt(void) {}
private:
};

//构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
	friend class TranslationUnit;
public:
	~LabelStmt(void) {}
	int Tag(void) const { return Tag(); }
protected:
	LabelStmt(void) : _tag(GenTag()) {}
private:
	static int GenTag(void) {
		static int tag = 0;
		return ++tag;
	}
	int _tag; //使用整型的tag值，而不直接用字符串
};

class IfStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~IfStmt(void) {}
protected:
	IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
		: _cond(cond), _then(then), _else(els) {}
private:
	Expr* _cond;
	Stmt* _then;
	Stmt* _else;
};

class JumpStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~JumpStmt(void) {}
	void SetLabel(LabelStmt* label) { _label = label; }
protected:
	JumpStmt(LabelStmt* label) : _label(label) {}
private:
	LabelStmt* _label;
};

class CompoundStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~CompoundStmt(void) {}

protected:
	CompoundStmt(const std::list<Stmt*>& stmts)
		: _stmts(stmts) {}

private:
	std::list<Stmt*> _stmts;
};


/********** Expr ************/

class Expr : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~Expr(void) {}

	Type* Ty(void) { return _ty; }

	const Type* Ty(void) const {
		assert(nullptr != _ty);
		return _ty;
	}

	virtual bool IsLVal(void) const = 0;
	//void SetConsant(bool isConstant) { _isConstant = isConstant; }
	//bool IsConstant(void) const { return _isConstant; }
protected:
	/*you can construct a expression without specifying a type,
	then the type should be evaluated in TypeChecking() */
	explicit Expr(Type* type, bool isConstant = false)
		: _ty(type)/*, _isConstant(isConstant)*/ {}
	/*do type checking and evaluating the expression type;
	called after construction*/
	virtual Expr* TypeChecking(void) = 0;

	Type* _ty;
	//bool _isConstant;
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

	virtual Constant* ToConstant(void) { return nullptr; }
	virtual const Constant* ToConstant(void) const { return nullptr; }

protected:
	Variable(Type* type, int offset = VAR, bool isConstant = false)
		: Expr(type, isConstant), _offset(offset), _storage(0) {}
	//do nothing
	virtual Variable* TypeChecking(void) { return this; }

protected:
	int _storage;
private:
	//the relative address
	int _offset;
};


//integer, character, floating
class Constant : public Variable
{
	friend class TranslationUnit;
public:
	~Constant(void) {}
	virtual bool IsLVal(void) const { return false; }
	unsigned long long IVal(void) const { return _ival; }
	long double FVal(void) const { return _fval; }
protected:
	Constant(ArithmType* type, unsigned long long val)
		: Variable(type, VAR, true), _ival(val) {
		assert(type->IsInteger());
	}

	Constant(ArithmType* type, long double val)
		: Variable(type, VAR, true), _fval(val) {
		assert(type->IsFloat());
	}

	virtual Constant* TypeChecking(void) { return this; }

private:
	union {
		unsigned long long _ival;
		long double _fval;
	};
};

//临时变量
class TempVar : public Expr
{
	friend class TranslationUnit;
public:
	virtual ~TempVar(void) {}
	virtual bool IsLVal(void) const { return true; }

protected:
	explicit TempVar(Type* type)
		: Expr(type), _tag(GenTag()) {}
	virtual TempVar* TypeChecking(void) { return this; }
private:
	static int GenTag(void) {
		static int tag = 0;
		return ++tag;
	}
	int _tag;
};

// cond ？ true ： false
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

/**********
'+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
'=',(复合赋值运算符被拆分为两个运算)
'==', '!=', '<=', '>=', 
'&&', '||'
'['(下标运算符), '.'(成员运算符), '->'(成员运算符)
','(逗号运算符), 
***********/
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
	UnaryOp(int op, Expr* operand, Type* type = nullptr)
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


/*************** Declaration ******************/

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
	static Constant* NewConstantInteger(ArithmType* type, unsigned long long val);
	static Constant* NewConstantFloat(ArithmType* type, long double val);
	static TempVar* NewTempVar(Type* type);
	static UnaryOp* NewUnaryOp(int op, Expr* operand, Type* type=nullptr);

	/*************** Statement *******************/
	static EmptyStmt* NewEmptyStmt(void);
	static IfStmt* NewIfStmt(Expr* cond, Stmt* then, Stmt* els=nullptr);
	static JumpStmt* NewJumpStmt(LabelStmt* label);
	static LabelStmt* NewLabelStmt(void);
	static CompoundStmt* NewCompoundStmt(std::list<Stmt*>& stmts);

	/*************** Function Definition ***************/
	static FuncDef* NewFuncDef(FuncType* type, CompoundStmt* stmt);
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

#endif
