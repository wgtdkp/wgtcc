#include "tac.h"

#include "mem_pool.h"

using namespace tac;

static MemPoolImp<TAC>        tacPool;
static MemPoolImp<Variable>   variablePool;
static MemPoolImp<Constant>   constantPool;
static MemPoolImp<Temporary>  temporaryPool;


Constant* Constant::New(size_t width, OperandType ty, uint64_t val) {
  return new (constantPool.Alloc()) Constant(width, ty, val);
}


Constant* Constant::Zero() {
  static auto zero = Constant::New(8, OperandType::SIGNED, 0);
  return zero;
}


Constant* Constant::One() {
  static auto one = Constant::New(8, OperandType::SIGNED, 1);
  return one;
}


TAC* TAC::NewBinary(Operator op, Operand* des,
                           Operand* lhs, Operand* rhs) {
  return new (tacPool.Alloc()) TAC(op, des, lhs, rhs);
}


TAC* TAC::NewUnary(Operator op, Operand* des, Operand* operand) {
  return new (tacPool.Alloc()) TAC(op, des, operand, nullptr);
}


TAC* TAC::NewAssign(Operand* des, Operand* src) {
  return new (tacPool.Alloc()) TAC(Operator::ASSIGN, des, src, nullptr);
}


TAC* TAC::NewDesSSAssign(Operand* des, Operand* src, ssize_t offset) {
  return new (tacPool.Alloc()) TAC(Operator::DES_SS_ASSIGN, des, src, offset);
}


TAC* TAC::NewSrcSSAssign(Operand* des, Operand* src, ssize_t offset) {
  return new (tacPool.Alloc()) TAC(Operator::SRC_SS_ASSIGN, des, src, offset); 
}


TAC* TAC::NewJump(TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::JUMP, nullptr, des);
}


TAC* TAC::NewIf(Operand* cond, TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::IF, cond, des);
}


TAC* TAC::NewIfFalse(Operand* cond, TAC* des) {
  return new (tacPool.Alloc()) TAC(Operator::IF_FALSE, cond, des);
}
