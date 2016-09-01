#ifndef _WGTCC_TOKEN_H_
#define _WGTCC_TOKEN_H_

#include "error.h"

#include <cassert>
#include <cstring>

#include <iostream>
#include <list>
#include <set>
#include <string>
#include <unordered_map>


class Generator;
class Parser;
class Token;
class TokenSequence;


typedef std::set<std::string> HideSet;
typedef std::list<Token> TokenList;


struct SourceLocation {
  const std::string* fileName_;
  const char* lineBegin_;
  unsigned line_;
  unsigned column_;

  const char* Begin() const {
    return lineBegin_ + column_ - 1; 
  }
};


struct Token
{
  friend class Parser;
  
public:
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
    NEW_LINE = '\n',

    DSHARP = 128, //'##'
    PTR,
    INC,
    DEC,
    LEFT,
    RIGHT,
    LE,
    GE,
    EQ,
    NE,
    LOGICAL_AND,
    LOGICAL_OR,

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
    C_CONSTANT,
    F_CONSTANT,
    LITERAL,

    //For the parser, a identifier is a typedef name or user defined type
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

  explicit Token(int tag=Token::END): tag_(tag) {}
  Token(int tag,
        const SourceLocation& loc,
        const std::string& str,
        bool ws=false)
      : tag_(tag), ws_(ws), loc_(loc), str_(str) {}
  //Token(int tag, const char* fileName, int line,
  //        int column, char* lineBegin, StrPair& str)
  //        : tag_(tag), fileName_(fileName), line_(line),
  //          column_(column), lineBegin_(lineBegin), str_(str) {}
  
  Token(const Token& other) {
    *this = other;
  }

  Token& operator=(const Token& other) {
    tag_ = other.tag_;
    //fileName_ = other.fileName_;
    //line_ = other.line_;
    ws_ = other.ws_;
    //column_ = other.column_;
    //lineBegin_ = other.lineBegin_;
    loc_ = other.loc_;
    //begin_ = other.begin_;
    //end_ = other.end_;
    str_ = other.str_;

    hs_ = other.hs_;
    
    return *this;
  }

  virtual ~Token() {}
  
  //Token::NOTOK represents not a kw.
  static int KeyWordTag(const std::string& key) {
    auto kwIter = kwTypeMap_.find(key);
    
    if (kwTypeMap_.end() == kwIter)
      return Token::NOTOK;	//not a key word type
    
    return kwIter->second;
  }

  //int Tag() const {
  //	return tag_;
  //}

  //size_t Size() const {
  //	return end_ - begin_;
  //}
  
  //std::string Str() const {
  //	return std::string(begin_, end_);
  //}

  static bool IsKeyWord(const std::string& name);

  static bool IsKeyWord(int tag) {
    return CONST <= tag && tag <= STATIC_ASSERT;
  }

  bool IsKeyWord() const {
    return IsKeyWord(tag_);
  }

  bool IsPunctuator() const {
    return 0 <= tag_ && tag_ <= ELLIPSIS;
  }

  bool IsLiteral() const {
    return tag_ == LITERAL;
  }

  bool IsConstant() const {
    return CONSTANT <= tag_ && tag_ <= F_CONSTANT;
  }

  bool IsIdentifier() const {
    return IDENTIFIER == tag_;
  }

  bool IsEOF() const {
    return tag_ == Token::END;
  }

  bool IsTypeSpecQual() const { 
    return CONST <= tag_ && tag_ <= ENUM;
  }
  
  bool IsDecl() const {
    return CONST <= tag_ && tag_ <= REGISTER;
  }

  static const char* Lexeme(int tag) {
    auto iter = TagLexemeMap_.find(tag);
    if (iter == TagLexemeMap_.end())
      return nullptr;
      
    return iter->second;
  }
  
  int tag_;
  bool ws_ { false };
    
  

  // Line index of the begin
  //unsigned line_ { 1 };
  //unsigned column_ { 1 };		 
  //const std::string* fileName_ { nullptr };
  //const char* lineBegin_ { nullptr };
  SourceLocation loc_;
  /*
  * ws_ standards for weither there is preceding white space
  * This is to simplify the '#' operator(stringize) in macro expansion
  */

  //char* begin_ { nullptr };

  //char* end_ { nullptr };
  std::string str_;

  
  HideSet* hs_ { nullptr };

  static const std::unordered_map<std::string, int> kwTypeMap_;
  static const std::unordered_map<int, const char*> TagLexemeMap_;
  //static const char* _tokenTable[TOKEN_NUM - OFFSET - 1];
};


struct TokenSequence
{
public:
  TokenSequence(): tokList_(new TokenList()),
      begin_(tokList_->begin()), end_(tokList_->end()) {}

  explicit TokenSequence(const Token&& tok) {
    TokenSequence();
    InsertBack(&tok);
  }

  explicit TokenSequence(TokenList* tokList): tokList_(tokList),
      begin_(tokList->begin()), end_(tokList->end()) {}

  TokenSequence(TokenList* tokList,
      TokenList::iterator begin, TokenList::iterator end)
      : tokList_(tokList), begin_(begin), end_(end) {}
  
  ~TokenSequence() {}

  TokenSequence(const TokenSequence& other) {
    *this = other;
  }

  const TokenSequence& operator=(const TokenSequence& other) {
    tokList_ = other.tokList_;
    begin_ = other.begin_;
    end_ = other.end_;

    return *this;
  }

  void Copy(const TokenSequence& other) {
    tokList_ = new TokenList(other.begin_, other.end_);
    begin_ = tokList_->begin();
    end_ = tokList_->end();
  }

  Token* Expect(int expect);

  bool Try(int tag) {
    if (Peek()->tag_ == tag) {
      Next();
      return true;
    }
    return false;
  }

  bool Test(int tag) {
    return Peek()->tag_ == tag;
  }

  Token* Next() {
    auto ret = Peek();
    if (!ret->IsEOF())
      ++begin_;
    return ret;
  }

  void PutBack() {
    assert(begin_ != tokList_->begin());
    //if (begin_ == tokList_->begin()) {
    //  PrintTokList(*tokList_);
    //}
    --begin_;
  }

  Token* Peek();

  Token* Peek2() {
    if (Empty())
      return Peek(); // Return the Token::END
    Next();
    auto ret = Peek();
    PutBack();
    return ret;
  }

  Token* Back() {
    auto back = end_;
    return &(*--back);
  }

  TokenList::iterator Mark() {
    return begin_;
  }

  void ResetTo(TokenList::iterator mark) {
    begin_ = mark;
  }

  bool Empty();

  void InsertBack(const TokenSequence& ts) {
    //assert(tokList_ == seq.tokList_);
    auto pos = tokList_->insert(end_, ts.begin_, ts.end_);
    if (begin_ == end_) {
      begin_ = pos;
    }
  }

  void InsertBack(const Token* tok) {
    auto pos = tokList_->insert(end_, *tok);
    if (begin_ == end_) {
      begin_ = pos;
    }
  }

  void InsertFront(const TokenSequence& ts) {
    begin_ = tokList_->insert(begin_, ts.begin_, ts.end_);
  }

  void InsertFront(const Token* tok) {
    //assert(tokList_ == seq.tokList_);
    begin_ = tokList_->insert(begin_, *tok);
  }

  bool IsBeginOfLine() const;
  TokenSequence GetLine();


  void SetParser(Parser* parser) {
    parser_ = parser;
  }

  void Print() const;

  TokenList* tokList_;
  TokenList::iterator begin_;
  TokenList::iterator end_;
  
private:

  Parser* parser_ {nullptr};
};

#endif
