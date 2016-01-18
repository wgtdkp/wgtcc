#include "ast.h"
#include "expr.h"

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

Constant* TranslationUnit::NewConstant(ArithmType* type, size_t val) {
	return new Constant(type, val);
}

Constant* TranslationUnit::NewConstant(PointerType* type) {
	return new Constant(type);
}

Constant* TranslationUnit::NewConstantInt(size_t val)
{
	auto type = Type::NewArithmType(T_INT);
	return NewConstant(type, val);
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
