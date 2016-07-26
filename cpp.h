#ifndef _WGTCC_CPP_H_
#define _WGTCC_CPP_H_

#include "string_pair.h"
#include "token.h"
#include "lexer.h"

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>

class Lexer;
class Macro;
struct CondDirective;

typedef std::map<std::string, Macro> MacroMap; 
typedef std::list<std::string> ParamList;
typedef std::map<std::string, TokenSeq> ParamMap;
typedef std::set<std::string> HideSet;
typedef std::stack<CondDirective> PPCondStack;
typedef std::list<std::string> PathList;


class Macro
{
public:
    Macro(TokenSeq& repSeq, bool preDef=false)
            : _funcLike(false), _variadic(false),
              _preDef(preDef),_repSeq(repSeq) {}

    Macro(bool variadic, ParamList& params,
            TokenSeq& repSeq, bool preDef=false)
            : _funcLike(true), _preDef(preDef), _params(params),
              _repSeq(repSeq) {}
    
    ~Macro(void) {}

    /*
    std::string Name(void) {
        return _tok->Str();
    }
    */

    bool FuncLike(void) {
        return _funcLike;
    }

    bool ObjLike(void) {
        return !FuncLike();
    }

    bool Variadic(void) {
        return _variadic;
    }

    bool PreDef(void) {
        return _preDef;
    }

    ParamList& Params(void) {
        return _params;
    }

    TokenSeq& RepSeq(void) {
        return _repSeq;
    }

private:
    //Token* _tok;
    bool _funcLike;
    bool _variadic;
    bool _preDef;
    ParamList _params;
    TokenSeq _repSeq;

};

struct CondDirective
{
    int _tag;
    bool _enabled;
    bool _cond;
};

class Preprocessor
{
public:
    Preprocessor(const std::string* fileName)
            : _curFileName(fileName), _curLine(1), _curCond(true) {
        Init();
    }

    ~Preprocessor(void) {}

    void Process(TokenSeq& os);
    void Expand(TokenSeq& os, TokenSeq& is);
    void Subst(TokenSeq& os, TokenSeq& is, HideSet& hs, ParamMap& params);
    void Glue(TokenSeq& os, TokenSeq& is);
    void Glue(TokenSeq& os, Token* tok);
    void Stringize(char*& begin, char*& end, TokenSeq is);
    void Stringize(std::string& str, TokenSeq is);
    void ParseActualParam(TokenSeq& is, Macro* macro, ParamMap& paramMap);
    int GetDirective(TokenSeq& is);
    void ReplaceDefOp(TokenSeq& is);
    void ReplaceIdent(TokenSeq& is);
    void ParseDirective(TokenSeq& os, TokenSeq& is, int directive);
    
    TokenSeq GetLine(TokenSeq& is);
    void ParseIf(TokenSeq ls);
    void ParseIfdef(TokenSeq ls);
    void ParseIfndef(TokenSeq ls);
    void ParseElif(TokenSeq ls);
    void ParseElse(TokenSeq ls);
    void ParseEndif(TokenSeq ls);
    void ParseInclude(TokenSeq& is, TokenSeq ls);
    void ParseDef(TokenSeq ls);
    void ParseUndef(TokenSeq ls);
    void ParseLine(TokenSeq ls);
    void ParseError(TokenSeq ls);
    void ParsePragma(TokenSeq ls);
    void IncludeFile(TokenSeq& is, const std::string* fileName);
    bool ParseIdentList(ParamList& params, TokenSeq& is);
    

    Macro* FindMacro(const std::string& name) {
        auto res = _macroMap.find(name);
        if (res == _macroMap.end())
            return nullptr;
        return &res->second;
    }

    void AddMacro(const std::string& name,
            std::string* text, bool preDef=false);

    void AddMacro(const std::string& name, const Macro& macro) {
        auto res = _macroMap.find(name);
        if (res != _macroMap.end()) {
            // TODO(wgtdkp): give warning
            _macroMap.erase(res);
        }
        _macroMap.insert(std::make_pair(name, macro));
    }

    void RemoveMacro(const std::string& name) {
        auto res = _macroMap.find(name);
        if (res == _macroMap.end())
            return;
        if(res->second.PreDef()) // can't undef predefined macro
            return;
        _macroMap.erase(res);
    }

    std::string* SearchFile(const std::string& name, bool libHeader=false);

    void AddSearchPath(const std::string& path) {
        _searchPathList.push_back(path);
    }

    bool Hidden(const std::string& name) {
        return _hs.find(name) != _hs.end();
    }

    bool NeedExpand(void) const {
        if (_ppCondStack.empty())
            return true;
        auto top = _ppCondStack.top();
        return top._enabled && top._cond;
    }

private:
    void Init(void);

    HideSet _hs;
    PPCondStack _ppCondStack;
    const std::string* _curFileName;
    int _curLine;
    int _lineLine;
    bool _curCond;
    
    
    
    MacroMap _macroMap;
    PathList _searchPathList;
};

#endif
