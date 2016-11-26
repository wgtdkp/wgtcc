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
class Scanner;
class Token;
class TokenSequence;

using HideSet   = std::set<std::string>;
using TokenList = std::list<const Token*>;


struct SourceLocation {
  const std::string* filename;
  const char* line_begin;
  unsigned line;
  unsigned column;

  const char* Begin() const { return line_begin + column - 1; }
};


class Token {
  friend class Scanner;
public:
  enum {
    // Punctuators
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
    // Punctuators end

    // KEYWORD BEGIN
    // TYPE QUALIFIER BEGIN
    CONST,
    RESTRICT,
    VOLATILE,
    ATOMIC,
    // TYPE QUALIFIER END

    // TYPE SPECIFIER BEGIN
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
    // TYPE SPECIFIER END

    ATTRIBUTE, // GNU extension __attribute__
    // FUNCTION SPECIFIER BEGIN
    INLINE,
    NORETURN,	//_Noreturn
    // FUNCTION SPECIFIER END

    ALIGNAS, //_Alignas
    // For syntactic convenience
    STATIC_ASSERT, //_Static_assert
    // STORAGE CLASS SPECIFIER BEGIN
    TYPEDEF,
    EXTERN,
    STATIC,
    THREAD,	//_Thread_local
    AUTO,
    REGISTER,
    // STORAGE CLASS SPECIFIER END
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
    // KEYWORD END
     
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
    PP_NONE,
    PP_EMPTY,
    

    IGNORE,
    INVALID,
    END,
    NOTOK = -1,
  };

  static Token* New(int tag);
  static Token* New(const Token& other);
  static Token* New(int tag,
                    const SourceLocation& loc,
                    const std::string& str,
                    bool ws=false);

  Token& operator=(const Token& other) {
    tag_  = other.tag_;
    ws_   = other.ws_;
    loc_  = other.loc_;
    str_  = other.str_;
    hs_   = other.hs_;
    return *this;
  }

  virtual ~Token() {}
  
  //Token::NOTOK represents not a kw.
  static int KeyWordTag(const std::string& key) {
    auto iter = kw_type_map_.find(key);
    
    if (kw_type_map_.end() == iter)
      return Token::NOTOK;	//not a key word type
    
    return iter->second;
  }

  static bool IsKeyWord(const std::string& name);

  static bool IsKeyWord(int tag) {
    return CONST <= tag && tag < IDENTIFIER;
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
    auto iter = tag_lexeme_map_.find(tag);
    if (iter == tag_lexeme_map_.end())
      return nullptr;
      
    return iter->second;
  }

  int tag() const { return tag_; }
  void set_tag(int tag) { tag_ = tag; }
  bool ws() const { return ws_; }
  void set_ws(bool ws) { ws_ = ws; }
  SourceLocation& loc() { return loc_; }
  const SourceLocation& loc() const { return loc_; }
  void set_loc(const SourceLocation& loc) { loc_ = loc; }
  std::string& str() { return str_; }
  const std::string& str() const { return str_; }
  void set_str(const std::string& str) { str_ = str; }
  HideSet* hs() { return hs_; }
  const HideSet* hs() const { return hs_; }
  void set_hs(HideSet* hs) {hs_ = hs; }

private:
  explicit Token(int tag): tag_(tag) {}
  Token(int tag,
        const SourceLocation& loc,
        const std::string& str,
        bool ws=false)
      : tag_(tag), ws_(ws), loc_(loc), str_(str) {}

  Token(const Token& other) {
    *this = other;
  }

  static const std::unordered_map<std::string, int> kw_type_map_;
  static const std::unordered_map<int, const char*> tag_lexeme_map_;

  int tag_;
  bool ws_ { false };

  // Line index of the begin
  //unsigned line_ { 1 };
  //unsigned column_ { 1 };		 
  //const std::string* filename_ { nullptr };
  //const char* line_begin_ { nullptr };
  SourceLocation loc_;
  
  // ws_ standards for weither there is preceding white space
  // This is to simplify the '#' operator(stringize) in macro expansion
  std::string str_;
  HideSet* hs_ { nullptr };
};


struct TokenSequence {
  friend class Preprocessor;

public:
  TokenSequence(): tok_list_(new TokenList()),
      begin_(tok_list_->begin()), end_(tok_list_->end()) {}

  explicit TokenSequence(Token* tok) {
    TokenSequence();
    InsertBack(tok);
  }

  explicit TokenSequence(TokenList* tok_list)
      : tok_list_(tok_list),
        begin_(tok_list->begin()),
        end_(tok_list->end()) {}

  TokenSequence(TokenList* tok_list,
                TokenList::iterator begin,
                TokenList::iterator end)
      : tok_list_(tok_list), begin_(begin), end_(end) {}
  
  ~TokenSequence() {}

  TokenSequence(const TokenSequence& other) {
    *this = other;
  }

  const TokenSequence& operator=(const TokenSequence& other) {
    tok_list_ = other.tok_list_;
    begin_ = other.begin_;
    end_ = other.end_;
    return *this;
  }

  void Copy(const TokenSequence& other) {
    tok_list_ = new TokenList(other.begin_, other.end_);
    begin_ = tok_list_->begin();
    end_ = tok_list_->end();
    for (auto iter = begin_; iter != end_; ++iter)
      *iter = Token::New(**iter);
  }

  void UpdateHeadLocation(const SourceLocation& loc) {
    assert(!Empty());
    auto tok = const_cast<Token*>(Peek());
    tok->loc() = loc;
  }

  void FinalizeSubst(bool leading_ws, const HideSet& hs) {
    auto ts = *this;
    while (!ts.Empty()) {
      auto tok = const_cast<Token*>(ts.Next());
      if (!tok->hs())
        tok->set_hs(new HideSet(hs));
      else
        tok->hs()->insert(hs.begin(), hs.end());
    }
    // Even if the token sequence is empty
    const_cast<Token*>(Peek())->set_ws(leading_ws);
  }

  const Token* Expect(int expect);

  bool Try(int tag) {
    if (Peek()->tag() == tag) {
      Next();
      return true;
    }
    return false;
  }

  bool Test(int tag) {
    return Peek()->tag() == tag;
  }

  const Token* Next() {
    auto ret = Peek();
    if (!ret->IsEOF()) {
      ++begin_;
      Peek(); // May skip newline token, but why ?
    }
    return ret;
  }

  void PutBack() {
    assert(begin_ != tok_list_->begin());
    --begin_;
    if ((*begin_)->tag() == Token::NEW_LINE)
      PutBack();
  }

  const Token* Peek();

  const Token* Peek2() {
    if (Empty())
      return Peek(); // Return the Token::END
    Next();
    auto ret = Peek();
    PutBack();
    return ret;
  }

  const Token* Back() {
    auto back = end_;
    return *--back;
  }

  void PopBack() {
    assert(!Empty());
    assert(end_ == tok_list_->end());
    auto size_eq1 = tok_list_->back() == *begin_;
    tok_list_->pop_back();
    end_ = tok_list_->end();
    if (size_eq1)
      begin_ = end_;
  }

  TokenList::iterator Mark() {
    return begin_;
  }

  void ResetTo(TokenList::iterator mark) {
    begin_ = mark;
  }

  bool Empty();

  void InsertBack(TokenSequence& ts) {
    auto pos = tok_list_->insert(end_, ts.begin_, ts.end_);
    if (begin_ == end_) {
      begin_ = pos;
    }
  }

  void InsertBack(const Token* tok) {
    auto pos = tok_list_->insert(end_, tok);
    if (begin_ == end_) {
      begin_ = pos;
    }
  }

  // If there is preceding newline
  void InsertFront(TokenSequence& ts) {
    auto pos = GetInsertFrontPos();
    begin_ = tok_list_->insert(pos, ts.begin_, ts.end_);
  }

  void InsertFront(const Token* tok) {
    auto pos = GetInsertFrontPos();
    begin_ = tok_list_->insert(pos, tok);
  }

  bool IsBeginOfLine() const;
  TokenSequence GetLine();


  void SetParser(Parser* parser) {
    parser_ = parser;
  }

  void Print(FILE* fp=stdout) const;

private:
  // Find a insert position with no preceding newline
  TokenList::iterator GetInsertFrontPos() {
    auto pos = begin_;
    if (pos == tok_list_->begin())
      return pos;
    --pos;
    while (pos != tok_list_->begin() && (*pos)->tag() == Token::NEW_LINE)
      --pos;
    return ++pos;
  }

  TokenList* tok_list_;
  TokenList::iterator begin_;
  TokenList::iterator end_;
  
  Parser* parser_ {nullptr};
};

#endif
