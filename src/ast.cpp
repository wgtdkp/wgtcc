#include "ast.h"
#include "expr.h"

/*
BinaryOp* TranslationUnit::NewBinaryOp(Type* type, int op, Expr* lhs, Expr* rhs) {
	return new BinaryOp(type, op, lhs, rhs);
}
*/

CommaOp* TranslationUnit::NewCommaOp(Expr* lhs, Expr* rhs)
{
	return (new CommaOp(lhs, rhs))->TypeChecking();
}

SubScriptingOp* TranslationUnit::NewSubScriptingOp(Expr* lhs, Expr* rhs)
{
	return (new SubScriptingOp(lhs, rhs))->TypeChecking();
}

MemberRefOp* TranslationUnit::NewMemberRefOp(Expr* lhs, const char* memberName, bool isPtrOp)
{
	return (new MemberRefOp(lhs, memberName, isPtrOp))->TypeChecking();
}

MultiplicativeOp* TranslationUnit::NewMultiplicativeOp(Expr* lhs, Expr* rhs, int op)
{
	return (new MultiplicativeOp(lhs, rhs, op))->TypeChecking();
}

AdditiveOp* TranslationUnit::NewAdditiveOp(Expr* lhs, Expr* rhs, bool isAdd)
{
	return (new AdditiveOp(lhs, rhs, isAdd))->TypeChecking();
}

ShiftOp* TranslationUnit::NewShiftOp(Expr* lhs, Expr* rhs, bool isLeft)
{
	return (new ShiftOp(lhs, rhs, isLeft))->TypeChecking();
}

BitwiseAndOp* TranslationUnit::NewBitwiseAndOp(Expr* lhs, Expr* rhs)
{
	return (new BitwiseAndOp(lhs, rhs))->TypeChecking();
}

BitwiseOrOp* TranslationUnit::NewBitwiseOrOp(Expr* lhs, Expr* rhs)
{
	return (new BitwiseOrOp(lhs, rhs))->TypeChecking();
}

BitwiseXorOp* TranslationUnit::NewBitwiseXorOp(Expr* lhs, Expr* rhs)
{
	return (new BitwiseXorOp(lhs, rhs))->TypeChecking();
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

PostfixIncDecOp* TranslationUnit::NewPostfixIncDecOp(Expr* operand, bool isInc)
{
	return (new PostfixIncDecOp(operand, isInc))->TypeChecking();
}

PrefixIncDecOp* TranslationUnit::NewPrefixIncDecOp(Expr* operand, bool isInc)
{
	return (new PrefixIncDecOp(operand, isInc))->TypeChecking();
}

AddrOp* TranslationUnit::NewAddrOp(Expr* operand)
{
	return (new AddrOp(operand))->TypeChecking();
}

CastOp* TranslationUnit::NewCastOp(Expr* operand, Type* desType)
{
	return (new CastOp(operand, desType))->TypeChecking();
}
