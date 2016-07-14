#include "scope.h"

#include "ast.h"

#include <cassert>


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
    // TODO(wgtdkp):
    // Set offset when 'ident' is an object
    Object* obj = ident->ToObject();
    if (obj) {
        obj->SetOffset(_offset);
        _offset += obj->Ty()->Width();
    }
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

/*
std::pair<IdentMap::iterator, bool> Scope::Concat(Scope* other)
{
    auto offset = _offset;
    auto iter = other->_identMap->begin();
    for (; iter != other->_identMap->end(); iter++) {
        auto pair = _curScope->insert(iter->first, iter->second);
    }
    type->MemberMap()->SetOffset(offset + AnonType->Width());
}
*/

void Scope::Print(void)
{
    printf("external symbol map:\n");
    
    auto iter = _identMap.begin();
    for (; iter != _identMap.end(); iter++) {
        auto name = iter->first;
        auto ident = iter->second;
        if (ident->Ty()->ToFuncType()) {
            printf("%s\t[function]\n", name.c_str());
        } else {
            printf("%s\t[object]\n", name.c_str());
        }
    }
}
