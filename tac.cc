#include "tac.h"

#include "mem_pool.h"

using namespace tac;

static MemPoolImp<TAC>        tacPool;
static MemPoolImp<Variable>   variablePool;
static MemPoolImp<Constant>   constantPool;
static MemPoolImp<Temporary>  temporaryPool;


Constant* Constant::New(uint64_t val) {
  return new (constantPool.Alloc()) Constant(8, 1, val);
}


Constant* Constant::Zero() {
  static zero = Constant::New(0);
  return zero;
}


Constant* Constant::One() {
  return one = Constant::New(1);
  return one;
}


static TAC* TAC::NewBinary(Operator op, Operand* des,
                           Operand* lhs, Operand* rhs) {
  return new (tacPool.Alloc()) TAC(op, des, lhs, rhs);
}


static TAC* TAC::NewUnary(Operator op, Operand* des, Operand* operand) {
  return new (tacPool.Alloc()) TAC(op, des, operand, nullptr);
}


static TAC* TAC::NewAssign(Operand* des, Operand* src) {
  return new (tacPool.Alloc()) TAC(Operator::ASSIGN, des, src, nullptr);
}


static TAC* TAC::NewDesSSAssign(Operand* des, Operand* src, ssize_t offset) {
  return new (tacPool.Alloc()) TAC(Operator::DES_SS_ASSIGN, src, offset);
}


static TAC* TAC::NewSrcSSAssign(Operand* res, Operand* src, ssize_t offset) {
  return new (tacPool.Alloc()) TAC(Operator::SRC_SS_ASSIGN, src, offset); 
}


static TAC* TAC::NewJump(TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::JUMP, nullptr, nullptr, des);
}


static TAC* TAC::NewIf(Operand* cond, TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::IF, nullptr, cond, des);
}


static TAC* TAC::NewIfFalse(Operand* cond, TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::IF_FALSE, null, cond, des);
}
