#include "env.h"
#include "ast.h"
#include "error.h"

using namespace std;


/************** Env ******************/

Variable* Env::Find(const char* name)
{
	auto symb = _symbMap.find(name);
	if (_symbMap.end() != symb)
		return symb->second;
	if (nullptr == _parent) return false;
	return _parent->Find(name);
}

const Variable* Env::Find(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Variable*>(thi->Find(name));
}

Type* Env::FindTypeInCurScope(const char* name)
{
	auto type = _symbMap.find(name);
	if (_symbMap.end() == type)
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
	auto var = _symbMap.find(name);
	if (_symbMap.end() == var)
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
	auto symb = Find(name);
	if (nullptr == symb)
		return nullptr;
	//found the type in current scope
	return symb->IsVar() ? nullptr : symb->Ty();
} 

const Type* Env::FindType(const char* name) const
{
	auto thi = const_cast<Env*>(this);
	return const_cast<const Type*>(thi->FindType(name));
}

Variable* Env::FindVar(const char* name)
{
	auto symb = Find(name);
	if (nullptr == symb)
		return nullptr;
	//found the type in current scope
	return symb->IsVar() ? symb : nullptr;
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
	//auto iter = _symbMap.find(name);
	//不允许覆盖完整的类型，也不允许覆盖不完整的类型，而应该修改此不完整的类型为完整的
	assert(nullptr == FindTypeInCurScope(name));
	//assert(!iter->second->Ty()->IsComplete());
	auto var = TranslationUnit::NewVariable(type, Variable::TYPE);
	_symbMap[name] = var;
	return var->Ty();
}

Variable* Env::InsertVar(const char* name, Type* type)
{
	//不允许重复定义，这应该由调用方检查
	assert(nullptr == FindVarInCurScope(name));
	auto var = TranslationUnit::NewVariable(type, _offset);
	_symbMap[name] = var;
	_offset += type->Align();
	return var;
}

bool Env::operator==(const Env& other) const
{
	if (this->_symbMap.size() != other._symbMap.size())
		return false;
	auto iterThis = this->_symbMap.begin();
	auto iterOther = other._symbMap.begin();
	for (; iterThis != this->_symbMap.end(); iterThis++, iterOther++) {
		if (0 != strcmp(iterThis->first, iterOther->first))
			return false;
		if (*iterThis->second != *iterOther->second)
			return false;
	}
	return true;
}

Constant* Env::InsertConstant(const char* name, Constant* constant) {
	//注意enumeration constant 与一般变量处于同一命名空间
	assert(nullptr == FindVarInCurScope(name));
	_symbMap[name] = constant;
	return constant;
}

Type* Env::FindTagInCurScope(const char* tag)
{
	auto type = _tagMap.find(tag);
	if (_tagMap.end() == type) return nullptr;
	return type->second;
}

Type* Env::InsertTag(const char* tag, Type* type)
{
	assert(nullptr == FindTagInCurScope(tag));
	_tagMap[tag] = type;
	return type;
}

Type* Env::FindTag(const char* tag)
{
	auto type = FindTagInCurScope(tag);
	if (nullptr != type) return type;
	if (nullptr == _parent) return false;
	return _parent->FindTag(tag);
}
