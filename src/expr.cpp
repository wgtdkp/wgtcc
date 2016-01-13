#include "expr.h"
#include "error.h"

SubScriptingOp* SubScriptingOp::TypeChecking(void)
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

MemberRefOp* MemberRefOp::TypeChecking(void)
{
	StructUnionType* structUnionType;
	if (_isPtrOp) {
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
	
	_rhs = structUnionType->Find(_memberName);
	if (nullptr == _rhs)
		Error("'%s' is not a member of '%s'", _memberName, "[obj]");
	else
		_ty = _rhs->Ty();
	return this;
}

MultiplicativeOp* MultiplicativeOp::TypeChecking(void)
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

AdditiveOp* AdditiveOp::TypeChecking(void)
{
	auto lhsType = _lhs->Ty()->ToArithmType();
	auto rhsType = _rhs->Ty()->ToArithmType();
	

	//TODO: type promotion
	_ty = _lhs->Ty();
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


PostfixIncDecOp* PostfixIncDecOp::TypeChecking(void)
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

PrefixIncDecOp* PrefixIncDecOp::TypeChecking(void)
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

AddrOp* AddrOp::TypeChecking(void)
{
	FuncType* funcType = _operand->Ty()->ToFuncType();
	if (nullptr != funcType && !_operand->IsLVal())
		Error("expression must be an lvalue or function designator");
	_ty = Type::NewPointerType(_operand->Ty());
	return this;
}

CastOp* CastOp::TypeChecking(void)
{
	//the _ty has been initiated to desType
	if (!_ty->IsScalar())
		Error("the cast type should be arithemetic type or pointer");
	if (_ty->IsReal() && nullptr != _operand->Ty()->ToPointerType())
		Error("can't cast a pointer to floating");
	else if (nullptr != _ty->ToPointerType() && _operand->Ty()->IsReal())
		Error("can't cast a floating to pointer");

	return this;
}

