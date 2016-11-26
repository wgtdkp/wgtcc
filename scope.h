#ifndef _WGTCC_SCOPE_H_
#define _WGTCC_SCOPE_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

class Identifier;
class Token;

enum class ScopeType {
  FILE,
  PROTO,
  BLOCK,
  FUNC,
};


class Scope {
  friend class StructType;
  using TagList  = std::vector<Identifier*>;
  using IdentMap = std::map<std::string, Identifier*>;

public:
  static Scope* New(Scope* parent, ScopeType type);
  ~Scope() {}
  Scope* parent() { return parent_; }
  void set_parent(Scope* parent) { parent_ = parent; }
  ScopeType type() const { return type_; }

  Identifier* Find(const Token* tok);
  Identifier* FindInCurScope(const Token* tok);
  Identifier* FindTag(const Token* tok);
  Identifier* FindTagInCurScope(const Token* tok);
  TagList AllTagsInCurScope() const;
  void Insert(Identifier* ident);
  void InsertTag(Identifier* ident);
  void Print();

  bool operator==(const Scope& other) const { return type_ == other.type_; }
  IdentMap::iterator begin() { return ident_map_.begin(); }
  IdentMap::iterator end() { return ident_map_.end(); }
  size_t size() const { return ident_map_.size(); }
  void Insert(const std::string& name, Identifier* ident);

private:
  explicit Scope(Scope* parent, ScopeType type)
      : parent_(parent), type_(type) {}
  
  Identifier* Find(const std::string& name);
  Identifier* FindInCurScope(const std::string& name);
  Identifier* FindTag(const std::string& name);
  Identifier* FindTagInCurScope(const std::string& name);
  static std::string TagName(const std::string& name) {
    return name + "@:tag";
  }
  static bool IsTagName(const std::string& name) {
    return name.size() > 5 && name[name.size() - 5] == '@';
  }
  const Scope& operator=(const Scope& other);
  Scope(const Scope& scope);

  Scope* parent_;
  ScopeType type_;
  IdentMap ident_map_;
};

#endif
