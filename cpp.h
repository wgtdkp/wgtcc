#ifndef _WGTCC_CPP_H_
#define _WGTCC_CPP_H_

#include "scanner.h"

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>

class Macro;
struct CondDirective;

using MacroMap    = std::map<std::string, Macro>; 
using ParamList   = std::list<std::string>;
using ParamMap    = std::map<std::string, TokenSequence>;
using PPCondStack = std::stack<CondDirective>;
using PathList    = std::list<std::string>;


class Macro {
public:
  Macro(const TokenSequence& rep_seq, bool pre_def=false)
      : func_like_(false), variadic_(false),
        pre_def_(pre_def), rep_seq_(rep_seq) {}
  Macro(bool variadic, ParamList& param_list,
        TokenSequence& rep_seq, bool pre_def=false)
      : func_like_(true), variadic_(variadic), pre_def_(pre_def),
        param_list_(param_list), rep_seq_(rep_seq) {}
  ~Macro() {}

  bool func_like() { return func_like_; }
  bool obj_like() { return !func_like(); }
  bool variadic() { return variadic_; }
  bool pre_def() { return pre_def_; }
  ParamList& param_list() { return param_list_; }
  TokenSequence pep_seq(const std::string* filename, unsigned line);

private:
  bool func_like_;
  bool variadic_;
  bool pre_def_;
  ParamList param_list_;
  TokenSequence rep_seq_;
};


struct CondDirective {
  int tag_;
  bool enabled_;
  bool cond_;
};


class Preprocessor {
public:
  Preprocessor(const std::string* filename)
      : cur_line_(1), line_line_(0), cur_cond_(true) {
    // Add predefined
    Init();
  }

  ~Preprocessor() {}
  void Finalize(TokenSequence os);
  void Process(TokenSequence& os);
  void Expand(TokenSequence& os, TokenSequence is, bool in_cond=false);
  void Subst(TokenSequence& os, TokenSequence is,
             bool leading_ws, const HideSet& hs, ParamMap& param_list);
  void Glue(TokenSequence& os, TokenSequence is);
  void Glue(TokenSequence& os, const Token* tok);
  const Token* Stringize(TokenSequence is);
  void Stringize(std::string& str, TokenSequence is);
  const Token* ParseActualParam(TokenSequence& is, Macro* macro, ParamMap& param_map);
  int GetDirective(TokenSequence& is);
  void ReplaceDefOp(TokenSequence& is);
  void ReplaceIdent(TokenSequence& is);
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
  void IncludeFile(TokenSequence& is, const std::string* filename);
  bool ParseIdentList(ParamList& param_list, TokenSequence& is);
  

  Macro* FindMacro(const std::string& name) {
    auto res = macro_map_.find(name);
    if (res == macro_map_.end())
      return nullptr;
    return &res->second;
  }

  void AddMacro(const std::string& name,
      std::string* text, bool pre_def=false);

  void AddMacro(const std::string& name, const Macro& macro) {
    auto res = macro_map_.find(name);
    if (res != macro_map_.end()) {
      // TODO(wgtdkp): give warning
      macro_map_.erase(res);
    }
    macro_map_.insert(std::make_pair(name, macro));
  }

  void RemoveMacro(const std::string& name) {
    auto res = macro_map_.find(name);
    if (res == macro_map_.end())
      return;
    if(res->second.pre_def()) // cannot undef predefined macro
      return;
    macro_map_.erase(res);
  }

  std::string* SearchFile(const std::string& name,
                          const bool lib_header,
                          bool next,
                          const std::string& cur_path);

  void AddSearchPath(std::string path);
  void HandleTheFileMacro(TokenSequence& os, const Token* macro);
  void HandleTheLineMacro(TokenSequence& os, const Token* macro);
  void UpdateFirstTokenLine(TokenSequence ts);

  bool NeedExpand() const {
    if (pp_cond_stack_.empty())
      return true;
    auto top = pp_cond_stack_.top();
    return top.enabled_ && top.cond_;
  }
  
private:
  void Init();

  PPCondStack pp_cond_stack_;
  unsigned cur_line_;
  unsigned line_line_;
  bool cur_cond_;
  
  MacroMap macro_map_;
  PathList path_list_;  
};

#endif
