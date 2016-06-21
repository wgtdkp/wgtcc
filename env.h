#ifndef _WGTCC_ENV_H_
#define _WGTCC_ENV_H_

#include "type.h"

#include <cassert>
#include <cstring>

#include <map>
#include <list>
#include <string>


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

    Variable* Find(const std::string& name);
    const Variable* Find(const std::string& name) const;

    Type* FindTypeInCurScope(const std::string& name);
    const Type* FindTypeInCurScope(const std::string& name) const;

    Variable* FindVarInCurScope(const std::string& name);
    const Variable* FindVarInCurScope(const std::string& name) const;
         
    Type* FindType(const std::string& name);
    const Type* FindType(const std::string& name) const;

    Variable* FindVar(const std::string& name);
    const Variable* FindVar(const std::string& name) const;

    void InsertType(const std::string& name, Variable* var);
    void InsertVar(const std::string& name, Variable* var);

    bool operator==(const Env& other) const;
    Env* Parent(void) { return _parent; }
    const Env* Parent(void) const { return _parent; }

    Type* FindTagInCurScope(const std::string& tag);
    Type* InsertTag(const std::string& tag, Type* type);
    Type* FindTag(const std::string& tag);

    Constant* InsertConstant(const std::string& name, Constant* constant);

private:
    typedef std::map<std::string, Variable*> SymbMap;
    typedef std::map<std::string, Type*> TagMap;

    Env* _parent;
    int _offset;
    SymbMap _symbMap;
    TagMap _tagMap;
};

#endif
