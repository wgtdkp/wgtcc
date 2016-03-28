#ifndef _ENV_H_
#define _ENV_H_

#include <map>
#include <list>
#include <cassert>
#include "type.h"


struct StrCmp
{
	bool operator()(const char* lhs, const char* rhs) const {
		return strcmp(lhs, rhs) < 0;
	}
};

class Env
{
	friend class StructUnionType;
public:
	explicit Env(Env* parent=nullptr) 
		: _parent(parent), _offset(0) {}

	Variable* Find(const char* name);
	const Variable* Find(const char* name) const;

	Type* FindTypeInCurScope(const char* name);
	const Type* FindTypeInCurScope(const char* name) const;

	Variable* FindVarInCurScope(const char* name);
	const Variable* FindVarInCurScope(const char* name) const;
		 
	Type* FindType(const char* name);
	const Type* FindType(const char* name) const;

	Variable* FindVar(const char* name);
	const Variable* FindVar(const char* name) const;

	Type* InsertType(const char* name, Type* type);
	Variable* InsertVar(const char* name, Type* type);

	bool operator==(const Env& other) const;
	Env* Parent(void) { return _parent; }
	const Env* Parent(void) const { return _parent; }

	Type* FindTagInCurScope(const char* tag);
	Type* InsertTag(const char* tag, Type* type);
	Type* FindTag(const char* tag);

	Constant* InsertConstant(const char* name, Constant* constant);
private:
	typedef std::map<const char*, Variable*, StrCmp> SymbMap;
	typedef std::map<const char*, Type*, StrCmp> TagMap;

	Env* _parent;
	int _offset;
	SymbMap _symbMap;
	TagMap _tagMap;
};

#endif
