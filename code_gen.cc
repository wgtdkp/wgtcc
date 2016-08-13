#include "code_gen.h"

#include "ast.h"
#include "parser.h"
#include "type.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <iterator>
#include <queue>


#define REG(reg)    (Register::Get(Register::reg))


MemPoolImp<Immediate>   immediatePool;
MemPoolImp<Memory>      memoryPool;
MemPoolImp<Register>    registerPool;

static auto immFalse = Immediate::New(0, 4);
static auto immTrue = Immediate::New(1, 4);


/*
 * Register
 */

Register* Register::_regs[N_REG] = {
    New(RAX), New(RBX), New(RCX), New(RDX),
    New(RSI), New(RDI), New(RBP), New(RSP),
    New(R8), New(R9), New(R10), New(R11),
    New(R12), New(R13), New(R14), New(R15),
    New(XMM0), New(XMM1), New(XMM2), New(XMM3),
    New(XMM4), New(XMM5), New(XMM6), New(XMM7),
    New(RIP)
};

const char* Register::_reprs[N_REG][4] = {
    {"al", "ax", "eax", "rax"},
    {"bl", "bx", "ebx", "rbx"},
    {"cl", "cx", "ecx", "rcx"},
    {"dl", "dx", "edx", "rdx"},
    {"dil", "di", "edx", "rdx"},
    {"sil", "si", "esi", "rsi"},
    {"bpl", "bp", "ebp", "rbp"},
    {"spl", "sp", "esp", "rsp"},
    {"r8l", "r8w", "r8d", "r8"},
    {"r9l", "r9w", "r9d", "r9"},
    {"r10l", "r10w", "r10d", "r10"},
    {"11l", "r11w", "r11d", "r11"},
    {"r12l", "r12w", "r12d", "r12"},
    {"r13l", "r13w", "r13d", "r13"},
    {"r14l", "r14w", "r14d", "r14"},
    {"r15l", "r15w", "r15d", "r15"},
    {"xmm0", "xmm0", "xmm0", "xmm0"},
    {"xmm1", "xmm1", "xmm1", "xmm1"},
    {"xmm2", "xmm2", "xmm2", "xmm2"},
    {"xmm3", "xmm3", "xmm3", "xmm3"},
    {"xmm4", "xmm4", "xmm4", "xmm4"},
    {"xmm5", "xmm5", "xmm5", "xmm5"},
    {"xmm6", "xmm6", "xmm6", "xmm6"},
    {"xmm7", "xmm7", "xmm7", "xmm7"},
    {"rip", "rip", "rip", "rip"}
};

Register* Generator::_argRegs[N_ARG_REG] = {
    REG(RDI), REG(RSI), REG(RDX),
    REG(RCX), REG(R8), REG(R9)
};

Register* Generator::_argVecRegs[N_ARG_VEC_REG] = {
    REG(XMM0), REG(XMM1), REG(XMM2), REG(XMM3),
    REG(XMM4), REG(XMM5), REG(XMM6), REG(XMM7)
};


Register* Register::New(int tag, int width)
{
    auto ret = new (registerPool.Alloc()) Register(tag, width);
    ret->_pool = &registerPool;

    return ret;
}
    
std::string Register::Repr(void) const
{
    int idx;
    switch (_width) {
    case 1: idx = 0; break;
    case 2: idx = 1; break;
    case 4: idx = 2; break;
    case 8: idx = 3; break;
    default: assert(false);
    }

    return std::string("%") + _reprs[_tag][idx];
}


/*
 * Immediate
 */

Immediate* Immediate::New(long val, int width)
{
    auto ret = new (immediatePool.Alloc()) Immediate(val, width);
    ret->_pool = &immediatePool;

    return ret;
}

std::string Immediate::Repr(void) const
{
    std::string repr = "$";
    repr += std::to_string(Val());

    return repr;
}


/*
 * Memory
 */

Memory* Memory::New(bool isFloat, int width, Register* base, int disp,
        Register* index, int scale) {
    auto ret = new (memoryPool.Alloc())
            Memory(isFloat, width, base, disp, index, scale);
    ret->_pool = &memoryPool;

    return ret;
}

Memory* Memory::New(Type* type, Register* base, int disp,
        Register* index, int scale) {
    auto ret = new (memoryPool.Alloc())
            Memory(type->IsFloat(), type->Width(), base, disp, index, scale);
    ret->_pool = &memoryPool;

    return ret;
}


Register* Generator::Load(Register* des, Operand* src)
{
    std::string load;
    if (src->ToRegister()) {
        return src->ToRegister();
    } else if (src->ToImmediate()) {
        auto imm = src->ToImmediate();
        if (des == nullptr)
            des = AllocReg(imm->Width(), false);
        load = imm->Width() == 4 ? "movl": "movq";
        EmitLoad(load, des, imm->Val());
        return des;
    }

    auto mem = src->ToMemory();
    if (des == nullptr) {
        des = AllocReg(mem->Width(), mem->IsFloat());
        _except = des;
    }
    if (mem->IsFloat()) {
        if (mem->Width() == 4)
            load = "movss";
        else
            load = "movsd";
    } else {
        switch (mem->Width()) {
        case 1: load = "movb"; break;
        case 2: load = "movw"; break;
        case 4: load = "movl"; break;
        case 8: load = "movq"; break;
        }
    }
    EmitLoad(load, des, mem);

    return des;

    // We will never load a struct/union
}

std::string Memory::Repr(void) const
{
    std::string repr = std::to_string(_disp) + "(";
    
    repr += _base->Repr();
    if (_index) {
        repr += "," + _index->Repr() + std::to_string(_scale);
    }
    repr += ")";
    
    return repr;
}


static ParamClass Classify(Type* paramType, int offset=0)
{
    if (paramType->IsInteger() || paramType->ToPointerType()
            || paramType->ToArrayType()) {
        return ParamClass::INTEGER;
    }
    
    if (paramType->ToArithmType()) {
        auto type = paramType->ToArithmType();
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
    auto type = paramType->ToStructUnionType();
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

/*
static void FieldsIn8Bytes(std::vector<ParamClass>& res,
        StructUnionType* type, int idx)
{
    auto offset = idx * 8;
    auto p = type->Begin();
    auto q = std::next(p, 1);
    for (; q != type->End(); q++) {
        if (p->Offset() <= offset && q->Offset() > offset) {

        }
        p++;
    }
}
*/

/*
static ParamClass FieldClass(std::vector<ParamClass>& classes, int begin)
{
    auto leftClass = classes[begin++];
    ParamClass rightClass = (begin + 1 == types.size())
            ? leftClass: FieldClass(types, begin);
    
    if (leftClass == rightClass)
        return leftClass;
    
    if (leftClass == PC_NO_CLASS)
        return rightClass;
    else if (rightClass == PC_NO_CLASS)
        return leftClass;
    
    if (leftClass == PC_MEMORY || rightClass == PC_MEMORY)
        return PC_MEMORY;

    if (leftClass == PC_INTEGER || rightClass == PC_INTEGER)
        return PC_INTEGER;

    if (leftClass == PC_COMPLEX_X87 || rightClass == PC_COMPLEX_X87
            || leftClass == PC_X87_UP || rightClass = PC_X87_UP)
            || leftClass == PC_X87 || rightClass == PC_X87) {
        return PC_MEMORY;
    }
    
    return PC_SSE;
}
*/


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
 * <  cmp, setl, movzbl
 * >  cmp, setg, movzbl
 * <= cmp, setle, movzbl
 * >= cmp, setle, movzbl
 * == cmp, sete, movzbl
 * != cmp, setne, movzbl
 * && GenAndOp
 * || GenOrOp
 * [  GenSubScriptingOp
 * .  GenMemberRefOp
 */
 

//Expression
Operand* Generator::GenBinaryOp(BinaryOp* binaryOp)
{
    // TODO(wgtdkp):
    // Evaluate from left to right
    auto op = binaryOp->_op;

    if (op == Token::AND_OP) {
        return GenAndOp(binaryOp);
    } else if (op == Token::OR_OP) {
        return GenOrOp(binaryOp);
    }

/*
    auto lhs = binaryOp->_lhs->Accept(this);
    auto rhs = binaryOp->_rhs->Accept(this);
*/

    switch (op) {
    case '.':
        return GenMemberRefOp(binaryOp);
    case ']': {
        return GenSubScriptingOp(binaryOp);
    }
    case '+':
        return GenAddOp(binaryOp);
    case '-':
        break;//return GenSubOp(binaryOp->_lhs, binaryOp->_rhs);
    case '=':
        return GenAssignOp(binaryOp);
    }

/*
    if (!lhs->ToRegister() && !rhs->ToRegister()) {
        auto reg = AllocReg();
        EmitMOV(lhs, reg);
        lhs = reg;
    } else if (!lhs->ToRegister()) {
        // Operators obey commutative law
        if (rhs->ToRegister() && (op == '+'
                || op == '*' || op == '|' 
                || op == '&' || op == '^'
                || op == Token::EQ_OP
                || op == Token::NE_OP)) {
            std::swap(lhs, rhs);
        } else {
            auto reg = AllocReg();
            EmitMOV(lhs, reg);
            lhs = reg;
        }
    }
*/
    assert(false);
    return nullptr;
}

Memory* Generator::GenAssignOp(BinaryOp* assign)
{
    // TODO(wgtdkp):
    auto lhs = assign->_lhs->Accept(this)->ToMemory();
    auto rhs = Load(nullptr, assign->_rhs->Accept(this));

    assert(lhs);    
    
    if (assign->Type()->IsScalar()) {
        EmitStore(lhs, rhs);
    } else {
        // TODO(wgtdkp):
        // struct/union assignment
        Error("not implemented yet");
    }

    Free(rhs);
    return lhs;
}


/*
 * Expression a && b
 *     cmp $0, a
 *     je .L1
 *     cmp $0, b
 *     je .L1
 *     mov $1, %eax
 *     jmp .L2
 *.L1: mov $0, %eax
 *.L2:
 */

// TODO(wgtdkp):
// 1. both operands are constant
// 2. one of the operands is constant
Operand* Generator::GenAndOp(BinaryOp* binaryOp)
{
    LabelStmt* labelFalse = nullptr;

    auto lhs = Load(nullptr, binaryOp->_lhs->Accept(this));

    EmitCMP(0, lhs);
    Free(lhs);
    EmitJE(labelFalse = LabelStmt::New());

    auto labelTrue = LabelStmt::New();

    auto rhs = Load(nullptr, binaryOp->_rhs->Accept(this));

    EmitCMP(0, rhs);
    Free(rhs); 

    EmitJE(labelFalse);

    auto ret = Load(nullptr, immTrue);
    EmitJMP(labelTrue);

    EmitLabel(labelFalse);
    Load(ret, immFalse);
    EmitLabel(labelTrue);

    return ret;
}


Operand* Generator::GenOrOp(BinaryOp* binaryOp)
{
    LabelStmt* labelTrue = nullptr;
    
    auto lhs = Load(nullptr, binaryOp->_lhs->Accept(this));
    EmitCMP(0, lhs);
    Free(lhs);

    EmitJNE(labelTrue = LabelStmt::New());

    auto labelFalse = LabelStmt::New();

    auto rhs = Load(nullptr, binaryOp->_rhs->Accept(this));

    EmitCMP(0, rhs);
    Free(rhs);
    EmitJNE(labelTrue);

    auto ret = Load(nullptr, immFalse);
    EmitJMP(labelFalse);

    EmitLabel(labelTrue);
    Load(ret, immTrue);
    EmitLabel(labelFalse);

    return ret;
}


Memory* Generator::GenSubScriptingOp(BinaryOp* subScript)
{
    auto lhs = subScript->_lhs->Accept(this)->ToMemory();
    auto derivedType = subScript->_lhs->Type()->ToPointerType()->Derived();
    auto width = derivedType->Width();
    
    // Must do constant folding during parsing
    if (subScript->_rhs->ToConstant()) {
        auto rhs = subScript->_rhs->ToConstant();
        auto disp = rhs->IVal() * width;
        lhs->_disp += disp;
        lhs->_isFloat = derivedType->IsFloat();
        lhs->_width = derivedType->Width();
        return  lhs;        
    }

    auto rhs = Load(nullptr, subScript->_rhs->Accept(this));
    if (lhs->_index == nullptr) {
        lhs->_index = rhs;
        lhs->_scale = width;
        lhs->_isFloat = derivedType->IsFloat();
        lhs->_width = derivedType->Width();
        return  lhs;
    }

    auto reg = lhs->_index;
    EmitLEA(reg, lhs);
    Free(lhs->_base);

    return Memory::New(derivedType, reg, 0, rhs, width);
}

// Generate no assembly code
Memory* Generator::GenMemberRefOp(BinaryOp* binaryOp)
{
    auto lhs = binaryOp->_lhs->Accept(this)->ToMemory();
    auto rhs = binaryOp->_rhs->ToObject();
    assert(lhs);

    lhs->_disp += rhs->Offset();
    lhs->_isFloat = rhs->Type()->IsFloat();
    lhs->_width = rhs->Type()->Width();
    return lhs;
}


Register* Generator::GenAddOp(BinaryOp* binaryOp)
{
    // Evaluate from left to right
    auto lhs = Load(nullptr, binaryOp->_lhs->Accept(this));
    auto rhs = binaryOp->_rhs->Accept(this);
    
    
    
    assert(lhs && rhs);
    assert(lhs->Width() == rhs->Width());

    Emit(std::string("add"), lhs, rhs);
    
    // We done calc 'lhs + rhs', if rhs has alloc any register,
    // it will be freed.
    Free(rhs);
    return lhs; // Must be a register
}


Operand* Generator::GenUnaryOp(UnaryOp* unaryOp)
{
    // TODO(wgtdkp):
    switch (unaryOp->_op) {
    case Token::PREFIX_INC: break;
    case Token::PREFIX_DEC: break;
    case Token::POSTFIX_INC: break;
    case Token::POSTFIX_DEC: break;
    case Token::ADDR: break;
    case Token::DEREF:
        return GenDerefOp(unaryOp);
        break;
    case Token::PLUS: break;
    case Token::MINUS: break;
    case '~': break;
    case '!': break;
    case Token::CAST:
        return GenCastOp(unaryOp);
        break;
    default: Error("internal error");
    }

    return nullptr;
}

Memory* Generator::GenDerefOp(UnaryOp* deref)
{
    auto addr = Load(nullptr, deref->_operand->Accept(this));

    return Memory::New(deref->Type(), addr, 0);
}


/*
 * desType and srcType must be scalar type
 * It means that they are arithmetic type or pointer
 */

/* src: memory, des: register
 * src: char,   des: float  --- movs(z)bl + cvtsi2ss
 * src: short,  des: float  --- movs(z)wl + cvtsi2ss
 * src: int,    des: float  --- cvtsi2ss
 * src: long,   des: float  --- cvtsi2ssq  (src:long long)
 * src: char, short, int , long     des: double --- cvtsi2sd
 *
 * src: float,  des: char   --- movss + cvttss2si + change width(1)
 * src: float,  des: short  --- movss + cvttss2si + change width(2)
 * src: float,  des: int    --- movss + cvttss2si
 * src: float,  des: long   --- movss + cvttss2siq
 * src: float,  des: unsigned char  --- cvttss2si + change width(1)
 * src: float,  des: unsigned short --- cvttss2si + change width(2)
 * src: float,  des: unsigned int   --- movss + cvttss2siq + change width(4)
 * src: float,  des: unsigned long  --- too complicate, just use movss + cvttss2siq
 * src: double, des: (unsigned) char, short, int, long, change 'ss' to 'sd'

 * src: float,  des: double     --- cvtss2sd
 * src: double, des: float      --- cvtsd2ss


 * integer to integer
 * char, char   --- movzbl + change width(1)
 * char, short  --- movsbl + change width(2)
 * char, int    --- movsbl
 * char, long   --- movsbq
 * unchage, if des is unsigned

 * short, char   --- movzwl + change width(1)
 * short, short  --- movzwl + change width(2)
 * short, int    --- movswl
 * short, long   --- movswq
 * unchage, if des is unsigned

 * int, char   --- movl + change width(1)
 * int, short  --- movl + change width(2)
 * int, int    --- movl
 * int, long   --- movl + cltq + change width(8)
 * unchage, if des is unsigned

 * unsigned int, char   --- movl + change width(1)
 * unsigned int, short  --- movl + change width(2)
 * unsigned int, int    --- movl
 * unsigned int, long   --- movl + change width(8)
 * unchage, if des is unsigned

 * long, char   --- movq + change width(1)
 * long, short  --- movq + change width(2)
 * long, int    --- movq + change width(4)
 * long, long   --- movq
 * unchage, if des is unsigned


 */
Register* Generator::GenCastOp(UnaryOp* castOp)
{
    auto src = Load(nullptr, castOp->_operand->Accept(this));
    auto desType = castOp->Type();
    auto srcType = castOp->_operand->Type();
    
    std::string cast;

    if (srcType->IsFloat()) {
        if (desType->IsFloat()) {
            if (srcType->Width() == desType->Width())
                return src;

            if (srcType->Width() == 4 && desType->Width() == 8)
                cast = "movss2sd";
            else
                cast = "movsd2ss";
            EmitCAST(cast, src, src, desType->Width());
            return src;
        }
        
        auto desArithmType = desType->ToArithmType();
        if (desType->ToPointerType())
            desArithmType = Type::NewArithmType(T_LONG);

        switch (desArithmType->Tag()) {
        case T_CHAR: case T_SHORT: case T_INT:
            cast = "cvttss2si"; break;
        case T_LONG: case T_UNSIGNED: case T_UNSIGNED | T_LONG:
            cast = "cvttss2siq"; break;
        case T_UNSIGNED | T_CHAR: case T_UNSIGNED | T_SHORT:
            cast = "cvttss2si"; break;
        default: Error("internal error");
        }

        auto width = srcType->Width();

        if (width == 8)
            cast[5] = 'd';
        width = std::max(4, desArithmType->Width());
        auto des = AllocReg(width, false);
        EmitCAST(cast, des, src);
        des->SetWidth(desArithmType->Width());
        Free(src);
        return des;
    } if ((srcType->IsInteger() || srcType->ToPointerType())
            && (desType->IsInteger() || desType->ToPointerType())) {
        // char, short, int, long, long long
        ArithmType* srcArithmType = srcType->ToArithmType();
        ArithmType* desArithmType = desType->ToArithmType();
        if (srcType->ToPointerType())
            srcArithmType = Type::NewArithmType(T_LONG);
        if (desType->ToPointerType())
            desArithmType = Type::NewArithmType(T_LONG);
        
        switch (srcArithmType->Width()) {
        case 1:
            switch (desArithmType->Width()) {
            case 1: cast = "movzbl"; break;
            case 2: case 4: cast = "movsbl"; break;
            case 8: cast = "movsbq"; break;
            default: Error("internal error");
            }
            if (srcArithmType->Tag() & T_UNSIGNED)
                cast[3] = 'z';
            break;
        case 2:
            switch (desArithmType->Width()) {
            case 1: case 2: cast = "movzwl"; break;
            case 4: cast = "movswl"; break;
            case 8: cast = "movswq"; break;
            default: Error("internal error");
            }
            if (srcArithmType->Tag() & T_UNSIGNED)
                cast[3] = 'z';
            break;
        case 4:
            cast = "movl"; break;
        case 8:
            cast = "movq"; break;
        default:
            Error("internal error: error width");
        }
        auto width = std::max(4, desArithmType->Width());
        // Same width, no convertion
        if (width == src->Width()) {
            src->SetWidth(desArithmType->Width());
            return src;
        }

        EmitCAST(cast, src, src, width);
        src->SetWidth(desArithmType->Width());
        
        if (srcArithmType->Tag() == T_INT
                && (desArithmType->Tag() & T_LONG)) {
            Emit("cltq");
        }
        return src;
    } else {
        assert(srcType->IsInteger());
        assert(desType->IsFloat());

        switch (srcType->Width()) {
        case 1: cast = "cvtsi2ss"; break;
        case 2: cast = "cvtsi2ss"; break;
        case 4: cast = "cvtsi2ss"; break;
        case 8: cast = "cvtsi2ssq"; break;
        default: Error("internal error");
        }

        auto des = AllocReg(desType->Width(), true);
        if (desType->Width() == 8)
            cast[7] = 'd';
        EmitCAST(cast, des, src);
        Free(src);
        return des;
    }
}

Memory* Generator::GenObject(Object* obj)
{
    if (obj->Linkage() == L_NONE || obj->Storage() & S_STATIC) {
        return Memory::New(obj->Type(), REG(RBP), obj->Offset());
    } else {
        Error("internal error: not implemented yet");
    }

    return nullptr;
}


Operand* Generator::GenConditionalOp(ConditionalOp* condOp)
{
    // TODO(wgtdkp):
    return nullptr;
}

Register* Generator::GenFuncCall(FuncCall* funcCall)
{
    auto args = funcCall->Args();
    //auto params = funcCall->Designator()->Params();
    auto funcType = funcCall->Designator()->Type()->ToFuncType();

    Register* desReg = nullptr;


    for (auto argReg: _argRegs) {
        assert(!argReg->Allocated());
    }

    _argRegUsed = 0;
    _argVecRegUsed = 0;
    // Prepare return value
    // TODO(wgtdkp): Tell the Caller where is the return value,
    // or the caller find it by himself?
    auto cls = Classify(funcCall->Type());
    if (cls == ParamClass::MEMORY) {
        // TODO(wgtdkp):
        // Alloc the return value on the stack
        auto addr = AllocArgStack();
        auto reg = AllocArgReg(8);
        // Address of the return value 
        // should be treated as the first hidden param
        assert(reg->Is(Register::RDI));
        PushReturnAddr(addr, reg);

        desReg = TryAllocReg(REG(RAX), 8);
    } else if (cls == ParamClass::SSE) {
        desReg = TryAllocReg(REG(XMM0), funcCall->Type()->Width());
    } else if (cls == ParamClass::INTEGER) {
        desReg = TryAllocReg(REG(RAX), funcCall->Type()->Width());
    } else {
        assert(false);
    }

    // Prepare arguments
    for (auto arg: *args) {
        //arg->Accept(this);
        //PushFuncArg(arg);
        auto cls = Classify(arg->Type());
        PushFuncArg(arg, cls);
    }

    if (funcType->Variadic()) {
        // Setup %al to vector register used
        Emit("mov $%d, %%al", _argVecRegUsed);
    }
    Emit("call %s", funcType->Name());

   return desReg;
}


void Generator::PushReturnAddr(int addr, Register* reg)
{
    Emit("mov %daddr(%%rbp), %%rdi");
}

void Generator::PushFuncArg(Expr* arg, ParamClass cls)
{
    if (cls == ParamClass::INTEGER) {
        auto reg = AllocArgReg(arg->Type()->Width());
        if (!reg) {
            // Push the argument on the stack
        } else {
            // TODO(wgtdkp): Tell it which register to use
            arg->Accept(this);
        }
    } else if (cls == ParamClass::SSE) {
        auto reg = AllocArgVecReg();
        if (!reg) {
            // Push the argument on the stack
        } else {
            // TODO(wgtdkp): Tell it which register to use
            arg->Accept(this);
        }
    } else if (cls == ParamClass::MEMORY) {
        // Push the argument on the stack
        // ERROR(wgtdkp): the argument should be pushed by reversed order
    } else {
        assert(false);
    }
}

Immediate* Generator::GenConstant(Constant* cons)
{
    // TODO(wgtdkp):
    if (cons->Type()->IsFloat()) {
        Error("not implemented yet");
    }
    return Immediate::New(cons->IVal(), 4);
}

Register* Generator::GenTempVar(TempVar* tempVar)
{
    // TODO(wgtdkp):
    return nullptr;
}

Operand* Generator::GenIfStmt(IfStmt* ifStmt)
{
    return nullptr;
}

Operand* Generator::GenJumpStmt(JumpStmt* jumpStmt)
{
    return nullptr;
}

Operand* Generator::GenReturnStmt(ReturnStmt* returnStmt)
{
    return nullptr;
}

Operand* Generator::GenLabelStmt(LabelStmt* labelStmt)
{
    fprintf(_outFile, ".L%d:\n", labelStmt->Tag());
    return nullptr;
}

Operand* Generator::GenCompoundStmt(CompoundStmt* compoundStmt)
{
    return nullptr;
}


class Comp
{
public:
    bool operator()(Object* lhs, Object* rhs) {
        return lhs->Type()->Align() < rhs->Type()->Align();
    }
};

//Function Definition
Register* Generator::GenFuncDef(FuncDef* funcDef)
{
    Emit("%s:", funcDef->Type()->Name().c_str());

    // Save %rbp
    EmitPUSH(REG(RBP));
    Emit(std::string("mov"), REG(RBP), REG(RSP));


    // Get offset to %rbp from Generator
    // Resolve offset of all objects in _stmt->_scope;

    // Save arguments passed by registers
    std::priority_queue<Object*, std::vector<Object*>, Comp> heap;

    auto params = funcDef->Params();    

    auto scope = funcDef->Body()->Scope();
    
    // Sort all objects by alignment in descending order
    // Except for parameters
    auto iter = scope->begin();
    for (; iter != scope->end(); iter++) {
        if (!iter->second->ToObject())
            continue;
        for (auto param: params) {
            if (iter->second == param)
                goto next;
        }

        // TODO(wgtdkp):
        // External and static objects are not allocated on stack

        heap.push(iter->second->ToObject());
    next:;
    }

    assert(Top() == 0);

    // Alloc memory for objects
    while (!heap.empty()) {
        auto obj = heap.top();
        heap.pop();

        Push(obj->Type()->Width(), obj->Type()->Align());
        obj->SetOffset(Top());
    }

    Memory* retAddrMem;
    // Save parameters passed by register
    for (auto param: params) {
        auto cls = Classify(param->Type());
        if (cls == ParamClass::INTEGER) {
            auto reg = AllocArgReg(param->Type()->Width());
            if (reg == nullptr)
                break;
            
            auto width = param->Type()->Width();
            Push(width, param->Type()->Align());
            param->SetOffset(Top());

            auto mem = Memory::New(param->Type(), REG(RBP), Top());
            EmitStore(mem, reg);

            // It may be the return address
            if (reg->Is(Register::RDI))
                retAddrMem = mem;
        } else {
            // TODO(wgtdkp):
            Error("not implemented yet");
        }
    }

    // Gen body
    for (auto stmt: funcDef->Body()->Stmts()) {
        Free(stmt->Accept(this));

    }

    // Makeup return register
    Register* retReg = nullptr;
    auto cls = Classify(funcDef->Type()->Derived());
    switch (cls) {
    case ParamClass::MEMORY:
        EmitLoad("movq", REG(RAX), retAddrMem);
        retReg = REG(RAX);
        break;
    case ParamClass::INTEGER:
        retReg = REG(RAX);
        break;
    case ParamClass::SSE:
        retReg = REG(XMM0);
        break;
    default:
        assert(false);
    }

    EmitPOP(REG(RBP));
    return retReg;
}

//Translation Unit
Operand* Generator::GenTranslationUnit(TranslationUnit* unit)
{
    // Setup env
    REG(RBP)->SetAllocated(true);
    REG(RIP)->SetAllocated(true);

     for (auto node: unit->ExtDecls()) {
         node->Accept(this);
     }
     return nullptr;
}

void Generator::Gen(void)
{
    GenTranslationUnit(_parser->Unit());
}

static inline char Postfix(int width)
{
    switch (width) {
    case 1: return 'b';
    case 2: return 'w';
    case 4: return 'l';
    case 8: return 'q';
    default: assert(false); return '\0';
    }
}

void Generator::Emit(const char* format, ...)
{
    fprintf(_outFile, "\t");
    va_list args;
    va_start(args, format);
    vfprintf(_outFile, format, args);
    va_end(args);
    fprintf(_outFile, "\t\n");
}

void Generator::EmitCAST(const std::string& cast,
        Register* des, Register* src, int desWidth)
{
    assert(des->Width() == src->Width());
    if (des != src) {
        Emit("%s\t%s, %s", cast.c_str(),
                src->Repr().c_str(), des->Repr().c_str());
    } else {
        auto srcRepr = src->Repr();
        des->SetWidth(desWidth);
        auto desRepr = des->Repr();
        Emit("%s\t%s, %s", cast.c_str(), srcRepr.c_str(), desRepr.c_str());
    }
}

void Generator::EmitLoad(const std::string& load, Register* des, int src)
{
    //assert(des->Width() == 4 || des->Width() == 8);
    Emit("%s\t$%d, %s", load.c_str(), src, des->Repr().c_str());
}


void Generator::EmitLoad(const std::string& load, Register* des, Memory* src)
{
    //assert(des->Width() == src->Width());
    Emit("%s\t%s, %s", load.c_str(),
            src->Repr().c_str(), des->Repr().c_str());
}

void Generator::EmitStore(Memory* des, Register* src)
{
    assert(des->Width() == src->Width());
    Emit("mov%c\t%s, %s", Postfix(des->Width()),
            src->Repr().c_str(), des->Repr().c_str());
}

// Emit binary operator
void Generator::Emit(const std::string& inst, Register* des, Operand* src)
{
    assert(des->Width() == src->Width());
    Emit("%s%c\t%s, %s", inst.c_str(), Postfix(src->Width()),
            src->Repr().c_str(), des->Repr().c_str());
}

void Generator::EmitLEA(Register* des, Memory* src)
{
    Emit("lea\t%s, %s", src->Repr().c_str(), des->Repr().c_str());
}

void Generator::EmitJE(LabelStmt* label)
{
    Emit("je\t.l%d", label->Tag());
}

void Generator::EmitJNE(LabelStmt* label)
{
    Emit("jne\t.l%d", label->Tag());
}

void Generator::EmitJMP(LabelStmt* label)
{
    Emit("jmp\t.l%d", label->Tag());
}

void Generator::EmitCMP(int imm, Register* reg)
{
    Emit("cmp\t%d, %s", imm, reg->Repr().c_str());
}

void Generator::EmitPUSH(Operand* operand)
{
    Emit("push\t%s", operand->Repr().c_str());
}

void Generator::EmitPOP(Operand* operand)
{
    Emit("pop\t%s", operand->Repr().c_str());
}

void Generator::EmitLabel(LabelStmt* label)
{
    Emit(".L%d:", label->Tag());
    fprintf(_outFile, ".L%d:\n", label->Tag());
}

/*
void Generator::Fix(Operand*& first, Operand*& second)
{
    if (second->ToImmediate())
        return;
    if (first->ToImmediate())
        return;

    if (second->ToRegister()) {
        if (first == second) {
            auto r11 = REG(R11);
            r11->SetWidth(second->Width());
            Emit("mov", r11, second);
            Free(second);
        }
    }
}
*/

Register* Generator::AllocReg(int width, bool flt)
{
    const static std::vector<int> regs {
        Register::RAX, Register::RCX, Register::RDX,
        Register::RSI, Register::RDI, Register::R8,
        Register::R9, Register::R10
        // %R11 is reserved to resolve conflict
        /*, Register::R11*/
    };

    const static std::vector<int> xregs = {
        Register::XMM0, Register::XMM1, Register::XMM2,
        Register::XMM3, Register::XMM4, Register::XMM5,
        Register::XMM6, Register::XMM7
    };

    // TODO(wgtdkp):
    /*
    const static int calleeSaved[] = {
        Register::RBX, Register::R12, Register::R13,
        Register::R14, Register::R15, Register::RBP,
        Register::RSP
    };
    */
    Register* exceptReg[2] = {nullptr, nullptr};
    if (_except) {
        if (_except->ToRegister())
            exceptReg[0] = _except->ToRegister();
        else if (_except->ToMemory()) {
            auto mem = _except->ToMemory();
            exceptReg[0] = mem->_base;
            exceptReg[1] = mem->_index;
        }
    }

    if (exceptReg[0])
        assert(exceptReg[0]->Allocated());
    if (exceptReg[1])
        assert(exceptReg[1]->Allocated());

    const std::vector<int>* pregs = flt ? &xregs: &regs; 

    // Get free general purpose register
    for (auto tag: *pregs) {
        auto reg = Register::Get(tag);
        if (reg->Allocated())
            continue;
        reg->SetAllocated(true);
        reg->SetWidth(width);
        return reg;
    }

    // exceptReg could being used by lhs, 
    // i don't want to spill out lhs
    for (auto tag: *pregs) {
        auto reg = Register::Get(tag);
        if (reg == exceptReg[0] || reg == exceptReg[1])
            continue;
        Spill(reg);

        reg->SetAllocated(true);
        reg->SetWidth(width);
        return reg;
    }

    Error("internal error: no enough register");
    return nullptr; // Make compiler happy
}

Register* Generator::TryAllocReg(Register* reg, int width)
{
    if (reg->Allocated())
        return nullptr;
    reg->SetAllocated(true);
    reg->SetWidth(width);
    return reg;
}

Register* Generator::AllocArgReg(int width) {
    if (_argRegUsed >= N_ARG_REG)
        return nullptr;

    auto ret = _argRegs[_argRegUsed++];
    ret = TryAllocReg(ret, width);
    assert(ret);
    return ret;
}

void Generator::Free(Operand* operand)
{
    if (operand == nullptr)
        return;
    if (operand->ToImmediate())
        return;

    Register* reg;
    if ((reg = operand->ToRegister())) {
        if (reg == REG(RBP) || reg == REG(RIP))
            return;

        reg->SetAllocated(false);
        if (reg->Spilled())
            Reload(reg);
    } else {
        auto mem = operand->ToMemory();
        Free(mem->_base);
        Free(mem->_index);
    }
}

void Generator::Spill(Register* reg)
{

    assert(reg->Allocated());

    Push(reg->Width(), reg->Width());
    auto mem = Memory::New(reg->IsXReg(), reg->Width(), REG(RBP), Top());
    EmitStore(mem, reg);
    
    reg->AddSpill(mem);
    reg->SetAllocated(false);
}

void Generator::Reload(Register* reg)
{
    assert(!reg->Allocated());
    assert(reg->Spilled());

    auto offset = Top();
    Pop();

    auto mem = reg->RemoveSpill();
    assert(offset == mem->_disp);
    reg->SetWidth(mem->Width());
    Load(reg, mem);

    reg->SetAllocated(true);
}
