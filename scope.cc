#include "scope.h"

#include "ast.h"

#include <cassert>
#include <iostream>


Identifier* Scope::Find(const std::string& name)
{
    auto ident = _identMap.find(name);
    if (ident != _identMap.end())
        return ident->second;
    
    if (_type == S_FILE || _parent == nullptr)
        return nullptr;
    return _parent->Find(name);
}


Identifier* Scope::FindInCurScope(const std::string& name)
{
    auto ident = _identMap.find(name);
    if (ident == _identMap.end())
        return nullptr;
    return ident->second;
}


void Scope::Insert(const std::string& name, Identifier* ident)
{
    assert(FindInCurScope(name) == nullptr);

    _identMap[name] = ident;
}


Identifier* Scope::FindTag(const std::string& name) {
    auto tag = Find(TagName(name));
    if (tag) {
        assert(tag->ToType());
    }
    return tag;
}


Identifier* Scope::FindTagInCurScope(const std::string& name) {
    auto tag = FindInCurScope(TagName(name));
    assert(tag == nullptr || tag->ToType());
    
    return tag;
}


void Scope::Print(void)
{
    std::cout << "scope: " << this << std::endl;

    auto iter = _identMap.begin();
    for (; iter != _identMap.end(); iter++) {
        auto name = iter->first;
        auto ident = iter->second;
        if (ident->ToType()) {
            std::cout << name << "\t[type:\t" << ident->Type()->Str() << "]" << std::endl;
        } else {
            std::cout << name << "\t[object:\t";
            std::cout << ident->Type()->Str() << "]" << std::endl;
        }
    }
    std::cout << std::endl;
}
