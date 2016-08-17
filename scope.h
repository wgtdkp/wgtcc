#ifndef _WGTCC_SCOPE_H_
#define _WGTCC_SCOPE_H_

#include <iostream>
#include <map>
#include <string>


class Identifier;
class Token;


enum ScopeType {
    S_FILE,
    S_PROTO,
    S_BLOCK,
    S_FUNC,
};


class Scope
{
    friend class StructUnionType;
    typedef std::map<std::string, Identifier*> IdentMap;

public:
    explicit Scope(Scope* parent, enum ScopeType type)
            : _parent(parent), _type(type) {}
    
    ~Scope(void) {}

    Scope* Parent(void) {
        return _parent;
    }

    void SetParent(Scope* parent) {
        _parent = parent;
    }

    enum ScopeType Type(void) const {
        return _type;
    }
    
    std::string TagName(const std::string& name) {
        return name + "@tag";
    }

    Identifier* Find(const Token* tok);
    Identifier* FindInCurScope(const Token* tok);
    Identifier* FindTag(const Token* tok);
    Identifier* FindTagInCurScope(const Token* tok);

    void Insert(Identifier* ident);

    void InsertTag(Identifier* ident);

    void Print(void);

    bool operator==(const Scope& other) const {
        return _type == other._type;
    }

    IdentMap::iterator begin(void) {
        return _identMap.begin();
    }

    IdentMap::iterator end(void) {
        return _identMap.end();
    }

    size_t size(void) const {
        return _identMap.size();
    }

private:
    Identifier* Find(const std::string& name);

    Identifier* FindInCurScope(const std::string& name);

    Identifier* FindTag(const std::string& name);

    Identifier* FindTagInCurScope(const std::string& name);

    void Insert(const std::string& name, Identifier* ident);

    const Scope& operator=(const Scope& other);
    Scope(const Scope& scope);
    //typedef std::map<std::string, Type*> TagMap;

    Scope* _parent;
    enum ScopeType _type;

    IdentMap _identMap;
};

#endif
