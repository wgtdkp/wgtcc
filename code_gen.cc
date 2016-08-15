#include "code_gen.h"


static std::string GetObjectAddr(Object* obj)
{
    if ((obj->Linkage() != L_NONE) || (obj->Storage() & S_STATIC)) {
        return obj->Label() + "(#rip)";
    } else {
        return std::to_string(obj->Offset()) + "(#rbp)";
    }
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


static void Push(const char* reg)
{
    if (reg[0] == 'x' && reg[1] == 'm' && reg[2] == 'm') {
        Emit("sub $8, #rsp");
        Emit("movsd #%s, (#rsp)", reg);
    } else {
        Emit("push #%s", reg);
    }
}


static void Pop(const char* reg)
{
    if (reg[0] == 'x' && reg[1] == 'm' && reg[2] == 'm') {
        Emit("movsd (#rsp), #%s", reg);
        Emit("add $8, #rsp");
    } else {
        Emit("pop #%s", reg);
    }
}


static inline void Spill(bool flt)
{
    Push(flt ? "xmm0": "rax");
}


static inline void Restore(bool flt)
{
    const char* des = flt ? "xmm0": "rax";
    const char* backup = flt ? "xmm1": "rcx";
    const char* inst = flt ? "movsd": "mov";
    Emit("%s #%s, #%s", inst, des, backup);
    Pop(des);
}

static inline void GetOperands(const char*& des,
        const char*& src, bool flt, iht width)
{
    src = flt ? "xmm1": (width == 8 ? "rcx": "ecx");
    des = flt ? "xmm0": (width == 8 ? "rax": "eax");
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
void Generator::GenBinaryOp(BinaryOp* binary)
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
    GetOperands(des, src, flt, width);
}


void Generator::GenSubScriptingOp(BinaryOp* subScript)
{
    // As the lhs will always be array
    // we expect the address of lhs in %rax
    subScript->_lhs->Accept(this);
    Spill(false);
    subScript->_rhs->Accept(this);
    Restore(false);

    auto width = ref->Type()->Width();
    auto flt = ref->Type()->IsFloat();
    if (_expectLVal || !subScript->Type()->IsScalar()) {
        Emit("lea (#rax, #rcx, %d), #rax", width);
    } else {
        const char* src;
        const char* des;
        auto load = GetLoad(flt, width);
        GetOperands(des, src, flt, width);
        Emit("%s (#rax, #rcx, %d), %s", load, width, des);
    }
}


void Generator::GenMemberRefOp(BinaryOp* ref)
{
    // As the lhs will always be struct/union
    // we expect the address of lhs in %rax
    ref->_lhs->Accept(this);

    auto offset = ref->_rhs->Offset();
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
    auto width = assgin->Type()->Width();
    if (assign->Type()->IsScalar()) {
        auto inst = flt ? (width == 8 ? "movsd": "movss"): "mov";
        const char* src;
        const char* des;
        GetOperands(des, src, flt, width);
        Emit("%s %s, (%s)", src, des);
    } else {

    }
}


void Generator::GenCompOp(BinaryOp* binary)
{
    auto type = binary->Type()->ToArithmType();

    auto flt = type->IsFloat();
    auto width = type->Width();
    auto sign = !(type->Tag() & T_UNSIGNED);

    auto cmp = flt ? (width == 4 ? "ucomiss": "ucomisd"): "cmp";
    const char* set;
    switch (binary->_op) {
    case '<':
        set = (flt || !sign) ? "setb": "setl";
        break;
    case '>':
        set = (flt || !sign) ? "seta": "setg";
        break;
    case Token::LE_OP:
        set = (flt || !sign) ? "setbe": "setle";
        break;
    case Token::GE_OP:

    case Token::EQ_OP:
    case Token::NE_OP:
    }

    auto cmp = flt ? (width == 8 ? "ucomisd": "ucomiss"): "cmp";
    auto src = flt ? "#xmm1": "#rcx";
    auto des = flt ? "#xmm0": "#rax";
    Emit("%s %s, %s", cmp, des, scr);
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
        Emit("imul $%d, #rax", width);
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
    
    if (_expectLVal || !deref->type()->IsScalar()) {
        // Just let it!
    } else {
        auto width = deref->Type()->Width();
        auto flt = deref->Type()->IsFloat();
        auto load = GetLoad(flt, width);
        const char* src;
        const char* des;
        Emit("%s (#rax), %s", load, des);
    }
}


void Generator::GenObject(Object* obj)
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
        GetOperands(des, src, flt, width);
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
        const char* inst;
        switch (width) {
        case 1: inst = "movsbq"; break;
        case 2: inst = "movswq"; break;
        case 4: inst = "movl": break;
        case 8: inst = "movq"; break;
        }
        if (inst[4] == 0)
            return;
        if (srcType->ToArithmType()->Tag() & T_UNSIGNED)
            inst[3] = 'z';
        Emit("%s %s, #rax", inst, GetReg(width));
    }
}


void Generator::Emit(const char* format, ...)
{
    fprintf(_outFile, "\t");

    std::string str(format);
    auto pos = str.find(' ');
    if (!std::string::npos) {
        str[pos] = '\t';
        while ((pos = str.find('#', pos)) != std::string::npos)
            str.replace(pos, 1, "%%");
    } else {
        assert(false);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(_outFile, str.c_str(), args);
    va_end(args);
    fprintf(_outFile, "\n");
}
