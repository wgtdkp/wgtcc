#include "ast.h"

#include "code_gen.h"
#include "error.h"
#include "mem_pool.h"
#include "parser.h"
#include "token.h"
#include "evaluator.h"


static MemPoolImp<BinaryOp>         binaryOpPool;
static MemPoolImp<ConditionalOp>    conditionalOpPool;
static MemPoolImp<FuncCall>         funcCallPool;
static MemPoolImp<Declaration>   initializationPool;
static MemPoolImp<Object>           objectPool;
static MemPoolImp<Identifier>       identifierPool;
static MemPoolImp<Enumerator>       enumeratorPool;
static MemPoolImp<Constant>         constantPool;
static MemPoolImp<TempVar>          tempVarPool;
static MemPoolImp<UnaryOp>          unaryOpPool;
static MemPoolImp<EmptyStmt>        emptyStmtPool;
static MemPoolImp<IfStmt>           ifStmtPool;
static MemPoolImp<JumpStmt>         jumpStmtPool;
static MemPoolImp<ReturnStmt>       returnStmtPool;
static MemPoolImp<LabelStmt>        labelStmtPool;
static MemPoolImp<CompoundStmt>     compoundStmtPool;
static MemPoolImp<FuncDef>          funcDefPool;



/*
 * Accept
 */

void Declaration::Accept(Visitor* v)
{
  v->VisitDeclaration(this);
}

void EmptyStmt::Accept(Visitor* v) {
  //assert(false);
  //v->VisitEmptyStmt(this);
}

void LabelStmt::Accept(Visitor* v) {
  v->VisitLabelStmt(this);
}

void IfStmt::Accept(Visitor* v) {
  v->VisitIfStmt(this);
}

void JumpStmt::Accept(Visitor* v) {
  v->VisitJumpStmt(this);
}

void ReturnStmt::Accept(Visitor* v) {
  v->VisitReturnStmt(this);
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

void Constant::Accept(Visitor* v) {
  v->VisitConstant(this);
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
Expr* Expr::MayCast(Expr* expr)
{
  auto type = Type::MayCast(expr->Type());
  // If the types are equal, no need cast
  if (type != expr->Type()) { // Pointer comparison is enough
    return UnaryOp::New(Token::CAST, expr, type);
  }
  return expr;
}


Expr* Expr::MayCast(Expr* expr, ::Type* desType)
{
  expr = MayCast(expr);
  auto srcType = expr->Type();
  if (desType->ToPointer() && srcType->ToPointer())
    if (desType->IsVoidPointer() || srcType->IsVoidPointer())
      return expr;
  if (!desType->Compatible(*expr->Type()))
    expr = UnaryOp::New(Token::CAST, expr, desType);
  return expr;
}


BinaryOp* BinaryOp::New(const Token* tok, Expr* lhs, Expr* rhs)
{
  return New(tok, tok->tag_, lhs, rhs);
}

BinaryOp* BinaryOp::New(const Token* tok, int op, Expr* lhs, Expr* rhs)
{
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

  auto ret = new (binaryOpPool.Alloc()) BinaryOp(tok, op, lhs, rhs);
  ret->pool_ = &binaryOpPool;
  
  ret->TypeChecking();
  return ret;    
}


ArithmType* BinaryOp::Convert()
{
  // Both lhs and rhs are ensured to be have arithmetic type
  auto lhsType = lhs_->Type()->ToArithm();
  auto rhsType = rhs_->Type()->ToArithm();
  assert(lhsType && rhsType);
  auto type = ArithmType::MaxType(lhsType, rhsType);
  if (lhsType != type) {// Pointer comparation is enough!
    lhs_ = UnaryOp::New(Token::CAST, lhs_, type);
  }
  if (rhsType != type) {
    rhs_ = UnaryOp::New(Token::CAST, rhs_, type);
  }
  
  return type;
}


/*
 * Type checking
 */

void Expr::EnsureCompatibleOrVoidPointer(::Type* lhs, ::Type* rhs) const
{
  if (lhs->ToPointer() && rhs->ToPointer()
      && (lhs->IsVoidPointer() || rhs->IsVoidPointer())) {
    return;
  }
  EnsureCompatible(lhs, rhs);
}

void Expr::EnsureCompatible(::Type* lhs, ::Type* rhs) const
{
  if (!lhs->Compatible(*rhs))
    Error(this, "incompatible types");
}

void BinaryOp::TypeChecking()
{
  switch (op_) {
  case '.':
    return MemberRefOpTypeChecking();

  //case ']':
  //    return SubScriptingOpTypeChecking();

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

void BinaryOp::CommaOpTypeChecking()
{
  type_ = rhs_->Type();
}

void BinaryOp::SubScriptingOpTypeChecking()
{
  auto lhsType = lhs_->Type()->ToPointer();
  if (!lhsType)
    Error(this, "an pointer expected");
  if (!rhs_->Type()->IsInteger())
    Error(this, "the operand of [] should be intger");

  // The type of [] operator is the derived type
  type_ = lhsType->Derived();    
}

void BinaryOp::MemberRefOpTypeChecking()
{
  type_ = rhs_->Type();
}

void BinaryOp::MultiOpTypeChecking()
{
  if (!lhs_->Type()->ToArithm() || !rhs_->Type()->ToArithm())
    Error(this, "operands should have arithmetic type");
  if ('%' == op_ &&
      !(lhs_->Type()->IsInteger() && rhs_->Type()->IsInteger())) {
    Error(this, "operands of '%%' should be integers");
  }
  type_ = Convert();
}

/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *     1. lhs of MINUS operator, and rhs must be integer or pointer;
 *     2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking()
{
  auto lhsType = lhs_->Type()->ToPointer();
  auto rhsType = rhs_->Type()->ToPointer();
  if (lhsType) {
    if (op_ == '-') {
      if (rhsType) {
        if (!lhsType->Compatible(*rhsType))
          Error(this, "invalid operands to binary -");
        type_ = ArithmType::New(T_LONG); // ptrdiff_t
      } else if (!rhs_->Type()->IsInteger()) {
        Error(this, "invalid operands to binary -");
      } else {
        type_ = lhsType;
      }
    } else if (!rhs_->Type()->IsInteger()) {
      Error(this, "invalid operands to binary +");
    } else {
      type_ = lhsType;
    }
  } else if (rhsType) {
    if (op_ == '+' && !lhs_->Type()->IsInteger())
      Error(this, "invalid operands to binary '+'");
    else if (op_ == '-' && !lhsType)
      Error(this, "invalid operands to binary '-'");
    type_ = op_ == '-' ? ArithmType::New(T_LONG): rhs_->Type();
    std::swap(lhs_, rhs_); // To simplify code gen
  } else {
    if (!lhs_->Type()->ToArithm() || !rhs_->Type()->ToArithm())
      Error(this, "invalid operands to binary %s", tok_->str_.c_str());
    type_ = Convert();
  }
}

void BinaryOp::ShiftOpTypeChecking()
{
  auto lhsType = lhs_->Type()->ToArithm();
  auto rhsType = rhs_->Type()->ToArithm();
  if (!lhsType || !lhsType->IsInteger() || !rhsType || !rhsType->IsInteger())
    Error(this, "expect integers for shift operator");
  lhs_ = Expr::MayCast(lhs_, ArithmType::IntegerPromote(lhsType));
  rhs_ = Expr::MayCast(rhs_, ArithmType::IntegerPromote(rhsType));
  type_ = lhs_->Type();
}

void BinaryOp::RelationalOpTypeChecking()
{
  if (lhs_->Type()->ToPointer() || rhs_->Type()->ToPointer()) {
    EnsureCompatible(lhs_->Type(), rhs_->Type());
  } else {
    if (!lhs_->Type()->IsReal() || !rhs_->Type()->IsReal()) {
      Error(this, "expect real type of operands");
    }
    Convert();
  } 
  type_ = ArithmType::New(T_INT);
}

void BinaryOp::EqualityOpTypeChecking()
{
  if (lhs_->Type()->ToPointer() || rhs_->Type()->ToPointer()) {
    EnsureCompatibleOrVoidPointer(lhs_->Type(), rhs_->Type());
  } else {
    if (!lhs_->Type()->ToArithm() || !rhs_->Type()->ToArithm())
      Error(this, "invalid operands to binary %s", tok_->str_.c_str());
    Convert();
  }

  type_ = ArithmType::New(T_INT);
}

void BinaryOp::BitwiseOpTypeChecking()
{
  if (!lhs_->Type()->IsInteger() || !rhs_->Type()->IsInteger())
    Error(this, "operands of '&' should be integer");
  type_ = Convert();
}

void BinaryOp::LogicalOpTypeChecking()
{
  if (!lhs_->Type()->IsScalar() || !rhs_->Type()->IsScalar())
    Error(this, "the operand should be arithmetic type or pointer");
  type_ = ArithmType::New(T_INT);
}

void BinaryOp::AssignOpTypeChecking()
{
  if (!lhs_->IsLVal()) {
    Error(lhs_->Tok(), "lvalue expression expected");
  }/* else if (lhs_->Type()->IsConst()) {
    Error(lhs_->Tok(), "cannot modifiy 'const' qualified expression");
  } */
  
  if (!lhs_->Type()->ToArithm() || !rhs_->Type()->ToArithm()) {
    EnsureCompatibleOrVoidPointer(lhs_->Type(), rhs_->Type());
  }
  
  // The other constraints are lefted to cast operator
  rhs_ = Expr::MayCast(rhs_, lhs_->Type());
  type_ = lhs_->Type();
}


/*
 * Unary Operators
 */

UnaryOp* UnaryOp::New(int op, Expr* operand, ::Type* type)
{
  auto ret = new (unaryOpPool.Alloc()) UnaryOp(op, operand, type);
  ret->pool_ = &unaryOpPool;
  
  ret->TypeChecking();
  return ret;
}

bool UnaryOp::IsLVal() {
  // only deref('*') could be lvalue;
  // so it's only deref will override this func
  switch (op_) {
  case Token::DEREF: return !Type()->ToArray();
  //case Token::CAST: return operand_->IsLVal();
  default: return false;
  }
}


ArithmType* UnaryOp::Convert()
{
  auto arithmType = operand_->Type()->ToArithm();
  assert(arithmType); 
  if (arithmType->IsInteger())
    arithmType = ArithmType::IntegerPromote(arithmType);
  operand_ = Expr::MayCast(operand_, arithmType);
  return arithmType;
}


void UnaryOp::TypeChecking()
{
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

void UnaryOp::IncDecOpTypeChecking()
{
  if (!operand_->IsLVal()) {
    Error(this, "lvalue expression expected");
  }/* else if (operand_->Type()->IsConst()) {
    Error(this, "cannot modifiy 'const' qualified expression");
  }*/
  if (!operand_->Type()->IsReal() && !operand_->Type()->ToPointer())
    Error(this, "expect operand of real type or pointer");
  type_ = operand_->Type();
}

void UnaryOp::AddrOpTypeChecking()
{
  FuncType* funcType = operand_->Type()->ToFunc();
  if (funcType == nullptr && !operand_->IsLVal())
    Error(this, "expression must be an lvalue or function designator");
  type_ = PointerType::New(operand_->Type());
}

void UnaryOp::DerefOpTypeChecking()
{
  auto pointerType = operand_->Type()->ToPointer();
  if (!pointerType)
    Error(this, "pointer expected for deref operator '*'");
  type_ = pointerType->Derived();    
}

void UnaryOp::UnaryArithmOpTypeChecking()
{
  if (Token::PLUS == op_ || Token::MINUS == op_) {
    if (!operand_->Type()->ToArithm())
      Error(this, "Arithmetic type expected");
    Convert();
    type_ = operand_->Type();
  } else if ('~' == op_) {
    if (!operand_->Type()->IsInteger())
      Error(this, "integer expected for operator '~'");
    Convert();
    type_ = operand_->Type();
  } else if (!operand_->Type()->IsScalar()) {
    Error(this, "arithmetic type or pointer expected for operator '!'");
  } else {
    type_ = ArithmType::New(T_INT);
  }
}


void UnaryOp::CastOpTypeChecking()
{
  auto operandType = Type::MayCast(operand_->Type());
    
  // The type_ has been initiated to dest type
  if (type_->ToVoid()) {
    // The expression becomes a void expression
  } else if (!type_->IsScalar() || !operandType->IsScalar()) {
    Error(this, "the cast type should be arithemetic type or pointer");
  } else if (type_->IsFloat() && operandType->ToPointer()) {
    Error(this, "cannot cast a pointer to floating");
  } else if (type_->ToPointer() && operandType->IsFloat()) {
    Error(this, "cannot cast a floating to pointer");
  }
}


/*
 * Conditional Operator
 */

ConditionalOp* ConditionalOp::New(const Token* tok,
    Expr* cond, Expr* exprTrue, Expr* exprFalse)
{
  auto ret = new (conditionalOpPool.Alloc())
      ConditionalOp(cond, exprTrue, exprFalse);
  ret->pool_ = &conditionalOpPool;

  ret->TypeChecking();
  return ret;
}


ArithmType* ConditionalOp::Convert()
{
  auto lhsType = exprTrue_->Type()->ToArithm();
  auto rhsType = exprFalse_->Type()->ToArithm();
  assert(lhsType && rhsType);
  auto type = ArithmType::MaxType(lhsType, rhsType);
  if (lhsType != type) {// Pointer comparation is enough!
    exprTrue_ = UnaryOp::New(Token::CAST, exprTrue_, type);
  }
  if (rhsType != type) {
    exprFalse_ = UnaryOp::New(Token::CAST, exprFalse_, type);
  }
  
  return type;
}

void ConditionalOp::TypeChecking()
{
  if (!cond_->Type()->IsScalar()) {
    Error(cond_->Tok(), "scalar is required");
  }

  auto lhsType = exprTrue_->Type();
  auto rhsType = exprFalse_->Type();
  if (lhsType->ToArithm() && rhsType->ToArithm()) {
    type_ = Convert();
  } else {
    EnsureCompatibleOrVoidPointer(lhsType, rhsType);
    type_ = lhsType;
  }
}


/*
 * Function Call
 */

FuncCall* FuncCall::New(Expr* designator, const ArgList& args)
{
  auto ret = new (funcCallPool.Alloc()) FuncCall(designator, args);
  ret->pool_ = &funcCallPool;

  ret->TypeChecking();
  return ret;
}


void FuncCall::TypeChecking()
{
  auto pointerType = designator_->Type()->ToPointer();
  if (pointerType) {
    if (!pointerType->Derived()->ToFunc())
      Error(designator_, "called object is not a function or function pointer");
    // Convert function pointer to function type
    designator_ = UnaryOp::New(Token::DEREF, designator_);
  }
  auto funcType = designator_->Type()->ToFunc();
  if (!funcType) {
    Error(designator_, "called object is not a function or function pointer");
  }

  auto i = 1;
  auto arg = args_.begin();
  for (auto param: funcType->Params()) {
    if (arg == args_.end())
      Error(this, "too few arguments for function call");
    *arg = Expr::MayCast(*arg, param->Type());
    ++arg;
    ++i;
  }
  if (arg != args_.end() && !funcType->Variadic())
    Error(this, "too many arguments for function call");

  // C11 6.5.2.2 [6]: promote float to double if it has no prototype
  while (arg != args_.end()) {
    if ((*arg)->Type()->IsFloat() && (*arg)->Type()->Width() == 4) {
      auto type = ArithmType::New(T_DOUBLE);
      *arg = UnaryOp::New(Token::CAST, *arg, type);
    }
    ++arg;
  }

  type_ = funcType->Derived();
}


/*
 * Identifier
 */

Identifier* Identifier::New(const Token* tok,
    ::Type* type, enum Linkage linkage)
{
  auto ret = new (identifierPool.Alloc()) Identifier(tok, type, linkage);
  ret->pool_ = &identifierPool;
  return ret;
}


Enumerator* Enumerator::New(const Token* tok, int val)
{
  auto ret = new (enumeratorPool.Alloc()) Enumerator(tok, val);
  ret->pool_ = &enumeratorPool;
  return ret;
}


Declaration* Declaration::New(Object* obj)
{
  auto ret = new (initializationPool.Alloc()) Declaration(obj);
  ret->pool_ = &initializationPool;
  return ret;
}

void Declaration::AddInit(Initializer init)
{
  init.expr_ = Expr::MayCast(init.expr_, init.type_);

  auto res = inits_.insert(init);
  if (!res.second) {
    inits_.erase(res.first);
    inits_.insert(init);
  }
}


/*
 * Object
 */

Object* Object::New(const Token* tok, ::Type* type, 
    int storage, enum Linkage linkage,
    unsigned char bitFieldBegin, unsigned char bitFieldWidth)
{
  auto ret = new (objectPool.Alloc())
      Object(tok, type, storage, linkage, bitFieldBegin, bitFieldWidth);
  ret->pool_ = &objectPool;

  static long id = 0;
  if (ret->IsStatic() || ret->Anonymous())
    ret->id_ = ++id;

  return ret;
}

/*
Object* Object::Copy(const Object& other)
{
  auto ret = new (ObjectPool.Alloc()) Object();
  *ret = other;
  return ret;
}
*/

/*
 * Constant
 */

Constant* Constant::New(const Token* tok, int tag, long val)
{
  auto type = ArithmType::New(tag);
  auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
  ret->pool_ = &constantPool;
  return ret;
}

Constant* Constant::New(const Token* tok, int tag, double val)
{
  auto type = ArithmType::New(tag);
  auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
  ret->pool_ = &constantPool;
  return ret;
}

Constant* Constant::New(const Token* tok, int tag, const std::string* val)
{
  auto derived = ArithmType::New(tag);
  auto type = ArrayType::New(val->size() / derived->Width(), derived);

  auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
  ret->pool_ = &constantPool;

  static long id = 0;
  ret->id_ = ++id;

  return ret;
}


std::string Constant::SValRepr() const
{
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

TempVar* TempVar::New(::Type* type)
{
  auto ret = new (tempVarPool.Alloc()) TempVar(type);
  ret->pool_ = &tempVarPool;
  return ret;
}


/*
 * Statement
 */

EmptyStmt* EmptyStmt::New()
{
  auto ret = new (emptyStmtPool.Alloc()) EmptyStmt();
  ret->pool_ = &emptyStmtPool;
  return ret;
}


//else stmt Ĭ���� null
IfStmt* IfStmt::New(Expr* cond, Stmt* then, Stmt* els)
{
  auto ret = new (ifStmtPool.Alloc()) IfStmt(cond, then, els);
  ret->pool_ = &ifStmtPool;
  return ret;
}


CompoundStmt* CompoundStmt::New(std::list<Stmt*>& stmts, ::Scope* scope)
{
  auto ret = new (compoundStmtPool.Alloc()) CompoundStmt(stmts, scope);
  ret->pool_ = &compoundStmtPool;
  return ret;
}


JumpStmt* JumpStmt::New(LabelStmt* label)
{
  auto ret = new (jumpStmtPool.Alloc()) JumpStmt(label);
  ret->pool_ = &jumpStmtPool;
  return ret;
}


ReturnStmt* ReturnStmt::New(Expr* expr)
{
  auto ret = new (returnStmtPool.Alloc()) ReturnStmt(expr);
  ret->pool_ = &returnStmtPool;
  return ret;
}


LabelStmt* LabelStmt::New()
{
  auto ret = new (labelStmtPool.Alloc()) LabelStmt();
  ret->pool_ = &labelStmtPool;
  return ret;
}


FuncDef* FuncDef::New(Identifier* ident, LabelStmt* retLabel)
{
  auto ret = new (funcDefPool.Alloc()) FuncDef(ident, retLabel);
  ret->pool_ = &funcDefPool;

  return ret;
}


bool Initializer::operator<(const Initializer& rhs) const
{
  if (offset_ < rhs.offset_)
    return true;
  return (offset_ == rhs.offset_ && bitFieldBegin_ < rhs.bitFieldBegin_);
}
