#ifndef _WGTCC_CPP_H_
#define _WGTCC_CPP_H_

#include "string_pair.h"
#include "token.h"
#include "scanner.h"

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>

class Scanner;
class Macro;
struct CondDirective;

typedef std::map<std::string, Macro> MacroMap; 
typedef std::list<std::string> ParamList;
typedef std::map<std::string, TokenSequence> ParamMap;
typedef std::stack<CondDirective> PPCondStack;
typedef std::list<std::string> PathList;


class Macro
{
public:
  Macro(const TokenSequence& repSeq, bool preDef=false)
      : funcLike_(false), variadic_(false),
        preDef_(preDef), repSeq_(repSeq) {}

  Macro(bool variadic, ParamList& params,
        TokenSequence& repSeq, bool preDef=false)
      : funcLike_(true), variadic_(variadic), preDef_(preDef),
        params_(params), repSeq_(repSeq) {}
  
  ~Macro() {}

  /*
  std::string Name() {
    return tok_->Str();
  }
  */

  bool FuncLike() {
    return funcLike_;
  }

  bool ObjLike() {
    return !FuncLike();
  }

  bool Variadic() {
    return variadic_;
  }

  bool PreDef() {
    return preDef_;
  }

  ParamList& Params() {
    return params_;
  }

  TokenSequence RepSeq(const std::string* fileName, unsigned line);

private:
  //Token* tok_;
  bool funcLike_;
  bool variadic_;
  bool preDef_;
  ParamList params_;
  TokenSequence repSeq_;
};


struct CondDirective
{
  int tag_;
  bool enabled_;
  bool cond_;
};


class Preprocessor
{
public:
  Preprocessor(const std::string* fileName)
    : curLine_(1), lineLine_(0), curCond_(true) {}

  ~Preprocessor() {}
  void Finalize(TokenSequence os);
  void Process(TokenSequence& os);
  void Expand(TokenSequence& os, TokenSequence& is, bool inCond=false);
  void Subst(TokenSequence& os, TokenSequence& is, HideSet* hs, ParamMap& params);
  void Glue(TokenSequence& os, TokenSequence& is);
  void Glue(TokenSequence& os, Token* tok);
  std::string Stringize(TokenSequence is);
  void Stringize(std::string& str, TokenSequence is);
  Token* ParseActualParam(TokenSequence& is, Macro* macro, ParamMap& paramMap);
  int GetDirective(TokenSequence& is);
  void ReplaceDefOp(TokenSequence& is);
  void ReplaceIdent(TokenSequence is);
  void ParseDirective(TokenSequence& os, TokenSequence& is, int directive);
  void ParseIf(TokenSequence ls);
  void ParseIfdef(TokenSequence ls);
  void ParseIfndef(TokenSequence ls);
  void ParseElif(TokenSequence ls);
  void ParseElse(TokenSequence ls);
  void ParseEndif(TokenSequence ls);
  void ParseInclude(TokenSequence& is, TokenSequence ls);
  void ParseDef(TokenSequence ls);
  void ParseUndef(TokenSequence ls);
  void ParseLine(TokenSequence ls);
  void ParseError(TokenSequence ls);
  void ParsePragma(TokenSequence ls);
  void IncludeFile(TokenSequence& is, const std::string* fileName);
  bool ParseIdentList(ParamList& params, TokenSequence& is);
  

  Macro* FindMacro(const std::string& name) {
    auto res = macroMap_.find(name);
    if (res == macroMap_.end())
      return nullptr;
    return &res->second;
  }

  void AddMacro(const std::string& name,
      std::string* text, bool preDef=false);

  void AddMacro(const std::string& name, const Macro& macro) {
    auto res = macroMap_.find(name);
    if (res != macroMap_.end()) {
      // TODO(wgtdkp): give warning
      macroMap_.erase(res);
    }
    macroMap_.insert(std::make_pair(name, macro));
  }

  void RemoveMacro(const std::string& name) {
    auto res = macroMap_.find(name);
    if (res == macroMap_.end())
      return;
    if(res->second.PreDef()) // can't undef predefined macro
      return;
    macroMap_.erase(res);
  }

  std::string* SearchFile(
      const std::string& name,
      bool libHeader,
      bool next,
      const std::string* curPath=nullptr);

  void AddSearchPath(const std::string& path);
  void HandleTheFileMacro(TokenSequence& os, Token* macro);
  void HandleTheLineMacro(TokenSequence& os, Token* macro);
  void UpdateTokenLocation(Token* tok);

  //bool Hidden(const std::string& name) {
  //    return hs_.find(name) != hs_.end();
  //}

  bool NeedExpand() const {
    if (ppCondStack_.empty())
      return true;
    auto top = ppCondStack_.top();
    return top.enabled_ && top.cond_;
  }

private:
  void Init();

  //HideSet hs_;
  PPCondStack ppCondStack_;
  unsigned curLine_;
  unsigned lineLine_;
  bool curCond_;
  
  MacroMap macroMap_;
  PathList searchPathList_;
};

#endif
