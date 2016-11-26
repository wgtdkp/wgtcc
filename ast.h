#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "error.h"
#include "token.h"
#include "type.h"

#include <cassert>
#include <cctype>
#include <list>
#include <memory>
#include <string>

class Scope;
class Visitor;

class ASTNode;

// Expressions
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class TempVar;
class ASTConstant;

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

using ExtDecl     = ASTNode;
using StmtList    = std::list<Stmt*>;
using CaseList    = std::list<CaseStmt*>;
using InitList    = std::set<Initializer>;
using ArgList     = std::vector<Expr*>;
using ExtDeclList = std::list<ExtDecl*>;

/*
 * AST Nodes
 */

class ASTNode {
public:
  virtual ~ASTNode() {}
  virtual void Accept(Visitor* v) = 0;

protected:
  ASTNode() {}
  //MemPool* pool_ {nullptr};
};


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
  static LabelStmt* New(Stmt* sub_stmt);
  ~LabelStmt() {}
  virtual void Accept(Visitor* v);
  std::string Repr() const { return ".L" + std::to_string(tag_); }

  Stmt* sub_stmt() { return sub_stmt_; }
  const Stmt* sub_stmt() const { return sub_stmt_; }

protected:
  LabelStmt(Stmt* sub_stmt): sub_stmt_(sub_stmt), tag_(GenTag()) {}

private:
  static int GenTag() {
    static int tag = 0;
    return ++tag;
  }

  Stmt* const sub_stmt_;  
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

  Expr* expr() { return expr_; }
  const Expr* expr() const { return expr_; }

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

  Expr* cond() { return cond_; }
  const Expr* cond() const { return cond_; }
  Stmt* then() { return then_; }
  const Stmt* then() const { return then_; }
  Stmt* els() { return els_; }
  const Stmt* els() const { return els_; }

protected:
  IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
      : cond_(cond), then_(then), els_(els) {}

private:
  Expr* const cond_;
  Stmt* const then_;
  Stmt* const els_;
};


class ForStmt: public Stmt {
public:
  static ForStmt* New(CompoundStmt* decl, Expr* init,
                      Expr* cond, Expr* step, Stmt* body);
  virtual ~ForStmt() {}
  virtual void Accept(Visitor* v);

  CompoundStmt* Decl() { return decl_; }
  const CompoundStmt* Decl() const { return decl_; }
  Expr* init() { return init_; }
  const Expr* init() const { return init_; }
  Expr* cond() { return cond_; }
  const Expr* cond() const { return cond_; }
  Expr* step() { return step_; }
  const Expr* step() const { return step_; }
  Stmt* body() { return body_; }
  const Stmt* body() const { return body_; }

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
  static WhileStmt* New(Expr* cond, Stmt* body, bool do_while);
  virtual ~WhileStmt() {}
  virtual void Accept(Visitor* v);

  Expr* cond() { return cond_; }
  const Expr* cond() const { return cond_; }
  Stmt* body() { return body_; }
  const Stmt* body() const { return body_; }
  bool IsDoWhile() const { return do_while_; }

protected:
  WhileStmt(Expr* cond, Stmt* body, bool do_while)
      : cond_(cond), body_(body), do_while_(do_while) {}

private:
  Expr* const cond_;
  Stmt* const body_;
  const bool do_while_;
};


class SwitchStmt: public Stmt {
public:
  static SwitchStmt* New(Expr* select, Stmt* body=nullptr);
  virtual ~SwitchStmt() {}
  virtual void Accept(Visitor* v);

  Expr* select() { return select_; }
  const Expr* select() const { return select_; }
  Stmt* body() { return body_; }
  const Stmt* body() const { return body_; }
  void set_body(Stmt* body) { body_ = body; } 
  DefaultStmt* deft() { return deft_; }
  const DefaultStmt* deft() const { return deft_; }
  void set_deft(DefaultStmt* deft) {deft_ = deft; }
  CaseList& case_list() { return case_list_; }
  const CaseList& case_list() const { return case_list_; }
  void AddCase(CaseStmt* c) { case_list_.push_back(c); }
  bool IsCaseOverlapped(CaseStmt* case_stmt);

protected:
  SwitchStmt(Expr* select, Stmt* body): select_(select), body_(body) {}

private:
  Expr* const select_;
  Stmt* body_;
  DefaultStmt* deft_;
  CaseList case_list_;  
};


class CaseStmt: public Stmt {
public:
  static CaseStmt* New(int begin, int end, Stmt* body);
  virtual ~CaseStmt() {}
  virtual void Accept(Visitor* v);

  int begin() const { return begin_; }
  int end() const { return end_; }
  Stmt* body() { return body_; }
  const Stmt* body() const { return body_; }

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

  LabelStmt* label() { return label_; }
  const LabelStmt* label() const { return label_; }
  void set_label(LabelStmt* label) { label_ = label; }

protected:
  GotoStmt(LabelStmt* label): label_(label) {}

private:
  LabelStmt* label_;  
};


class DefaultStmt: public Stmt {
public:
  static DefaultStmt* New(Stmt* sub_stmt);
  virtual ~DefaultStmt() {}
  virtual void Accept(Visitor* v);

  Stmt* sub_stmt() { return sub_stmt_; }
  const Stmt* sub_stmt() const { return sub_stmt_; }

protected:
  DefaultStmt(Stmt* sub_stmt): sub_stmt_(sub_stmt) {}

private:
  Stmt* const sub_stmt_;  
};


class CompoundStmt : public Stmt {
public:
  static CompoundStmt* New(const StmtList& stmt_list, Scope* scope=nullptr);
  virtual ~CompoundStmt() {}
  virtual void Accept(Visitor* v);

  StmtList& stmt_list() { return stmt_list_; }
  const StmtList& stmt_list() const { return stmt_list_; }
  Scope* scope() { return scope_; }
  const Scope* scope() const { return scope_; }

protected:
  CompoundStmt(const StmtList& stmt_list, Scope* scope=nullptr)
      : stmt_list_(stmt_list), scope_(scope) {}

private:
  StmtList stmt_list_;
  Scope* const scope_;
};


struct Initializer {
  Initializer(Type* t, int off, Expr* e,
              uint8_t bf_begin=0, uint8_t bf_width=0)
      : type(t), offset(off),
        bitfield_begin(bf_begin),
        bitfield_width(bf_width),
        expr(e) {}

  bool operator<(const Initializer& rhs) const;

  // It could be the object it self or, it will be the member
  // that was initialized
  Type* type;
  int offset;
  uint8_t bitfield_begin;
  uint8_t bitfield_width;
  Expr* expr;
};


class Declaration: public Stmt {
public:
  static Declaration* New(Object* obj);
  virtual ~Declaration() {}
  virtual void Accept(Visitor* v);
  
  InitList& init_list() { return init_list_; }
  const InitList& init_list() const { return init_list_; }
  Object* obj() { return obj_; }
  const Object* obj() const { return obj_; }
  
  void AddInit(Initializer init);

protected:
  Declaration(Object* obj): obj_(obj) {}

  Object* obj_;
  InitList init_list_;
};


/*
 * Expr
 *  BinaryOp
 *  UnaryOp
 *  ConditionalOp
 *  FuncCall
 *   ASTConstant
 *  Identifier
 *  Object
 *  TempVar
 */

class Expr : public Stmt {
public:
  virtual ~Expr() {}
  virtual bool IsLVal() = 0;
  virtual void TypeChecking() = 0;
  void EnsureCompatible(const QualType lhs, const QualType rhs) const;
  void EnsureCompatibleOrVoidPointer(const QualType lhs,
                                     const QualType rhs) const;

  const Token* tok() const { return tok_; }
  void set_tok(const Token* tok) { tok_ = tok; }
  Type* type() { return type_.ptr(); }
  const Type* type() const { return type_.ptr(); }

  static Expr* MayCast(Expr* expr);
  static Expr* MayCast(Expr* expr, QualType des_type);

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
    case ']': return !type()->ToArray();
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
  
  int op() const { return op_; }
  Expr* lhs() { return lhs_; }
  const Expr* lhs() const { return lhs_; }
  Expr* rhs() { return rhs_; }
  const Expr* rhs() const { return rhs_; }

private:
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

  int op() const { return op_; }
  Expr* operand() { return operand_; }
  const Expr* operand() const { return operand_; }

protected:
  UnaryOp(int op, Expr* operand, QualType type=nullptr)
    : Expr(operand->tok(), type), op_(op) {
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
public:
  static ConditionalOp* New(const Token* tok,
      Expr* cond, Expr* expr_true, Expr* expr_false);
  virtual ~ConditionalOp() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return false; }
  ArithmType* Convert();
  virtual void TypeChecking();

  Expr* cond() { return cond_; }
  const Expr* cond() const { return cond_; }
  Expr* expr_true() { return expr_true_; }
  const Expr* expr_true() const { return expr_true_; }
  Expr* expr_false() { return expr_false_; }
  const Expr* expr_false() const { return expr_false_; }

protected:
  ConditionalOp(Expr* cond, Expr* expr_true, Expr* expr_false)
      : Expr(cond->tok(), nullptr), cond_(MayCast(cond)),
        expr_true_(MayCast(expr_true)), expr_false_(MayCast(expr_false)) {}

private:
  Expr* cond_;
  Expr* expr_true_;
  Expr* expr_false_;
};


class FuncCall : public Expr {
public:
  static FuncCall* New(Expr* designator, const ArgList& args);
  ~FuncCall() {}
  virtual void Accept(Visitor* v);

  // A function call is ofcourse not lvalue
  virtual bool IsLVal() { return false; }
  virtual void TypeChecking();

  const std::string& Name() const { return tok_->str(); }
  ::FuncType* FuncType() { return designator_->type()->ToFunc(); }

  Expr* designator() { return designator_; }
  const Expr* designator() const { return designator_; }
  ArgList& args() { return args_; }
  const ArgList& args() const { return args_; }

protected:
  FuncCall(Expr* designator, const ArgList& args)
    : Expr(designator->tok(), nullptr),
      designator_(designator), args_(args) {}

  Expr* designator_;
  ArgList args_;
};


class  ASTConstant: public Expr {
public:
  static  ASTConstant* New(const Token* tok, int tag, long val);
  static  ASTConstant* New(const Token* tok, int tag, double val);
  static  ASTConstant* New(const Token* tok, int tag, const std::string* val);
  ~ ASTConstant() {}
  virtual void Accept(Visitor* v);
  virtual bool IsLVal() { return false; }
  virtual void TypeChecking() {}

  long ival() const { return ival_; }
  double fval() const { return fval_; }
  const std::string* sval() const { return sval_; }
  
  std::string SValRepr() const;
  std::string Repr() const {
    return std::string(".LC") + std::to_string(id_);
  }

protected:
   ASTConstant(const Token* tok, QualType type, long val)
      : Expr(tok, type), ival_(val) {}
   ASTConstant(const Token* tok, QualType type, double val)
      : Expr(tok, type), fval_(val) {}
   ASTConstant(const Token* tok, QualType type, const std::string* val)
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


enum class Linkage {
  NONE,
  EXTERNAL,
  INTERNAL,
};


class Identifier: public Expr {
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
    if (ToObject() || ToEnumerator() || linkage_ != Linkage::NONE)
      return nullptr;
    return this;
  }
  
  virtual const std::string Name() const { return tok_->str(); }
  
  Linkage linkage() const { return linkage_; }
  void SetLinkage(Linkage linkage) { linkage_ = linkage; }
  virtual void TypeChecking() {}

protected:
  Identifier(const Token* tok, QualType type, enum Linkage linkage)
      : Expr(tok, type), linkage_(linkage) {}

  // An identifier has property linkage
  enum Linkage linkage_;
};


class Enumerator: public Identifier {
public:
  static Enumerator* New(const Token* tok, int val);
  virtual ~Enumerator() {}
  virtual void Accept(Visitor* v);
  virtual Enumerator* ToEnumerator() { return this; }
  int Val() const { return cons_->ival(); }

protected:
  Enumerator(const Token* tok, int val)
      : Identifier(tok, ArithmType::New(T_INT), Linkage::NONE),
        cons_( ASTConstant::New(tok, T_INT, (long)val)) {}

   ASTConstant* cons_;
};


class Object : public Identifier {
public:
  static Object* New(const Token* tok, QualType type,
                     int storage=0, Linkage linkage=Linkage::NONE,
                     uint8_t bitfield_begin=0,
                     uint8_t bitfield_width=0);
  static Object* NewAnony(const Token* tok, QualType type,
                          int storage=0, Linkage linkage=Linkage::NONE,
                          uint8_t bitfield_begin=0,
                          uint8_t bitfield_width=0);

  ~Object() {}
  virtual void Accept(Visitor* v);
  virtual Object* ToObject() { return this; }
  virtual bool IsLVal() {
    // TODO(wgtdkp): not all object is lval?
    return true;
  }
  bool IsStatic() const {
    return (storage() & S_STATIC) || (linkage() != Linkage::NONE);
  }
  int storage() const { return storage_; }
  void set_storage(int storage) { storage_ = storage; }
  int align() const { return align_; }

  void set_align(int align) {
    assert(align > 0);
    // Allowing reduce alignment to implement __attribute__((packed))
    //if (align < align_)
    //  Error(this, "alignment specifier cannot reduce alignment");
    align_ = align;
  }
  int offset() const { return offset_; }
  void set_offset(int offset) { offset_ = offset; }
  Declaration* decl() { return decl_; }
  const Declaration* decl() const { return decl_; }
  void set_decl(Declaration* decl) { decl_ = decl; }
  uint8_t bitfield_begin() const { return bitfield_begin_; }
  uint8_t bitfield_end() const { return bitfield_begin_ + bitfield_width_; }
  uint8_t bitfield_width() const { return bitfield_width_; }
  static uint64_t bitfield_mask(Object* bitfield) {
    return bitfield_mask(bitfield->bitfield_begin_, bitfield->bitfield_width_);
  }
  static uint64_t bitfield_mask(uint8_t begin, uint8_t width) {
    auto end = begin + width;
    return ((~0UL << (64 - end)) >> (64 - width)) << begin;
  }
  bool anonymous() const { return anonymous_; }  
  bool HasInit() const { return decl_ && decl_->init_list().size(); }
  virtual const std::string Name() const { return Identifier::Name(); }

  std::string Repr() const {
    assert(IsStatic() || anonymous_);
    if (anonymous_)
      return "anonymous." + std::to_string(id_);
    if (linkage_ == Linkage::NONE)
      return Name() + "." + std::to_string(id_);
    return Name();
  }

protected:
  Object(const Token* tok, QualType type, int storage=0,
         enum Linkage linkage=Linkage::NONE,
         uint8_t bitfield_begin=0,
         uint8_t bitfield_width=0)
      : Identifier(tok, type, linkage),
        storage_(storage), offset_(0),
        align_(type->align()), decl_(nullptr),
        bitfield_begin_(bitfield_begin),
        bitfield_width_(bitfield_width),
        anonymous_(false) {}
  
private:
  int storage_;
  int offset_;
  int align_;
  Declaration* decl_;
  uint8_t bitfield_begin_;
  // 0 means it's not a bitfield
  uint8_t bitfield_width_;

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
  ::FuncType* FuncType() { return ident_->type()->ToFunc(); }
  CompoundStmt* body() { return body_; }
  void set_body(CompoundStmt* body) { body_ = body; }
  std::string Name() const { return ident_->Name(); }
  enum Linkage linkage() { return ident_->linkage(); }
  virtual void Accept(Visitor* v);

private:
  FuncDef(Identifier* ident): ident_(ident) {}

  Identifier* ident_;
  CompoundStmt* body_;
};


class TranslationUnit : public ASTNode {
public:
  static TranslationUnit* New();
  virtual ~TranslationUnit() {}
  virtual void Accept(Visitor* v);
  void Add(ExtDecl* ext_decl) { ext_decl_list_.push_back(ext_decl); }
  ExtDeclList& ext_decl_list() { return ext_decl_list_; }
  const ExtDeclList& ext_decl_list() const { return ext_decl_list_; }

private:
  TranslationUnit() {}
  ExtDeclList ext_decl_list_;
};

#endif
