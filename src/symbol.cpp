#include "symbol.h"
#include "ast.h"
#include "expr.h"
#include "error.h"

using namespace std;

/***************** Type *********************/

bool Type::IsInteger(void) const {
	auto arithmType = ToArithmType();
	if (nullptr == arithmType) return false;
	return arithmType->IsInteger();
}

bool Type::IsReal(void) const {
	auto arithmType = ToArithmType();
	if (nullptr == arithmType) return false;
	return arithmType->IsReal();
}


ArithmType* _arithmTypes[ArithmType::SIZE] = { 0 };



VoidType* Type::NewVoidType(void)
{
	return new VoidType();
}

ArithmType* Type::NewArithmType(int tag) {
	if (nullptr == _arithmTypes[tag]) {
		_arithmTypes[tag] = new ArithmType(tag);
	}
	return _arithmTypes[tag];
}

//static IntType* NewIntType();
FuncType* Type::NewFuncType(Type* derived, int funcSpec, const std::list<Type*>& params) {
	return new FuncType(derived, funcSpec, params);
}

PointerType* Type::NewPointerType(Type* derived) {
	return new PointerType(derived);
}

StructUnionType* Type::NewStructUnionType(bool isStruct) {
	return new StructUnionType(isStruct);
}

static EnumType* NewEnumType() {
	assert(false);
	return nullptr;
}


/*************** ArithmType *********************/
int ArithmType::CalcWidth(int tag) {
	switch (tag) {
	case TBOOL: case TCHAR: case TUCHAR: return 1;
	case TSHORT: case TUSHORT: return _machineWord >> 1;
	case TINT: case TUINT: return _machineWord;
	case TLONG: case TULONG: return _machineWord;
	case TLLONG: case TULLONG: return _machineWord << 1;
	case TFLOAT: return _machineWord;
	case TDOUBLE: case TLDOUBLE: return _machineWord << 1;
	case TFCOMPLEX: return _machineWord << 1;
	case TDCOMPLEX: case TLDCOMPLEX: return _machineWord << 2;
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
	//TODO: do we need to check the type of return value when deciding 
	//equality of two function types ??
	if (*_derived != *otherFunc->_derived)
		return false;
	if (_params.size() != otherFunc->_params.size())
		return false;
	auto thisIter = _params.begin();
	auto otherIter = _params.begin();
	for (; thisIter != _params.end(); thisIter++) {
		if (*(*thisIter) != *(*otherIter))
			return false;
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
	auto otherIter = _params.begin();
	for (; thisIter != _params.end(); thisIter++) {
		if ((*thisIter)->Compatible(*(*otherIter)))
			return false;
		otherIter++;
	}
	return true;
}


/********* StructUnionType ***********/

Variable* StructUnionType::Find(const char* name) {
	return _mapMember->FindVar(name);
}

const Variable* StructUnionType::Find(const char* name) const {
	return _mapMember->FindVar(name);
}

int StructUnionType::CalcWidth(const Env* env)
{
	int width = 0;
	auto iter = env->_mapSymb.begin();
	for (; iter != env->_mapSymb.end(); iter++)
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
	//TODO: 
	return *this == other;
}

void StructUnionType::AddMember(const char* name, Type* type)
{
	auto newMember = _mapMember->InsertVar(name, type);
	if (!IsStruct())
		newMember->SetOffset(0);
}


/************** Env ******************/

Symbol* Env::Find(const char* name)
{
	auto symb = _mapSymb.find(name);
	if (_mapSymb.end() == symb) {
		if (nullptr == _parent)
			return nullptr;
		return _parent->Find(name);
	}

	return symb->second;
}

const Symbol* Env::Find(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Symbol*>(thi->Find(name));
}

Type* Env::FindTypeInCurScope(const char* name)
{
	auto type = _mapSymb.find(name);
	if (_mapSymb.end() == type)
		return nullptr;
	return type->second->IsVar() ? nullptr : type->second->Ty();
}

const Type* Env::FindTypeInCurScope(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Type*>(thi->FindTypeInCurScope(name));
}

Variable* Env::FindVarInCurScope(const char* name)
{
	auto var = _mapSymb.find(name);
	if (_mapSymb.end() == var)
		return nullptr;
	return var->second->IsVar() ? var->second : nullptr;
}

const Variable* Env::FindVarInCurScope(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Variable*>(thi->FindVarInCurScope(name));
}


Type* Env::FindType(const char* name)
{
	auto type = Find(name);
	if (nullptr == type)
		return nullptr;
	//found the type in current scope
	return type->IsVar() ? nullptr : type->Ty();
} 

const Type* Env::FindType(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Type*>(thi->FindType(name));
}

Variable* Env::FindVar(const char* name)
{
	auto type = Find(name);
	if (nullptr == type)
		return nullptr;
	//found the type in current scope
	return type->IsVar() ? type : nullptr;
}

const Variable* Env::FindVar(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Variable*>(thi->FindVar(name));
}

/*
如果名字不在符号表内，那么在当前作用域插入名字；
如果名字已经在符号表内，那么更新其类型；（这显然会造成内存泄露，旧的不完整类型可能被引用也可能没有被引用）
插入冲突应该由调用方检查；
*/
Type* Env::InsertType(const char* name, Type* type)
{
	auto iter = _mapSymb.find(name);
	//不允许覆盖完整的类型，也不允许覆盖不完整的类型，而应该修改此不完整的类型为完整的
	assert(!iter->second->Ty()->IsComplete());
	auto var = TranslationUnit::NewVariable(type, Variable::TYPE);
	_mapSymb[name] = var;
	return var->Ty();
}

Variable* Env::InsertVar(const char* name, Type* type)
{
	//不允许重复定义，这应该由调用方检查
	assert(_mapSymb.end() == _mapSymb.find(name));
	auto var = TranslationUnit::NewVariable(type, _offset);
	_mapSymb[name] = var;
	_offset += type->Align();
	return var;
}

bool Env::operator==(const Env& other) const
{
	if (this->_mapSymb.size() != other._mapSymb.size())
		return false;
	auto iterThis = this->_mapSymb.begin();
	auto iterOther = other._mapSymb.begin();
	for (; iterThis != this->_mapSymb.end(); iterThis++, iterOther++) {
		if (0 != strcmp(iterThis->first, iterOther->first))
			return false;
		if (*iterThis->second != *iterOther->second)
			return false;
	}
	return true;
}
