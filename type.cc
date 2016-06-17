#include "type.h"

#include "ast.h"
#include "env.h"

#include <cassert>


/***************** Type *********************/

bool Type::IsFloat(void) const {
	auto arithmType = ToArithmType();
	return arithmType && arithmType->IsFloat();
}

bool Type::IsInteger(void) const {
	auto arithmType = ToArithmType();
	return arithmType && arithmType->IsInteger();
}

VoidType* Type::NewVoidType(void)
{
	return new VoidType();
}

ArithmType* Type::NewArithmType(int typeSpec) {
	static ArithmType* charType 		= new ArithmType(T_CHAR);
	static ArithmType* ucharType 		= new ArithmType(T_UNSIGNED | T_CHAR);
	static ArithmType* shortType 		= new ArithmType(T_SHORT);
	static ArithmType* ushortType 		= new ArithmType(T_UNSIGNED | T_SHORT);
	static ArithmType* intType 			= new ArithmType(T_INT);
	static ArithmType* uintType 		= new ArithmType(T_UNSIGNED | T_INT);
	static ArithmType* longType 		= new ArithmType(T_LONG);
	static ArithmType* ulongType 		= new ArithmType(T_UNSIGNED | T_LONG);
	static ArithmType* longlongType 	= new ArithmType(T_LONG_LONG);
	static ArithmType* ulonglongType 	= new ArithmType(T_UNSIGNED | T_LONG_LONG);
	static ArithmType* floatType 		= new ArithmType(T_FLOAT);
	static ArithmType* doubleType 		= new ArithmType(T_DOUBLE);
	static ArithmType* longdoubleType 	= new ArithmType(T_LONG | T_DOUBLE);
	static ArithmType* boolType 		= new ArithmType(T_BOOL);
	static ArithmType* complexType 		= new ArithmType(T_FLOAT | T_COMPLEX);
	static ArithmType* doublecomplexType = new ArithmType(T_DOUBLE | T_COMPLEX);
	static ArithmType* longdoublecomplexType = new ArithmType(T_LONG | T_DOUBLE | T_COMPLEX);

	switch (typeSpec) {
	case T_CHAR:
	case T_SIGNED | T_CHAR:
		return charType;

	case T_UNSIGNED | T_CHAR:
		return ucharType;

	case T_SHORT:
	case T_SIGNED | T_SHORT:
	case T_SHORT | T_INT:
	case T_SIGNED | T_SHORT | T_INT:
		return shortType;

	case T_UNSIGNED | T_SHORT:
	case T_UNSIGNED | T_SHORT | T_INT:
		return ushortType;

	case T_INT:
	case T_SIGNED:
	case T_SIGNED | T_INT:
		return intType;

	case T_UNSIGNED:
	case T_UNSIGNED | T_INT:
		return uintType;

	case T_LONG:
	case T_SIGNED | T_LONG:
	case T_LONG | T_INT:
	case T_SIGNED | T_LONG | T_INT:
		return longType;

	case T_UNSIGNED | T_LONG:
	case T_UNSIGNED | T_LONG | T_INT:
		return ulongType;

	case T_LONG_LONG:
	case T_SIGNED | T_LONG_LONG:
	case T_LONG_LONG | T_INT:
	case T_SIGNED | T_LONG_LONG | T_INT:
		return longlongType;

	case T_UNSIGNED | T_LONG_LONG:
	case T_UNSIGNED | T_LONG_LONG | T_INT:
		return ulonglongType;

	case T_FLOAT:
		return floatType;

	case T_DOUBLE:
		return doubleType;

	case T_LONG | T_DOUBLE:
		return longdoubleType;

	case T_BOOL:
		return boolType;

	case T_FLOAT | T_COMPLEX:
		return complexType;

	case T_DOUBLE | T_COMPLEX:
		return doublecomplexType;

	case T_LONG | T_DOUBLE | T_COMPLEX:
		return longdoublecomplexType;

	default: assert(0);
	}

	return nullptr; // make compiler happy
}

ArrayType* Type::NewArrayType(long long len, Type* eleType)
{
	return new ArrayType(len, eleType);
}

//static IntType* NewIntType();
FuncType* Type::NewFuncType(Type* derived, int funcSpec,
		bool hasEllipsis, const std::list<Type*>& params) {
	return new FuncType(derived, funcSpec, hasEllipsis, params);
}

PointerType* Type::NewPointerType(Type* derived) {
	return new PointerType(derived);
}

StructUnionType* Type::NewStructUnionType(bool isStruct) {
	return new StructUnionType(isStruct);
}

/*
static EnumType* Type::NewEnumType() {
	// TODO(wgtdkp):
	assert(false);
	return nullptr;
}
*/

/*************** ArithmType *********************/
int ArithmType::CalcWidth(int spec) {
	switch (spec) {
	case T_BOOL:
	case T_CHAR:
	case T_UNSIGNED | T_CHAR:
		return 1;

	case T_SHORT:
	case T_UNSIGNED | T_SHORT:
		return _machineWord >> 1;

	case T_INT:
	case T_UNSIGNED | T_INT:
		return _machineWord;

	case T_LONG:
	case T_UNSIGNED | T_LONG:
		return _machineWord;

	case T_LONG_LONG:
	case T_UNSIGNED | T_LONG_LONG:
		return _machineWord << 1;

	case T_FLOAT:
		return _machineWord;

	case T_DOUBLE:
	case T_LONG | T_DOUBLE:
		return _machineWord << 1;

	case T_FLOAT | T_COMPLEX:
		return _machineWord << 1;

	case T_DOUBLE | T_COMPLEX:
	case T_LONG | T_DOUBLE | T_COMPLEX:
		return _machineWord << 2;
	}

	return _machineWord;
}


/*************** PointerType *****************/

bool PointerType::operator==(const Type& other) const {
	auto pointerType = other.ToPointerType();
	if (nullptr == pointerType) return false;
	return *_derived == *pointerType->_derived;
}

bool PointerType::Compatible(const Type& other) const {
	//TODO: compatibility ???
	assert(false);
	return false;
}


/************* FuncType ***************/

bool FuncType::operator==(const Type& other) const
{
	auto otherFunc = other.ToFuncType();
	if (nullptr == otherFunc) return false;
	// TODO: do we need to check the type of return value when deciding 
	// equality of two function types ??
	if (*_derived != *otherFunc->_derived)
		return false;
	if (_params.size() != otherFunc->_params.size())
		return false;
	auto thisIter = _params.begin();
	auto otherIter = otherFunc->_params.begin();
	while (thisIter != _params.end()) {
		if (*(*thisIter) != *(*otherIter))
			return false;

		thisIter++;
		otherIter++;
	}

	return true;
}

bool FuncType::Compatible(const Type& other) const
{
	auto otherFunc = other.ToFuncType();
	//the other type is not an function type
	if (nullptr == otherFunc) return false;
	//TODO: do we need to check the type of return value when deciding 
	//compatibility of two function types ??
	if (_derived->Compatible(*otherFunc->_derived))
		return false;
	if (_params.size() != otherFunc->_params.size())
		return false;
	auto thisIter = _params.begin();
	auto otherIter = otherFunc->_params.begin();
	while (thisIter != _params.end()) {
		if ((*thisIter)->Compatible(*(*otherIter)))
			return false;

		thisIter++;
		otherIter++;
	}

	return true;
}

/********* StructUnionType ***********/

StructUnionType::StructUnionType(bool isStruct)
		:  Type(0, false), _isStruct(isStruct), _mapMember(new Env()) {
}

Variable* StructUnionType::Find(const char* name) {
	return _mapMember->FindVar(name);
}

const Variable* StructUnionType::Find(const char* name) const {
	return _mapMember->FindVar(name);
}

int StructUnionType::CalcWidth(const Env* env)
{
	int width = 0;
	auto iter = env->_symbMap.begin();
	for (; iter != env->_symbMap.end(); iter++)
		width += iter->second->Ty()->Width();
	return width;
}

bool StructUnionType::operator==(const Type& other) const
{
	auto structUnionType = other.ToStructUnionType();
	if (nullptr == structUnionType) return false;
	return *_mapMember == *structUnionType->_mapMember;
}

bool StructUnionType::Compatible(const Type& other) const {
	// TODO: 
	return *this == other;
}

void StructUnionType::AddMember(const char* name, Type* type)
{
	auto newMember = _mapMember->InsertVar(name, type);
	if (!IsStruct())
		newMember->SetOffset(0);
}
