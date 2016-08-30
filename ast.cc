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
  if (*desType != *expr->Type())
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
  case ',':
  case '.':
  case '=': 
  case '*':
  case '/':
  case '%':
  case '+':
  case '-':
  case '&':
  case '^':
  case '|':
  case '<':
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


ArithmType* BinaryOp::Promote(void)
{
  // Both lhs and rhs are ensured to be have arithmetic type
  auto lhsType = lhs_->Type()->ToArithmType();
  auto rhsType = rhs_->Type()->ToArithmType();
  
  auto type = MaxType(lhsType, rhsType);
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

void BinaryOp::TypeChecking(void)
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

void BinaryOp::CommaOpTypeChecking(void)
{
  type_ = rhs_->Type();
}

void BinaryOp::SubScriptingOpTypeChecking(void)
{
  auto lhsType = lhs_->Type()->ToPointerType();
  if (nullptr == lhsType) {
    Error(tok_, "an pointer expected");
  }
  if (!rhs_->Type()->IsInteger()) {
    Error(tok_, "the operand of [] should be intger");
  }

  // The type of [] operator is the derived type
  type_ = lhsType->Derived();    
}

void BinaryOp::MemberRefOpTypeChecking(void)
{
  type_ = rhs_->Type();
}

void BinaryOp::MultiOpTypeChecking(void)
{
  auto lhsType = lhs_->Type()->ToArithmType();
  auto rhsType = rhs_->Type()->ToArithmType();
  if (lhsType == nullptr || rhsType == nullptr) {
    Error(tok_, "operands should have arithmetic type");
  }

  if ('%' == op_ && 
      !(lhs_->Type()->IsInteger()
      && rhs_->Type()->IsInteger())) {
    Error(tok_, "operands of '%%' should be integers");
  }

  //TODO: type promotion
  type_ = Promote();
}

/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *     1. lhs of MINUS operator, and rhs must be integer or pointer;
 *     2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking(void)
{
  auto lhsType = lhs_->Type()->ToPointerType();
  auto rhsType = rhs_->Type()->ToPointerType();
  if (lhsType) {
    if (op_ == Token::MINUS) {
      if ((rhsType && *lhsType != *rhsType)
          || !rhs_->Type()->IsInteger()) {
        Error(tok_, "invalid operands to binary -");
      }
      type_ = ArithmType::New(T_LONG); // ptrdiff_t
    } else if (!rhs_->Type()->IsInteger()) {
      Error(tok_, "invalid operands to binary -");
    } else {
      type_ = lhs_->Type();
    }
  } else if (rhsType) {
    if (op_ != Token::ADD || !lhs_->Type()->IsInteger()) {
      Error(tok_, "invalid operands to binary '%s'",
          tok_->str_.c_str());
    }
    type_ = rhs_->Type();
    std::swap(lhs_, rhs_); // To simplify code gen
  } else {
    auto lhsType = lhs_->Type()->ToArithmType();
    auto rhsType = rhs_->Type()->ToArithmType();
    if (lhsType == nullptr || rhsType == nullptr) {
      Error(tok_, "invalid operands to binary %s",
          tok_->str_.c_str());
    }

    type_ = Promote();
  }
}

void BinaryOp::ShiftOpTypeChecking(void)
{
  if (!lhs_->Type()->IsInteger() || !rhs_->Type()->IsInteger()) {
    Error(tok_, "expect integers for shift operator '%s'",
        tok_->str_);
  }

  Promote();
  type_ = lhs_->Type();
}

void BinaryOp::RelationalOpTypeChecking(void)
{
  if (!lhs_->Type()->IsReal() || !rhs_->Type()->IsReal()) {
    Error(tok_, "expect integer/float"
        " for relational operator '%s'", tok_->str_);
  }

  Promote();
  type_ = ArithmType::New(T_INT);    
}

void BinaryOp::EqualityOpTypeChecking(void)
{
  auto lhsType = lhs_->Type()->ToPointerType();
  auto rhsType = rhs_->Type()->ToPointerType();
  if (lhsType || rhsType) {
    if (!lhsType->Compatible(*rhsType)) {
      Error(tok_, "incompatible pointers of operands");
    }
  } else {
    auto lhsType = lhs_->Type()->ToArithmType();
    auto rhsType = rhs_->Type()->ToArithmType();
    if (lhsType == nullptr || rhsType == nullptr) {
      Error(tok_, "invalid operands to binary %s",
          tok_->str_);
    }

    Promote();
  }

  type_ = ArithmType::New(T_INT);    
}

void BinaryOp::BitwiseOpTypeChecking(void)
{
  if (!lhs_->Type()->IsInteger() || !rhs_->Type()->IsInteger()) {
    Error(tok_, "operands of '&' should be integer");
  }
  
  type_ = Promote();
}

void BinaryOp::LogicalOpTypeChecking(void)
{
  if (!lhs_->Type()->IsScalar() || !rhs_->Type()->IsScalar()) {
    Error(tok_, "the operand should be arithmetic type or pointer");
  }
  
  Promote();
  type_ = ArithmType::New(T_INT);
}

void BinaryOp::AssignOpTypeChecking(void)
{
  if (!lhs_->IsLVal()) {
    Error(lhs_->Tok(), "lvalue expression expected");
  } else if (lhs_->Type()->IsConst()) {
    Error(lhs_->Tok(), "can't modifiy 'const' qualified expression");
  } else if (!lhs_->Type()->Compatible(*rhs_->Type())) {
    Error(lhs_->Tok(), "uncompatible types");
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

bool UnaryOp::IsLVal(void) {
  // only deref('*') could be lvalue;
  // so it's only deref will override this func
  switch (op_) {
  case Token::DEREF: return !Type()->ToArrayType();
  //case Token::CAST: return operand_->IsLVal();
  default: return false;
  }
}


ArithmType* UnaryOp::Promote(void)
{
  //
  auto arithmType = operand_->Type()->ToArithmType(); 
  auto type = MaxType(arithmType, arithmType);
  if (type != arithmType) {
    operand_ = UnaryOp::New(Token::CAST, operand_, type);
  }
  return type;
}


void UnaryOp::TypeChecking(void)
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

void UnaryOp::IncDecOpTypeChecking(void)
{
  if (!operand_->IsLVal()) {
    Error(tok_, "lvalue expression expected");
  } else if (operand_->Type()->IsConst()) {
    Error(tok_, "can't modifiy 'const' qualified expression");
  }

  type_ = operand_->Type();
}

void UnaryOp::AddrOpTypeChecking(void)
{
  FuncType* funcType = operand_->Type()->ToFuncType();
  if (funcType == nullptr && !operand_->IsLVal()) {
    Error(tok_, "expression must be an lvalue or function designator");
  }
  
  type_ = PointerType::New(operand_->Type());
}

void UnaryOp::DerefOpTypeChecking(void)
{
  auto pointerType = operand_->Type()->ToPointerType();
  if (pointerType == nullptr) {
    Error(tok_, "pointer expected for deref operator '*'");
  }

  type_ = pointerType->Derived();    
}

void UnaryOp::UnaryArithmOpTypeChecking(void)
{
  if (Token::PLUS == op_ || Token::MINUS == op_) {
    if (!operand_->Type()->IsArithm()) {
      Error(tok_, "Arithmetic type expected");
    }
    Promote();
    type_ = operand_->Type();
  } else if ('~' == op_) {
    if (!operand_->Type()->IsInteger()) {
      Error(tok_, "integer expected for operator '~'");
    }
    Promote();
    type_ = operand_->Type();
  } else if (!operand_->Type()->IsScalar()) {
    Error(tok_, "arithmetic type or pointer expected for operator '!'");
  } else {
    type_ = ArithmType::New(T_INT);
  }
}


void UnaryOp::CastOpTypeChecking(void)
{
  // The type_ has been initiated to dest type
  if (type_->ToVoidType()) {
    // The expression becomes a void expression
  } else if (type_->ToPointerType() && operand_->Type()->ToArrayType()) {
  } else if (type_->ToPointerType() && operand_->Type()->ToFuncType()) {
  } else if (!type_->IsScalar() || !operand_->Type()->IsScalar()) {
    Error(tok_, "the cast type should be arithemetic type or pointer");
  } else if (type_->IsFloat() && operand_->Type()->ToPointerType()) {
    Error(tok_, "can't cast a pointer to floating");
  } else if (type_->ToPointerType() && operand_->Type()->IsFloat()) {
    Error(tok_, "can't cast a floating to pointer");
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


ArithmType* ConditionalOp::Promote(void)
{
  auto lhsType = exprTrue_->Type()->ToArithmType();
  auto rhsType = exprFalse_->Type()->ToArithmType();
  
  auto type = MaxType(lhsType, rhsType);
  if (lhsType != type) {// Pointer comparation is enough!
    exprTrue_ = UnaryOp::New(Token::CAST, exprTrue_, type);
  }
  if (rhsType != type) {
    exprFalse_ = UnaryOp::New(Token::CAST, exprFalse_, type);
  }
  
  return type;
}

void ConditionalOp::TypeChecking(void)
{
  if (!cond_->Type()->IsScalar()) {
    Error(cond_->Tok(), "scalar is required");
  }

  auto lhsType = exprTrue_->Type();
  auto rhsType = exprFalse_->Type();
  if (!lhsType->Compatible(*rhsType)) {
    Error(exprTrue_->Tok(), "incompatible types of true/false expression");
  } else {
    type_ = lhsType;
  }

  lhsType = lhsType->ToArithmType();
  rhsType = rhsType->ToArithmType();
  if (lhsType && rhsType) {
    type_ = Promote();
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


void FuncCall::TypeChecking(void)
{
  auto pointerType = designator_->Type()->ToPointerType();
  if (pointerType) {
    if (!pointerType->Derived()->ToFuncType()) {
      Error(designator_, "called object is not a function or function pointer");
    }
    // Convert function pointer to function type
    designator_ = UnaryOp::New(Token::DEREF, designator_);
  }
  auto funcType = designator_->Type()->ToFuncType();
  if (!funcType) {
    Error(designator_, "called object is not a function or function pointer");
  }

  const auto& funcName = designator_->Tok()->str_;
  auto arg = args_.begin();
  for (auto paramType: funcType->ParamTypes()) {
    if (arg == args_.end()) {
      Error(tok_, "too few arguments for function '%s'", funcName.c_str());
    }
    if (!paramType->Compatible(*(*arg)->Type())) {
      // TODO(wgtdkp): function name
      Error(tok_, "incompatible type for argument 1 of '%s'", funcName.c_str());
    }
    *arg = Expr::MayCast(*arg, paramType);
    ++arg;
  }
  
  if (arg != args_.end() && !funcType->Variadic()) {
    Error(tok_, "too many arguments for function '%s'", funcName.c_str());
  }

  // c11 6.5.2.2 [6]: promote float to double if it has no prototype
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
  return ret;
}


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
  ret->id_ = id++;

  return ret;
}


std::string Constant::SValRepr(void) const
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

EmptyStmt* EmptyStmt::New(void)
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

LabelStmt* LabelStmt::New(void)
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
