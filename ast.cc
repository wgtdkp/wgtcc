#include "ast.h"

#include "error.h"
#include "evaluator.h"
#include "mem_pool.h"
#include "parser.h"
#include "token.h"


static MemPoolImp<BinaryOp>         binary_op_pool;
static MemPoolImp<ConditionalOp>    conditionalop_pool;
static MemPoolImp<FuncCall>         funccall_pool;
static MemPoolImp<Declaration>      initialization_pool;
static MemPoolImp<Object>           object_pool;
static MemPoolImp<Identifier>       identifier_pool;
static MemPoolImp<Enumerator>       enumerator_pool;
static MemPoolImp< ASTConstant>     constant_pool;
static MemPoolImp<TempVar>          tempvar_pool;
static MemPoolImp<UnaryOp>          unaryop_pool;

static MemPoolImp<IfStmt>           ifstmt_pool;
static MemPoolImp<ForStmt>          forstmt_pool;
static MemPoolImp<WhileStmt>        whilestmt_pool;
static MemPoolImp<SwitchStmt>       switchstmt_pool;
static MemPoolImp<CaseStmt>         casestmt_pool;
static MemPoolImp<DefaultStmt>      defaultstmt_pool;
static MemPoolImp<BreakStmt>        breakstmt_pool;
static MemPoolImp<ContinueStmt>     continuestmt_pool;
static MemPoolImp<ReturnStmt>       returnstmt_pool;
static MemPoolImp<LabelStmt>        labelstmt_pool;
static MemPoolImp<GotoStmt>         gotostmt_pool;
static MemPoolImp<CompoundStmt>     compoundstmt_pool;
static MemPoolImp<EmptyStmt>        emptystmt_pool;

static MemPoolImp<FuncDef>          funcdef_pool;
static MemPoolImp<TranslationUnit>  translationunit_pool;

/*
 * Accept
 */

void Declaration::Accept(Visitor* v)
{
  v->VisitDeclaration(this);
}


void EmptyStmt::Accept(Visitor* v) {
  // Nothing to do
}


void LabelStmt::Accept(Visitor* v) {
  v->VisitLabelStmt(this);
}


void BreakStmt::Accept(Visitor* v) {
  v->VisitBreakStmt(this);
}


void ContinueStmt::Accept(Visitor* v) {
  v->VisitContinueStmt(this);
}


void IfStmt::Accept(Visitor* v) {
  v->VisitIfStmt(this);
}


void ForStmt::Accept(Visitor* v) {
  v->VisitForStmt(this);
}


void WhileStmt::Accept(Visitor* v) {
  v->VisitWhileStmt(this);
}


void SwitchStmt::Accept(Visitor* v) {
  v->VisitSwitchStmt(this);
}


void CaseStmt::Accept(Visitor* v) {
  v->VisitCaseStmt(this);
}


void ReturnStmt::Accept(Visitor* v) {
  v->VisitReturnStmt(this);
}


void GotoStmt::Accept(Visitor* v) {
  v->VisitGotoStmt(this);
}


void DefaultStmt::Accept(Visitor* v) {
  v->VisitDefaultStmt(this);
}


void CompoundStmt::Accept(Visitor* v) {
  v->VisitCompoundStmt(this);
}


void BinaryOp::Accept(Visitor* v) {
  v->VisitBinaryOp(this);
}


void UnaryOp::Accept(Visitor* v) {
  v->VisitUnaryOp(this);
}


void ConditionalOp::Accept(Visitor* v) {
  v->VisitConditionalOp(this);
}


void FuncCall::Accept(Visitor* v) { 
  v->VisitFuncCall(this);
}


void Identifier::Accept(Visitor* v) {
  v->VisitIdentifier(this);
}


void Object::Accept(Visitor* v) {
  v->VisitObject(this);
}


void  ASTConstant::Accept(Visitor* v) {
  v->VisitASTConstant(this);
}


void Enumerator::Accept(Visitor* v)
{
  v->VisitEnumerator(this);
}


void TempVar::Accept(Visitor* v) {
  v->VisitTempVar(this);
}


void FuncDef::Accept(Visitor* v) {
  v->VisitFuncDef(this);
}


void TranslationUnit::Accept(Visitor* v) {
  v->VisitTranslationUnit(this);
}


// Casting array to pointer, function to pointer to function
Expr* Expr::MayCast(Expr* expr) {
  auto type = Type::MayCast(expr->type());
  // If the types are equal, no need cast
  if (type != expr->type()) { // Pointer comparison is enough
    return UnaryOp::New(Token::CAST, expr, type);
  }
  return expr;
}


Expr* Expr::MayCast(Expr* expr, QualType des_type) {
  expr = MayCast(expr);
  auto src_type = expr->type();
  if (des_type->ToPointer() && src_type->ToPointer())
    if (des_type->IsVoidPointer() || src_type->IsVoidPointer())
      return expr;
  if (!des_type->Compatible(*expr->type()))
    expr = UnaryOp::New(Token::CAST, expr, des_type);
  return expr;
}


BinaryOp* BinaryOp::New(const Token* tok, Expr* lhs, Expr* rhs) {
  return New(tok, tok->tag(), lhs, rhs);
}


BinaryOp* BinaryOp::New(const Token* tok, int op, Expr* lhs, Expr* rhs) {
  switch (op) {
  case ',': case '.': case '=': 
  case '*': case '/': case '%':
  case '+': case '-': case '&':
  case '^': case '|': case '<':
  case '>':
  case Token::LEFT:
  case Token::RIGHT:
  case Token::LE:
  case Token::GE:
  case Token::EQ:
  case Token::NE: 
  case Token::LOGICAL_AND:
  case Token::LOGICAL_OR:
    break;
  default:
    assert(0);
  }

  auto ret = new (binary_op_pool.Alloc()) BinaryOp(tok, op, lhs, rhs);
  ret->TypeChecking();
  return ret;
}


ArithmType* BinaryOp::Convert() {
  // Both lhs and rhs are ensured to be have arithmetic type
  auto lhs_type = lhs_->type()->ToArithm();
  auto rhs_type = rhs_->type()->ToArithm();
  assert(lhs_type && rhs_type);
  auto type = ArithmType::MaxType(lhs_type, rhs_type);
  if (lhs_type != type) { // Pointer comparation is enough!
    lhs_ = UnaryOp::New(Token::CAST, lhs_, type);
  }
  if (rhs_type != type) {
    rhs_ = UnaryOp::New(Token::CAST, rhs_, type);
  }
  
  return type;
}


/*
 * Type checking
 */

void Expr::EnsureCompatibleOrVoidPointer(const QualType lhs,
                                         const QualType rhs) const {
  if (lhs->ToPointer() && rhs->ToPointer() &&
      (lhs->IsVoidPointer() || rhs->IsVoidPointer())) {
    return;
  }
  EnsureCompatible(lhs, rhs);
}


void Expr::EnsureCompatible(const QualType lhs, const QualType rhs) const {
  if (!lhs->Compatible(*rhs))
    Error(this, "incompatible types");
}


void BinaryOp::TypeChecking() {
  switch (op_) {
  case '.':
    return MemberRefOpTypeChecking();

  case '*':
  case '/':
  case '%':
    return MultiOpTypeChecking();

  case '+':
  case '-':
    return AdditiveOpTypeChecking();

  case Token::LEFT:
  case Token::RIGHT:
    return ShiftOpTypeChecking();

  case '<':
  case '>':
  case Token::LE:
  case Token::GE:
    return RelationalOpTypeChecking();

  case Token::EQ:
  case Token::NE:
    return EqualityOpTypeChecking();

  case '&':
  case '^':
  case '|':
    return BitwiseOpTypeChecking();

  case Token::LOGICAL_AND:
  case Token::LOGICAL_OR:
    return LogicalOpTypeChecking();

  case '=':
    return AssignOpTypeChecking();
  
  case ',':
    return CommaOpTypeChecking();
  default:
    assert(0);
  }
}


void BinaryOp::CommaOpTypeChecking() {
  type_ = rhs_->type();
}


void BinaryOp::SubScriptingOpTypeChecking() {
  auto lhs_type = lhs_->type()->ToPointer();
  if (!lhs_type) {
    Error(this, "an pointer expected");
  }
  if (!rhs_->type()->IsInteger()) {
    Error(this, "the operand of [] should be intger");
  }

  // The type of [] operator is the derived type
  type_ = lhs_type->derived();    
}


void BinaryOp::MemberRefOpTypeChecking() {
  type_ = rhs_->type();
}


void BinaryOp::MultiOpTypeChecking() {
  if (!lhs_->type()->ToArithm() || !rhs_->type()->ToArithm()) {
    Error(this, "operands should have arithmetic type");
  }
  if ('%' == op_ &&
      !(lhs_->type()->IsInteger() && rhs_->type()->IsInteger())) {
    Error(this, "operands of '%%' should be integers");
  }
  type_ = Convert();
}


/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *    1. lhs of MINUS operator, and rhs must be integer or pointer;
 *    2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking() {
  auto lhs_type = lhs_->type()->ToPointer();
  auto rhs_type = rhs_->type()->ToPointer();
  if (lhs_type) {
    if (op_ == '-') {
      if (rhs_type) {
        if (!lhs_type->Compatible(*rhs_type))
          Error(this, "invalid operands to binary -");
        type_ = ArithmType::New(T_LONG); // ptrdiff_t
      } else if (!rhs_->type()->IsInteger()) {
        Error(this, "invalid operands to binary -");
      } else {
        type_ = lhs_type;
      }
    } else if (!rhs_->type()->IsInteger()) {
      Error(this, "invalid operands to binary +");
    } else {
      type_ = lhs_type;
    }
  } else if (rhs_type) {
    if (op_ == '+' && !lhs_->type()->IsInteger()) {
      Error(this, "invalid operands to binary '+'");
    } else if (op_ == '-' && !lhs_type) {
      Error(this, "invalid operands to binary '-'");
    }
    type_ = op_ == '-' ? ArithmType::New(T_LONG): rhs_->type();
    std::swap(lhs_, rhs_); // To simplify code gen
  } else {
    if (!lhs_->type()->ToArithm() || !rhs_->type()->ToArithm()) {
      Error(this, "invalid operands to binary %s", tok_->str().c_str());
    }
    type_ = Convert();
  }
}


void BinaryOp::ShiftOpTypeChecking() {
  auto lhs_type = lhs_->type()->ToArithm();
  auto rhs_type = rhs_->type()->ToArithm();
  if (!lhs_type || !lhs_type->IsInteger() || !rhs_type || !rhs_type->IsInteger())
    Error(this, "expect integers for shift operator");
  lhs_ = Expr::MayCast(lhs_, ArithmType::IntegerPromote(lhs_type));
  rhs_ = Expr::MayCast(rhs_, ArithmType::IntegerPromote(rhs_type));
  type_ = lhs_->type();
}


void BinaryOp::RelationalOpTypeChecking() {
  if (lhs_->type()->ToPointer() || rhs_->type()->ToPointer()) {
    EnsureCompatible(lhs_->type(), rhs_->type());
  } else {
    if (!lhs_->type()->IsReal() || !rhs_->type()->IsReal()) {
      Error(this, "expect real type of operands");
    }
    Convert();
  } 
  type_ = ArithmType::New(T_INT);
}


void BinaryOp::EqualityOpTypeChecking() {
  if (lhs_->type()->ToPointer() || rhs_->type()->ToPointer()) {
    EnsureCompatibleOrVoidPointer(lhs_->type(), rhs_->type());
  } else {
    if (!lhs_->type()->ToArithm() || !rhs_->type()->ToArithm())
      Error(this, "invalid operands to binary %s", tok_->str().c_str());
    Convert();
  }

  type_ = ArithmType::New(T_INT);
}


void BinaryOp::BitwiseOpTypeChecking() {
  if (!lhs_->type()->IsInteger() || !rhs_->type()->IsInteger())
    Error(this, "operands of '&' should be integer");
  type_ = Convert();
}


void BinaryOp::LogicalOpTypeChecking() {
  if (!lhs_->type()->IsScalar() || !rhs_->type()->IsScalar())
    Error(this, "the operand should be arithmetic type or pointer");
  type_ = ArithmType::New(T_INT);
}


void BinaryOp::AssignOpTypeChecking() {
  if (!lhs_->IsLVal()) {
    Error(lhs_, "lvalue expression expected");
  } else if (lhs_->IsConstQualified()) {
    Error(lhs_, "left operand of '=' is const qualified");
  }

  if (!lhs_->type()->ToArithm() || !rhs_->type()->ToArithm()) {
    EnsureCompatibleOrVoidPointer(lhs_->type(), rhs_->type());
  }
  
  // The other constraints are lefted to cast operator
  rhs_ = Expr::MayCast(rhs_, lhs_->type());
  type_ = lhs_->type();
}


/*
 * Unary Operators
 */

UnaryOp* UnaryOp::New(int op, Expr* operand, QualType type) {
  auto ret = new (unaryop_pool.Alloc()) UnaryOp(op, operand, type);
  ret->TypeChecking();
  return ret;
}


bool UnaryOp::IsLVal() {
  // Only deref('*') could be lvalue;
  // So it's only deref will override this func
  switch (op_) {
  case Token::DEREF: return !type()->ToArray();
  default: return false;
  }
}


ArithmType* UnaryOp::Convert() {
  auto arithm_type = operand_->type()->ToArithm();
  assert(arithm_type); 
  if (arithm_type->IsInteger())
    arithm_type = ArithmType::IntegerPromote(arithm_type);
  operand_ = Expr::MayCast(operand_, arithm_type);
  return arithm_type;
}


void UnaryOp::TypeChecking() {
  switch (op_) {
  case Token::POSTFIX_INC:
  case Token::POSTFIX_DEC:
  case Token::PREFIX_INC:
  case Token::PREFIX_DEC:
    return IncDecOpTypeChecking();

  case Token::ADDR:
    return AddrOpTypeChecking();

  case Token::DEREF:
    return DerefOpTypeChecking();

  case Token::PLUS:
  case Token::MINUS:
  case '~':
  case '!':
    return UnaryArithmOpTypeChecking();

  case Token::CAST:
    return CastOpTypeChecking();

  default:
    assert(false);
  }
}


void UnaryOp::IncDecOpTypeChecking() {
  if (!operand_->IsLVal()) {
    Error(this, "lvalue expression expected");
  } else if (operand_->IsConstQualified()) {
    Error(this, "increment/decrement of const qualified expression");
  }

  if (!operand_->type()->IsReal() && !operand_->type()->ToPointer()) {
    Error(this, "expect operand of real type or pointer");
  }
  type_ = operand_->type();
}


void UnaryOp::AddrOpTypeChecking() {
  auto func_type = operand_->type()->ToFunc();
  if (func_type == nullptr && !operand_->IsLVal())
    Error(this, "expression must be an lvalue or function designator");
  type_ = PointerType::New(operand_->type());
}


void UnaryOp::DerefOpTypeChecking() {
  auto pointer_type = operand_->type()->ToPointer();
  if (!pointer_type)
    Error(this, "pointer expected for deref operator '*'");
  type_ = pointer_type->derived();    
}


void UnaryOp::UnaryArithmOpTypeChecking() {
  if (Token::PLUS == op_ || Token::MINUS == op_) {
    if (!operand_->type()->ToArithm())
      Error(this, "Arithmetic type expected");
    Convert();
    type_ = operand_->type();
  } else if ('~' == op_) {
    if (!operand_->type()->IsInteger())
      Error(this, "integer expected for operator '~'");
    Convert();
    type_ = operand_->type();
  } else if (!operand_->type()->IsScalar()) {
    Error(this, "arithmetic type or pointer expected for operator '!'");
  } else {
    type_ = ArithmType::New(T_INT);
  }
}


void UnaryOp::CastOpTypeChecking() {
  auto operand_type = Type::MayCast(operand_->type());
    
  // The type_ has been initiated to dest type
  if (type_->ToVoid()) {
    // The expression becomes a void expression
  } else if (!type_->IsScalar() || !operand_type->IsScalar()) {
    Error(this, "the cast type should be arithemetic type or pointer");
  } else if (type_->IsFloat() && operand_type->ToPointer()) {
    Error(this, "cannot cast a pointer to floating");
  } else if (type_->ToPointer() && operand_type->IsFloat()) {
    Error(this, "cannot cast a floating to pointer");
  }
}


/*
 * Conditional Operator
 */

ConditionalOp* ConditionalOp::New(const Token* tok, Expr* cond,
                                  Expr* expr_true, Expr* expr_false) {
  auto ret = new (conditionalop_pool.Alloc())
             ConditionalOp(cond, expr_true, expr_false);
  ret->TypeChecking();
  return ret;
}


ArithmType* ConditionalOp::Convert() {
  auto lhs_type = expr_true_->type()->ToArithm();
  auto rhs_type = expr_false_->type()->ToArithm();
  assert(lhs_type && rhs_type);
  auto type = ArithmType::MaxType(lhs_type, rhs_type);
  if (lhs_type != type) { // Pointer comparation is enough!
    expr_true_ = UnaryOp::New(Token::CAST, expr_true_, type);
  }
  if (rhs_type != type) {
    expr_false_ = UnaryOp::New(Token::CAST, expr_false_, type);
  }
  
  return type;
}


void ConditionalOp::TypeChecking() {
  if (!cond_->type()->IsScalar()) {
    Error(cond_->tok(), "scalar is required");
  }

  auto lhs_type = expr_true_->type();
  auto rhs_type = expr_false_->type();
  if (lhs_type->ToArithm() && rhs_type->ToArithm()) {
    type_ = Convert();
  } else {
    EnsureCompatibleOrVoidPointer(lhs_type, rhs_type);
    type_ = lhs_type;
  }
}


/*
 * Function Call
 */

FuncCall* FuncCall::New(Expr* designator, const ArgList& args) {
  auto ret = new (funccall_pool.Alloc()) FuncCall(designator, args);
  ret->TypeChecking();
  return ret;
}


void FuncCall::TypeChecking() {
  auto pointer_type = designator_->type()->ToPointer();
  if (pointer_type) {
    if (!pointer_type->derived()->ToFunc())
      Error(designator_, "called object is not a function or function pointer");
    // Convert function pointer to function type
    designator_ = UnaryOp::New(Token::DEREF, designator_);
  }
  auto func_type = designator_->type()->ToFunc();
  if (!func_type) {
    Error(designator_, "called object is not a function or function pointer");
  }

  auto arg = args_.begin();
  for (auto param: func_type->param_list()) {
    if (arg == args_.end())
      Error(this, "too few arguments for function call");
    *arg = Expr::MayCast(*arg, param->type());
    ++arg;
  }
  if (arg != args_.end() && !func_type->Variadic())
    Error(this, "too many arguments for function call");

  // C11 6.5.2.2 [6]: promote float to double if it has no prototype
  while (arg != args_.end()) {
    if ((*arg)->type()->IsFloat() && (*arg)->type()->width() == 4) {
      auto type = ArithmType::New(T_DOUBLE);
      *arg = UnaryOp::New(Token::CAST, *arg, type);
    }
    ++arg;
  }

  type_ = func_type->derived();
}


/*
 * Identifier
 */

Identifier* Identifier::New(const Token* tok,
                            QualType type,
                            enum Linkage linkage) {
  auto ret = new (identifier_pool.Alloc()) Identifier(tok, type, linkage);
  return ret;
}


Enumerator* Enumerator::New(const Token* tok, int val) {
  auto ret = new (enumerator_pool.Alloc()) Enumerator(tok, val);
  return ret;
}


Declaration* Declaration::New(Object* obj) {
  auto ret = new (initialization_pool.Alloc()) Declaration(obj);
  return ret;
}

void Declaration::AddInit(Initializer init) {
  init.expr = Expr::MayCast(init.expr, init.type);

  auto res = init_list_.insert(init);
  if (!res.second) {
    init_list_.erase(res.first);
    init_list_.insert(init);
  }
}


/*
 * Object
 */

Object* Object::New(const Token* tok, QualType type,
                    int storage, enum Linkage linkage,
                    uint8_t bitfield_begin,
                    uint8_t bitfield_width) {
  auto ret = new (object_pool.Alloc())
             Object(tok, type, storage, linkage, bitfield_begin, bitfield_width);

  static long id = 0;
  if (ret->IsStatic() || ret->anonymous())
    ret->id_ = ++id;
  return ret;
}


Object* Object::NewAnony(const Token* tok, QualType type,
                         int storage, enum Linkage linkage,
                         uint8_t bitfield_begin,
                         uint8_t bitfield_width) {
  auto ret = new (object_pool.Alloc())
             Object(tok, type, storage, linkage, bitfield_begin, bitfield_width);
  ret->anonymous_ = true;

  static long id = 0;
  if (ret->IsStatic() || ret->anonymous_)
    ret->id_ = ++id;
  return ret;
}


/*
 *  ASTConstant
 */

 ASTConstant*  ASTConstant::New(const Token* tok, int tag, long val) {
  auto type = ArithmType::New(tag);
  auto ret = new (constant_pool.Alloc())  ASTConstant(tok, type, val);
  return ret;
}


 ASTConstant*  ASTConstant::New(const Token* tok, int tag, double val) {
  auto type = ArithmType::New(tag);
  auto ret = new (constant_pool.Alloc())  ASTConstant(tok, type, val);
  return ret;
}


 ASTConstant*  ASTConstant::New(const Token* tok, int tag, const std::string* val) {
  auto derived = ArithmType::New(tag);
  auto type = ArrayType::New(val->size() / derived->width(), derived);
  auto ret = new (constant_pool.Alloc())  ASTConstant(tok, type, val);

  static long id = 0;
  ret->id_ = ++id;
  return ret;
}


std::string  ASTConstant::SValRepr() const {
  std::vector<char> buf(4 * sval_->size() + 1);
  for (size_t i = 0; i < sval_->size(); ++i) {
    int c = (*sval_)[i];
    sprintf(&buf[i * 4], "\\x%1x%1x", (c >> 4) & 0xf, c & 0xf);
  }
  return std::string(buf.begin(), buf.end() - 1);
}


/*
 * TempVar
 */

TempVar* TempVar::New(QualType type) {
  return new (tempvar_pool.Alloc()) TempVar(type);
}


/*
 * Statement
 */

EmptyStmt* EmptyStmt::New() {
  return new (emptystmt_pool.Alloc()) EmptyStmt();
}


// The else stmt could be null
IfStmt* IfStmt::New(Expr* cond, Stmt* then, Stmt* els) {
  return new (ifstmt_pool.Alloc()) IfStmt(cond, then, els);
}

ForStmt* ForStmt::New(CompoundStmt* decl, Expr* init,
                      Expr* cond, Expr* step, Stmt* body) {
  return new (forstmt_pool.Alloc()) ForStmt(decl, init, cond, step, body);
}


WhileStmt* WhileStmt::New(Expr* cond, Stmt* body, bool do_while) {
  return new (whilestmt_pool.Alloc()) WhileStmt(cond, body, do_while);
}


SwitchStmt* SwitchStmt::New(Expr* select, Stmt* body) {
  return new (switchstmt_pool.Alloc()) SwitchStmt(select, body);
}


bool SwitchStmt::IsCaseOverlapped(CaseStmt* case_stmt) {
  auto begin = case_stmt->begin();
  auto end = case_stmt->end();
  for (const auto& c: case_list_) {
    if ((c->begin() <= begin && begin <= c->end()) ||
        (c->begin() <= end && end <= c->end())) {
      return true;
    }
  }
  return false;
}


CaseStmt* CaseStmt::New(int begin, int end, Stmt* body) {
  return new (casestmt_pool.Alloc()) CaseStmt(begin, end, body);
}





CompoundStmt* CompoundStmt::New(const StmtList& stmts, Scope* scope) {
  return new (compoundstmt_pool.Alloc()) CompoundStmt(stmts, scope);
}


ReturnStmt* ReturnStmt::New(Expr* expr) {
  return new (returnstmt_pool.Alloc()) ReturnStmt(expr);
}


GotoStmt* GotoStmt::New(LabelStmt* label) {
  return new (gotostmt_pool.Alloc()) GotoStmt(label);
}


DefaultStmt* DefaultStmt::New(Stmt* sub_stmt) {
  return new (defaultstmt_pool.Alloc()) DefaultStmt(sub_stmt);
}


LabelStmt* LabelStmt::New(Stmt* sub_stmt) {
  return new (labelstmt_pool.Alloc()) LabelStmt(sub_stmt);
}


BreakStmt* BreakStmt::New() {
  return new (breakstmt_pool.Alloc()) BreakStmt();
}

ContinueStmt* ContinueStmt::New() {
  return new (continuestmt_pool.Alloc()) ContinueStmt();
}


FuncDef* FuncDef::New(Identifier* ident) {
  return new (funcdef_pool.Alloc()) FuncDef(ident);
}


TranslationUnit* TranslationUnit::New() {
  return new (translationunit_pool.Alloc()) TranslationUnit();
}


bool Initializer::operator<(const Initializer& rhs) const {
  if (offset < rhs.offset)
    return true;
  return (offset == rhs.offset && bitfield_begin < rhs.bitfield_begin);
}
