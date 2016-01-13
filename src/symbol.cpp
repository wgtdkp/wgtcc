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

ArithmType* Type::NewArithmType(int tag) {
	if (nullptr == _arithmTypes[tag]) {
		_arithmTypes[tag] = new ArithmType(tag);
	}
	return _arithmTypes[tag];
}

//static IntType* NewIntType();
FuncType* Type::NewFuncType(Type* derived, const std::list<Type*>& params) {
	return new FuncType(derived, params);
}

PointerType* Type::NewPointerType(Type* derived) {
	return new PointerType(derived);
}

StructUnionType* Type::NewStructUnionType(Env* env) {
	return new StructUnionType(env);
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
	return _env->FindVar(name);
}

const Variable* StructUnionType::Find(const char* name) const {
	return _env->FindVar(name);
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
	return *_env == *structUnionType->_env;
}

bool StructUnionType::Compatible(const Type& other) const {
	//TODO: 
	return *this == other;
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

void Env::InsertType(const char* name, Type* type)
{
	//find in current scope
	auto symb = _mapSymb.find(name);
	if (_mapSymb.end() != symb) {
		//if (symb->second->IsVar())
		//	_mapSymb.erase(symb);
		//else {
			//TODO: error redefine
			Error("redefine of symbol '%s'", name);
		//}
	}
	_mapSymb[name] =  TranslationUnit::NewVariable(type, Variable::TYPE);
}

void Env::InsertVar(const char* name, Type* type)
{
	auto symb = _mapSymb.find(name);
	if (_mapSymb.end() != symb) {
		//if (!symb->second->IsVar())
		//	_mapSymb.erase(symb);
		//else {
			//TODO: error redefine
			Error("redefine of symbol '%s'", name);
		//}
	}
	_mapSymb[name] = TranslationUnit::NewVariable(type, _offset);
	_offset += type->Width();
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
