#include "ast.h"
#include "error.h"

/********************* Expression ***********************/

ConditionalOp* ConditionalOp::TypeChecking(void)
{

	//TODO: type checking

	//TODO: type evaluation

	return this;
}


BinaryOp* BinaryOp::TypeChecking(void)
{
	switch (_op) {
	case '[': return SubScriptingOpTypeChecking();
	case '*': case '/': case '%': return MultiOpTypeChecking();
	case '+': case '-':return AdditiveOpTypeChecking();
	case Token::LEFT_OP: case Token::RIGHT_OP: return ShiftOpTypeChecking();
	case '<': case '>': case Token::LE_OP: case Token::GE_OP: return RelationalOpTypeChecking();
	case Token::EQ_OP: case Token::NE_OP: return EqualityOpTypeChecking();
	case '&': case '^': case '|': return BitwiseOpTypeChecking();
	case Token::AND_OP: case Token::OR_OP: return LogicalOpTypeChecking();
	case '=': return AssignOpTypeChecking();
	default: assert(0); return nullptr; //make compiler happy
	}
}

BinaryOp* BinaryOp::SubScriptingOpTypeChecking(void)
{
	auto lhsType = _lhs->Ty()->ToPointerType();
	if (nullptr == lhsType)
		Error("an pointer expected");
	if (!_rhs->Ty()->IsInteger())
		Error("the operand of [] should be intger");

	//the type of [] operator is the type the pointer pointed to
	_ty = lhsType->Derived();
	return this;
}

BinaryOp* BinaryOp::MemberRefOpTypeChecking(const char* rhsName)
{
	StructUnionType* structUnionType;
	if (Token::PTR_OP == _op) {
		auto pointer = _lhs->Ty()->ToPointerType();
		if (nullptr == pointer)
			Error("pointer expected for operator '->'");
		else {
			structUnionType = pointer->Derived()->ToStructUnionType();
			if (nullptr == structUnionType)
				Error("pointer to struct/union expected");
		}
	} else {
		structUnionType = _lhs->Ty()->ToStructUnionType();
		if (nullptr == structUnionType)
			Error("an struct/union expected");
	}

	if (nullptr == structUnionType)
		return this; //the _rhs is lefted nullptr

	_rhs = structUnionType->Find(rhsName);
	if (nullptr == _rhs)
		Error("'%s' is not a member of '%s'", rhsName, "[obj]");
	else
		_ty = _rhs->Ty();
	return this;
}

BinaryOp* BinaryOp::MultiOpTypeChecking(void)
{
	auto lhsType = _lhs->Ty()->ToArithmType();
	auto rhsType = _rhs->Ty()->ToArithmType();
	if (nullptr == lhsType || nullptr == rhsType)
		Error("operand should be arithmetic type");
	if ('%' == _op && !(_lhs->Ty()->IsInteger() && _rhs->Ty()->IsInteger()))
		Error("operand of '%%' should be integer");

	//TODO: type promotion
	_ty = _lhs->Ty();
	return this;
}

BinaryOp* BinaryOp::AdditiveOpTypeChecking(void)
{
	auto lhsType = _lhs->Ty()->ToArithmType();
	auto rhsType = _rhs->Ty()->ToArithmType();


	//TODO: type promotion

	_ty = _lhs->Ty();
	return this;
}

BinaryOp* BinaryOp::ShiftOpTypeChecking(void)
{
	//TODO: type checking

	_ty = _lhs->Ty();
	return this;
}

BinaryOp* BinaryOp::RelationalOpTypeChecking(void)
{
	//TODO: type checking

	_ty = Type::NewArithmType(T_BOOL);
	return this;
}

BinaryOp* BinaryOp::EqualityOpTypeChecking(void)
{
	//TODO: type checking

	_ty = Type::NewArithmType(T_BOOL);
	return this;
}

BinaryOp* BinaryOp::BitwiseOpTypeChecking(void)
{
	if (_lhs->Ty()->IsInteger() || _rhs->Ty()->IsInteger())
		Error("operands of '&' should be integer");
	//TODO: type promotion
	_ty = Type::NewArithmType(T_INT);
	return this;
}

BinaryOp* BinaryOp::LogicalOpTypeChecking(void)
{
	//TODO: type checking
	if (!_lhs->Ty()->IsScalar() || !_rhs->Ty()->IsScalar())
		Error("the operand should be arithmetic type or pointer");
	_ty = Type::NewArithmType(T_BOOL);
	return this;
}

BinaryOp* BinaryOp::AssignOpTypeChecking(void)
{
	//TODO: type checking
	if (!_lhs->IsLVal()) {
		//TODO: error
		Error("lvalue expression expected");
	} else if (_lhs->Ty()->IsConst()) {
		Error("can't modifiy 'const' qualified expression");
	}

	_ty = _lhs->Ty();
	return this;
}


/************** Unary Operators *****************/

UnaryOp* UnaryOp::TypeChecking(void)
{
	switch (_op) {
	case Token::POSTFIX_INC: case Token::POSTFIX_DEC:
	case Token::PREFIX_INC: case Token::PREFIX_DEC: return IncDecOpTypeChecking();
	case Token::ADDR: return AddrOpTypeChecking();
	case Token::DEREF: return DerefOpTypeChecking();
	case Token::PLUS: case Token::MINUS:
	case '~': case '!': return UnaryArithmOpTypeChecking();
	case Token::CAST: return CastOpTypeChecking();
	default: assert(false); return nullptr;
	}
}

UnaryOp* UnaryOp::IncDecOpTypeChecking(void)
{
	if (!_operand->IsLVal()) {
		//TODO: error
		Error("lvalue expression expected");
	} else if (_operand->Ty()->IsConst()) {
		Error("can't modifiy 'const' qualified expression");
	}

	_ty = _operand->Ty();
	return this;
}

UnaryOp* UnaryOp::AddrOpTypeChecking(void)
{
	FuncType* funcType = _operand->Ty()->ToFuncType();
	if (nullptr != funcType && !_operand->IsLVal())
		Error("expression must be an lvalue or function designator");
	_ty = Type::NewPointerType(_operand->Ty());
	return this;
}

UnaryOp* UnaryOp::DerefOpTypeChecking(void)
{
	auto pointer = _operand->Ty()->ToPointerType();
	if (nullptr == pointer)
		Error("pointer expected for deref operator '*'");

	_ty = pointer->Derived();
	return this;
}

UnaryOp* UnaryOp::UnaryArithmOpTypeChecking(void)
{
	if (Token::PLUS == _op || Token::MINUS == _op) {
		if (!_operand->Ty()->IsArithm())
			Error("Arithmetic type expected");
	} else if ('~' == _op) {
		if (!_operand->Ty()->IsInteger())
			Error("integer expected for operator '~'");
	} else {//'!'
		if (!_operand->Ty()->IsScalar())
			Error("arithmetic type or pointer expected for operator '!'");
	}

	_ty = _operand->Ty();
	return this;
}

UnaryOp* UnaryOp::CastOpTypeChecking(void)
{
	//the _ty has been initiated to desType
	if (!_ty->IsScalar())
		Error("the cast type should be arithemetic type or pointer");
	if (_ty->IsFloat() && nullptr != _operand->Ty()->ToPointerType())
		Error("can't cast a pointer to floating");
	else if (nullptr != _ty->ToPointerType() && _operand->Ty()->IsFloat())
		Error("can't cast a floating to pointer");

	return this;
}

FuncCall* FuncCall::TypeChecking(void)
{
	auto funcType = _designator->Ty()->ToFuncType();
	if (nullptr == funcType)
		Error("not a function type");
	else
		_ty = funcType->Derived();

	//TODO: check if args and params are compatible type

	return this;
}


ConditionalOp* TranslationUnit::NewConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
{
	return (new ConditionalOp(cond, exprTrue, exprFalse))->TypeChecking();
}

BinaryOp* TranslationUnit::NewBinaryOp(int op, Expr* lhs, Expr* rhs)
{
	switch (op) {
	case '[': case '*': case '/': case '%': case '+': case '-': 
	case Token::LEFT_OP: case Token::RIGHT_OP: case '<': case '>': 
	case Token::LE_OP: case Token::GE_OP: case Token::EQ_OP: case Token::NE_OP: 
	case '&': case '^': case '|': case Token::AND_OP: case Token::OR_OP: break;
	default: assert(0);
	}
	return (new BinaryOp(op, lhs, rhs))->TypeChecking();
}

BinaryOp* TranslationUnit::NewMemberRefOp(int op, Expr* lhs, const char* rhsName)
{
	assert('.' == op || Token::PTR_OP == op);
	//the initiation of rhs is lefted in type checking
	return (NewBinaryOp(op, lhs, nullptr))->MemberRefOpTypeChecking(rhsName);
}

/*
UnaryOp* TranslationUnit::NewUnaryOp(Type* type, int op, Expr* expr) {
	return new UnaryOp(type, op, expr);
}
*/


FuncCall* TranslationUnit::NewFuncCall(Expr* designator, const std::list<Expr*>& args) {
	return (new FuncCall(designator, args))->TypeChecking();
}

Variable* TranslationUnit::NewVariable(Type* type, int offset) {
	return new Variable(type, offset);
}

Constant* TranslationUnit::NewConstantInteger(ArithmType* type, unsigned long long val)
{
	return new Constant(type, val);
}

Constant* TranslationUnit::NewConstantFloat(ArithmType* type, long double val)
{
	return new Constant(type, val);
}


TempVar* TranslationUnit::NewTempVar(Type* type)
{
	return new TempVar(type);
}

UnaryOp* TranslationUnit::NewUnaryOp(int op, Expr* operand, Type* type)
{
	return (new UnaryOp(op, operand, type))->TypeChecking();
}


/********** Statement ***********/

//显然空stmt只需要一个
EmptyStmt* TranslationUnit::NewEmptyStmt(void)
{
	static auto inst = new EmptyStmt();
	return inst;
}

//else stmt 默认是 null
IfStmt* TranslationUnit::NewIfStmt(Expr* cond, Stmt* then, Stmt* els)
{
	return new IfStmt(cond, then, els);
}

CompoundStmt* TranslationUnit::NewCompoundStmt(std::list<Stmt*>& stmts)
{
	return (new CompoundStmt(stmts));
}

JumpStmt* TranslationUnit::NewJumpStmt(LabelStmt* label)
{
	return new JumpStmt(label);
}

LabelStmt* TranslationUnit::NewLabelStmt(void)
{
	return new LabelStmt();
}

FuncDef* TranslationUnit::NewFuncDef(FuncType* type, CompoundStmt* stmt)
{
	return new FuncDef(type, stmt);
}