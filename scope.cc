#include "scope.h"

#include "ast.h"
#include "mem_pool.h"

#include <cassert>
#include <iostream>

static MemPoolImp<Scope>  scope_pool;


Scope* Scope::New(Scope* parent, ScopeType type) {
  return new (scope_pool.Alloc()) Scope(parent, type);
}


Identifier* Scope::Find(const Token* tok) {
  auto ret = Find(tok->str());
  if (ret) ret->set_tok(tok);
  return ret;
}


Identifier* Scope::FindInCurScope(const Token* tok) {
  auto ret = FindInCurScope(tok->str());
  if (ret) ret->set_tok(tok);
  return ret;
}


Identifier* Scope::FindTag(const Token* tok) {
  auto ret = FindTag(tok->str());
  if (ret) ret->set_tok(tok);
  return ret;
}


Identifier* Scope::FindTagInCurScope(const Token* tok) {
  auto ret = FindTagInCurScope(tok->str());
  if (ret) ret->set_tok(tok);
  return ret;
}


void Scope::Insert(Identifier* ident) {
  Insert(ident->Name(), ident);
}


void Scope::InsertTag(Identifier* ident) {
  Insert(TagName(ident->Name()), ident);
}


Identifier* Scope::Find(const std::string& name) {
  auto ident = ident_map_.find(name);
  if (ident != ident_map_.end())
    return ident->second;
  if (type_ == ScopeType::FILE || parent_ == nullptr)
    return nullptr;
  return parent_->Find(name);
}


Identifier* Scope::FindInCurScope(const std::string& name) {
  auto ident = ident_map_.find(name);
  if (ident == ident_map_.end())
    return nullptr;
  return ident->second;
}


void Scope::Insert(const std::string& name, Identifier* ident) {
  assert(FindInCurScope(name) == nullptr);
  ident_map_[name] = ident;
}


Identifier* Scope::FindTag(const std::string& name) {
  auto tag = Find(TagName(name));
  if (tag) assert(tag->ToTypeName());
  return tag;
}


Identifier* Scope::FindTagInCurScope(const std::string& name) {
  auto tag = FindInCurScope(TagName(name));
  assert(tag == nullptr || tag->ToTypeName());
  return tag;
}


Scope::TagList Scope::AllTagsInCurScope() const {
  TagList tags;
  for (auto& kv: ident_map_) {
    if (IsTagName(kv.first))
      tags.push_back(kv.second);
  }
  return tags;
}


void Scope::Print() {
  std::cout << "scope: " << this << std::endl;

  auto iter = ident_map_.begin();
  for (; iter != ident_map_.end(); ++iter) {
    auto name = iter->first;
    auto ident = iter->second;
    if (ident->ToTypeName()) {
      std::cout << name << "\t[type:\t"
                << ident->type()->Str() << "]" << std::endl;
    } else {
      std::cout << name << "\t[object:\t";
      std::cout << ident->type()->Str() << "]" << std::endl;
    }
  }
  std::cout << std::endl;
}
