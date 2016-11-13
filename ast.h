#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "error.h"
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

// Expressions
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class TempVar;
class Constant;

class Identifier;
class Object;
class Initializer;
class Declaration;
class Enumerator;

// Statements
class Stmt;
class IfStmt;
class ForStmt;
class SwitchStmt;
class CaseStmt;
class WhileStmt;
class LabelStmt;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class GotoStmt;
class DefaultStmt;
class EmptyStmt;
class CompoundStmt;

class FuncDef;
class TranslationUnit;

using StmtList = std::list<Stmt*>;
using CaseList = std::list<CaseStmt*>;


/*
 * AST Node
 */

class ASTNode {
public:
  virtual ~ASTNode() {}
  virtual void Accept(Visitor* v) = 0;

protected:
  ASTNode() {}
  //MemPool* pool_ {nullptr};
};

typedef ASTNode ExtDecl;


/*
 * Statements
 */

class Stmt : public ASTNode {
public:
  virtual ~Stmt() {}

protected:
   Stmt() {}
};


class EmptyStmt : public Stmt {
public:
  static EmptyStmt* New();
  virtual ~EmptyStmt() {}
  virtual void Accept(Visitor* v);

protected:
  EmptyStmt() {}
};


class LabelStmt : public Stmt {
public:
  static LabelStmt* New(Stmt* subStmt);
  ~LabelStmt() {}
  virtual void Accept(Visitor* v);
  std::string Repr() const { return ".L" + std::to_string(tag_); }

  Stmt* GetSubStmt() { return subStmt_; }
  const Stmt* GetSubStmt() const { return subStmt_; }

protected:
  LabelStmt(Stmt* subStmt): subStmt_(subStmt), tag_(GenTag()) {}

private:
  static int GenTag() {
    static int tag = 0;
    return ++tag;
  }

  Stmt* const subStmt_;  
  int tag_; // 使用整型的tag值，而不直接用字符串
};


class BreakStmt: public Stmt {
public:
  static BreakStmt* New();
  virtual ~BreakStmt() {}
  virtual void Accept(Visitor* v);

protected:
  BreakStmt() {}
};


class ContinueStmt: public Stmt {
public:
  static ContinueStmt* New();
  virtual ~ContinueStmt() {}
  virtual void Accept(Visitor* v);

protected:
  ContinueStmt() {}
};


class ReturnStmt: public Stmt {
public:
  static ReturnStmt* New(Expr* expr);
  virtual ~ReturnStmt() {}
  virtual void Accept(Visitor* v);

  Expr* GetExpr() { return expr_; }
  const Expr* GetExpr() const { return expr_; }

protected:
  ReturnStmt(Expr* expr): expr_(expr) {}

private:
  Expr* const expr_;
};


class IfStmt : public Stmt {
public:
  static IfStmt* New(Expr* cond, Stmt* then, Stmt* els=nullptr);
  virtual ~IfStmt() {}
  virtual void Accept(Visitor* v);

  Expr* GetCond() { return cond_; }
  const Expr* GetCond() const { return cond_; }
  Stmt* GetThen() { return then_; }
  const Stmt* GetThen() const { return then_; }
  Stmt* GetElse() { return else_; }
  const Stmt* GetElse() const { return else_; }

protected:
  IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
      : cond_(cond), then_(then), else_(els) {}

private:
  Expr* const cond_;
  Stmt* const then_;
  Stmt* const else_;
};


class ForStmt: public Stmt {
public:
  static ForStmt* New(CompoundStmt* decl, Expr* init,
                      Expr* cond, Expr* step, Stmt* body);
  virtual ~ForStmt() {}
  virtual void Accept(Visitor* v);

  CompoundStmt* GetDecl() { return decl_; }
  const CompoundStmt* GetDecl() const { return decl_; }
  Expr* GetInit() { return init_; }
  const Expr* GetInit() const { return init_; }
  Expr* GetCond() { return cond_; }
  const Expr* GetCond() const { return cond_; }
  Expr* GetStep() { return step_; }
  const Expr* GetStep() const { return step_; }
  Stmt* GetBody() { return body_; }
  const Stmt* GetBody() const { return body_; }

protected:
  ForStmt(CompoundStmt* decl, Expr* init,
          Expr* cond, Expr* step, Stmt* body)
      : decl_(decl), init_(init),
        cond_(cond), step_(step), body_(body) {}
private:
  CompoundStmt* const decl_;
  Expr* const init_;
  Expr* const cond_;
  Expr* const step_;
  Stmt* const body_;
};


class WhileStmt: public Stmt {
public:
  static WhileStmt* New(Expr* cond, Stmt* body, bool isDoWhile);
  virtual ~WhileStmt() {}
  virtual void Accept(Visitor* v);

  Expr* GetCond() { return cond_; }
  const Expr* GetCond() const { return cond_; }
  Stmt* GetBody() { return body_; }
  const Stmt* GetBody() const { return body_; }
  bool IsDoWhile() const { return isDoWhile_; }

protected:
  WhileStmt(Expr* cond, Stmt* body, bool isDoWhile)
      : cond_(cond), body_(body), isDoWhile_(isDoWhile) {}

private:
  Expr* const cond_;
  Stmt* const body_;
  const bool isDoWhile_;
};


class SwitchStmt: public Stmt {
public:
  static SwitchStmt* New(Expr* select, Stmt* body=nullptr);
  virtual ~SwitchStmt() {}
  virtual void Accept(Visitor* v);

  Expr* GetSelect() { return select_; }
  const Expr* GetSelect() const { return select_; }
  Stmt* GetBody() { return body_; }
  const Stmt* GetBody() const { return body_; }
  void SetBody(Stmt* body) { body_ = body; } 
  DefaultStmt* GetDefault() { return default_; }
  const DefaultStmt* GetDefault() const { return default_; }
  void SetDefault(DefaultStmt* deft) {default_ = deft; }
  CaseList& GetCaseList() { return caseList_; }
  const CaseList& GetCaseList() const { return caseList_; }
  void AddCase(CaseStmt* c) { caseList_.push_back(c); }
  bool IsCaseOverlapped(CaseStmt* caseStmt);

protected:
  SwitchStmt(Expr* select, Stmt* body): select_(select), body_(body) {}

private:
  Expr* const select_;
  Stmt* body_;
  DefaultStmt* default_;
  CaseList caseList_;  
};


class CaseStmt: public Stmt {
public:
  static CaseStmt* New(int begin, int end, Stmt* body);
  virtual ~CaseStmt() {}
  virtual void Accept(Visitor* v);

  int GetBegin() const { return begin_; }
  int GetEnd() const { return end_; }
  Stmt* GetBody() { return body_; }
  const Stmt* GetBody() const { return body_; }

protected:
  CaseStmt(int begin, int end, Stmt* body)
      : begin_(begin), end_(end), body_(body) {}

private:
  const int begin_;
  const int end_;
  Stmt* const body_;
};


class GotoStmt: public Stmt {
public:
  static GotoStmt* New(LabelStmt* label);
  virtual ~GotoStmt() {}
  virtual void Accept(Visitor* v);

  LabelStmt* GetLabel() { return label_; }
  const LabelStmt* GetLabel() const { return label_; }
  void SetLabel(LabelStmt* label) { label_ = label; }

protected:
  GotoStmt(LabelStmt* label): label_(label) {}

private:
  LabelStmt* label_;  
};


class DefaultStmt: public Stmt {
public:
  static DefaultStmt* New(Stmt* subStmt);
  virtual ~DefaultStmt() {}
  virtual void Accept(Visitor* v);

  Stmt* GetSubStmt() { return subStmt_; }
  const Stmt* GetSubStmt() const { return subStmt_; }

protected:
  DefaultStmt(Stmt* subStmt): subStmt_(subStmt) {}

private:
  Stmt* const subStmt_;  
};


class CompoundStmt : public Stmt {
public:
  static CompoundStmt* New(StmtList& stmtList, ::Scope* scope=nullptr);
  virtual ~CompoundStmt() {}
  virtual void Accept(Visitor* v);

  StmtList& GetStmts() { return stmtList_; }
  const StmtList& GetStmtList() const { return stmtList_; }
  Scope* GetScope() { return scope_; }
  const Scope* GetScope() const { return scope_; }

protected:
  CompoundStmt(const StmtList& stmtList, ::Scope* scope=nullptr)
      : stmtList_(stmtList), scope_(scope) {}

private:
  StmtList stmtList_;
  Scope* const scope_;
};


struct Initializer {
  Initializer(Type* type, int offset, Expr* expr,
              unsigned char bitFieldBegin=0,
              unsigned char bitFieldWidth=0)
      : type_(type), offset_(offset),
        bitFieldBegin_(bitFieldBegin),
        bitFieldWidth_(bitFieldWidth),
        expr_(expr) {}       

  bool operator<(const Initializer& rhs) const;

  // It could be the object it self or, it will be the member
  // that was initialized
  Type* type_;
  int offset_;
  unsigned char bitFieldBegin_;
  unsigned char bitFieldWidth_;

  Expr* expr_;
};


class Declaration: public Stmt {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  typedef std::set<Initializer> InitList;

public:
  static Declaration* New(Object* obj);
  virtual ~Declaration() {}
  virtual void Accept(Visitor* v);
  InitList& Inits() { return inits_; }
  Object* Obj() { return obj_; }
  void AddInit(Initializer init);

protected:
  Declaration(Object* obj): obj_(obj) {}

  Object* obj_;
  InitList inits_;
};


/*
 * Expr
 *  BinaryOp
 *  UnaryOp
 *  ConditionalOp
 *  FuncCall
 *  Constant
 *  Identifier
 *  Object
 *  TempVar
 */

class Expr : public Stmt {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  virtual ~Expr() {}
  ::Type* Type() { return type_.GetPtr(); }
  virtual bool IsLVal() = 0;
  virtual void TypeChecking() = 0;
  void EnsureCompatible(const QualType lhs, const QualType rhs) const;
  void EnsureCompatibleOrVoidPointer(const QualType lhs,
                                     const QualType rhs) const;
  const Token* Tok() const { return tok_; }
  void SetTok(const Token* tok) { tok_ = tok; }

  static Expr* MayCast(Expr* expr);
  static Expr* MayCast(Expr* expr, QualType desType);

  bool IsConstQualified() const { return type_.IsConstQualified(); }
  bool IsRestrictQualified() const { return type_.IsRestrictQualified(); }
  bool IsVolatileQualified() const { return type_.IsVolatileQualified(); }

protected:
  // You can construct a expression without specifying a type,
  // then the type should be evaluated in TypeChecking()
  Expr(const Token* tok, QualType type): tok_(tok), type_(type) {}

  const Token* tok_;
  QualType type_;
};


/*
 * '+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
 * '=',(复合赋值运算符被拆分为两个运算)
 * '==', '!=', '<=', '>=',
 * '&&', '||'
 * '['(下标运算符), '.'(成员运算符)
 * ','(逗号运算符),
 */
class BinaryOp : public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;
  friend class Declaration;

public:
  static BinaryOp* New(const Token* tok, Expr* lhs, Expr* rhs);
  static BinaryOp* New(const Token* tok, int op, Expr* lhs, Expr* rhs);
  virtual ~BinaryOp() {}
  virtual void Accept(Visitor* v);
  // Member ref operator is a lvalue
  virtual bool IsLVal() {
    switch (op_) {
    case '.':
    case ']': return !Type()->ToArray();
    default: return false;
    }
  }

  ArithmType* Convert();

  virtual void TypeChecking();
  void SubScriptingOpTypeChecking();
  void MemberRefOpTypeChecking();
  void MultiOpTypeChecking();
  void AdditiveOpTypeChecking();
  void ShiftOpTypeChecking();
  void RelationalOpTypeChecking();
  void EqualityOpTypeChecking();
  void BitwiseOpTypeChecking();
  void LogicalOpTypeChecking();
  void AssignOpTypeChecking();
  void CommaOpTypeChecking();
  
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
class UnaryOp : public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static UnaryOp* New(int op, Expr* operand, QualType type=nullptr);
  virtual ~UnaryOp() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal();
  ArithmType* Convert();
  void TypeChecking();
  void IncDecOpTypeChecking();
  void AddrOpTypeChecking();
  void DerefOpTypeChecking();
  void UnaryArithmOpTypeChecking();
  void CastOpTypeChecking();

protected:
  UnaryOp(int op, Expr* operand, QualType type=nullptr)
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
class ConditionalOp : public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static ConditionalOp* New(const Token* tok,
      Expr* cond, Expr* exprTrue, Expr* exprFalse);
  virtual ~ConditionalOp() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return false; }
  ArithmType* Convert();
  virtual void TypeChecking();

protected:
  ConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
      : Expr(cond->Tok(), nullptr), cond_(MayCast(cond)),
        exprTrue_(MayCast(exprTrue)), exprFalse_(MayCast(exprFalse)) {}

private:
  Expr* cond_;
  Expr* exprTrue_;
  Expr* exprFalse_;
};


class FuncCall : public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:        
  typedef std::vector<Expr*> ArgList;

public:
  static FuncCall* New(Expr* designator, const ArgList& args);
  ~FuncCall() {}
  virtual void Accept(Visitor* v);

  // A function call is ofcourse not lvalue
  virtual bool IsLVal() { return false; }
  ArgList* Args() { return &args_; }
  Expr* Designator() { return designator_; }
  const std::string& Name() const { return tok_->str_; }
  ::FuncType* FuncType() { return designator_->Type()->ToFunc(); }
  virtual void TypeChecking();

protected:
  FuncCall(Expr* designator, const ArgList& args)
    : Expr(designator->Tok(), nullptr),
      designator_(designator), args_(args) {}

  Expr* designator_;
  ArgList args_;
};


class Constant: public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static Constant* New(const Token* tok, int tag, long val);
  static Constant* New(const Token* tok, int tag, double val);
  static Constant* New(const Token* tok, int tag, const std::string* val);
  ~Constant() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return false; }
  virtual void TypeChecking() {}
  long IVal() const { return ival_; }
  double FVal() const { return fval_; }
  const std::string* SVal() const { return sval_; }
  std::string SValRepr() const;
  std::string Repr() const {
    return std::string(".LC") + std::to_string(id_);
  }

protected:
  Constant(const Token* tok, QualType type, long val)
      : Expr(tok, type), ival_(val) {}
  Constant(const Token* tok, QualType type, double val)
      : Expr(tok, type), fval_(val) {}
  Constant(const Token* tok, QualType type, const std::string* val)
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


class TempVar : public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static TempVar* New(QualType type);
  virtual ~TempVar() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return true; }
  virtual void TypeChecking() {}

protected:
  TempVar(QualType type): Expr(nullptr, type), tag_(GenTag()) {}
  
private:
  static int GenTag() {
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


class Identifier: public Expr {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static Identifier* New(const Token* tok, QualType type, Linkage linkage);
  virtual ~Identifier() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return false; }
  virtual Object* ToObject() { return nullptr; }
  virtual Enumerator* ToEnumerator() { return nullptr; }

   // An identifer can be:
   //   object, sturct/union/enum tag, typedef name, function, label.
   Identifier* ToTypeName() {
    // A typename has no linkage
    // And a function has external or internal linkage
    if (ToObject() || ToEnumerator() || linkage_ != L_NONE)
      return nullptr;
    return this;
  }
  virtual const std::string Name() const { return tok_->str_; }
  enum Linkage Linkage() const { return linkage_; }
  void SetLinkage(enum Linkage linkage) { linkage_ = linkage; }
  virtual void TypeChecking() {}

protected:
  Identifier(const Token* tok, QualType type, enum Linkage linkage)
      : Expr(tok, type), linkage_(linkage) {}

  // An identifier has property linkage
  enum Linkage linkage_;
};


class Enumerator: public Identifier {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;

public:
  static Enumerator* New(const Token* tok, int val);
  virtual ~Enumerator() {}
  virtual void Accept(Visitor* v);
  virtual Enumerator* ToEnumerator() { return this; }
  int Val() const { return cons_->IVal(); }

protected:
  Enumerator(const Token* tok, int val)
      : Identifier(tok, ArithmType::New(T_INT), L_NONE),
        cons_(Constant::New(tok, T_INT, (long)val)) {}

  Constant* cons_;
};


class Object : public Identifier {
  template<typename T> friend class Evaluator;
  friend class AddrEvaluator;
  friend class Generator;
  friend class LValGenerator;

public:
  static Object* New(const Token* tok, QualType type, int storage=0,
                     enum Linkage linkage=L_NONE,
                     unsigned char bitFieldBegin=0,
                     unsigned char bitFieldWidth=0);
  static Object* NewAnony(const Token* tok, QualType type, int storage=0,
                          enum Linkage linkage=L_NONE,
                          unsigned char bitFieldBegin=0,
                          unsigned char bitFieldWidth=0);

  ~Object() {}
  virtual void Accept(Visitor* v);
  virtual Object* ToObject() { return this; }
  virtual bool IsLVal() {
    // TODO(wgtdkp): not all object is lval?
    return true;
  }
  bool IsStatic() const {
    return (Storage() & S_STATIC) || (Linkage() != L_NONE);
  }
  int Storage() const { return storage_; }
  void SetStorage(int storage) { storage_ = storage; }
  int Align() const { return align_; }

  void SetAlign(int align) {
    assert(align > 0);
    // Allowing reduce alignment to implement __attribute__((packed))
    //if (align < align_)
    //  Error(this, "alignment specifier cannot reduce alignment");
    align_ = align;
  }
  int Offset() const { return offset_; }
  void SetOffset(int offset) { offset_ = offset; }
  Declaration* Decl() { return decl_; }
  void SetDecl(Declaration* decl) { decl_ = decl; }
  unsigned char BitFieldBegin() const { return bitFieldBegin_; }
  unsigned char BitFieldEnd() const { return bitFieldBegin_ + bitFieldWidth_; }
  unsigned char BitFieldWidth() const { return bitFieldWidth_; }
  static unsigned long BitFieldMask(Object* bitField) {
    return BitFieldMask(bitField->bitFieldBegin_, bitField->bitFieldWidth_);
  }
  static unsigned long BitFieldMask(unsigned char begin, unsigned char width) {
    auto end = begin + width;
    return ((0xFFFFFFFFFFFFFFFFUL << (64 - end)) >> (64 - width)) << begin;
  }
  bool HasInit() const { return decl_ && decl_->Inits().size(); }
  bool Anonymous() const { return anonymous_; }
  virtual const std::string Name() const { return Identifier::Name(); }

  std::string Repr() const {
    assert(IsStatic() || anonymous_);
    if (anonymous_)
      return "anonymous." + std::to_string(id_);
    if (linkage_ == L_NONE)
      return Name() + "." + std::to_string(id_);
    return Name();
  }

protected:
  Object(const Token* tok, QualType type, int storage=0,
         enum Linkage linkage=L_NONE,
         unsigned char bitFieldBegin=0,
         unsigned char bitFieldWidth=0)
      : Identifier(tok, type, linkage),
        storage_(storage), offset_(0),
        align_(type->Align()), decl_(nullptr),
        bitFieldBegin_(bitFieldBegin),
        bitFieldWidth_(bitFieldWidth),
        anonymous_(false) {}
  
private:
  int storage_;
  int offset_;
  int align_;
  Declaration* decl_;
  unsigned char bitFieldBegin_;
  // 0 means it's not a bitfield
  unsigned char bitFieldWidth_;

  bool anonymous_;
  long id_ {0};
};


/*
 * Declaration
 */

class FuncDef : public ExtDecl {  
public:
  using ParamList = std::vector<Object*>;

  static FuncDef* New(Identifier* ident);
  virtual ~FuncDef() {}
  ::FuncType* FuncType() { return ident_->Type()->ToFunc(); }
  CompoundStmt* Body() { return body_; }
  void SetBody(CompoundStmt* body) { body_ = body; }
  std::string Name() const { return ident_->Name(); }
  enum Linkage Linkage() { return ident_->Linkage(); }
  virtual void Accept(Visitor* v);
  
  Identifier* ident_;
  CompoundStmt* body_;

protected:
  FuncDef(Identifier* ident): ident_(ident) {}
};


class TranslationUnit : public ASTNode {
public:
  static TranslationUnit* New();
  virtual ~TranslationUnit() {}
  virtual void Accept(Visitor* v);
  void Add(ExtDecl* extDecl) { extDecls_.push_back(extDecl); }
  std::list<ExtDecl*>& ExtDecls() { return extDecls_; }
    
  std::list<ExtDecl*> extDecls_;

private:
  TranslationUnit() {}
};

#endif
