#ifndef _WGTCC_CPP_H_
#define _WGTCC_CPP_H_

#include "string_pair.h"
#include "token.h"
#include "lexer.h"

#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>

class Lexer;
class Macro;

typedef std::map<std::string, Macro> MacroMap; 
typedef std::list<std::string> ParamList;
typedef std::map<std::string, TokenSeq> ParamMap;
typedef std::set<std::string> HideSet;
typedef std::stack<std::pair<int, int>> PPCondStack;
typedef std::list<std::string> PathList;



class Macro
{
public:
    Macro(TokenSeq& repSeq, bool preDef=false)
            : _funcLike(false), _preDef(preDef),_repSeq(repSeq) {}

    Macro(ParamList& params, TokenSeq& repSeq, bool preDef=false)
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

    ParamList& Params(void) {
        return _params;
    }

    TokenSeq& RepSeq(void) {
        return _repSeq;
    }

private:
    //Token* _tok;
    bool _funcLike;
    bool _preDef;
    ParamList _params;
    TokenSeq _repSeq;

};


class Preprocessor
{
public:
    Preprocessor(void) {
        Init();
    }

    ~Preprocessor(void) {}

    void GenTokList(void);

    void Process(TokenSeq& os, TokenSeq& is);

    void Expand(TokenSeq& os, TokenSeq& is);

    void Subst(TokenSeq& os, TokenSeq& is, HideSet& hs, ParamMap& params);

    void Glue(TokenSeq& os, TokenSeq& is);

    void Glue(TokenSeq& os, Token& tok);

    Token Stringize(TokenSeq* ts);

    void ParseActualParam(TokenSeq& is, Macro* macro, ParamMap& paramMap);

    int GetDirective(TokenSeq& is);

    TokenSeq GetLine(TokenSeq& is);

    void ParseDirective(TokenSeq& is, int directive);

    void ParseIf(TokenSeq& is);
    void ParseIfdef(TokenSeq& is);
    void ParseIfndef(TokenSeq& is);
    void ParseElif(TokenSeq& is);
    void ParseElse(TokenSeq& is);
    void ParseEndif(TokenSeq& is);
    void ParseInclude(TokenSeq& is);

    void IncludeFile(TokenSeq& is, const std::string& fileName);


    TokenList* TokList(void) {
        return _tokList;
    }

    Macro* FindMacro(const std::string& name) {
        auto res = _macroMap.find(name);
        if (res == _macroMap.end())
            return nullptr;
        return &res->second;
    }

    void AddMacro(const std::string& name, const Macro& macro) {
        _macroMap.insert(std::make_pair(name, macro));
    }

    std::string SearchFile(const std::string& name, bool libHeader=false);

    void AddSearchPath(const std::string& path) {
        _searchPathList.push_back(path);
    }

    bool Hidden(const std::string& name) {
        return _hs.find(name) != _hs.end();
    }

private:
    void Init(void);

    TokenList* _tokList;
    HideSet _hs;
    PPCondStack _ppCondStack;
    
    MacroMap _macroMap;
    PathList _searchPathList;
};

#endif
