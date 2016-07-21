#ifndef _WGTCC_CPP_H_
#define _WGTCC_CPP_H_

#include "string_pair.h"
#include "token.h"
#include "lexer.h"

#include <list>
#include <map>
#include <set>
#include <string>

class Lexer;
class Macro;
class TokenSeq;

typedef std::map<std::string, Macro> MacroMap; 
typedef std::list<std::string> ParamList;
typedef std::map<std::string, TokenSeq> ParamMap;
typedef std::set<std::string> HideSet;


struct TokenSeq
{
public:
    explicit TokenSeq(TokenList* tokList): _tokList(tokList),
            _begin(tokList->begin()), _end(tokList->end()) {}

    TokenSeq(TokenList* tokList,
            TokenList::iterator begin, TokenList::iterator end)
            : _tokList(tokList), _begin(begin), _end(end) {}
    
    ~TokenSeq(void) {}

    TokenSeq(const TokenSeq& other) {
        *this = other;
    }

    const TokenSeq& operator=(const TokenSeq& other) {
        _tokList = other._tokList;
        _begin = other._begin;
        _end = other._end;

        return *this;
    }

    Token& Front(void) {
        return *_begin;
    }

    Token& Second(void) {
        auto pos = _begin;
        return *(++pos);
    }

    Token& Back(void) {
        auto pos = _end;
        return *(--pos);
    }

    void Forward(void) {
        ++_begin;
    }

    bool Empty(void) {
        return _begin == _end;
    }

    void InsertBack(const TokenSeq& seq) {
        //assert(_tokList == seq._tokList);
        _tokList->insert(_end, seq._begin, seq._end);
    }

    void InsertBack(const Token& tok) {
        //assert(_tokList == seq._tokList);
        _tokList->insert(_end, tok);
    }

    void InsertFront(const TokenSeq& seq) {
        _tokList->insert(_begin, seq._begin, seq._end);
        _begin = seq._begin;
    }

    void InsertFront(const Token& tok) {
        //assert(_tokList == seq._tokList);
        _tokList->insert(_begin, tok);
        --_begin;
    }

    TokenList* _tokList;
    TokenList::iterator _begin;
    TokenList::iterator _end;
};


class Macro
{
public:
    Macro(Token* tok, TokenSeq& repSeq)
            : _tok(tok), _funcLike(false), _repSeq(repSeq) {}

    Macro(Token* tok, ParamList& params, TokenSeq& repSeq)
            : _tok(tok), _funcLike(true), _params(params), _repSeq(repSeq) {}
    
    ~Macro(void) {}

    std::string Name(void) {
        return _tok->Str();
    }

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
    Token* _tok;
    bool _funcLike;
    ParamList _params;
    TokenSeq _repSeq;

};


class Preprocessor
{
public:
    Preprocessor(void) {}

    ~Preprocessor(void) {}

    void GenTokList(void);

    void Process(TokenList& ol, TokenList& il);

    void Expand(TokenSeq& os, TokenSeq& is);

    void Subst(TokenSeq& os, TokenSeq& is, HideSet& hs, ParamMap& params);

    void Glue(TokenSeq& os, TokenSeq& is);

    void Glue(TokenSeq& os, Token& tok);

    Token Stringize(TokenSeq* ts);

    void ParseActualParam(TokenSeq& is, Macro* macro, ParamMap& paramMap);

    TokenList* TokList(void) {
        return _tokList;
    }

    Macro* FindMacro(const std::string& name) {
        auto res = _macros.find(name);
        if (res == _macros.end())
            return nullptr;
        return &res->second;
    }

    bool Hidden(const std::string& name) {
        return _hs.find(name) != _hs.end();
    }

private:
    TokenList* _tokList;
    MacroMap _macros;
    HideSet _hs;
};

#endif
