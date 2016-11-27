#include "tac.h"

#include "ast.h"
#include "mem_pool.h"
#include "type.h"


static MemPoolImp<TAC>        tac_pool;
static MemPoolImp<Variable>   variable_pool;
static MemPoolImp<Constant>   constant_pool;
static MemPoolImp<Temporary>  temporary_pool;


Variable* Variable::New(const Object* obj) {
  auto type = obj->type();
  if (obj->IsStatic()) {
    // Only static (including global) variable got a name
    return new (variable_pool.Alloc())
           Variable(type->width(), ToTACOperandType(type), obj->Name());
  } else {
    return new (variable_pool.Alloc())
           Variable(type->width(), ToTACOperandType(type), obj->offset());
  }
}


Variable* Variable::New(const Type* type, Operand* base) {
  return new (variable_pool.Alloc())
         Variable(type->width(), ToTACOperandType(type), base);
}


Variable* Variable::New(const Variable& other) {
  return new (variable_pool.Alloc()) Variable(other);
}


Constant* Constant::New(const ASTConstant* c) {
  auto type = c->type();
  uint64_t val = 0;
  if (type->IsFloat())
    val = (union {uint64_t a; double b;}){.b = c->fval()}.a;
  else
    val = c->ival();
  return new (constant_pool.Alloc()) Constant(type->width(),
                                              ToTACOperandType(type),
                                              val);
}


Temporary* Temporary::New(const Type* type) {
  return new (temporary_pool.Alloc()) Temporary(type->width(),
                                                ToTACOperandType(type));
}


Constant* Constant::Zero() {
  static auto zero = new (temporary_pool.Alloc())
                     Constant(8, OperandType::SIGNED, 0);
  return zero;
}


Constant* Constant::One() {
  static auto one = new (temporary_pool.Alloc())
                    Constant(8, OperandType::SIGNED, 1);
  return one;
}


TAC* TAC::NewBinary(Operator op, Operand* des,
                           Operand* lhs, Operand* rhs) {
  return new (tac_pool.Alloc()) TAC(op, des, lhs, rhs);
}


TAC* TAC::NewUnary(Operator op, Operand* des, Operand* operand) {
  return new (tac_pool.Alloc()) TAC(op, des, operand, nullptr);
}


TAC* TAC::NewAssign(Operand* des, Operand* src) {
  return new (tac_pool.Alloc()) TAC(Operator::ASSIGN, des, src, nullptr);
}


TAC* TAC::NewJump(TAC* des) {
  return new (tac_pool.Alloc()) TAC(Operator::JUMP, nullptr, des);
}


TAC* TAC::NewIf(Operand* cond, TAC* des) {
  return new (tac_pool.Alloc()) TAC(Operator::IF, cond, des);
}


TAC* TAC::NewIfFalse(Operand* cond, TAC* des) {
  return new (tac_pool.Alloc()) TAC(Operator::IF_FALSE, cond, des);
}


OperandType ToTACOperandType(const Type* type) {
  if (type->ToPointer() || type->ToFunc() || type->ToArray())
    return OperandType::UNSIGNED;
  if (type->IsInteger())
    return type->IsUnsigned() ? OperandType::UNSIGNED: OperandType::SIGNED;
  if (type->IsFloat())
    return OperandType::FLOAT;
  return OperandType::AGGREGATE;
}