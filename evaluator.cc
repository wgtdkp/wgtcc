#include "evaluator.h"

#include "ast.h"
#include "token.h"


template<typename T>
void Evaluator<T>::VisitBinaryOp(BinaryOp* binary) {
#define L   Evaluator<T>().Eval(binary->lhs())
#define R   Evaluator<T>().Eval(binary->rhs())
#define LL  Evaluator<long>().Eval(binary->lhs())
#define LR  Evaluator<long>().Eval(binary->rhs())

  if (binary->type()->ToPointer()) {
    auto val = Evaluator<Addr>().Eval(binary);
    if (val.label_.size()) {
      Error(binary, "expect constant integer expression");
    }
    val_ = static_cast<T>(val.offset_);
    return;
  }

  switch (binary->op()) {
  case '+': val_ = L + R; break; 
  case '-': val_ = L - R; break;
  case '*': val_ = L * R; break;
  case '/': {
    auto l = L, r = R;
    if (r == 0)
      Error(binary, "division by zero");
    val_ = l / r;
  } break;
  case '%': {
    auto l = LL, r = LR;
    if (r == 0)
      Error(binary, "division by zero");
    val_ = l % r;
  } break;
  // Bitwise operators that do not accept float
  case '|': val_ = LL | LR; break;
  case '&': val_ = LL & LR; break;
  case '^': val_ = LL ^ LR; break;
  case Token::LEFT: val_ = LL << LR; break;
  case Token::RIGHT: val_ = LL >> LR; break;

  case '<': val_ = L < R; break;
  case '>': val_ = L > R; break;
  case Token::LOGICAL_AND: val_ = L && R; break;
  case Token::LOGICAL_OR: val_ = L || R; break;
  case Token::EQ: val_ = L == R; break;
  case Token::NE: val_ = L != R; break;
  case Token::LE: val_ = L <= R; break;
  case Token::GE: val_ = L >= R; break;
  case '=': case ',': val_ = R; break;
  case '.': {
    auto addr = Evaluator<Addr>().Eval(binary);
    if (addr.label_.size())
      Error(binary, "expect constant expression");
    val_ = addr.offset_;
  } 
  default: assert(false);
  }

#undef L
#undef R
#undef LL
#undef LR
}


template<typename T>
void Evaluator<T>::VisitUnaryOp(UnaryOp* unary) {
#define VAL     Evaluator<T>().Eval(unary->operand())
#define LVAL    Evaluator<long>().Eval(unary->operand())

  switch (unary->op()) {
  case Token::PLUS: val_ = VAL; break;
  case Token::MINUS: val_ = -VAL; break;
  case '~': val_ = ~LVAL; break;
  case '!': val_ = !VAL; break;
  case Token::CAST:
    if (unary->type()->IsInteger())
      val_ = static_cast<long>(VAL);
    else
      val_ = VAL;
    break;
  case Token::ADDR: {
    auto addr = Evaluator<Addr>().Eval(unary->operand());
    if (addr.label_.size())
      Error(unary, "expect constant expression");
    val_ = addr.offset_;
  } break;
  default: Error(unary, "expect constant expression");
  }

#undef LVAL
#undef VAL
}


template<typename T>
void Evaluator<T>::VisitConditionalOp(ConditionalOp* cond_op) {
  bool cond;
  auto cond_type = cond_op->cond()->type();
  if (cond_type->IsInteger()) {
    auto val = Evaluator<long>().Eval(cond_op->cond());
    cond = val != 0;
  } else if (cond_type->IsFloat()) {
    auto val = Evaluator<double>().Eval(cond_op->cond());
    cond  = val != 0.0;
  } else if (cond_type->ToPointer()) {
    auto val = Evaluator<Addr>().Eval(cond_op->cond());
    cond = val.label_.size() || val.offset_;
  } else {
    assert(false);
  }

  if (cond) {
    val_ = Evaluator<T>().Eval(cond_op->expr_true());
  } else {
    val_ = Evaluator<T>().Eval(cond_op->expr_false());
  }
}


void Evaluator<Addr>::VisitBinaryOp(BinaryOp* binary) {
#define LR   Evaluator<long>().Eval(binary->rhs())
#define R   Evaluator<Addr>().Eval(binary->rhs())
  
  auto l = Evaluator<Addr>().Eval(binary->lhs());
  
  int width = 1;
  auto pointer_type = binary->type()->ToPointer();
  if (pointer_type)
    width = pointer_type->derived()->width();

  switch (binary->op()) {
  case '+':
    assert(pointer_type);
    addr_.label_ = l.label_;
    addr_.offset_ = l.offset_ + LR * width;
    break;
  case '-':
    assert(pointer_type);
    addr_.label_ = l.label_;
    addr_.offset_ = l.offset_ + LR * width;
    break;
  case '.': {
    addr_.label_ = l.label_;
    auto type = binary->lhs()->type()->ToStruct();
    auto offset = type->GetMember(binary->rhs()->tok()->str())->offset();
    addr_.offset_ = l.offset_ + offset;
    break;
  }
  default: assert(false);
  }
#undef LR
#undef R
}


void Evaluator<Addr>::VisitUnaryOp(UnaryOp* unary) {
  auto addr = Evaluator<Addr>().Eval(unary->operand());

  switch (unary->op()) {
  case Token::CAST:
  case Token::ADDR:
  case Token::DEREF:
    addr_ = addr; break;
  default: assert(false);
  }
}


void Evaluator<Addr>::VisitConditionalOp(ConditionalOp* cond_op) {
  bool cond;
  auto cond_type = cond_op->cond()->type();
  if (cond_type->IsInteger()) {
    auto val = Evaluator<long>().Eval(cond_op->cond());
    cond = val != 0;
  } else if (cond_type->IsFloat()) {
    auto val = Evaluator<double>().Eval(cond_op->cond());
    cond  = val != 0.0;
  } else if (cond_type->ToPointer()) {
    auto val = Evaluator<Addr>().Eval(cond_op->cond());
    cond = val.label_.size() || val.offset_;
  } else {
    assert(false);
  }

  if (cond) {
    addr_ = Evaluator<Addr>().Eval(cond_op->expr_true());
  } else {
    addr_ = Evaluator<Addr>().Eval(cond_op->expr_false());
  }
}


void Evaluator<Addr>::VisitASTConstant(ASTConstant* cons)  {
  if (cons->type()->IsInteger()) {
    addr_ = {"", static_cast<int>(cons->ival())};
  } else if (cons->type()->ToArray()) {
    // TODO(wgtdkp):
    //Generator().ConsLabel(cons); // Add the literal to rodatas_.
    //addr_.label_ = Generator::rodatas_.back().label_;
    addr_.offset_ = 0;
  } else {
    assert(false);
  }
}
