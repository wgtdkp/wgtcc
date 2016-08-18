#include "code_gen.h"

#include "parser.h"
#include "token.h"

#include <cstdarg>


extern std::string inFileName;
extern std::string outFileName;



static inline std::string ObjectLabel(Object* obj)
{
    assert(obj->IsStatic());
    if (obj->Linkage() == L_NONE)
        return obj->Name() + "." + std::to_string((long)obj);
    return obj->Name();
}

static const char* GetObjectAddr(Object* obj)
{
    return "";
    /*
    if ((obj->Linkage() != L_NONE) || (obj->Storage() & S_STATIC)) {
        return obj->Label() + "(#rip)";
    } else {
        return std::to_string(obj->Offset()) + "(#rbp)";
    }
    */
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
    const char* inst = flt ? "movsd": "mov";
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
    if (op == ']')
        return GenSubScriptingOp(binary);
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
    if (_expectLVal || !subScript->Type()->IsScalar()) {
        Emit("lea (#rax, #rcx, %d), #rax", width);
    } else {
        const char* src;
        const char* des;
        auto load = GetLoad(flt, width);
        GetOperands(src, des, flt, width);
        Emit("%s (#rax, #rcx, %d), %s", load, width, des);
    }
}


void Generator::GenMemberRefOp(BinaryOp* ref)
{
    // As the lhs will always be struct/union
    // we expect the address of lhs in %rax
    ref->_lhs->Accept(this);

    auto offset = ref->_rhs->ToObject()->Offset();
    if (_expectLVal || !ref->Type()->IsScalar()) {
        Emit("lea %d(#rax), #rax", offset);
    } else {
        auto width = ref->Type()->Width();
        auto flt = ref->Type()->IsFloat();
        auto load = GetLoad(flt, width);
        if (flt)
            Emit("%s %d(#rax), #rax", load, offset);
        else
            Emit("%s %d(#rax), #xmm0", load, offset);
    }
}


void Generator::GenAssignOp(BinaryOp* assign)
{
    _expectLVal = true;
    assign->_lhs->Accept(this);
    _expectLVal = false;
    Spill(false);
    assign->_rhs->Accept(this);
    Restore(false);

    auto flt = assign->Type()->IsFloat();
    auto width = assign->Type()->Width();
    if (assign->Type()->IsScalar()) {
        auto inst = flt ? (width == 8 ? "movsd": "movss"): "mov";
        const char* src;
        const char* des;
        GetOperands(src, des, flt, width);
        Emit("%s %s, (%s)", inst, src, des);
    } else {

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
    // Whatever, the address of the operand is in %rax    
    deref->_operand->Accept(this);
    
    if (_expectLVal || !deref->Type()->IsScalar()) {
        // Just let it!
    } else {
        auto width = deref->Type()->Width();
        auto flt = deref->Type()->IsFloat();
        auto load = GetLoad(flt, width);
        const char* src;
        const char* des;
        GetOperands(src, des, flt, width);
        Emit("%s (#rax), %s", load, des);
    }
}


void Generator::VisitObject(Object* obj)
{
    // TODO(wgtdkp): handle static object
    if (_expectLVal || !obj->Type()->IsScalar()) {
        // Return the address of the object in rax
        Emit("lea %s, #rax", GetObjectAddr(obj));
    } else {
        auto width = obj->Type()->Width();
        auto flt = obj->Type()->IsFloat();
        auto load = GetLoad(flt, width);
        const char* src;
        const char* des;
        GetOperands(src, des, flt, width);
        Emit("%s %s, %s", load, GetObjectAddr(obj), des);
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

