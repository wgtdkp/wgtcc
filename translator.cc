#include "translator.h"

TACList TACTranslator::tacList_;


void TACTranslator::VisitBinaryOp(BinaryOp* binary) {
  auto op = binary->op_;

  if (op == '=') {
    return GenAssignOp(binary);
  } else if (op == Token::LOGICAL_AND) {
    return GenAndOp(binary);
  } else if (op == Token::LOGICAL_OR) {
    return GenOrOp(binary);
  } else if (op == '.') {
    return GenMemberRefOp(binary);
  } else if (op == ',') {
    return GenCommaOp(binary);
  } else if (binary->lhs_->Type()->ToPointer() &&
             (op == '+' || op == '-')) {
    return GenPointerArithm(binary);
  }

  auto lhs = TACTranslator().Visit(binary->lhs_);
  auto rhs = TACTranslator().Visit(binary->rhs_);
  tac::Operator tacop;
  switch (op) {

  }
}


void TACTranslator::GenAssignOp(BinaryOp* assign) {
  auto lhs = LValGenerator().Visit(assign->lhs_);
  auto rhs = TACTranslator().Visit(assign->rhs_);
  operand_ = tac::TAC::New(tac::Operator::ASSIGN, lhs, rhs);
  Gen(operand_);
}

// TODO(wgtdkp): elimit construction of labelEnd
void TACTranslator::GenAndOp(BinaryOp* andOp) {
  auto t = tac::Temporary::New(andOp->Type());
  auto lhs = TACTranslator().Visit(andOp->lhs_);
  
  auto labelEnd = tac::TAC::NewLabel();
  auto assignZero = tac::TAC::New(t, tac::Constant::Zero());
  auto assignOne = tac::TAC::New(t, tac::Constant::One());
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignZero, lhs));
  
  auto rhs = TACTranslator().Visit(andOp->rhs_);
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignZero, rhs));
  Gen(assignOne);
  Gen(tac::TAC::New(tac::Operator::JUMP, LabelEnd));
  Gen(assignZero);
  Gen(labelEnd);
}


void TACTranslator::GenOrOp(BinaryOp* orOp) {
  auto t = tac::Temporary::New(orOp->Type());
  auto lhs = TACTranslator().Visit(orOp->lhs_);
  
  auto labelEnd = tac::TAC::NewLabel();
  auto assignZero = tac::TAC::New(t, tac::Constant::Zero());
  auto assignOne = tac::TAC::New(t, tac::Constant::One());
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignOne, lhs));

  auto rhs = TACTranslator().Visit(orOp->rhs_);
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignOne, rhs));
  Gen(assignZero);
  Gen(tac::TAC::New(tac::Operator::JUMP, labelEnd));
  Gen(assignOne);
  Gen(labelEnd);
}


void LValTranslator::VisitBinaryOp(BinaryOp* binaryOp) {
  assert(binary->op_ == '.');
  
}

void VisitUnaryOp(UnaryOp* unaryOp);
void VisitObject(Object* obj);
void VisitIdentifier(Identifier* ident);
