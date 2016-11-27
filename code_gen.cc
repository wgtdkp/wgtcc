#include "code_gen.h"

#include "evaluator.h"
#include "parser.h"
#include "token.h"

#include <cstdarg>
#include <queue>
#include <set>


extern std::string in_filename;
extern std::string out_filename;
extern bool debug;

const std::string* Generator::last_file = nullptr;
Parser* Generator::parser_ = nullptr;
FILE* Generator::out_file_ = nullptr;
RODataList Generator::rodatas_;
std::vector<Declaration*> Generator::static_decl_list_;
int Generator::offset_ = 0;
int Generator::ret_addr_offset_ = 0;
FuncDef* Generator::cur_func_ = nullptr;


/*
 * Register usage:
 *  xmm0: accumulator of floating datas;
 *  xmm8: temp register for param passing(xmm0)
 *  xmm9: source operand register;
 *  xmm10: tmp register for floating data swap;
 *  rax: accumulator;
 *  r12, r13: temp register for rdx and rcx
 *  r11: source operand register;
 *  r10: base register when LValGenerator eval the address.
 *  rcx: tempvar register, like the tempvar of 'switch'
 *       temp register for struct copy
 */

static std::vector<const char*> regs {
  "%rdi", "%rsi", "%rdx",
  "%rcx", "%r8", "%r9"
};

static std::vector<const char*> xregs {
  "%xmm0", "%xmm1", "%xmm2", "%xmm3",
  "%xmm4", "%xmm5", "%xmm6", "%xmm7"
};


static ParamClass Classify(Type* param_type, int offset=0) {
  if (param_type->IsInteger() || param_type->ToPointer()
      || param_type->ToArray()) {
    return ParamClass::INTEGER;
  }
  
  if (param_type->ToArithm()) {
    auto type = param_type->ToArithm();
    if (type->tag() == T_FLOAT || type->tag() == T_DOUBLE)
      return ParamClass::SSE;
    if (type->tag() == (T_LONG | T_DOUBLE)) {
      // TODO(wgtdkp):
      return ParamClass::SSE;
      assert(false); 
      return ParamClass::X87;
    }
    
    // TODO(wgtdkp):
    assert(false);
    // It is complex
    if ((type->tag() & T_LONG) && (type->tag() & T_DOUBLE))
      return ParamClass::COMPLEX_X87;
  }
  auto type = param_type->ToStruct();
  assert(type);
  return ParamClass::MEMORY;
  // TODO(wgtdkp): Support agrregate type 
  assert(false);
  /*
  auto type = param_type->ToStruct();
  assert(type);

  if (type->width() > 4 * 8)
    return PC_MEMORY;

  std::vector<ParamClass> classes;
  int cnt = (type->width() + 7) / 8;
  for (int i = 0; i < cnt; ++i) {
    auto  types = FieldsIn8Bytes(type, i);
    assert(types.size() > 0);
    
    auto fieldClass = (types.size() == 1)
        ? PC_NO_CLASS: FieldClass(types, 0);
    classes.push_back(fieldClass);
    
  }

  bool sawX87 = false;
  for (int i = 0; i < classes.size(); ++i) {
    if (classes[i] == PC_MEMORY)
      return PC_MEMORY;
    if (classes[i] == PC_X87_UP && sawX87)
      return PC_MEMORY;
    if (classes[i] == PC_X87)
      sawX87 = true;
  }
  */
  return ParamClass::NO_CLASS; // Make compiler happy
}


std::string Generator::ConsLabel(ASTConstant* c) {
  if (c->type()->IsInteger()) {
    return "$" + std::to_string(c->ival());
  } else if (c->type()->IsFloat()) {
    double valsd = c->fval();
    float  valss = valsd;
    // TODO(wgtdkp): Add rodata
    auto width = c->type()->width();
    long val = (width == 4)? (union {float valss; int val;}){valss}.val:
                             (union {double valsd; long val;}){valsd}.val;
    const ROData& rodata = ROData(val, width);
    rodatas_.push_back(rodata);
    return rodata.label_;
  } else { // Literal
    const ROData& rodata = ROData(c->SValRepr());
    rodatas_.push_back(rodata);
    return rodata.label_; // return address
  }
}


static const char* GetLoad(int width, bool flt=false) {
  switch (width) {
  case 1: return "movzbq";
  case 2: return "movzwq";
  case 4: return !flt ? "movl": "movss";
  case 8: return !flt ? "movq": "movsd";
  default: assert(false); return nullptr;
  }
}


static std::string GetInst(const std::string& inst, int width, bool flt) {
  if (flt)  {
    return inst + (width == 4 ? "ss": "sd");
  } else {
    switch (width) {
    case 1: return inst + "b";
    case 2: return inst + "w";
    case 4: return inst + "l";
    case 8: return inst + "q";
    default: assert(false);
    }
    return inst; // Make compiler happy
  } 
}


static std::string GetInst(const std::string& inst, Type* type) {
  assert(type->IsScalar());
  return GetInst(inst, type->width(), type->IsFloat());
}


static std::string GetReg(int width) {
  switch (width) {
  case 1: return "%al";
  case 2: return "%ax";
  case 4: return "%eax";
  case 8: return "%rax";
  default: assert(false); return "";
  }
}


static std::string GetDes(int width, bool flt) {
  if (flt) {
    return "%xmm0";
  }
  return GetReg(width);
}


static std::string GetSrc(int width, bool flt) {
  if (flt) {
    return "%xmm9";
  }
  switch (width) {
  case 1: return "%r11b";
  case 2: return "%r11w";
  case 4: return "%r11d";
  case 8: return "%r11";
  default: assert(false); return "";
  }
}


// The 'reg' always be 8 bytes  
int Generator::Push(const std::string& reg) {
  offset_ -= 8;
  auto mov = reg[1] == 'x' ? "movsd": "movq";
  Emit(mov, reg, ObjectAddr(offset_));
  return offset_;
}


int Generator::Push(Type* type) {
  if (type->IsFloat()) {
    return Push("%xmm0");
  } else if (type->IsScalar()) {
    return Push("%rax");
  } else {
    offset_ -= type->width();
    offset_ = Type::MakeAlign(offset_, 8);
    CopyStruct({"", "%rbp", offset_}, type->width());
    return offset_;
  }
}


// The 'reg' must be 8 bytes
int Generator::Pop(const std::string& reg) {
  auto mov = reg[1] == 'x' ? "movsd": "movq";
  Emit(mov, ObjectAddr(offset_), reg);
  offset_ += 8;
  return offset_;
}


void Generator::Spill(bool flt) {
  Push(flt ? "%xmm0": "%rax");
}


void Generator::Restore(bool flt) {
  const auto& src = GetSrc(8, flt);
  const auto& des = GetDes(8, flt);
  const auto& inst = GetInst("mov", 8, flt);
  Emit(inst, des, src);
  Pop(des);
}


void Generator::Save(bool flt) {
  if (flt) {        
    Emit("movsd", "%xmm0", "%xmm9");
  } else {
    Emit("movq", "%rax", "%r11");
  }
}


/*
 * Operator/Instruction mapping:
 * +  add
 * -  sub
 * *  mul
 * /  div
 * %  div
 * << sal
 * >> sar
 * |  or
 * &  and
 * ^  xor
 * =  mov
 * <  cmp, setl, movzbq
 * >  cmp, setg, movzbq
 * <= cmp, setle, movzbq
 * >= cmp, setle, movzbq
 * == cmp, sete, movzbq
 * != cmp, setne, movzbq
 * && GenAndOp
 * || GenOrOp
 * ]  GenSubScriptingOp
 * .  GenMemberRefOp
 */
void Generator::VisitBinaryOp(BinaryOp* binary) {
  EmitLoc(binary);
  auto op = binary->op();

  if (op == '=')
    return GenAssignOp(binary);
  if (op == Token::LOGICAL_AND)
    return GenAndOp(binary);
  if (op == Token::LOGICAL_OR)
    return GenOrOp(binary);
  if (op == '.')
    return GenMemberRefOp(binary);
  if (op == ',')
    return GenCommaOp(binary);
  // Why lhs()->type() ?
  // Because, the type of pointer subtraction is arithmetic type
  if (binary->lhs()->type()->ToPointer() &&
      (op == '+' || op == '-')) {
    return GenPointerArithm(binary);
  }

  // Careful: for compare operator, the type of the expression
  // is always integer, while the type of lhs and rhs could be float
  // After convertion, lhs and rhs always has the same type
  auto type = binary->lhs()->type();
  auto width = type->width();
  auto flt = type->IsFloat();
  auto sign = !type->IsUnsigned();

  Visit(binary->lhs());
  Spill(flt);
  Visit(binary->rhs());
  Restore(flt);

  const char* inst = nullptr;

  switch (op) {
  case '*': return GenMulOp(width, flt, sign); 
  case '/': case '%': return GenDivOp(flt, sign, width, op);
  case '<': 
    return GenCompOp(width, flt, (flt || !sign) ? "setb": "setl");
  case '>':
    return GenCompOp(width, flt, (flt || !sign) ? "seta": "setg");
  case Token::LE:
    return GenCompOp(width, flt, (flt || !sign) ? "setbe": "setle");
  case Token::GE:
    return GenCompOp(width, flt, (flt || !sign) ? "setae": "setge");
  case Token::EQ:
    return GenCompOp(width, flt, "sete");
  case Token::NE:
    return GenCompOp(width, flt, "setne");

  case '+': inst = "add"; break;
  case '-': inst = "sub"; break;
  case '|': inst = "or"; break;
  case '&': inst = "and"; break;
  case '^': inst = "xor"; break;
  case Token::LEFT: case Token::RIGHT:
    inst = op == Token::LEFT ? "sal": (sign ? "sar": "shr");
    Emit("movq %r11, %rcx");
    Emit(GetInst(inst, width, flt), "%cl", GetDes(width, flt));
    return;
  }
  Emit(GetInst(inst, width, flt), GetSrc(width, flt), GetDes(width, flt));
}


void Generator::GenCommaOp(BinaryOp* comma) {
  VisitExpr(comma->lhs());
  VisitExpr(comma->rhs());
}


void Generator::GenMulOp(int width, bool flt, bool sign) {
  auto inst = flt ? "mul": (sign ? "imul": "mul");
  
  if (flt) {
    Emit(GetInst(inst, width, flt), "%xmm9", "%xmm0");
  } else {
    Emit(GetInst(inst, width, flt), GetSrc(width, flt));
  }
}


void Generator::GenCompZero(Type* type) {
  auto width = type->width();
  auto flt = type->IsFloat();
  
  if (!flt) {
    Emit("cmp", "$0", GetReg(width));
  } else {
    Emit("pxor", "%xmm9", "%xmm9");
    auto cmp = width == 8 ? "ucomisd": "ucomiss";
    Emit(cmp, "%xmm9", "%xmm0");
  }
}


void Generator::GenAndOp(BinaryOp* and_op) {
  VisitExpr(and_op->lhs());
  GenCompZero(and_op->lhs()->type());

  auto labelFalse = LabelStmt::New();
  Emit("je", labelFalse);

  VisitExpr(and_op->rhs());
  GenCompZero(and_op->rhs()->type());

  Emit("je", labelFalse);
  
  Emit("movq", "$1", "%rax");
  auto labelTrue = LabelStmt::New();
  Emit("jmp", labelTrue);
  EmitLabel(labelFalse->Repr());
  Emit("xorq", "%rax", "%rax"); // Set %rax to 0
  EmitLabel(labelTrue->Repr());
}


void Generator::GenOrOp(BinaryOp* or_op) {
  VisitExpr(or_op->lhs());
  GenCompZero(or_op->lhs()->type());

  auto labelTrue = LabelStmt::New();
  Emit("jne", labelTrue);

  VisitExpr(or_op->rhs());
  GenCompZero(or_op->rhs()->type());

  Emit("jne", labelTrue);
  
  Emit("xorq", "%rax", "%rax"); // Set %rax to 0
  auto labelFalse = LabelStmt::New();
  Emit("jmp", labelFalse);
  EmitLabel(labelTrue->Repr());
  Emit("movq", "$1", "%rax");
  EmitLabel(labelFalse->Repr());    
}


void Generator::GenMemberRefOp(BinaryOp* ref) {
  // As the lhs will always be struct/union 
  auto addr = LValGenerator().GenExpr(ref->lhs());
  const auto& name = ref->rhs()->tok()->str();
  auto struct_type = ref->lhs()->type()->ToStruct();
  auto member = struct_type->GetMember(name);

  addr.offset_ += member->offset();

  if (!ref->type()->IsScalar()) {
    Emit("leaq", addr, "%rax");
  } else {
    if (member->bitfield_width()) {
      EmitLoadBitField(addr.Repr(), member);
    } else {
      EmitLoad(addr.Repr(), ref->type());
    }
  }
}


void Generator::EmitLoadBitField(const std::string& addr, Object* bitfield) {
  auto type = bitfield->type()->ToArithm();
  assert(type && type->IsInteger());

  EmitLoad(addr, type);
  Emit("andq", Object::BitFieldMask(bitfield), "%rax");

  auto shiftRight = (type->tag() & T_UNSIGNED) ? "shrq": "sarq";    
  auto left = 64 - bitfield->bitfield_begin_ - bitfield->bitfield_width_;
  auto right = 64 - bitfield->bitfield_width_;
  Emit("salq", left, "%rax");
  Emit(shiftRight, right, "%rax");
}


// FIXME(wgtdkp): for combined assignment operator, if the rvalue expr
// has some side-effect, the rvalue will be evaluated twice!
void Generator::GenAssignOp(BinaryOp* assign) {
  // The base register of addr is %r10, %rip, %rbp
  auto addr = LValGenerator().GenExpr(assign->lhs());
  // Base register of static object maybe %rip
  // Visit rhs() may changes r10
  if (addr.base_ == "%r10")
    Push(addr.base_);
  VisitExpr(assign->rhs());
  if (addr.base_ == "%r10")
    Pop(addr.base_);

  if (assign->type()->IsScalar()) {
      EmitStore(addr, assign->type());
  } else {
    // struct/union type
    // The address of rhs is in %rax
    CopyStruct(addr, assign->type()->width());
  }
}


void Generator::EmitStoreBitField(const ObjectAddr& addr, Type* type) {
  auto arithm_type = type->ToArithm();
  assert(arithm_type && arithm_type->IsInteger());

  // The value to be stored is in %rax now
  auto mask = Object::BitFieldMask(addr.bitfield_begin_, addr.bitfield_width_);

  Emit("salq", addr.bitfield_begin_, "%rax");
  Emit("andq", mask, "%rax");
  Emit("movq", "%rax", "%r11");
  EmitLoad(addr.Repr(), arithm_type);
  Emit("andq", ~mask, "%rax");
  Emit("orq", "%r11", "%rax");

  EmitStore(addr.Repr(), type);
}


void Generator::CopyStruct(ObjectAddr desAddr, int width) {
  int units[] = {8, 4, 2, 1};
  Emit("movq", "%rax", "%rcx");
  ObjectAddr srcAddr = {"", "%rcx", 0};
  for (auto unit: units) {
    while (width >= unit) {
      EmitLoad(srcAddr.Repr(), unit, false);
      EmitStore(desAddr.Repr(), unit, false);
      desAddr.offset_ += unit;
      srcAddr.offset_ += unit;
      width -= unit;
    }
  }
}


void Generator::GenCompOp(int width, bool flt, const char* set) {
  std::string cmp;
  if (flt) {
    cmp = width == 8 ? "ucomisd": "ucomiss";
  } else {
    cmp = GetInst("cmp", width, flt);
  }

  Emit(cmp, GetSrc(width, flt), GetDes(width, flt));
  Emit(set, "%al");
  Emit("movzbq", "%al", "%rax");
}


void Generator::GenDivOp(bool flt, bool sign, int width, int op) {
  if (flt) {
    auto inst = width == 4 ? "divss": "divsd";
    Emit(inst, "%xmm9", "%xmm0");
    return;
  }
  if (!sign) {
    Emit("xor", "%rdx", "%rdx");
    Emit(GetInst("div", width, flt), GetSrc(width, flt));
  } else {
    Emit(width == 4 ? "cltd": "cqto");
    Emit(GetInst("idiv", width, flt), GetSrc(width, flt));      
  }
  if (op == '%')
    Emit("movq", "%rdx", "%rax");
}

 
void Generator::GenPointerArithm(BinaryOp* binary) {
  assert(binary->op() == '+' || binary->op() == '-');
  // For '+', we have swapped lhs() and rhs() to ensure that 
  // the pointer is at lhs.
  Visit(binary->lhs());
  Spill(false);
  Visit(binary->rhs());
  Restore(false);

  auto type = binary->lhs()->type()->ToPointer()->derived();
  auto width = type->width();
  if (binary->op() == '+') {
    if (width > 1)
      Emit("imulq", width, "%r11");
    Emit("addq", "%r11", "%rax");
  } else {
    Emit("subq", "%r11", "%rax");
    if (width > 1) {
      Emit("movq", width, "%r11");
      GenDivOp(false, true, 8, '/');
    }
  }
}


// Only objects Allocated on stack
void Generator::VisitObject(Object* obj) {
  EmitLoc(obj);
  auto addr = LValGenerator().GenExpr(obj).Repr();

  if (!obj->type()->IsScalar()) {
    // Return the address of the object in rax
    Emit("leaq", addr, "%rax");
  } else {
    EmitLoad(addr, obj->type());
  }
}


void Generator::GenCastOp(UnaryOp* cast) {
  auto des_type = cast->type();
  auto src_type = cast->operand()->type();

  if (src_type->IsFloat() && des_type->IsFloat()) {
    if (src_type->width() == des_type->width())
      return;
    auto inst = src_type->width() == 4 ? "cvtss2sd": "cvtsd2ss";
    Emit(inst, "%xmm0", "%xmm0");
  } else if (src_type->IsFloat()) {
    // Handle bool
    if (des_type->IsBool()) {
      Emit("pxor", "%xmm9", "%xmm9");
      GenCompOp(src_type->width(), true, "setne");
    } else {
      auto inst = src_type->width() == 4 ? "cvttss2si": "cvttsd2si";
      Emit(inst, "%xmm0", "%rax");
    }
  } else if (des_type->IsFloat()) {
    auto inst = des_type->width() == 4 ? "cvtsi2ss": "cvtsi2sd";
    Emit(inst, "%rax", "%xmm0");
  } else if (src_type->ToPointer()
      || src_type->ToFunc()
      || src_type->ToArray()) {
    // Handle bool
    if (des_type->IsBool()) {
      Emit("testq", "%rax", "%rax");
      Emit("setne", "%al");
    }
  } else {
    assert(src_type->ToArithm());
    int width = src_type->width();
    auto sign = !src_type->IsUnsigned();
    const char* inst;
    switch (width) {
    case 1:
      inst = sign ? "movsbq": "movzbq";
      Emit(inst, GetReg(width), "%rax");
      break;
    case 2:
      inst = sign ? "movswq": "movzwq";
      Emit(inst, GetReg(width), "%rax");
      break;
    case 4: inst = "movl"; 
      if (des_type->width() == 8)
        Emit("cltq");
      break;
    case 8: break;
    }
    // Handle bool
    if (des_type->IsBool()) {
      Emit("testq", "%rax", "%rax");
      Emit("setne", "%al");
    }
  }
}


void Generator::VisitUnaryOp(UnaryOp* unary) {
  EmitLoc(unary);
  switch  (unary->op()) {
  case Token::PREFIX_INC:
    return GenIncDec(unary->operand(), false, "add");
  case Token::PREFIX_DEC:
    return GenIncDec(unary->operand(), false, "sub");
  case Token::POSTFIX_INC:
    return GenIncDec(unary->operand(), true, "add");
  case Token::POSTFIX_DEC:
    return GenIncDec(unary->operand(), true, "sub");
  case Token::ADDR: {
    auto addr = LValGenerator().GenExpr(unary->operand()).Repr();
    Emit("leaq", addr, "%rax");
  } return;
  case Token::DEREF:
    return GenDerefOp(unary);
  case Token::PLUS:
    return VisitExpr(unary->operand());
  case Token::MINUS:
    return GenMinusOp(unary);
  case '~':
    VisitExpr(unary->operand());
    return Emit("notq", "%rax");
  case '!':
    VisitExpr(unary->operand());
    GenCompZero(unary->operand()->type());
    Emit("sete", "%al");
    Emit("movzbl", "%al", "%eax"); // type of !operator is int
    return;
  case Token::CAST:
    Visit(unary->operand());
    GenCastOp(unary);
    return;
  default: assert(false);
  }
}


void Generator::GenDerefOp(UnaryOp* deref) {
  VisitExpr(deref->operand());
  if (deref->type()->IsScalar()) {
    ObjectAddr addr {"", "%rax", 0};
    EmitLoad(addr.Repr(), deref->type());
  } else {
    // Just let it go!
  }
}


void Generator::GenMinusOp(UnaryOp* minus) {
  auto width = minus->type()->width();
  auto flt = minus->type()->IsFloat();

  VisitExpr(minus->operand());

  if (flt) {
    Emit("pxor", "%xmm9", "%xmm9");
    Emit(GetInst("sub", width, flt), "%xmm0", "%xmm9");
    Emit(GetInst("mov", width, flt), "%xmm9", "%xmm0");
  } else {
    Emit(GetInst("neg", width, flt), GetDes(width, flt));
  }
}


void Generator::GenIncDec(Expr* operand,
                          bool postfix,
                          const std::string& inst) {
  auto width = operand->type()->width();
  auto flt = operand->type()->IsFloat();
  
  auto addr = LValGenerator().GenExpr(operand).Repr();
  EmitLoad(addr, operand->type());
  if (postfix) Save(flt);

  ASTConstant* c;
  auto pointer_type = operand->type()->ToPointer();
   if (pointer_type) {
    long width = pointer_type->derived()->width();
    c = ASTConstant::New(operand->tok(), T_LONG, width);
  } else if (operand->type()->IsInteger()) {
    c = ASTConstant::New(operand->tok(), T_LONG, 1L);
  } else {
    if (width == 4)
      c = ASTConstant::New(operand->tok(), T_FLOAT, 1.0f);
    else
      c = ASTConstant::New(operand->tok(), T_DOUBLE, 1.0);
  }

  Emit(GetInst(inst, operand->type()), ConsLabel(c), GetDes(width, flt));
  EmitStore(addr, operand->type());
  if (postfix && flt) {
    Emit("movsd", "%xmm9", "%xmm0");
  } else if (postfix) {
    Emit("mov", "%r11", "%rax");
  }
}


void Generator::VisitConditionalOp(ConditionalOp* cond_op) {
  EmitLoc(cond_op);
  auto if_stmt = IfStmt::New(cond_op->cond_,
      cond_op->expr_true_, cond_op->expr_false_);
  VisitIfStmt(if_stmt);
}


void Generator::VisitEnumerator(Enumerator* enumer) {
  EmitLoc(enumer);
  auto cons = ASTConstant::New(enumer->tok(), T_INT, (long)enumer->Val());
  Visit(cons);
}


// Ident must be function
void Generator::VisitIdentifier(Identifier* ident) {
  EmitLoc(ident);
  Emit("leaq", ident->Name(), "%rax");
}


void Generator::VisitASTConstant(ASTConstant* c) {
  EmitLoc(c);
  auto label = ConsLabel(c);

  if (!c->type()->IsScalar()) {
    Emit("leaq", label, "%rax");
  } else {
    auto width = c->type()->width();
    auto flt = c->type()->IsFloat();
    auto load = GetInst("mov", width, flt);
    auto des = GetDes(width, flt);
    Emit(load, label, des);
  }
}


// Use %ecx as temp register
// TempVar is only used for condition expression of 'switch'
// and struct copy
void Generator::VisitTempVar(TempVar* temp_var) {
  assert(temp_var->type()->IsInteger());
  Emit("movl", "%ecx", "%eax");
}


void Generator::VisitDeclaration(Declaration* decl) {
  EmitLoc(decl->obj_);
  auto obj = decl->obj_;

  if (!obj->IsStatic()) {
    // The object has no linkage and has 
    // no static storage(the object is on stack).
    // If it has no initialization,
    // then it's value is random initialized.
    if (!obj->HasInit())
      return;

    int lastEnd = obj->offset();
    for (const auto& init: decl->Inits()) {
      ObjectAddr addr = ObjectAddr(obj->offset() + init.offset_);
      addr.bitfield_begin_ = init.bitfield_begin_;
      addr.bitfield_width_ = init.bitfield_width_;
      if (lastEnd != addr.offset_)
        EmitZero(ObjectAddr(lastEnd), addr.offset_ - lastEnd);
      VisitExpr(init.expr_);
      if (init.type_->IsScalar()) {
        EmitStore(addr, init.type_);
      } else if (init.type_->ToStruct()) {
        CopyStruct(addr, init.type_->width());
      } else {
        assert(false);
      }
      lastEnd = addr.offset_ + init.type_->width();
    }
    auto objEnd = obj->offset() + obj->type()->width();
    if (lastEnd != objEnd)
      EmitZero(ObjectAddr(lastEnd), objEnd - lastEnd);
    return;
  }

  if (obj->linkage() == NONE)
    static_decl_list_.push_back(decl);
  else
    GenStaticDecl(decl);
}


void Generator::GenStaticDecl(Declaration* decl) {
  auto obj = decl->obj_;
  assert(obj->IsStatic());

  const auto& label = obj->Repr();
  const auto width = obj->type()->width();
  const auto align = obj->align();

  // Omit the external without initilizer
  if ((obj->storage() & S_EXTERN) && !obj->HasInit())
    return;
  
  Emit(".data");
  auto glb = obj->linkage() == EXTERNAL ? ".globl": ".local";
  Emit(glb, label);

  if (!obj->HasInit()) {
    Emit(".comm", label + ", " +  std::to_string(width) +
                  ", " + std::to_string(align));
    return;
  }

  Emit(".align", std::to_string(align));
  Emit(".type", label, "@object");
  // Does not decide the size of obj
  Emit(".size", label, std::to_string(width));
  EmitLabel(label);
  
  int offset = 0;
  auto iter = decl->Inits().begin();
  for (; iter != decl->Inits().end();) {
    auto staticInit = GetStaticInit(iter,
        decl->Inits().end(), std::max(iter->offset_, offset));

    if (staticInit.offset_ > offset)
      Emit(".zero", std::to_string(staticInit.offset_ - offset));

    switch (staticInit.width_) {
    case 1:
      Emit(".byte", std::to_string(static_cast<char>(staticInit.val_)));
      break;
    case 2:
      Emit(".value", std::to_string(static_cast<short>(staticInit.val_)));
      break;
    case 4:
      Emit(".long", std::to_string(static_cast<int>(staticInit.val_)));
      break;
    case 8: {
      std::string val;
      if (staticInit.label_.size() == 0) {
        val = std::to_string(staticInit.val_);
      } else if (staticInit.val_ != 0) {
        val = staticInit.label_ + "+" + std::to_string(staticInit.val_);
      } else {
        val = staticInit.label_;
      }
      Emit(".quad", val);
    } break;
    default: assert(false);
    }
    offset = staticInit.offset_ + staticInit.width_;
  }
  // Decides the size of object
  if (width > offset)
    Emit(".zero", std::to_string(width - offset));
}


void Generator::VisitEmptyStmt(EmptyStmt* emptyStmt) {
  assert(false);
}


void Generator::VisitIfStmt(IfStmt* if_stmt) {
  VisitExpr(if_stmt->cond_);

  // Compare to 0
  auto elseLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();

  GenCompZero(if_stmt->cond_->type());

  if (if_stmt->els_) {
    Emit("je", elseLabel);
  } else {
    Emit("je", endLabel);
  }

  VisitStmt(if_stmt->then_);
  
  if (if_stmt->els_) {
    Emit("jmp", endLabel);
    EmitLabel(elseLabel->Repr());
    VisitStmt(if_stmt->els_);
  }
  
  EmitLabel(endLabel->Repr());
}


void Generator::VisitJumpStmt(GotoStmt* goto_stmt) {
  Emit("jmp", goto_stmt->label_);
}


void Generator::VisitLabelStmt(LabelStmt* label_stmt) {
  EmitLabel(label_stmt->Repr());
}


void Generator::VisitReturnStmt(ReturnStmt* returnStmt) {
  auto expr = returnStmt->expr_;
  if (expr) { // The return expr could be nil
    Visit(expr);
    if (expr->type()->ToStruct()) {
      // %rax now has the address of the struct/union
      ObjectAddr addr = ObjectAddr(ret_addr_offset_);
      Emit("movq", addr, "%r11");
      addr = {"", "%r11", 0};
      CopyStruct(addr, expr->type()->width());
      Emit("movq", "%r11", "%rax");
    }
  }
  Emit("jmp", cur_func_->retLabel_);
}


class Comp {
public:
  bool operator()(Object* lhs, Object* rhs) {
    return lhs->align() < rhs->align();
  }
};


void Generator::AllocObjects(Scope* scope, const FuncDef::ParamList& param_list) {
  int offset = offset_;

  auto paramSet = std::set<Object*>(param_list.begin(), param_list.end());
  std::priority_queue<Object*, std::vector<Object*>, Comp> heap;
  for (auto iter = scope->begin(); iter != scope->end(); ++iter) {
    auto obj = iter->second->ToObject();
    if (!obj || obj->IsStatic())
      continue;
    if (paramSet.find(obj) != paramSet.end())
      continue;
    heap.push(obj);
  }

  while (!heap.empty()) {
    auto obj = heap.top();
    heap.pop();

    offset -= obj->type()->width();
    auto align = obj->align();
    if (obj->type()->ToArray()) {
      // The alignment of an array is at least the aligment of a pointer
      // (as it is always cast to a pointer)
      align = std::min(align, 8);
    }
    offset = Type::MakeAlign(offset, align);
    obj->set_offset(offset);
  }

  offset_ = offset;
}


void Generator::VisitCompoundStmt(CompoundStmt* compStmt) {
  if (compStmt->scope_) {
    //compStmt
    AllocObjects(compStmt->scope_);
  }

  for (auto stmt: compStmt->stmt_list_) {
    Visit(stmt);
  }
}


void Generator::GetParamRegOffsets(int& gpOffset,
                                   int& fpOffset,
                                   int& overflow,
                                   FuncType* func_type) {
  TypeList types;
  for (auto param: func_type->param_list())
    types.push_back(param->type());
  auto locations = GetParamLocations(types, func_type->derived());
  gpOffset = 0;
  fpOffset = 48;
  overflow = 16;
  for (const auto& loc: locations.locs_) {
    if (loc[1] == 'x')
      fpOffset += 16;
    else if (loc[1] == 'm')
      overflow += 8;
    else
      gpOffset += 8;
  }
}


void Generator::GenBuiltin(FuncCall* funcCall) {
  typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
  } va_list_imp;

  auto ap = UnaryOp::New(Token::DEREF, funcCall->args_[0]);
  auto addr = LValGenerator().GenExpr(ap);
  auto type = funcCall->FuncType();
  
  auto offset = offsetof(va_list_imp, reg_save_area);
  addr.offset_ += offset;
  const auto& saveAreaAddr = addr.Repr();
  addr.offset_ -= offset;

  offset = offsetof(va_list_imp, overflow_arg_area);
  addr.offset_ += offset;
  const auto& overflowAddr = addr.Repr();
  addr.offset_ -= offset;

  offset = offsetof(va_list_imp, gp_offset);
  addr.offset_ += offset;
  const auto& gpOffsetAddr = addr.Repr();
  addr.offset_ -= offset;

  offset = offsetof(va_list_imp, fp_offset);
  addr.offset_ += offset;
  const auto& fpOffsetAddr = addr.Repr();
  addr.offset_ -= offset;

  if (type == Parser::va_start_type_) {
    Emit("leaq", "-176(%rbp)", "%rax");
    Emit("movq", "%rax", saveAreaAddr);
    
    int gpOffset, fpOffset, overflowOffset;
    GetParamRegOffsets(gpOffset, fpOffset,
                       overflowOffset, cur_func_->FuncType());
    Emit("leaq", ObjectAddr(overflowOffset), "%rax");
    Emit("movq", "%rax", overflowAddr);
    Emit("movl", gpOffset, "%eax");
    Emit("movl", "%eax", gpOffsetAddr);
    Emit("movl", fpOffset, "%eax");
    Emit("movl", "%eax", fpOffsetAddr);
  } else if (type == Parser::va_arg_type_) {
    static int cnt[2] = {0, 0};
    auto overflowLabel = ".L_va_arg_overflow" + std::to_string(++cnt[0]);
    auto endLabel = ".L_va_arg_end" + std::to_string(++cnt[1]);

    auto argType = funcCall->args_[1]->type()->ToPointer()->derived();
    auto cls = Classify(argType.ptr());
    if (cls == ParamClass::INTEGER) {
      Emit("movq", saveAreaAddr, "%rax");
      Emit("movq", "%rax", "%r11");
      Emit("movl", gpOffsetAddr, "%eax");
      Emit("cltq");
      Emit("cmpq", 48, "%rax");
      Emit("jae",  overflowLabel);
      Emit("addq", "%rax", "%r11");
      Emit("addq", 8, "%rax");
      Emit("movl", "%eax", gpOffsetAddr);
      Emit("movq", "%r11", "%rax");
      Emit("jmp",  endLabel);
    } else if (cls == ParamClass::SSE) {
      Emit("movq", saveAreaAddr, "%rax");
      Emit("movq", "%rax", "%r11");
      Emit("movl", fpOffsetAddr, "%eax");
      Emit("cltq");
      Emit("cmpq", 176, "%rax");
      Emit("jae",  overflowLabel);
      Emit("addq", "%rax", "%r11");
      Emit("addq", 16, "%rax");
      Emit("movl", "%eax", fpOffsetAddr);
      Emit("movq", "%r11", "%rax");
      Emit("jmp",  endLabel);
    } else if (cls == ParamClass::MEMORY) {
    } else {
      Error("internal error");
    }
    EmitLabel(overflowLabel);
    Emit("movq", overflowAddr, "%rax");
    Emit("movq", "%rax", "%r11");
    // Arguments passed by memory is aligned by at least 8 bytes
    Emit("addq", Type::MakeAlign(argType->width(), 8), "%r11");
    Emit("movq", "%r11", overflowAddr);
    EmitLabel(endLabel);
  } else {
    assert(false);
  }
}


void Generator::VisitFuncCall(FuncCall* funcCall) {
  EmitLoc(funcCall);
  auto func_type = funcCall->FuncType();
  if (Parser::IsBuiltin(func_type))
    return GenBuiltin(funcCall);

  auto base = offset_;
  // Alloc memory for return value if it is struct/union
  int retStructOffset;
  auto ret_type = funcCall->type()->ToStruct();
  if (ret_type) {
    retStructOffset = offset_;
    retStructOffset -= ret_type->width();
    retStructOffset = Type::MakeAlign(retStructOffset, ret_type->align());
    // No!!! you can't suppose that the 
    // visition of arguments won't change the value of %rdi
    //Emit("leaq %d(#rbp), #rdi", offset);
    offset_ = retStructOffset;
  }

  TypeList types;
  for (auto arg: funcCall->args_) {
    types.push_back(arg->type());
  }
  
  const auto& locations = GetParamLocations(types, ret_type);
  // align stack frame by 16 bytes
  const auto& locs = locations.locs_;
  auto byMemCnt = locs.size() - locations.regCnt_ - locations.xregCnt_;

  offset_ = Type::MakeAlign(offset_ - byMemCnt * 8, 16) + byMemCnt * 8;  
  for (int i = locs.size() - 1; i >=0; --i) {
    if (locs[i][1] == 'm') {
      Visit(funcCall->args_[i]);
      Push(funcCall->args_[i]->type());
    }
  }

  for (int i = locs.size() - 1; i >= 0; i--) {
    if (locs[i][1] == 'm')
      continue;
    Visit(funcCall->args_[i]);
    Push(funcCall->args_[i]->type());
  }

  for (const auto& loc: locs) {
    if (loc[1] != 'm')
      Pop(loc);
  }

  // If variadic, set %al to floating param number
  if (func_type->Variadic()) {
    Emit("movq", locations.xregCnt_, "%rax");
  }
  if (ret_type) {
    Emit("leaq", ObjectAddr(retStructOffset), "%rdi");
  }

  Emit("leaq", ObjectAddr(offset_), "%rsp");
  auto addr = LValGenerator().GenExpr(funcCall->designator());
  if (addr.base_.size() == 0 && addr.offset_ == 0) {
    Emit("call", addr.label_);
  } else {
    Emit("leaq", addr, "%r10");
    Emit("call", "*%r10");
  }

  // Reset stack frame
  offset_ = base;    
}


ParamLocations Generator::GetParamLocations(const TypeList& types,
                                            bool retStruct) {
  ParamLocations locations;

  locations.regCnt_ = retStruct;
  locations.xregCnt_ = 0;
  for (auto type: types) {
    auto cls = Classify(type);

    const char* reg = nullptr;
    if (cls == ParamClass::INTEGER) {
      if (locations.regCnt_ < regs.size())
        reg = regs[locations.regCnt_++];
    } else if (cls == ParamClass::SSE) {
      if (locations.xregCnt_ < xregs.size())
        reg = xregs[locations.xregCnt_++];
    }
    locations.locs_.push_back(reg ? reg: "%mem");
  }
  return locations;
}


void Generator::VisitFuncDef(FuncDef* func_def) {
  cur_func_ = func_def;

  auto name = func_def->Name();

  Emit(".text");
  if (func_def->linkage() == INTERNAL) {
    Emit(".local", name);
  } else {
    Emit(".globl", name);
  }
  Emit(".type", name, "@function");

  EmitLabel(name);
  Emit("pushq", "%rbp");
  Emit("movq", "%rsp", "%rbp");

  offset_ = 0;

  auto& param_list = func_def->FuncType()->param_list();
  // Arrange space to store param_list passed by registers
  bool retStruct = func_def->FuncType()->derived()->ToStruct();
  TypeList types;
  for (auto param: param_list)
    types.push_back(param->type());

  auto locations = GetParamLocations(types, retStruct);
  const auto& locs = locations.locs_;

  if (func_def->FuncType()->Variadic()) {
    GenSaveArea(); // 'offset' is now the begin of save area
    if (retStruct) {
      ret_addr_offset_ = offset_;
      offset_ += 8;
    }
    int regOffset = offset_;
    int xregOffset = offset_ + 48;
    int byMemOffset = 16;
    for (size_t i = 0; i < locs.size(); ++i) {
      if (locs[i][1] == 'm') {
        param_list[i]->set_offset(byMemOffset);
        //byMemOffset += 8;
        // TODO(wgtdkp): width of incomplete array ?
        // What about the var args, var args offset always increment by 8
        byMemOffset += param_list[i]->type()->width();
        byMemOffset = Type::MakeAlign(byMemOffset, 8);
      } else if (locs[i][1] == 'x') {
        param_list[i]->set_offset(xregOffset);
        xregOffset += 16;
      } else {
        param_list[i]->set_offset(regOffset);
        regOffset += 8;
      }
    }
  } else {
    if (retStruct) {
      ret_addr_offset_ = Push("%rdi");
    }
    int byMemOffset = 16;
    for (size_t i = 0; i < locs.size(); ++i) {
      if (locs[i][1] == 'm') {
        param_list[i]->set_offset(byMemOffset);
        // TODO(wgtdkp): width of incomplete array ?
        byMemOffset += param_list[i]->type()->width();
        byMemOffset = Type::MakeAlign(byMemOffset, 8);
        continue;
      }
      param_list[i]->set_offset(Push(locs[i]));
    }
  }

  AllocObjects(func_def->body()->Scope(), param_list);

  for (auto stmt: func_def->body_->stmt_list_) {
    Visit(stmt);
  }

  EmitLabel(func_def->retLabel_->Repr());
  Emit("leaveq");
  Emit("retq");
}


void Generator::GenSaveArea() {
  static const int begin = -176;
  int offset = begin;
  for (auto reg: regs) {
    Emit("movq", reg, ObjectAddr(offset));
    offset += 8;
  }
  Emit("testb", "%al", "%al");
  auto label = LabelStmt::New();
  Emit("je", label);
  for (auto xreg: xregs) {
    Emit("movaps", xreg, ObjectAddr(offset));
    offset += 16;
  }
  assert(offset == 0);
  EmitLabel(label->Repr());

  offset_ = begin;
}


void Generator::VisitTranslationUnit(TranslationUnit* unit) {
  for (auto ext_decl: unit->ext_decl_list()) {
    Visit(ext_decl);

    // float and string literal
    if (rodatas_.size())
      Emit(".section", ".rodata");
    for (auto rodata: rodatas_) {
      if (rodata.align_ == 1) { // Literal
        EmitLabel(rodata.label_);
        Emit(".string", "\"" + rodata.sval_ + "\"");
      } else if (rodata.align_ == 4) {
        Emit(".align", "4");
        EmitLabel(rodata.label_);
        Emit(".long", std::to_string(static_cast<int>(rodata.ival_)));
      } else {
        Emit(".align", "8");
        EmitLabel(rodata.label_);
        Emit(".quad", std::to_string(rodata.ival_));
      }
    }
    rodatas_.clear();

    for (auto staticDecl: static_decl_list_) {
      GenStaticDecl(staticDecl);
    }
    static_decl_list_.clear();
  }
}


void Generator::Gen() {
  Emit(".file", "\"" + in_filename + "\"");
  VisitTranslationUnit(parser_->Unit());
}


void Generator::EmitLoc(Expr* expr) {
  if (!debug) {
    return;
  }

  static int fileno = 0;
  if (expr->tok_ == nullptr) {
    return;
  }

  const auto loc = &expr->tok_->loc();
  if (loc->filename_ != last_file) {
    Emit(".file", std::to_string(++fileno) + " \"" + *loc->filename_ + "\"");
    last_file = loc->filename_;
  }
  Emit(".loc", std::to_string(fileno) + " " +
               std::to_string(loc->line_) + " 0");
  
  std::string line;
  for (const char* p = loc->line_begin_; *p && *p != '\n'; ++p)
    line.push_back(*p);
  Emit("# " + line);
}


void Generator::EmitLoad(const std::string& addr, Type* type) {
  assert(type->IsScalar());
  EmitLoad(addr, type->width(), type->IsFloat());
}


void Generator::EmitLoad(const std::string& addr, int width, bool flt) {
  auto load = GetLoad(width, flt);
  auto des = GetDes(width == 4 ? 4: 8, flt);
  Emit(load, addr, des);
}


void Generator::EmitStore(const ObjectAddr& addr, Type* type) {
  if (addr.bitfield_width_ != 0) {
    EmitStoreBitField(addr, type);
  } else {
    EmitStore(addr.Repr(), type);
  }
}


void Generator::EmitStore(const std::string& addr, Type* type) {
  EmitStore(addr, type->width(), type->IsFloat());
}


void Generator::EmitStore(const std::string& addr, int width, bool flt) {
  auto store = GetInst("mov", width, flt);
  auto des = GetDes(width, flt);
  Emit(store, des, addr);
}


void Generator::EmitLabel(const std::string& label) {
  fprintf(out_file_, "%s:\n", label.c_str());
}


void Generator::EmitZero(ObjectAddr addr, int width) {
  int units[] = {8, 4, 2, 1};
  Emit("xorq", "%rax", "%rax");
  for (auto unit: units) {
    while (width >= unit) {
      EmitStore(addr.Repr(), unit, false);
      addr.offset_ += unit;
      width -= unit;
    }
  }
}


void LValGenerator::VisitBinaryOp(BinaryOp* binary) {
  EmitLoc(binary);
  assert(binary->op() == '.');

  addr_ = LValGenerator().GenExpr(binary->lhs());
  const auto& name = binary->rhs()->tok()->str();
  auto struct_type = binary->lhs()->type()->ToStruct();
  auto member = struct_type->GetMember(name);

  addr_.offset_ += member->offset();
  addr_.bitfield_begin_ = member->bitfield_begin_;
  addr_.bitfield_width_ = member->bitfield_width_;
}


void LValGenerator::VisitUnaryOp(UnaryOp* unary) {
  EmitLoc(unary);
  assert(unary->op() == Token::DEREF);
  Generator().VisitExpr(unary->operand());
  Emit("movq", "%rax", "%r10");
  addr_ = {"", "%r10", 0};
}


void LValGenerator::VisitObject(Object* obj) {
  EmitLoc(obj);
  if (!obj->IsStatic() && obj->anonymous()) {
    assert(obj->decl());
    Generator().Visit(obj->decl());
    obj->set_decl(nullptr);
  }

  if (obj->IsStatic()) {
    addr_ = {obj->Repr(), "%rip", 0};
  } else {
    addr_ = {"", "%rbp", obj->offset()};
  }
}


// The identifier must be function
void LValGenerator::VisitIdentifier(Identifier* ident) {
  assert(!ident->ToTypeName());
  EmitLoc(ident);
  // Function address
  addr_ = {ident->Name(), "", 0};
}


void LValGenerator::VisitTempVar(TempVar* temp_var) {
  std::string label;
  switch (temp_var->type()->width()) {
  case 1: label = "%cl"; break;
  case 2: label = "%cx"; break;
  case 4: label = "%ecx"; break;
  case 8: label = "%rcx"; break;
  default: assert(false);
  }
  addr_ = {label, "", 0};
}


std::string ObjectAddr::Repr() const {
  auto ret = base_.size() ? "(" + base_ + ")": "";
  if (label_.size() == 0) {
    if (offset_ == 0) {
      return ret;
    }
    return std::to_string(offset_) + ret;
  } else {
    if (offset_ == 0) {
      return label_ + ret;
    }
    return label_ + "+" + std::to_string(offset_) + ret;
  }
}


StaticInitializer Generator::GetStaticInit(
    Declaration::InitList::iterator& iter,
    Declaration::InitList::iterator end,
    int offset) {
  auto init = iter++;
  auto width = init->type_->width();
  if (init->type_->IsInteger()) {
    if (init->bitfield_width_ == 0) {
      auto val = Evaluator<long>().Eval(init->expr_);
      return {init->offset_, width, val, ""};
    }
    int total_bits = 0;
    uint8_t val = 0;
    while (init != end && init->offset_ <= offset && total_bits < 8) {
      auto bit_val = Evaluator<long>().Eval(init->expr_);
      auto begin = init->bitfield_begin_;
      auto width = init->bitfield_width_;
      auto val_begin = 0;
      auto val_width = 0;
      auto mask = 0UL;
      if (init->offset_ < offset) {
        begin = 0;
        width -= (8 - init->bitfield_begin_);
        if (offset - init->offset_ > 1)
          width -= (offset - init->offset_ - 1) * 8;
        //width = std::max(8 - begin, width);
        val_begin = init->bitfield_width_ - width;
      }
      val_width = std::min(static_cast<uint8_t>(8 - begin), width);
      mask = Object::BitFieldMask(val_begin, val_width);
      val |= ((bit_val & mask) >> val_begin) << begin;
      total_bits = begin + val_width;
      if (width - val_width <= 0)
        ++init;
    }
    iter = init;
    return {offset, 1, val, ""};
  } else if (init->type_->IsFloat()) {
    auto val = Evaluator<double>().Eval(init->expr_);
    auto lval = (union {double val; long lval;}){val}.lval;
    return {init->offset_, width, lval, ""};
  } else if (init->type_->ToPointer()) {
    auto addr = Evaluator<Addr>().Eval(init->expr_);
    return {init->offset_, width, addr.offset_, addr.label_};
  } else { // Struct initializer
    Error(init->expr_, "initializer element is not constant");
    return StaticInitializer(); //Make compiler happy
  }
}
