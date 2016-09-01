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
  friend class StructType;
  typedef std::map<std::string, Identifier*> IdentMap;

public:
  explicit Scope(Scope* parent, enum ScopeType type)
      : parent_(parent), type_(type) {}
  
  ~Scope() {}

  Scope* Parent() {
    return parent_;
  }

  void SetParent(Scope* parent) {
    parent_ = parent;
  }

  enum ScopeType Type() const {
    return type_;
  }
  
  std::string TagName(const std::string& name) {
    return name + "@:tag";
  }

  Identifier* Find(const Token* tok);
  Identifier* FindInCurScope(const Token* tok);
  Identifier* FindTag(const Token* tok);
  Identifier* FindTagInCurScope(const Token* tok);

  void Insert(Identifier* ident);

  void InsertTag(Identifier* ident);

  void Print();

  bool operator==(const Scope& other) const {
    return type_ == other.type_;
  }

  IdentMap::iterator begin() {
    return identMap_.begin();
  }

  IdentMap::iterator end() {
    return identMap_.end();
  }

  size_t size() const {
    return identMap_.size();
  }

  void Insert(const std::string& name, Identifier* ident);

private:
  Identifier* Find(const std::string& name);

  Identifier* FindInCurScope(const std::string& name);

  Identifier* FindTag(const std::string& name);

  Identifier* FindTagInCurScope(const std::string& name);

  //void Insert(const std::string& name, Identifier* ident);

  const Scope& operator=(const Scope& other);
  Scope(const Scope& scope);
  //typedef std::map<std::string, Type*> TagMap;

  Scope* parent_;
  enum ScopeType type_;

  IdentMap identMap_;
};

#endif
