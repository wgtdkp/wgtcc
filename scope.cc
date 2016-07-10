#include "scope.h"

#include "ast.h"

#include <cassert>


Identifier* Scope::Find(const std::string& name)
{
    auto ident = _identMap.find(name);
    if (ident != _identMap.end())
        return ident->second;
    
    if (_type == S_FILE)
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
    if (tag) {
        assert(tag->ToType());
    }
    return tag;
}
