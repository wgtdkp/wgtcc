#include "code_gen.h"

#include "parser.h"
#include "token.h"

#include <cstdarg>


extern std::string inFileName;
extern std::string outFileName;


Parser* Generator::_parser = nullptr;
FILE* Generator::_outFile = nullptr;

std::string Generator::_cons;
RODataList Generator::_rodatas;

/*
std::string operator+(const char* lhs, const std::string&& rhs)
{
    return std::string(lhs) + rhs;
}
*/

static inline std::string ObjectLabel(Object* obj)
{
    assert(obj->IsStatic());
    if (obj->Linkage() == L_NONE)
        return obj->Name() + "." + std::to_string((long)obj);
    return obj->Name();
}


static const char* GetLoad(int width, bool flt=false)
{
    switch (width) {
    case 1: return "movzbq";
    case 2: return "movzwq";
    case 4: return !flt ? "mov": "movss";
    case 8: return !flt ? "mov": "movsd";
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


static inline void GetOperands(const char*& src,
        const char*& des, bool flt, int width)
{
    src = flt ? "xmm1": (width == 8 ? "rcx": "ecx");
    des = flt ? "xmm0": (width == 8 ? "rax": "eax");
}


void Generator::Push(const char* reg)
{
    if (reg[0] == 'x' && reg[1] == 'm' && reg[2] == 'm') {
        Emit("sub $8, #rsp");
        Emit("movsd #%s, (#rsp)", reg);
    } else {
        Emit("push #%s", reg);
    }
}


void Generator::Pop(const char* reg)
{
    if (reg[0] == 'x' && reg[1] == 'm' && reg[2] == 'm') {
        Emit("movsd (#rsp), #%s", reg);
        Emit("add $8, #rsp");
    } else {
        Emit("pop #%s", reg);
    }
}


void Generator::Restore(bool flt)
{
    const char* src = flt ? "xmm0": "rax";
    const char* des = flt ? "xmm1": "rcx";
    const char* inst = flt ? "movsd": "movq";
    Emit("%s #%s, #%s", inst, src, des);
    Pop(src);
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
    auto op = binary->_op;

    if (op == '=')
        return GenAssignOp(binary);
    if (op == Token::AND_OP)
        return GenAndOp(binary);
    if (op == Token::OR_OP)
        return GenOrOp(binary);
    if (op == '.')
        return GenMemberRefOp(binary);
    //if (op == ']')
    //    return GenSubScriptingOp(binary);
    if (binary->Type()->ToPointerType())
        return GenPointerArithm(binary);


    const char* inst;
    auto width = binary->Type()->Width();
    auto flt = binary->Type()->IsFloat();
    auto sign = !flt && !(binary->Type()->ToArithmType()->Tag() & T_UNSIGNED);

    binary->_lhs->Accept(this);
    Spill(flt);
    binary->_rhs->Accept(this);
    Restore(flt);

    switch (op) {
    case '+': inst = flt ? (width == 4 ? "addss": "addsd"): "add"; break;
    case '-': inst = flt ? (width == 4 ? "subss": "subsd"): "sub"; break;
    case '*': inst = flt ? (width == 4 ? "mulss": "mulsd"): "mul"; break; 
    case '/': case '%': return GenDivOp(flt, sign, width, op);
    case '<': 
        return GenCompOp(flt, width, (flt || !sign) ? "setb": "setl");
    case '>':
        return GenCompOp(flt, width, (flt || !sign) ? "seta": "setg");
    case Token::LE_OP:
        return GenCompOp(flt, width, (flt || !sign) ? "setbe": "setle");
    case Token::GE_OP:
        return GenCompOp(flt, width, (flt || !sign) ? "setae": "setge");
    case Token::EQ_OP:
        return GenCompOp(flt, width, "sete");
    case Token::NE_OP:
        return GenCompOp(flt, width, "setne");

    case '|': inst = "or"; break;
    case '&': inst = "and"; break;
    case '^': inst = "xor"; break;
    case Token::LEFT_OP: inst = "sal"; break;
    case Token::RIGHT_OP: inst = sign ? "sar": "shr"; break;
    }

    const char* src;
    const char* des;
    GetOperands(src, des, flt, width);
    Emit("%s %s, %s", inst, src, des);
}


void Generator::GenAndOp(BinaryOp* andOp)
{
    // TODO(wgtdkp):
}


void Generator::GenOrOp(BinaryOp* orOp)
{
    // TODO(wgtdkp):
}


/*
void Generator::GenSubScriptingOp(BinaryOp* subScript)
{
    // As the lhs will always be array
    // we expect the address of lhs in %rax
    subScript->_lhs->Accept(this);
    Spill(false);
    subScript->_rhs->Accept(this);
    Restore(false);

    auto width = subScript->Type()->Width();
    auto flt = subScript->Type()->IsFloat();
    if (!subScript->Type()->IsScalar()) {
        Emit("lea (#rax, #rcx, %d), #rax", width);
    } else {
        const char* src;
        const char* des;
        auto load = GetLoad(flt, width);
        GetOperands(src, des, flt, width);
        Emit("%s (#rax, #rcx, %d), %s", load, width, des);
    }
}
*/


void Generator::GenMemberRefOp(BinaryOp* ref)
{
    // As the lhs will always be struct/union 
    auto addr = LValGenerator().GenExpr(ref).Repr();

    if (!ref->Type()->IsScalar()) {
        Emit("lea %s, #rax", addr.c_str());
    } else {
        EmitLoad(addr, ref->Type());
    }
}


void Generator::GenAssignOp(BinaryOp* assign)
{
    // The base register of addr is %rdx
    auto addr = LValGenerator().GenExpr(assign->_lhs);
    GenExpr(assign->_rhs);

    if (assign->Type()->IsScalar()) {
        EmitStore(addr.Repr(), assign->Type());
    } else {
        // struct/union type
        // The address of rhs is in %rax
        auto len = assign->Type()->Width();
        int widths[] = {8, 4, 2, 1};
        int offset = 0;

        for (auto width: widths) {
            while (len >= width) {
                auto mov = GetInst("mov", width, false);
                Emit("%s %d(#rax), #rcx", mov.c_str(), offset);
                Emit("%s #rcx, %s", mov.c_str(), addr.Repr().c_str());
                addr._offset += width;
                offset += width;
                len -= width;
            }
        }
    }
}


void Generator::GenCompOp(bool flt, int width, const char* set)
{
    auto cmp = flt ? (width == 8 ? "ucomisd": "ucomiss"): "cmp";
    
    const char* src;
    const char* des;
    GetOperands(src, des, flt, width);

    Emit("%s %s, %s", cmp, src, des);
    Emit("%s #al", set);
    Emit("movzbl #al, #rax");
}


void Generator::GenDivOp(bool flt, bool sign, int width, int op)
{
    if (flt) {
        auto inst = width == 4 ? "divss": "divsd";
        Emit("%s #xmm1, #xmm0", inst);
        return;
    } else if (!sign) {
        Emit("xor #rdx, #rdx");
        Emit("div #rcx");
    } else {
        Emit("cqto");
        Emit("idiv #rcx");            
    }
    if (op == '%')
        Emit("mov #rdx, #rax");
}

 
void Generator::GenPointerArithm(BinaryOp* binary)
{
    // For '+', we have swap _lhs and _rhs to ensure that 
    // the pointer is at lhs.
    binary->_lhs->Accept(this);
    Push("rcx");
    Push("rax");
    binary->_rhs->Accept(this);
    
    auto type = binary->_lhs->Type()->ToPointerType()->Derived();
    if (type->Width() > 1)
        Emit("imul $%d, #rax", type->Width());
    Emit("mov #rax, #rcx");
    Pop("rax");
    if (binary->_op == '+')
        Emit("add #rcx, #rax");
    else
        Emit("sub #rcx, #rax");
    Pop("rcx");
}


void Generator::GenDerefOp(UnaryOp* deref)
{
    auto addr = LValGenerator().GenExpr(deref->_operand).Repr();
    Emit("lea %s, #rax", addr.c_str());
}


void Generator::VisitObject(Object* obj)
{
    auto addr = LValGenerator().GenExpr(obj).Repr();

    // TODO(wgtdkp): handle static object
    if (!obj->Type()->IsScalar()) {
        // Return the address of the object in rax
        Emit("lea %s, #rax", addr.c_str());
    } else {
        EmitLoad(addr, obj->Type());
    }
}


void Generator::GenCastOp(UnaryOp* cast)
{
    auto desType = cast->Type();
    auto srcType = cast->_operand->Type();

    if (srcType->IsFloat() && desType->IsFloat()) {
        if (srcType->Width() == desType->Width())
            return;
        const char* inst = srcType->Width() == 4 ? "movss2sd": "movsd2ss";
        Emit("%s #xmm0, #xmm0", inst);
    } else if (srcType->IsFloat()) {
        const char* inst = srcType->Width() == 4 ? "cvttss2si": "cvttsd2si";
        Emit("%s #xmm0, #rax", inst);
    } else if (desType->IsFloat()) {
        const char* inst = desType->Width() == 4 ? "cvtsi2ss": "cvtsi2sd";
        Emit("%s #rax, #xmm0", inst);
    } else {
        int width = srcType->Width();
        auto sign = !(srcType->ToArithmType()->Tag() & T_UNSIGNED);
        const char* inst;
        switch (width) {
        case 1: inst = sign ? "movsbq": "movzbq"; break;
        case 2: inst = sign ? "movswq": "movzwq"; break;
        case 4: inst = "movl"; break;
        case 8: inst = "movq"; break;
        }
        if (inst[4] == 0)
            return;
        Emit("%s %s, #rax", inst, GetReg(width));
    }
}


void Generator::VisitUnaryOp(UnaryOp* unaryOp)
{

}


void Generator::GenPrefixIncDec(UnaryOp* unary, const std::string& inst)
{
    // Need a special base register to reduce register conflict
    auto addr = LValGenerator().GenExpr(unary).Repr();
    auto des = EmitLoad(addr, unary->Type());

    Constant* cons;
    auto pointerType = unary->Type()->ToPointerType();
     if (pointerType) {
        long width = pointerType->Derived()->Width();
        cons = Constant::New(unary->Tok(), T_LONG, width);
    } else if (unary->Type()->IsInteger()) {
        cons = Constant::New(unary->Tok(), T_LONG, 1L);
    } else {
        if (unary->Type()->Width() == 4)
            cons = Constant::New(unary->Tok(), T_FLOAT, 1.0);
        else
            cons = Constant::New(unary->Tok(), T_DOUBLE, 1.0);
    }
    VisitConstant(cons);
    auto consLabel = _cons;

    auto addSub = GetInst(inst, unary->Type());    
    Emit("%s %s, %s", addSub.c_str(), consLabel.c_str(), des.c_str());
    
    EmitStore(addr, unary->Type());
}


void Generator::GenPostfixIncDec(UnaryOp* unary, const std::string& inst)
{
    // Need a special base register to reduce register conflict
    auto addr = LValGenerator().GenExpr(unary).Repr();
    auto des = EmitLoad(addr, unary->Type());
    auto saved = Save(des);

    Constant* cons;
    auto pointerType = unary->Type()->ToPointerType();
     if (pointerType) {
        long width = pointerType->Derived()->Width();
        cons = Constant::New(unary->Tok(), T_LONG, width);
    } else if (unary->Type()->IsInteger()) {
        cons = Constant::New(unary->Tok(), T_LONG, 1L);
    } else {
        if (unary->Type()->Width() == 4)
            cons = Constant::New(unary->Tok(), T_FLOAT, 1.0);
        else
            cons = Constant::New(unary->Tok(), T_DOUBLE, 1.0);
    }
    VisitConstant(cons);
    const std::string& consLabel = _cons;

    auto addSub = GetInst(inst, unary->Type());    
    Emit("%s %s, %s", addSub.c_str(), consLabel.c_str(), des.c_str());

    EmitStore(addr, unary->Type());
    Exchange(des, saved);
}


void Generator::Exchange(const std::string& lhs, const std::string& rhs)
{
    if (lhs == "xmm0" || rhs == "xmm0") {
        Emit("movsd #%s, #xmm2", lhs.c_str());
        Emit("movsd #%s, #%s", rhs.c_str(), lhs.c_str());
        Emit("movsd #xmm2, #%s", rhs.c_str());
    } else {
        Emit("xchgq #%s, #%s", lhs.c_str(), rhs.c_str());
    }

}


void Generator::VisitConditionalOp(ConditionalOp* condOp)
{

}


void Generator::VisitFuncCall(FuncCall* funcCall)
{

}


void Generator::VisitEnumerator(Enumerator* enumer)
{

}


void Generator::VisitIdentifier(Identifier* ident)
{

}


void Generator::VisitConstant(Constant* cons)
{
    if (cons->Type()->IsInteger()) {
        _cons = "$" + std::to_string(cons->IVal());
    } else if (cons->Type()->IsFloat()) {
        double valsd = cons->FVal();
        float  valss = valsd;
        // TODO(wgtdkp): Add rodata
        auto width = cons->Type()->Width();
        long val = width == 4 ? *(int*)&valss: *(long*)&valsd;
        const ROData& rodata = ROData(val, width);
        _rodatas.push_back(rodata);
        _cons = "$" + rodata._label;
    } else {
        const ROData& rodata = ROData(*cons->SVal());
        _rodatas.push_back(rodata);
        _cons = "$" + rodata._label;
    }
}


void Generator::VisitTempVar(TempVar* tempVar)
{

}


void Generator::VisitInitialization(Initialization* init)
{
    if (init->Obj()->IsStatic()) {
        for (auto initer: init->StaticInits()) {
            switch (initer._width) {
            case 1:
                Emit(".byte %d", static_cast<char>(initer._val));
                break;
            case 2:
                Emit(".value %d", static_cast<short>(initer._val));
                break;
            case 4:
                Emit(".long %d", static_cast<int>(initer._val));
                break;
            case 8: 
                if (initer._label.size() == 0)
                    Emit(".quad %ld", initer._val);
                else
                    Emit(".quad %s+%ld", initer._label.c_str(), initer._val);
                break;
            default: assert(false);
            }
        }
    }
}


void Generator::VisitEmptyStmt(EmptyStmt* emptyStmt)
{

}


void Generator::VisitIfStmt(IfStmt* ifStmt)
{

}


void Generator::VisitJumpStmt(JumpStmt* jumpStmt)
{

}


void Generator::VisitReturnStmt(ReturnStmt* returnStmt)
{

}


void Generator::VisitLabelStmt(LabelStmt* labelStmt)
{

}


void Generator::VisitCompoundStmt(CompoundStmt* compoundStmt)
{

}

void Generator::VisitFuncDef(FuncDef* funcDef)
{

}


void Generator::VisitTranslationUnit(TranslationUnit* unit)
{
    
}


void Generator::Gen(void)
{
    Emit(".file %s", inFileName.c_str());
    Emit(".data");

    for (auto obj: _parser->StaticObjects()) {
        auto label = ObjectLabel(obj);
        auto width = obj->Type()->Width();
        auto align = obj->Type()->Align();

        // omit the external without initilizer
        if ((obj->Storage() & S_EXTERN) && !obj->Init())
            continue;

        auto glb = obj->Linkage() == L_EXTERNAL ? ".globl": ".local";
        Emit("%s %s", glb, label.c_str());

        if (!obj->Init()) {    
            Emit(".comm %s, %d, %d", label.c_str(), width, align);
        } else {
            Emit(".align %d", align);
            Emit(".type %s, @object", label.c_str());
            Emit(".size %s, %d", label.c_str(), width);
            EmitLabel(label.c_str());
            VisitInitialization(obj->Init());
        }

    }

    VisitTranslationUnit(_parser->Unit());
}


std::string Generator::EmitLoad(const std::string& addr, Type* type)
{
    assert(type->IsScalar());

    auto width = type->Width();
    auto flt = type->IsFloat();
    auto load = GetLoad(width, flt);
    const char* src;
    const char* des;
    GetOperands(src, des, width, flt);
    Emit("%s, %s, %s", load, addr.c_str(), des);
    return des;
}


void Generator::EmitStore(const std::string& addr, Type* type)
{
    auto store = GetInst("mov", type);
    const char* src;
    const char* des;
    GetOperands(src, des, type->Width(), type->IsFloat());
    Emit("%s, #%s, %s", store.c_str(), src, addr.c_str());

}


void Generator::Emit(const char* format, ...)
{
    fprintf(_outFile, "\t");

    std::string str(format);
    auto pos = str.find(' ');
    if (pos != std::string::npos) {
        str[pos] = '\t';
        while ((pos = str.find('#', pos)) != std::string::npos)
            str.replace(pos, 1, "%%");
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(_outFile, str.c_str(), args);
    va_end(args);
    fprintf(_outFile, "\n");
}


void Generator::EmitLabel(const std::string& label)
{
    fprintf(_outFile, "%s:\n", label.c_str());
}



void LValGenerator::VisitBinaryOp(BinaryOp* binary)
{
    assert(binary->_op == '.');

    auto addr = LValGenerator().GenExpr(binary->_lhs);
    auto name = binary->_rhs->Tok()->Str();
    auto structType = binary->_lhs->Type()->ToStructUnionType();
    auto member = structType->GetMember(name);
    addr._offset += member->Offset();

    _addr = addr;
}


void LValGenerator::VisitUnaryOp(UnaryOp* unary)
{
    assert(unary->_op == Token::DEREF);


}


void LValGenerator::VisitObject(Object* obj)
{
    if (obj->IsStatic()) {
        _addr = {ObjectLabel(obj), "rip", 0};
    } else {
        _addr = {"", "rbp", obj->Offset()};
    }
}


// The identifier must be function
void LValGenerator::VisitIdentifier(Identifier* ident)
{
    assert(!ident->ToTypeName());

    // Function address
    _addr = {ident->Name(), "rip", 0};
}



std::string ObjectAddr::Repr(void)
{
    auto ret = "(#" + _base + ")";
    if (_label.size() == 0) {
        if (_offset == 0)
            return ret;
        else
            return std::to_string(_offset) + ret;
    } else {
        if (_offset == 0)
            return _label + ret;
        else
            return _label + "+" + std::to_string(_offset) + ret;
    }
}
