#include "code_gen.h"

#include "evaluator.h"
#include "parser.h"
#include "token.h"

#include <cstdarg>

#include <queue>
#include <set>


extern std::string inFileName;
extern std::string outFileName;


Parser* Generator::parser_ = nullptr;
FILE* Generator::outFile_ = nullptr;
//std::string Generator::_cons;
RODataList Generator::rodatas_;
std::vector<Declaration*> Generator::staticDecls_;
int Generator::offset_ = 0;
int Generator::retAddrOffset_ = 0;
FuncDef* Generator::curFunc_ = nullptr;


/*
 * Register usage:
 *  xmm8: accumulator of floating datas;
 *  xmm9: source operand register;
 *  xmm10: tmp register for floating data swap;
 *  rax: accumulator;
 *  r11: source operand register;
 *  r10: register stores intermediate data when LValGenerator
 *       eval the address.
 *  rcx: tempvar register, like the tempvar of 'switch'
 */

static std::vector<const char*> regs {
  "rdi", "rsi", "rdx",
  "rcx", "r8", "r9"
};

static std::vector<const char*> xregs {
  "xmm0", "xmm1", "xmm2", "xmm3",
  "xmm4", "xmm5", "xmm6", "xmm7"
};

static ParamClass Classify(Type* paramType, int offset=0)
{
  if (paramType->IsInteger() || paramType->ToPointer()
      || paramType->ToArray()) {
    return ParamClass::INTEGER;
  }
  
  if (paramType->ToArithm()) {
    auto type = paramType->ToArithm();
    if (type->Tag() == T_FLOAT || type->Tag() == T_DOUBLE)
      return ParamClass::SSE;
    if (type->Tag() == (T_LONG | T_DOUBLE)) {
      // TODO(wgtdkp):
      assert(false); 
      return ParamClass::X87;
    }
    
    // TODO(wgtdkp):
    assert(false);
    // It is complex
    if ((type->Tag() & T_LONG) && (type->Tag() & T_DOUBLE))
      return ParamClass::COMPLEX_X87;
    
  }

  // TODO(wgtdkp): Support agrregate type 
  assert(false);
  /*
  auto type = paramType->ToStruct();
  assert(type);

  if (type->Width() > 4 * 8)
    return PC_MEMORY;

  std::vector<ParamClass> classes;
  int cnt = (type->Width() + 7) / 8;
  for (int i = 0; i < cnt; i++) {
    auto  types = FieldsIn8Bytes(type, i);
    assert(types.size() > 0);
    
    auto fieldClass = (types.size() == 1)
        ? PC_NO_CLASS: FieldClass(types, 0);
    classes.push_back(fieldClass);
    
  }

  bool sawX87 = false;
  for (int i = 0; i < classes.size(); i++) {
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


std::string ObjectLabel(Object* obj)
{
  static int tag = 0;
  assert(obj->IsStatic());
  if (obj->Linkage() == L_NONE) {
    return obj->Name() + "." + std::to_string(tag++);
  }
  return obj->Name();
}


std::string Generator::ConsLabel(Constant* cons)
{
  if (cons->Type()->IsInteger()) {
    return "$" + std::to_string(cons->IVal());
  } else if (cons->Type()->IsFloat()) {
    double valsd = cons->FVal();
    float  valss = valsd;
    // TODO(wgtdkp): Add rodata
    auto width = cons->Type()->Width();
    long val = width == 4 ? *(int*)&valss: *(long*)&valsd;
    const ROData& rodata = ROData(val, width);
    rodatas_.push_back(rodata);
    return rodata.label_;
  } else { // Literal
    const ROData& rodata = ROData(cons->SValRepr());
    rodatas_.push_back(rodata);
    return rodata.label_; // return address
  }
}


static const char* GetLoad(int width, bool flt=false)
{
  switch (width) {
  case 1: return "movzbq";
  case 2: return "movzwq";
  case 4: return !flt ? "movl": "movss";
  case 8: return !flt ? "movq": "movsd";
  default: assert(false); return nullptr;
  }
}


static std::string GetInst(const std::string& inst, int width, bool flt)
{
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


static std::string GetInst(const std::string& inst, Type* type)
{
  assert(type->IsScalar());
  return GetInst(inst, type->Width(), type->IsFloat());
}


static const char* GetReg(int width)
{
  switch (width) {
  case 1: return "al";
  case 2: return "ax";
  case 4: return "eax";
  case 8: return "rax";
  default: assert(false); return nullptr;
  }
}

static const char* GetDes(int width, bool flt)
{
  if (flt) {
    return "xmm8";
  }
  return GetReg(width);
}

static const char* GetSrc(int width, bool flt)
{
  if (flt) {
    return "xmm9";
  }
  switch (width) {
  case 1: return "r11b";
  case 2: return "r11w";
  case 4: return "r11d";
  case 8: return "r11";
  default: assert(false); return "";
  }
}

/*
static const char* GetDes(Type* type)
{
  assert(type->IsScalar());
  return GetDes(type->Width(), type->IsFloat());
}


static const char* GetSrc(Type* type)
{
  assert(type->IsScalar());
  return GetSrc(type->Width(), type->IsFloat());
}
*/

/*
static inline void GetOperands(const char*& src,
    const char*& des, int width, bool flt)
{
  assert(width == 4 || width == 8);
  src = flt ? "xmm9": (width == 8 ? "r11": "r11d");
  des = flt ? "xmm8": (width == 8 ? "rax": "eax");
}
*/

// The 'reg' must be 8 bytes  
int Generator::Push(const char* reg)
{
  offset_ -= 8;
  auto mov = reg[0] == 'x' ? "movsd": "movq";
  Emit("%s #%s, %d(#rbp)", mov, reg, offset_);
  return offset_;
}


// The 'reg' must be 8 bytes
int Generator::Pop(const char* reg)
{
  auto mov = reg[0] == 'x' ? "movsd": "movq";
  Emit("%s %d(#rbp), #%s", mov, offset_, reg);
  offset_ += 8;
  return offset_;
}


void Generator::Spill(bool flt)
{
  Push(flt ? "xmm8": "rax");
}


void Generator::Restore(bool flt)
{
  auto src = GetSrc(8, flt);
  auto des = GetDes(8, flt);
  auto inst = GetInst("mov", 8, flt);
  Emit("%s #%s, #%s", inst.c_str(), des, src);
  Pop(des);
}


void Generator::Save(bool flt)
{
  if (flt) {        
    Emit("movsd #xmm8, #xmm9");
  } else {
    Emit("movq #rax, #r11");
  }
}


/*
 * Operaotr/Instruction mapping:
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
void Generator::VisitBinaryOp(BinaryOp* binary)
{
  auto op = binary->op_;

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
  if (binary->Type()->ToPointer())
    return GenPointerArithm(binary);

  // Careful: for compare operator, the type of the expression
  //     is always integer, while the type of lhs and rhs could be float
  // After convertion, lhs and rhs always has the same type
  auto type = binary->lhs_->Type();
  auto width = type->Width();
  auto flt = type->IsFloat();
  auto sign = type->ToPointer() 
           || !(type->ToArithm()->Tag() & T_UNSIGNED);

  Visit(binary->lhs_);
  Spill(flt);
  Visit(binary->rhs_);
  Restore(flt);

  const char* inst;

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
  case Token::LEFT: inst = "sal";
  case Token::RIGHT: inst = sign ? "sar": "shr";
    inst = op == Token::LEFT ? "sal": (sign ? "sar": "shr");
    Emit("movq #r11, #rcx");
    Emit("%s #cl, #%s", GetInst(inst, width, flt).c_str(),
        GetDes(width, flt));
    return;
  }
  Emit("%s #%s, #%s", GetInst(inst, width, flt).c_str(),
      GetSrc(width, flt), GetDes(width, flt));
}


void Generator::GenCommaOp(BinaryOp* comma)
{
  VisitExpr(comma->lhs_);
  VisitExpr(comma->rhs_);
}


void Generator::GenMulOp(int width, bool flt, bool sign)
{
  auto inst = flt ? "mul": (sign ? "imul": "mul");
  
  if (flt) {
    Emit("%s #xmm9, #xmm8", GetInst(inst, width, flt).c_str());
  } else {
    Emit("%s #%s", GetInst(inst, width, flt).c_str(), GetSrc(width, flt));
  }
}


void Generator::GenCompZero(Type* type)
{
  auto width = type->Width();
  auto flt = type->IsFloat();
  
  if (!flt) {
    Emit("cmp $0, #%s", GetReg(width));
  } else {
    Emit("pxor #xmm9, #xmm9");
    auto cmp = width == 8 ? "ucomisd": "ucomiss";
    Emit("%s #xmm9, #xmm8", cmp);
  }
}


void Generator::GenAndOp(BinaryOp* andOp)
{
  VisitExpr(andOp->lhs_);
  GenCompZero(andOp->lhs_->Type());

  auto labelFalse = LabelStmt::New();
  Emit("je %s", labelFalse->Label().c_str());

  VisitExpr(andOp->rhs_);
  GenCompZero(andOp->rhs_->Type());

  Emit("je %s", labelFalse->Label().c_str());
  
  Emit("movq $1, #rax");
  auto labelTrue = LabelStmt::New();
  Emit("jmp %s", labelTrue->Label().c_str());
  EmitLabel(labelFalse->Label());
  Emit("xorq #rax, #rax"); // Set %rax to 0
  EmitLabel(labelTrue->Label());
}


void Generator::GenOrOp(BinaryOp* orOp)
{
  VisitExpr(orOp->lhs_);
  GenCompZero(orOp->lhs_->Type());

  auto labelTrue = LabelStmt::New();
  Emit("jne %s", labelTrue->Label().c_str());

  VisitExpr(orOp->rhs_);
  GenCompZero(orOp->rhs_->Type());

  Emit("jne %s", labelTrue->Label().c_str());
  
  Emit("xorq #rax, #rax"); // Set %rax to 0
  auto labelFalse = LabelStmt::New();
  Emit("jmp %s", labelFalse->Label().c_str());
  EmitLabel(labelTrue->Label());
  Emit("movq $1, #rax");
  EmitLabel(labelFalse->Label());    
}


void Generator::GenMemberRefOp(BinaryOp* ref)
{
  // As the lhs will always be struct/union 
  auto addr = LValGenerator().GenExpr(ref->lhs_);
  const auto& name = ref->rhs_->Tok()->str_;
  auto structType = ref->lhs_->Type()->ToStruct();
  auto member = structType->GetMember(name);

  addr.offset_ += member->Offset();

  if (!ref->Type()->IsScalar()) {
    Emit("leaq %s, #rax", addr.Repr().c_str());
  } else {
    // TODO(wgtdkp): handle bit field
    if (member->BitFieldWidth()) {
      EmitLoadBitField(addr.Repr(), member);
    } else {
      EmitLoad(addr.Repr(), ref->Type());
    }
  }
}


void Generator::EmitLoadBitField(const std::string& addr, Object* bitField)
{
  auto type = bitField->Type()->ToArithm();
  assert(type && type->IsInteger());

  EmitLoad(addr, type);

  Emit("andq $%lu, #rax", Object::BitFieldMask(bitField));

  auto shiftRight = (type->Tag() & T_UNSIGNED) ? "shrq": "sarq";    
  auto left = 64 - bitField->bitFieldBegin_ - bitField->bitFieldWidth_;
  auto right = 64 - bitField->bitFieldWidth_;
  Emit("salq $%d, #rax", left);
  Emit("%s $%d, #rax", shiftRight, right);
}


void Generator::GenAssignOp(BinaryOp* assign)
{
  // The base register of addr is %r10
  VisitExpr(assign->rhs_);
  Spill(assign->Type()->IsFloat());
  auto addr = LValGenerator().GenExpr(assign->lhs_);
  Restore(assign->Type()->IsFloat());

  if (assign->Type()->IsScalar()) {
    if (addr.bitFieldWidth_ != 0) {
      EmitStoreBitField(addr, assign->Type());
    } else {
      EmitStore(addr.Repr(), assign->Type());
    }
  } else {
    // struct/union type
    // The address of rhs is in %rax
    CopyStruct(addr, assign->Type()->Width());
  }
}


void Generator::EmitStoreBitField(const ObjectAddr& addr, Type* type)
{
  auto arithmType = type->ToArithm();
  assert(arithmType && arithmType->IsInteger());

  // The value to be stored is in %rax now
  auto mask = Object::BitFieldMask(addr.bitFieldBegin_, addr.bitFieldWidth_);

  Emit("salq $%d, #rax", addr.bitFieldBegin_);
  Emit("andq $%lu, #rax", mask);
  Emit("movq #rax, #r11");
  EmitLoad(addr.Repr(), arithmType);
  Emit("andq $%lu, #rax", ~mask);
  Emit("orq #r11, #rax");

  EmitStore(addr.Repr(), type);
}


void Generator::CopyStruct(ObjectAddr desAddr, int width)
{
  int units[] = {8, 4, 2, 1};
  ObjectAddr srcAddr = {"", "rax", 0};
  for (auto unit: units) {
    while (width >= unit) {
      EmitLoad(srcAddr.Repr(), width, false);
      EmitStore(desAddr.Repr(), width, false);
      desAddr.offset_ += unit;
      srcAddr.offset_ += unit;
      width -= unit;
    }
  }
}


void Generator::GenCompOp(int width, bool flt, const char* set)
{
  std::string cmp;
  if (flt) {
    cmp = width == 8 ? "ucomisd": "ucomiss";
  } else {
    cmp = GetInst("cmp", width, flt);
  }

  Emit("%s #%s, #%s", cmp.c_str(), GetSrc(width, flt), GetDes(width, flt));
  Emit("%s #al", set);
  Emit("movzbq #al, #rax");
}


void Generator::GenDivOp(bool flt, bool sign, int width, int op)
{
  if (flt) {
    auto inst = width == 4 ? "divss": "divsd";
    Emit("%s #xmm9, #xmm8", inst);
    return;
  } else if (!sign) {
    Emit("xor #rdx, #rdx");
    Emit("%s #%s", GetInst("div", width, flt).c_str(), GetSrc(width, flt));
  } else {
    Emit(width == 4 ? "cltd": "cqto");
    Emit("%s #%s", GetInst("idiv", width, flt).c_str(), GetSrc(width, flt));      
  }
  if (op == '%')
    Emit("mov #rdx, #rax");
}

 
void Generator::GenPointerArithm(BinaryOp* binary)
{
  // For '+', we have swap lhs_ and rhs_ to ensure that 
  // the pointer is at lhs.
  Visit(binary->lhs_);
  Spill(false);
  Visit(binary->rhs_);
  
  auto type = binary->lhs_->Type()->ToPointer()->Derived();
  if (type->Width() > 1)
    Emit("imulq $%d, #rax", type->Width());
  Restore(false);
  if (binary->op_ == '+')
    Emit("addq #r11, #rax");
  else
    Emit("subq #r11, #rax");
}


void Generator::VisitObject(Object* obj)
{
  auto addr = LValGenerator().GenExpr(obj).Repr();
  if (obj->Anonymous()) {
    assert(obj->Decl());
    VisitStmt(obj->Decl());
    obj->SetDecl(nullptr);
  }

  if (!obj->Type()->IsScalar()) {
    // Return the address of the object in rax
    Emit("leaq %s, #rax", addr.c_str());
  } else {
    EmitLoad(addr, obj->Type());
  }
}


void Generator::GenCastOp(UnaryOp* cast)
{
  auto desType = cast->Type();
  auto srcType = cast->operand_->Type();

  if (srcType->IsFloat() && desType->IsFloat()) {
    if (srcType->Width() == desType->Width())
      return;
    const char* inst = srcType->Width() == 4 ? "cvtss2sd": "cvtsd2ss";
    Emit("%s #xmm8, #xmm8", inst);
  } else if (srcType->IsFloat()) {
    const char* inst = srcType->Width() == 4 ? "cvttss2si": "cvttsd2si";
    Emit("%s #xmm8, #rax", inst);
  } else if (desType->IsFloat()) {
    const char* inst = desType->Width() == 4 ? "cvtsi2ss": "cvtsi2sd";
    Emit("%s #rax, #xmm8", inst);
  } else if (srcType->ToPointer()
      || srcType->ToFunc()
      || srcType->ToArray()) {
    // Do nothing
  } else {
    assert(srcType->ToArithm());
    int width = srcType->Width();
    auto sign = !(srcType->ToArithm()->Tag() & T_UNSIGNED);
    const char* inst;
    switch (width) {
    case 1:
      inst = sign ? "movsbq": "movzbq";
      return Emit("%s #%s, #rax", inst, GetReg(width));
    case 2:
      inst = sign ? "movswq": "movzwq";
      return Emit("%s #%s, #rax", inst, GetReg(width));
    case 4: inst = "movl"; 
      if (desType->Width() == 8)
        Emit("cltq");
      return;
    case 8: return;
    }
  }
}


void Generator::VisitUnaryOp(UnaryOp* unary)
{
  switch  (unary->op_) {
  case Token::PREFIX_INC:
    return GenPrefixIncDec(unary->operand_, "add");
  case Token::PREFIX_DEC:
    return GenPrefixIncDec(unary->operand_, "sub");
  case Token::POSTFIX_INC:
    return GenPostfixIncDec(unary->operand_, "add");
  case Token::POSTFIX_DEC:
    return GenPostfixIncDec(unary->operand_, "sub");
  case Token::ADDR: {
    auto addr = LValGenerator().GenExpr(unary->operand_).Repr();
    Emit("leaq %s, #rax", addr.c_str());
  } return;
  case Token::DEREF:
    return GenDerefOp(unary);
  case Token::PLUS:
    return VisitExpr(unary->operand_);
  case Token::MINUS:
    return GenMinusOp(unary);
  case '~':
    VisitExpr(unary->operand_);
    return Emit("notq #rax");
  case '!':
    VisitExpr(unary->operand_);
    GenCompZero(unary->operand_->Type());
    Emit("sete #al");
    Emit("movzbl #al, #eax"); // type of !operator is int
    return;
  case Token::CAST:
    Visit(unary->operand_);
    GenCastOp(unary);
    return;
  default: assert(false);
  }
}


void Generator::GenDerefOp(UnaryOp* deref)
{
  VisitExpr(deref->operand_);
  if (deref->Type()->IsScalar()) {
    ObjectAddr addr {"", "rax", 0};
    EmitLoad(addr.Repr(), deref->Type());
  } else {
    // Just let it go!
  }
}


void Generator::GenMinusOp(UnaryOp* minus)
{
  auto width = minus->Type()->Width();
  auto flt = minus->Type()->IsFloat();

  VisitExpr(minus->operand_);

  if (flt) {
    Emit("pxor #xmm9, #xmm9");
    Emit("%s #xmm8, #xmm9", GetInst("sub", width, flt).c_str());
    Emit("%s #xmm9, #xmm8", GetInst("mov", width, flt).c_str());
  } else {
    Emit("%s #%s", GetInst("neg", width, flt).c_str(), GetDes(width, flt));
  }
}


void Generator::GenPrefixIncDec(Expr* operand, const std::string& inst)
{
  // Need a special base register to reduce register conflict
  auto addr = LValGenerator().GenExpr(operand).Repr();
  EmitLoad(addr, operand->Type());

  Constant* cons;
  auto pointerType = operand->Type()->ToPointer();
   if (pointerType) {
    long width = pointerType->Derived()->Width();
    cons = Constant::New(operand->Tok(), T_LONG, width);
  } else if (operand->Type()->IsInteger()) {
    cons = Constant::New(operand->Tok(), T_LONG, 1L);
  } else {
    if (operand->Type()->Width() == 4)
      cons = Constant::New(operand->Tok(), T_FLOAT, 1.0);
    else
      cons = Constant::New(operand->Tok(), T_DOUBLE, 1.0);
  }
  auto consLabel = ConsLabel(cons);

  auto addSub = GetInst(inst, operand->Type());  
  auto des = GetDes(operand->Type()->Width(), operand->Type()->IsFloat());  
  Emit("%s %s, #%s", addSub.c_str(), consLabel.c_str(), des);
  
  EmitStore(addr, operand->Type());
}


void Generator::GenPostfixIncDec(Expr* operand, const std::string& inst)
{
  // Need a special base register to reduce register conflict
  auto width = operand->Type()->Width();
  auto flt = operand->Type()->IsFloat();
  
  auto addr = LValGenerator().GenExpr(operand).Repr();
  EmitLoad(addr, operand->Type());
  Save(flt);

  Constant* cons;
  auto pointerType = operand->Type()->ToPointer();
   if (pointerType) {
    long width = pointerType->Derived()->Width();
    cons = Constant::New(operand->Tok(), T_LONG, width);
  } else if (operand->Type()->IsInteger()) {
    cons = Constant::New(operand->Tok(), T_LONG, 1L);
  } else {
    if (operand->Type()->Width() == 4)
      cons = Constant::New(operand->Tok(), T_FLOAT, 1.0f);
    else
      cons = Constant::New(operand->Tok(), T_DOUBLE, 1.0);
  }
  auto consLabel = ConsLabel(cons);

  auto addSub = GetInst(inst, operand->Type());    
  Emit("%s %s, #%s", addSub.c_str(), consLabel.c_str(), GetDes(width, flt));

  EmitStore(addr, operand->Type());
  Exchange(flt);
}


void Generator::Exchange(bool flt)
{
  if (flt) {
    Emit("movsd #xmm8, #xmm10");
    Emit("movsd #xmm9, #xmm8");
    Emit("movsd #xmm10, #xmm9");
  } else {
    Emit("xchgq #rax, #r11");
  }
}


void Generator::VisitConditionalOp(ConditionalOp* condOp)
{
  auto ifStmt = IfStmt::New(condOp->cond_,
      condOp->exprTrue_, condOp->exprFalse_);
  VisitIfStmt(ifStmt);
}


void Generator::VisitEnumerator(Enumerator* enumer)
{
  auto cons = Constant::New(enumer->Tok(), T_INT, (long)enumer->Val());
  Visit(cons);
}


// Ident must be function
void Generator::VisitIdentifier(Identifier* ident)
{
  Emit("leaq %s, #rax", ident->Name().c_str());
}


void Generator::VisitConstant(Constant* cons)
{
  auto label = ConsLabel(cons);

  if (!cons->Type()->IsScalar()) {
    Emit("leaq %s, #rax", label.c_str());
  } else {
    auto width = cons->Type()->Width();
    auto flt = cons->Type()->IsFloat();
    auto load = GetInst("mov", width, flt);
    auto des = GetDes(width, flt);
    Emit("%s %s, #%s", load.c_str(), label.c_str(), des);
  }
}


// Use %ecx as temp register
// TempVar is only used for condition expression of 'switch'
void Generator::VisitTempVar(TempVar* tempVar)
{
  assert(tempVar->Type()->IsInteger());

  Emit("movl #ecx, #eax");
}


void Generator::VisitDeclaration(Declaration* decl)
{
  auto obj = decl->obj_;

  if (!obj->IsStatic()) {
    // The object has no linkage and has 
    //     no static storage(the object is on stack).
    // If it has no initialization, then it's value is random
    //     initialized.
    if (!obj->HasInit())
      return;

    for (const auto& init: decl->Inits()) {
      ObjectAddr addr = {"", "rbp", obj->Offset() + init.offset_};
      if (init.type_->IsScalar()) {
        VisitExpr(init.expr_);
        EmitStore(addr.Repr(), init.type_);
      } else if (init.type_->ToStruct()) {
        VisitExpr(init.expr_);
        CopyStruct(addr, init.type_->Width());
      } else {
        assert(false);
      }
    }
    return;
  }

  if (obj->Linkage() == L_NONE)
    staticDecls_.push_back(decl);
  else
    GenStaticDecl(decl);
}

/*
// literal initializer
        GenLiteralInit(addr.Repr(), init.type_->ToArray(), init.expr_);

void Generator::GenLiteralInit(const std::string& addr, ArrayType* type, Expr* expr)
{
  assert(type);
  auto literal = Evaluator<Addr>().Eval(expr);
  auto sval = 

}
*/

void Generator::GenStaticDecl(Declaration* decl)
{
  auto obj = decl->obj_;
  assert(obj->IsStatic());

  auto label = ObjectLabel(obj);
  auto width = obj->Type()->Width();
  auto align = obj->Type()->Align();

  // Omit the external without initilizer
  if ((obj->Storage() & S_EXTERN) && !obj->HasInit())
    return;
  
  Emit(".data");
  auto glb = obj->Linkage() == L_EXTERNAL ? ".globl": ".local";
  Emit("%s %s", glb, label.c_str());

  if (!obj->HasInit()) {    
    Emit(".comm %s, %d, %d", label.c_str(), width, align);
    return;
  }

  Emit(".align %d", align);
  Emit(".type %s, @object", label.c_str());
  Emit(".size %s, %d", label.c_str(), width);
  EmitLabel(label.c_str());
  
  // TODO(wgtdkp): Add .zero
  int offset = 0;
  for (auto init: decl->Inits()) {
    auto staticInit = GetStaticInit(init);
    if (staticInit.offset_ > offset) {
      Emit(".zero %d", staticInit.offset_ - offset);
    }

    switch (staticInit.width_) {
    case 1:
      Emit(".byte %d", static_cast<char>(staticInit.val_));
      break;
    case 2:
      Emit(".value %d", static_cast<short>(staticInit.val_));
      break;
    case 4:
      Emit(".long %d", static_cast<int>(staticInit.val_));
      break;
    case 8:
      if (staticInit.label_.size() == 0) {
        Emit(".quad %ld", staticInit.val_);
      } else if (staticInit.val_ != 0) {
        Emit(".quad %s+%ld", staticInit.label_.c_str(), staticInit.val_);
      } else {
        Emit(".quad %s", staticInit.label_.c_str());
      }
      break;
    default: assert(false);
    }

    offset = staticInit.offset_ + staticInit.width_;
  }
}


void Generator::VisitEmptyStmt(EmptyStmt* emptyStmt)
{
  assert(false);
}


void Generator::VisitIfStmt(IfStmt* ifStmt)
{
  VisitExpr(ifStmt->cond_);

  // Compare to 0
  auto elseLabel = LabelStmt::New()->Label();
  auto endLabel = LabelStmt::New()->Label();

  GenCompZero(ifStmt->cond_->Type());

  if (ifStmt->else_) {
    Emit("je %s", elseLabel.c_str());
  } else {
    Emit("je %s", endLabel.c_str());
  }

  VisitStmt(ifStmt->then_);
  
  if (ifStmt->else_) {
    Emit("jmp %s", endLabel.c_str());
    EmitLabel(elseLabel.c_str());
    VisitStmt(ifStmt->else_);
  }
  
  EmitLabel(endLabel.c_str());
}


void Generator::VisitJumpStmt(JumpStmt* jumpStmt)
{
  Emit("jmp %s", jumpStmt->label_->Label().c_str());
}


void Generator::VisitLabelStmt(LabelStmt* labelStmt)
{
  EmitLabel(labelStmt->Label());
}


void Generator::VisitReturnStmt(ReturnStmt* returnStmt)
{
  auto expr = returnStmt->expr_;
  if (expr) {
    Visit(expr);
    if (expr->Type()->ToStruct()) {
      // %rax now has the address of the struct/union
      
      ObjectAddr addr = {"", "rbp", retAddrOffset_};
      Emit("movq %s, #r11", addr.Repr().c_str());
      addr = {"", "r11", 0};
      CopyStruct(addr, expr->Type()->Width());
      Emit("movq #r11, #rax");
    }
  }
  Emit("jmp %s", curFunc_->retLabel_->Label().c_str());
}


class Comp
{
public:
  bool operator()(Object* lhs, Object* rhs) {
    return lhs->Type()->Align() < rhs->Type()->Align();
  }
};


void Generator::AllocObjects(Scope* scope, const FuncDef::ParamList& params)
{
  int offset = offset_;

  auto paramSet = std::set<Object*>(params.begin(), params.end());
  std::priority_queue<Object*, std::vector<Object*>, Comp> heap;
  for (auto iter = scope->begin(); iter != scope->end(); iter++) {
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

    offset -= obj->Type()->Width();
    offset = Type::MakeAlign(offset, obj->Type()->Align());
    obj->SetOffset(offset);
  }

  offset_ = offset;
}


void Generator::VisitCompoundStmt(CompoundStmt* compStmt)
{
  if (compStmt->scope_) {
    //compStmt
    AllocObjects(compStmt->scope_);
  }

  for (auto stmt: compStmt->stmts_) {
    Visit(stmt);
  }
}


void Generator::VisitFuncCall(FuncCall* funcCall)
{
  auto base = offset_;
  // Alloc memory for return value if it is struct/union
  auto funcType = funcCall->FuncType();
  auto retType = funcCall->Type()->ToStruct();
  if (retType) {
    auto offset = offset_;
    offset -= funcCall->Type()->Width();
    offset = Type::MakeAlign(offset, funcCall->Type()->Align());
    Emit("leaq %d(#rbp), #rdi", offset);
    
    //Emit("subq $%d, #rsp", offset_ - offset);
    offset_ = offset;
  }

  std::vector<Type*> types;
  for (auto arg: funcCall->args_)
    types.push_back(arg->Type());
  
  auto locations = GetParamLocation(types, retType);
  //auto beforePass = offset_;
  for (int i = locations.size() - 1; i >=0; i--) {
    if (locations[i][0] == 'm') {
      Visit(funcCall->args_[i]);
      if (types[i]->IsFloat())
        Push("xmm8");
      else
        Push("rax");
    }
  }

  int fltCnt = 0;
  for (int i = locations.size() - 1; i >= 0; i--) {
    if (locations[i][0] == 'm')
      continue;
    Visit(funcCall->args_[i]);
    
    if (locations[i][0] == 'x') {
      ++fltCnt;
      auto inst = GetInst("mov", types[i]);
      Emit("%s #xmm8, #%s", inst.c_str(), locations[i]);
    } else {
      Emit("movq #rax, #%s", locations[i]);
    }
  }

  // If variadic, set %al to floating param number
  if (funcType->Variadic()) {
    Emit("movq $%d, %rax", fltCnt);
  }

  offset_ = Type::MakeAlign(offset_, 16);
  Emit("leaq %d(#rbp), #rsp", offset_);

  auto addr = LValGenerator().GenExpr(funcCall->Designator());
  if (addr._base.size() == 0 && addr.offset_ == 0) {
    Emit("call %s", addr.label_.c_str());
  } else {
    Emit("leaq %s, #r10", addr.Repr().c_str());
    Emit("call *#r10");
  }

  // Reset stack frame
  offset_ = base;    
}


std::vector<const char*> Generator::GetParamLocation(
    FuncType::TypeList& types, bool retStruct)
{
  std::vector<const char*> locations;

  size_t regCnt = retStruct, xregCnt = 0;
  for (auto type: types) {
    auto cls = Classify(type);

    const char* reg = nullptr;
    if (cls == ParamClass::INTEGER) {
      if (regCnt < regs.size())
        reg = regs[regCnt++];
    } else if (cls == ParamClass::SSE) {
      if (xregCnt < xregs.size())
        reg = xregs[xregCnt++];
    }
    locations.push_back(reg ? reg: "mem");
  }
  return locations;
}


void Generator::VisitFuncDef(FuncDef* funcDef)
{
  curFunc_ = funcDef;

  auto name = funcDef->Name();

  Emit(".text");
  if (funcDef->Linkage() == L_INTERNAL)
    Emit(".local %s", name.c_str());
  else
    Emit(".globl %s", name.c_str());
  Emit(".type %s, @function", name.c_str());

  EmitLabel(name);
  Emit("pushq #rbp");
  Emit("movq #rsp, #rbp");


  FuncDef::ParamList& params = funcDef->Params();

  offset_ = 0;

  // Arrange space to store params passed by registers
  auto retType = funcDef->Type()->Derived()->ToStruct();
  auto locations = GetParamLocation(funcDef->Type()->ParamTypes(), retType);

  if (funcDef->Type()->Variadic()) {
    GenSaveArea(); // 'offset' is now the begin of save area
    int regOffset = retType ? offset_ + 8: offset_;
    int xregOffset = offset_ + 8 * 8;
    int byMemOffset = 16;
    for (size_t i = 0; i < locations.size(); i++) {
      if (locations[i][0] == 'm') {
        params[i]->SetOffset(byMemOffset);
        byMemOffset += 8;
      } else if (locations[i][0] == 'x') {
        params[i]->SetOffset(xregOffset);
        xregOffset += 16;
      } else {
        params[i]->SetOffset(regOffset);
        regOffset += 8;
      }
    }
  } else {
    int byMemOffset = 16;
    for (size_t i = 0; i < locations.size(); i++) {
      if (locations[i][0] == 'm') {
        params[i]->SetOffset(byMemOffset);
        byMemOffset += 8;
        continue;
      }
      params[i]->SetOffset(Push(locations[i]));
    }
  }

  AllocObjects(funcDef->Body()->Scope(), params);

  for (auto stmt: funcDef->body_->stmts_) {
    Visit(stmt);
  }

  EmitLabel(funcDef->retLabel_->Label());
  Emit("leaveq");
  Emit("retq");
}


void Generator::GenSaveArea()
{
  static const int begin = -176;
  int offset = begin;
  for (auto reg: regs) {
    Emit("movq #%s, %d(#rbp)", reg, offset);
    offset += 8;
  }
  Emit("testb #al, #al");
  auto label = LabelStmt::New();
  Emit("je %s", label->Label().c_str());
  for (auto xreg: xregs) {
    Emit("movaps #%s, %d(#rbp)", xreg, offset);
    offset += 16;
  }
  assert(offset == 0);
  EmitLabel(label->Label());

  offset_ = begin;
}


void Generator::VisitTranslationUnit(TranslationUnit* unit)
{
  for (auto extDecl: unit->ExtDecls()) {
    Visit(extDecl);

    if (rodatas_.size())
      Emit(".section .rodata");
    for (auto rodata: rodatas_) {
      if (rodata.align_ == 1) {// Literal
        EmitLabel(rodata.label_);
        Emit(".string \"%s\"", rodata.sval_.c_str());
      } else if (rodata.align_ == 4) {
        Emit(".align 4");
        EmitLabel(rodata.label_);
        Emit(".long %d", static_cast<int>(rodata.ival_));
      } else {
        Emit(".align 8");
        EmitLabel(rodata.label_);
        Emit(".quad %ld", rodata.ival_);
      }
    }
    rodatas_.clear();

    for (auto staticDecl: staticDecls_) {
      GenStaticDecl(staticDecl);
    }
    staticDecls_.clear();
  }
}


void Generator::Gen()
{
  Emit(".file \"%s\"", inFileName.c_str());

  VisitTranslationUnit(parser_->Unit());
}


void Generator::EmitLoad(const std::string& addr, Type* type)
{
  assert(type->IsScalar());
  EmitLoad(addr, type->Width(), type->IsFloat());
}


void Generator::EmitLoad(const std::string& addr, int width, bool flt)
{
  auto load = GetLoad(width, flt);
  auto des = GetDes(width == 4 ? 4: 8, flt);
  Emit("%s %s, #%s", load, addr.c_str(), des);
}


void Generator::EmitStore(const std::string& addr, Type* type)
{
  EmitStore(addr, type->Width(), type->IsFloat());
}


void Generator::EmitStore(const std::string& addr, int width, bool flt)
{
  auto store = GetInst("mov", width, flt);
  auto des = GetDes(width, flt);
  Emit("%s #%s, %s", store.c_str(), des, addr.c_str());
}


void Generator::Emit(const char* format, ...)
{
  fprintf(outFile_, "\t");

  std::string str(format);
  auto pos = str.find(' ');
  if (pos != std::string::npos) {
    str[pos] = '\t';
    while ((pos = str.find('#', pos)) != std::string::npos)
      str.replace(pos, 1, "%%");
  }
  
  va_list args;
  va_start(args, format);
  vfprintf(outFile_, str.c_str(), args);
  va_end(args);
  fprintf(outFile_, "\n");
}


void Generator::EmitLabel(const std::string& label)
{
  fprintf(outFile_, "%s:\n", label.c_str());
}


void LValGenerator::VisitBinaryOp(BinaryOp* binary)
{
  assert(binary->op_ == '.');

  addr_ = LValGenerator().GenExpr(binary->lhs_);
  const auto& name = binary->rhs_->Tok()->str_;
  auto structType = binary->lhs_->Type()->ToStruct();
  auto member = structType->GetMember(name);

  addr_.offset_ += member->Offset();
  addr_.bitFieldBegin_ = member->bitFieldBegin_;
  addr_.bitFieldWidth_ = member->bitFieldWidth_;
}


void LValGenerator::VisitUnaryOp(UnaryOp* unary)
{
  assert(unary->op_ == Token::DEREF);
  Generator().VisitExpr(unary->operand_);
  Emit("movq #rax, #r10");

  addr_ = {"", "r10", 0};
}


void LValGenerator::VisitObject(Object* obj)
{
  if (obj->Anonymous()) {
    assert(obj->Decl());
    Generator().Visit(obj->Decl());
    obj->SetDecl(nullptr);
  }

  if (obj->IsStatic()) {
    addr_ = {ObjectLabel(obj), "rip", 0};
  } else {
    addr_ = {"", "rbp", obj->Offset()};
  }
}


// The identifier must be function
void LValGenerator::VisitIdentifier(Identifier* ident)
{
  assert(!ident->ToTypeName());

  // Function address
  addr_ = {ident->Name(), "", 0};
}


void LValGenerator::VisitTempVar(TempVar* tempVar)
{
  addr_ = {"%ecx", "", 0};
}


std::string ObjectAddr::Repr() const
{
  auto ret = _base.size() ? "(%" + _base + ")": "";
  if (label_.size() == 0) {
    if (offset_ == 0)
      return ret;
    else
      return std::to_string(offset_) + ret;
  } else {
    if (offset_ == 0)
      return label_ + ret;
    else
      return label_ + "+" + std::to_string(offset_) + ret;
  }
}


StaticInitializer Generator::GetStaticInit(const Initializer& init)
{
  // Delay until code gen
  auto width = init.type_->Width();
  if (init.type_->IsInteger()) {
    auto val = Evaluator<long>().Eval(init.expr_);
    return {init.offset_, width, val, ""};
  } else if (init.type_->IsFloat()) {
    auto val = Evaluator<double>().Eval(init.expr_);
    auto lval = *reinterpret_cast<long*>(&val);
    return {init.offset_, width, lval, ""};
  } else if (init.type_->ToPointer()) {
    auto addr = Evaluator<Addr>().Eval(init.expr_);
    return {init.offset_, width, addr.offset_, addr.label_};
  } else {
    assert(false);
    return StaticInitializer(); //Make compiler happy
  }
}
