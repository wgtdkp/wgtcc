#ifndef _WGTCC_TOKEN_H_
#define _WGTCC_TOKEN_H_

#include "error.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <list>
#include <unordered_map>


class Generator;
class Parser;
class Token;
class TokenSeq;

typedef std::list<Token> TokenList;


void PrintTokSeq(TokenSeq& tokSeq);
void PrintTokList(TokenList& tokList);

enum class Encoding {
    NONE,
    CHAR16,
    CHAR32,
    UTF8,
    WCHAR
};

struct Token
{
    friend class Parser;
    
public:
    explicit Token(int tag=Token::END): _tag(tag), _ws(false) {}

    //Token(int tag, const char* fileName, int line,
    //        int column, char* lineBegin, StrPair& str)
    //        : _tag(tag), _fileName(fileName), _line(line),
    //          _column(column), _lineBegin(lineBegin), _str(str) {}
    
    Token(const Token& other) {
        *this = other;
    }

    const Token& operator=(const Token& other) {
        _tag = other._tag;
        _fileName = other._fileName;
        _line = other._line;
        _ws = other._ws;
        //_column = other._column;
        _lineBegin = other._lineBegin;
        _begin = other._begin;
        _end = other._end;

        return *this;
    }

    virtual ~Token(void) {}
    
    //Token::NOTOK represents not a kw.
    static int KeyWordTag(const char* begin, const char* end) {
        std::string key(begin, end);
        auto kwIter = _kwTypeMap.find(key);
        
        if (_kwTypeMap.end() == kwIter)
            return Token::NOTOK;	//not a key word type
        
        return kwIter->second;
    }

    enum {
        /* punctuators */
        LPAR = '(',
        RPAR = ')',
        LSQB = '[',
        RSQB = ']',
        COLON = ':',
        COMMA = ',',
        SEMI = ';',
        ADD = '+',
        SUB = '-',
        MUL = '*',
        DIV = '/',
        OR = '|',
        AND = '&',
        XOR = '^',
        LESS = '<',
        GREATER = '>',
        EQUAL = '=',
        DOT = '.',
        MOD = '%',
        LBRACE = '{',
        RBRACE = '}',
        TILDE = '~',
        NOT = '!',
        COND = '?',
        SHARP = '#',

        DSHARP = 128, //'##'
        PTR_OP,
        INC_OP,
        DEC_OP,
        LEFT_OP,
        RIGHT_OP,
        LE_OP,
        GE_OP,
        EQ_OP,
        NE_OP,
        AND_OP,
        OR_OP,

        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        LEFT_ASSIGN,
        RIGHT_ASSIGN,
        AND_ASSIGN,
        XOR_ASSIGN,
        OR_ASSIGN,

        ELLIPSIS,
        /* punctuators end */

        /* key words */
        //type qualifier
        CONST,
        RESTRICT,
        VOLATILE,
        ATOMIC, //_Atomic

        //type specifier
        VOID,
        CHAR,
        SHORT,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        SIGNED,
        UNSIGNED,
        BOOL,		//_Bool
        COMPLEX,	//_Complex
        STRUCT,
        UNION,
        ENUM,

        TYPEDEF_NAME, //not used

        //function specifier
        INLINE,
        NORETURN,	//_Noreturn

        //alignment specifier
        ALIGNAS, //_Alignas

        //storage class specifier
        TYPEDEF,
        EXTERN,
        STATIC,
        THREAD,	//_Thread_local
        AUTO,
        REGISTER,

        BREAK,
        CASE,
        CONTINUE,
        DEFAULT,
        DO,
        ELSE,
        FOR,
        GOTO,
        IF,		
        RETURN,
        SIZEOF,
        SWITCH,		
        WHILE,
        ALIGNOF, //_Alignof
        GENERIC, //_Generic
        IMAGINARY, //_Imaginary
        STATIC_ASSERT, //_Static_assert
        /* key words end */
    
        IDENTIFIER,
        CONSTANT,
        I_CONSTANT,
        F_CONSTANT,
        STRING_LITERAL,

        //for the parser, a identifier is a typedef name or user defined type
        POSTFIX_INC,
        POSTFIX_DEC,
        PREFIX_INC,
        PREFIX_DEC,
        ADDR,  // '&'
        DEREF, // '*'
        PLUS,
        MINUS,
        CAST,

        // For preprocessor
        PP_IF,
        PP_IFDEF,
        PP_IFNDEF,
        PP_ELIF,
        PP_ELSE,
        PP_ENDIF,
        PP_INCLUDE,
        PP_DEFINE,
        PP_UNDEF,
        PP_LINE,
        PP_ERROR,
        PP_PRAGMA,
        PP_EMPTY,

        IGNORE,
        INVALID,
        END,
        NOTOK = -1,
    };


    int Tag(void) const {
        return _tag;
    }

    size_t Size(void) const {
        return _end - _begin;
    }
    
    std::string Str(void) const {
    	return std::string(_begin, _end);
    }

    std::string Literal(void) const {
        assert(_tag == STRING_LITERAL);
        const char* p = _begin;
        const char* q = _end - 1;
        while (*p != '"')
            ++p;
        while (*q != '"')
            --q;
        return std::string(p + 1, q);
    }

    static bool IsKeyWord(int tag) {
        return CONST <= tag && tag <= STATIC_ASSERT;
    }

    bool IsKeyWord(void) const {
        return IsKeyWord(_tag);
    }

    bool IsPunctuator(void) const {
        return 0 <= _tag && _tag <= ELLIPSIS;
    }

    bool IsLiteral(void) const {
        return _tag == STRING_LITERAL;
    }

    bool IsConstant(void) const {
        return CONSTANT <= _tag && _tag <= F_CONSTANT;
    }

    bool IsIdentifier(void) const {
        return IDENTIFIER == _tag;
    }

    bool IsEOF(void) const {
        return _tag == Token::END;
    }

    bool IsTypeSpecQual(void) const { 
        return CONST <= _tag && _tag <= ENUM;
    }
    
    bool IsDecl(void) const {
        return CONST <= _tag && _tag <= REGISTER;
    }
    
    static const char* Lexeme(int tag) {
        auto iter = _TagLexemeMap.find(tag);
        if (iter == _TagLexemeMap.end())
            return nullptr;
            
        return iter->second;
    }

    int Column(void) const {
        return _begin - _lineBegin + 1;
    }
    
    int _tag;
    
    const std::string* _fileName {nullptr};

    // Line index of the begin
    unsigned _line {1};
    
    /*
        * _ws standards for weither there is preceding white space
        * This is to simplify the '#' operator(stringize) in macro expansion
        */
    bool _ws {false};
        

    char* _lineBegin {nullptr};

    char* _begin {nullptr};

    char* _end {nullptr};
    
    //bool _needFree;

    static const std::unordered_map<std::string, int> _kwTypeMap;
    static const std::unordered_map<int, const char*> _TagLexemeMap;
    //static const char* _tokenTable[TOKEN_NUM - OFFSET - 1];
};


struct TokenSeq
{
public:
    TokenSeq(void): _tokList(new TokenList()),
            _begin(_tokList->begin()), _end(_tokList->end()) {}

    explicit TokenSeq(const Token&& tok) {
        TokenSeq();
        InsertBack(&tok);
    }

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

    Token* Expect(int expect);

    bool Try(int tag) {
        if (Next()->Tag() == tag)
            return true;
        PutBack();
        return false;
    }

    bool Test(int tag) {
        return Peek()->Tag() == tag;
    }

    Token* Next(void) {
        auto ret = Peek();
        if (!ret->IsEOF())
            ++_begin;
        return ret;
    }

    void PutBack(void) {
        //assert(_begin != _tokList->begin());
        if (_begin == _tokList->begin()) {
            PrintTokList(*_tokList);
        }
        --_begin;
    }

    Token* Peek(void);

    Token* Peek2(void) {
        if (Empty())
            return Peek(); // Return the Token::END
        Next();
        auto ret = Peek();
        PutBack();
        return ret;
    }

    Token* Back(void) {
        auto back = _end;
        return &(*--back);
    }

    // Could be costly
    /*
    size_t Size(void) {
        size_t size = 0;
        for (auto iter = _begin; iter != _end; iter++)
            ++size;
        return size;
    }
    */

    bool Empty(void) {
        return _begin == _end;
    }

    void InsertBack(const TokenSeq& ts) {
        //assert(_tokList == seq._tokList);
        auto pos = _tokList->insert(_end, ts._begin, ts._end);
        if (_begin == _end) {
            _begin = pos;
        }
    }

    void InsertBack(const Token* tok) {
        auto pos = _tokList->insert(_end, *tok);
        if (_begin == _end) {
            _begin = pos;
        }
    }

    void InsertFront(const TokenSeq& ts) {
        _begin = _tokList->insert(_begin, ts._begin, ts._end);
    }

    void InsertFront(const Token* tok) {
        //assert(_tokList == seq._tokList);
        _begin = _tokList->insert(_begin, *tok);
    }

    bool IsBeginOfLine(void) const;

    TokenList* _tokList;
    TokenList::iterator _begin;
    TokenList::iterator _end;
};

#endif

Encoding StringEncoding(const Token* tok);

