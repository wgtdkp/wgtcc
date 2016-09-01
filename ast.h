#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "error.h"
#include "string_pair.h"
#include "token.h"
#include "type.h"

#include <cassert>
#include <list>
#include <memory>
#include <string>


class Visitor;
template<typename T> class Evaluator;
class AddrEvaluator;
class Generator;

class Scope;
class Parser;
class ASTNode;
class Token;
class TokenSequence;

//Expression
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class TempVar;
class Constant;

class Identifier;
class Object;
class Enumerator;

//statement
class Stmt;
class IfStmt;
class JumpStmt;
class LabelStmt;
class EmptyStmt;
class CompoundStmt;
class FuncDef;
class TranslationUnit;


/*
 * AST Node
 */

class ASTNode
{
public:
  virtual ~ASTNode(void) {}
  
  virtual void Accept(Visitor* v) = 0;

protected:
  ASTNode(void) {}

  MemPool* pool_ {nullptr};
};

typedef ASTNode ExtDecl;


/*********** Statement *************/

class Stmt : public ASTNode
{
public:
  virtual ~Stmt(void) {}

protected:
   Stmt(void) {}
};


struct Initializer
{
  int offset_;
  Type* type_;
  Expr* expr_;

  bool operator<(const Initializer& rhs) const {
    return offset_ < rhs.offset_;
  }
};

class Declaration: public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  
  typedef std::set<Initializer> InitList;

public:
  static Declaration* New(Object* obj);

  virtual ~Declaration(void) {}

  virtual void Accept(Visitor* v);

  InitList& Inits(void) {
    return inits_;
  }

  //StaticInitList StaticInits(void) {
  //    return _staticInits;
  //}

  Object* Obj(void) {
    return obj_;
  }

  void AddInit(Initializer init);

protected:
  Declaration(Object* obj): obj_(obj) {}

  Object* obj_;
  InitList inits_;
};


class EmptyStmt : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static EmptyStmt* New(void);

  virtual ~EmptyStmt(void) {}
  
  virtual void Accept(Visitor* v);

protected:
  EmptyStmt(void) {}
};


// 构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
public:
  static LabelStmt* New(void);

  ~LabelStmt(void) {}
  
  virtual void Accept(Visitor* v);
  
  std::string Label(void) const {
    return ".L" + std::to_string(tag_);
  }

protected:
  LabelStmt(void): tag_(GenTag()) {}

private:
  static int GenTag(void) {
    static int tag = 0;
    return ++tag;
  }
  
  int tag_; // 使用整型的tag值，而不直接用字符串
};


class IfStmt : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
public:
  static IfStmt* New(Expr* cond, Stmt* then, Stmt* els=nullptr);

  virtual ~IfStmt(void) {}
  
  virtual void Accept(Visitor* v);

protected:
  IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
      : cond_(cond), then_(then), else_(els) {}

private:
  Expr* cond_;
  Stmt* then_;
  Stmt* else_;
};


class JumpStmt : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static JumpStmt* New(LabelStmt* label);

  virtual ~JumpStmt(void) {}
  
  virtual void Accept(Visitor* v);
  
  void SetLabel(LabelStmt* label) {
    label_ = label;
  }

protected:
  JumpStmt(LabelStmt* label): label_(label) {}

private:
  LabelStmt* label_;
};


class ReturnStmt: public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static ReturnStmt* New(Expr* expr);

  virtual ~ReturnStmt(void) {}
  
  virtual void Accept(Visitor* v);

protected:
  ReturnStmt(::Expr* expr): expr_(expr) {}

private:
  ::Expr* expr_;
};


typedef std::list<Stmt*> StmtList;

class CompoundStmt : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static CompoundStmt* New(StmtList& stmts, ::Scope* scope=nullptr);

  virtual ~CompoundStmt(void) {}
  
  virtual void Accept(Visitor* v);

  StmtList& Stmts(void) {
    return stmts_;
  }

  ::Scope* Scope(void) {
    return scope_;
  }

protected:
  CompoundStmt(const StmtList& stmts, ::Scope* scope=nullptr)
      : stmts_(stmts), scope_(scope) {}

private:
  StmtList stmts_;
  ::Scope* scope_;
};


/********** Expr ************/
/*
 * Expr
 *      BinaryOp
 *      UnaryOp
 *      ConditionalOp
 *      FuncCall
 *      Constant
 *      Identifier
 *          Object
 *      TempVar
 */

class Expr : public Stmt
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  virtual ~Expr(void) {}
  
  ::Type* Type(void) {
    return type_;
  }

  virtual bool IsLVal(void) = 0;

  virtual void TypeChecking(void) = 0;

  const Token* Tok(void) const {
    return tok_;
  }

  void SetTok(const Token* tok) {
    tok_ = tok;
  }

  static Expr* MayCast(Expr* expr);
  static Expr* MayCast(Expr* expr, ::Type* desType);

protected:
  /*
   * You can construct a expression without specifying a type,
   * then the type should be evaluated in TypeChecking()
   */
  Expr(const Token* tok, ::Type* type): tok_(tok), type_(type) {}

  const Token* tok_;
  ::Type* type_;
};


/***********************************************************
'+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
'=',(复合赋值运算符被拆分为两个运算)
'==', '!=', '<=', '>=',
'&&', '||'
'['(下标运算符), '.'(成员运算符)
','(逗号运算符),
*************************************************************/
class BinaryOp : public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;
  friend class Declaration;

public:
  static BinaryOp* New(const Token* tok, Expr* lhs, Expr* rhs);

  static BinaryOp* New(const Token* tok, int op, Expr* lhs, Expr* rhs);

  virtual ~BinaryOp(void) {}
  
  virtual void Accept(Visitor* v);
  
  //like member ref operator is a lvalue
  virtual bool IsLVal(void) {
    switch (op_) {
    case '.':
    case ']': return !Type()->ToArrayType();
    default: return false;
    }
  }

  ArithmType* Promote(void);

  virtual void TypeChecking(void);
  void SubScriptingOpTypeChecking(void);
  void MemberRefOpTypeChecking(void);
  void MultiOpTypeChecking(void);
  void AdditiveOpTypeChecking(void);
  void ShiftOpTypeChecking(void);
  void RelationalOpTypeChecking(void);
  void EqualityOpTypeChecking(void);
  void BitwiseOpTypeChecking(void);
  void LogicalOpTypeChecking(void);
  void AssignOpTypeChecking(void);
  void CommaOpTypeChecking(void);
  
protected:
  BinaryOp(const Token* tok, int op, Expr* lhs, Expr* rhs)
      : Expr(tok, nullptr), op_(op) {
        lhs_ = lhs, rhs_ = rhs;
        if (op != '.') {
          lhs_ = MayCast(lhs);
          rhs_ = MayCast(rhs);
        }
      }

  int op_;
  Expr* lhs_;
  Expr* rhs_;
};


/*
 * Unary Operator:
 * '++' (prefix/postfix)
 * '--' (prefix/postfix)
 * '&'  (ADDR)
 * '*'  (DEREF)
 * '+'  (PLUS)
 * '-'  (MINUS)
 * '~'
 * '!'
 * CAST // like (int)3
 */
class UnaryOp : public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static UnaryOp* New(int op, Expr* operand, ::Type* type=nullptr);

  virtual ~UnaryOp(void) {}
  
  virtual void Accept(Visitor* v);

  //TODO: like '*p' is lvalue, but '~i' is not lvalue
  virtual bool IsLVal(void);

  ArithmType* Promote(void);

  void TypeChecking(void);
  void IncDecOpTypeChecking(void);
  void AddrOpTypeChecking(void);
  void DerefOpTypeChecking(void);
  void UnaryArithmOpTypeChecking(void);
  void CastOpTypeChecking(void);

protected:
  UnaryOp(int op, Expr* operand, ::Type* type = nullptr)
    : Expr(operand->Tok(), type), op_(op) {
      operand_ = operand;
      if (op_ != Token::CAST && op_ != Token::ADDR) {
        operand_ = MayCast(operand);
      }
    }

  int op_;
  Expr* operand_;
};


// cond ? true ： false
class ConditionalOp : public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static ConditionalOp* New(const Token* tok,
      Expr* cond, Expr* exprTrue, Expr* exprFalse);
  
  virtual ~ConditionalOp(void) {}
  
  virtual void Accept(Visitor* v);

  virtual bool IsLVal(void) {
    return false;
  }

  ArithmType* Promote(void);
  
  virtual void TypeChecking(void);

protected:
  ConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
      : Expr(cond->Tok(), nullptr), cond_(MayCast(cond)),
        exprTrue_(MayCast(exprTrue)), exprFalse_(MayCast(exprFalse)) {}

private:
  Expr* cond_;
  Expr* exprTrue_;
  Expr* exprFalse_;
};


/************** Function Call ****************/
class FuncCall : public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:        
  typedef std::vector<Expr*> ArgList;

public:
  static FuncCall* New(Expr* designator, const ArgList& args);

  ~FuncCall(void) {}
  
  virtual void Accept(Visitor* v);

  //a function call is ofcourse not lvalue
  virtual bool IsLVal(void) {
    return false;
  }

  ArgList* Args(void) {
    return &args_;
  }

  Expr* Designator(void) {
    return designator_;
  }

  const std::string& Name(void) const {
    return tok_->str_;
  }

  ::FuncType* FuncType(void) {
    return designator_->Type()->ToFuncType();
  }

  virtual void TypeChecking(void);

protected:
  FuncCall(Expr* designator, const ArgList& args)
    : Expr(designator->Tok(), nullptr),
      designator_(designator), args_(args) {}

  Expr* designator_;
  ArgList args_;
};


class Constant: public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static Constant* New(const Token* tok, int tag, long val);
  static Constant* New(const Token* tok, int tag, double val);
  static Constant* New(const Token* tok, int tag, const std::string* val);

  ~Constant(void) {}
  
  virtual void Accept(Visitor* v);

  virtual bool IsLVal(void) {
    return false;
  }

  virtual void TypeChecking(void) {}

  long IVal(void) const {
    return ival_;
  }

  double FVal(void) const {
    return fval_;
  }

  const std::string* SVal(void) const {
    return sval_;
  }

  std::string SValRepr(void) const;

  std::string Label(void) const {
    return std::string(".LC") + std::to_string(id_);
  }

protected:
  Constant(const Token* tok, ::Type* type, long val)
      : Expr(tok, type), ival_(val) {}
  Constant(const Token* tok, ::Type* type, double val)
      : Expr(tok, type), fval_(val) {}
  Constant(const Token* tok, ::Type* type, const std::string* val)
      : Expr(tok, type), sval_(val) {}

  union {
    long ival_;
    double fval_;
    struct {
      long id_;
      const std::string* sval_;
    };
  };
};


//临时变量
class TempVar : public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static TempVar* New(::Type* type);

  virtual ~TempVar(void) {}
  
  virtual void Accept(Visitor* v);
  
  virtual bool IsLVal(void) {
    return true;
  }

  virtual void TypeChecking(void) {}

protected:
  TempVar(::Type* type): Expr(nullptr, type), tag_(GenTag()) {}
  
private:
  static int GenTag(void) {
    static int tag = 0;
    return ++tag;
  }

  int tag_;
};


enum Linkage {
  L_NONE,
  L_EXTERNAL,
  L_INTERNAL,
};


class Identifier: public Expr
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static Identifier* New(const Token* tok, ::Type* type, Linkage linkage);

  virtual ~Identifier(void) {}

  virtual void Accept(Visitor* v);

  virtual bool IsLVal(void) {
    return false;
  }

  virtual Object* ToObject(void) {
    return nullptr;
  }

  virtual Enumerator* ToEnumerator(void) {
    return nullptr;
  }

  /*
   * An identifer can be:
   *     object, sturct/union/enum tag, typedef name, function, label.
   */
   Identifier* ToTypeName(void) {
    // A typename has no linkage
    // And a function has external or internal linkage
    if (ToObject() || ToEnumerator() || _linkage != L_NONE)
      return nullptr;
    return this;
  }


  virtual const std::string& Name(void) const {
    return tok_->str_;
  }

  /*
  ::Scope* Scope(void) {
    return scope_;
  }
  */

  enum Linkage Linkage(void) const {
    return _linkage;
  }

  void SetLinkage(enum Linkage linkage) {
    _linkage = linkage;
  }

  /*
  virtual bool operator==(const Identifier& other) const {
    return Name() == other.Name()
      && *type_ == *other.type_
  }
  */

  virtual void TypeChecking(void) {}

protected:
  Identifier(const Token* tok, ::Type* type, enum Linkage linkage)
      : Expr(tok, type), _linkage(linkage) {}
  
  /*
  // An identifier has property scope
  ::Scope* scope_;
  */
  // An identifier has property linkage
  enum Linkage _linkage;
};


class Enumerator: public Identifier
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static Enumerator* New(const Token* tok, int val);

  virtual ~Enumerator(void) {}

  virtual void Accept(Visitor* v);

  virtual Enumerator* ToEnumerator(void) {
    return this;
  }

  int Val(void) const {
    return _cons->IVal();
  }

protected:
  Enumerator(const Token* tok, int val)
      : Identifier(tok, ArithmType::New(T_INT), L_NONE),
        _cons(Constant::New(tok, T_INT, (long)val)) {}

  Constant* _cons;
};


class Object : public Identifier
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static Object* New(const Token* tok, ::Type* type,
      int storage=0, enum Linkage linkage=L_NONE,
      unsigned char bitFieldBegin=0, unsigned char bitFieldWidth=0);

  ~Object(void) {}

  virtual void Accept(Visitor* v);
  
  virtual Object* ToObject(void) {
    return this;
  }

  virtual bool IsLVal(void) {
    // TODO(wgtdkp): not all object is lval?
    return true;
  }

  bool IsStatic(void) const {
    return (Storage() & S_STATIC) || (Linkage() != L_NONE);
  }

  int Storage(void) const {
    return storage_;
  }

  void SetStorage(int storage) {
    storage_ = storage;
  }

  int Offset(void) const {
    return offset_;
  }

  void SetOffset(int offset) {
    offset_ = offset;
  }

  Declaration* Decl(void) {
    return decl_;
  }

  void SetDecl(Declaration* decl) {
    decl_ = decl;
  }

  unsigned char BitFieldBegin(void) const {
    return bitFieldBegin_;
  }

  unsigned char BitFieldEnd(void) const {
    return bitFieldBegin_ + bitFieldWidth_;
  }

  unsigned char BitFieldWidth(void) const {
    return bitFieldWidth_;
  }

  static unsigned long BitFieldMask(Object* bitField) {
    return BitFieldMask(bitField->bitFieldBegin_, bitField->bitFieldWidth_);
  }

  static unsigned long BitFieldMask(unsigned char begin, unsigned char width) {
    auto end = begin + width;
    return ((0xFFFFFFFFFFFFFFFFUL << (64 - end)) >> (64 - width)) << begin;
  }


  bool HasInit(void) const {
    return decl_ && decl_->Inits().size();
  }

  bool Anonymous(void) const {
    return anonymous_;
  }

  void SetAnonymous(bool anonymous) {
    anonymous_ = anonymous;
  }

  virtual const std::string& Name(void) const {
    /*
    if (Anonymous()) {
      static auto anonyName = "anonymous<"
          + std::to_string(reinterpret_cast<unsigned long long>(this))
          + ">";
      return anonyName;
    }
    */
    return Identifier::Name();
  }

  /*
  bool operator==(const Object& other) const {
    // TODO(wgtdkp): Not implemented
    assert(false);
  }

  bool operator!=(const Object& other) const {
    return !(*this == other);
  }
  */
protected:
  Object(const Token* tok, ::Type* type,
         int storage=0, enum Linkage linkage=L_NONE,
         unsigned char bitFieldBegin=0, unsigned char bitFieldWidth=0)
      : Identifier(tok, type, linkage),
        storage_(storage), offset_(0), decl_(nullptr),
        bitFieldBegin_(bitFieldBegin), bitFieldWidth_(bitFieldWidth),
        anonymous_(false) {}

private:
  int storage_;
  
  // For code gen
  int offset_;

  Declaration* decl_;

  unsigned char bitFieldBegin_;
  // 0 means it's not a bitfield
  unsigned char bitFieldWidth_;

  bool anonymous_;
  //static size_t _labelId {0};
};



/*************** Declaration ******************/

class FuncDef : public ExtDecl
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  typedef std::vector<Object*> ParamList;
  
public:
  static FuncDef* New(Identifier* ident, LabelStmt* retLabel);

  virtual ~FuncDef(void) {}
  
  virtual FuncType* Type(void) {
    return ident_->Type()->ToFuncType();
  }

  ParamList& Params(void) {
    return params_;
  }

  CompoundStmt* Body(void) {
    return body_;
  }

  void SetBody(CompoundStmt* body) {
    body_ = body;
  }

  std::string Name(void) const {
    return ident_->Name();
  }

  enum Linkage Linkage(void) {
    return ident_->Linkage();
  }
  
  virtual void Accept(Visitor* v);

protected:
  FuncDef(Identifier* ident, LabelStmt* retLabel)
      : ident_(ident), retLabel_(retLabel) {}

private:
  Identifier* ident_;
  LabelStmt* retLabel_;
  ParamList params_;
  CompoundStmt* body_;
};


class TranslationUnit : public ASTNode
{
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static TranslationUnit* New(void) {
    return new TranslationUnit();
  }

  virtual ~TranslationUnit(void) {}

  virtual void Accept(Visitor* v);
  
  void Add(ExtDecl* extDecl) {
    extDecls_.push_back(extDecl);
  }

  std::list<ExtDecl*>& ExtDecls(void) {
    return extDecls_;
  }

private:
  TranslationUnit(void) {}

  std::list<ExtDecl*> extDecls_;
};


#endif
