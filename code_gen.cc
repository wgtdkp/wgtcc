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

static auto immFalse = Immediate::New(T_INT, 0);
static auto immTrue = Immediate::New(T_INT, 1);


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

Immediate* Immediate::New(Constant* cons)
{
    auto ret = new (immediatePool.Alloc()) Immediate(cons);
    ret->_pool = &immediatePool;

    return ret;
}

std::string Immediate::Repr(void) const
{
    std::string repr = "$";
    if (_cons->Type()->IsFloat())
        repr += std::to_string(_cons->FVal());
    else if (_cons->Type()->IsInteger())
        repr += std::to_string(_cons->IVal());
    else {
        assert(false);
    }

    return repr;
}


/*
 * Memory
 */

Memory* Memory::New(int width, Register* base, int disp,
        Register* index, int scale) {
    auto ret = new (memoryPool.Alloc())
            Memory(width, base, disp, index, scale);
    ret->_pool = &memoryPool;

    return ret;
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
    case '[': {
        return GenSubScriptingOp(binaryOp);
    }
    case '+':
        ;//return GenAddOp(binaryOp->_lhs, binaryOp->_rhs);
    case '-':
        ;//return GenSubOp(binaryOp->_lhs, binaryOp->_rhs);
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

Operand* Generator::GenAssignOp(BinaryOp* binaryOp)
{
    // TODO(wgtdkp):
    return nullptr;
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

    auto lhs = binaryOp->_lhs->Accept(this)->ToRegister();

    EmitCMP(0, lhs);
    EmitJE(labelFalse = LabelStmt::New());

    auto labelTrue = LabelStmt::New();

    auto rhs = binaryOp->_rhs->Accept(this)->ToRegister();
    
    EmitCMP(0, rhs);
    EmitJE(labelFalse);

    auto ret = AllocReg(4, false);
    EmitMOV(ret, immTrue);
    EmitJMP(labelTrue);

    EmitLabel(labelFalse);
    EmitMOV(0, ret);
    EmitLabel(labelTrue);

    return ret;
}


Operand* Generator::GenOrOp(BinaryOp* binaryOp)
{
    LabelStmt* labelTrue = nullptr;
    auto lhs = binaryOp->_lhs->Accept(this)->ToRegister();
    EmitCMP(0, lhs);
    EmitJNE(labelTrue = LabelStmt::New());

    auto ret = AllocReg(4, false);

    auto labelFalse = LabelStmt::New();

    auto rhs = binaryOp->_rhs->Accept(this)->ToRegister();

    EmitCMP(0, rhs);
    EmitJNE(labelTrue);

    EmitMOV(0, ret);
    EmitJMP(labelFalse);

    EmitLabel(labelTrue);
    EmitMOV(ret, immTrue);
    EmitLabel(labelFalse);

    return ret;
}


Register* Generator::GenSubScriptingOp(BinaryOp* binaryOp)
{
    auto lhs = binaryOp->_lhs->Accept(this)->ToRegister();
    auto derivedType = binaryOp->_lhs->Type()->ToArrayType()->Derived();
    auto width = derivedType->Width();
    if (binaryOp->_rhs->ToConstant()) {
        auto rhs = binaryOp->_rhs->ToConstant();
        
        auto disp = rhs->IVal() * width;
        if (derivedType->IsScalar()) {
            auto ret = AllocReg(width, derivedType->IsFloat());
            EmitMOV(Memory::New(width, lhs, disp), ret);
            Free(lhs);
            return ret;
        } else {
            EmitADD(lhs, disp);
            return lhs;
        }
    }

    auto rhs = binaryOp->_rhs->Accept(this)->ToRegister();
    if (derivedType->IsScalar()) {
        auto ret = AllocReg(width, derivedType->IsFloat());
        EmitMOV(Memory::New(width, lhs, 0, rhs, width), ret);
        Free(lhs);
        Free(rhs);
        return ret;
    } else {
        EmitLEA(Memory::New(width, lhs, 0, rhs, width), lhs);
        Free(rhs);
        return lhs;
    }
}

// Generate no assembly code
Register* Generator::GenMemberRefOp(BinaryOp* binaryOp)
{
    auto lhs = binaryOp->_lhs->Accept(this)->ToRegister();
    //auto rhs = binaryOp->_rhs->Accept(this)->ToMemory();
    auto rhs = binaryOp->_rhs->ToObject();
    auto width = rhs->Type()->Width();

    if (rhs->Type()->IsScalar()) {
        auto ret = AllocReg(width, rhs->Type()->IsFloat());
        EmitMOV(Memory::New(width, lhs, rhs->Offset()), ret);
        Free(lhs);
        return ret;
    } else {
        EmitLEA(Memory::New(width, lhs, rhs->Offset()), lhs);
        return lhs;
    }
}


Register* Generator::GenAddOp(BinaryOp* binaryOp)
{
    // Evaluate from left to right
    auto lhs = binaryOp->_lhs->Accept(this)->ToRegister();
    auto rhs = binaryOp->_rhs->Accept(this)->ToRegister();
    
    assert(lhs && rhs);
    assert(lhs->Width() == rhs->Width());

    EmitADD(lhs, rhs);
    
    // We done calc 'lhs + rhs', if rhs has alloc any register,
    // it will be freed.
    Free(rhs);
    return lhs; // Must be a register
}


Operand* Generator::GenUnaryOp(UnaryOp* unaryOp)
{
    // TODO(wgtdkp):
    return nullptr;
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
    auto src = castOp->_operand->Accept(this)->ToRegister();
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
            Emit(cast, src, src, desType->Width());
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
        Emit(cast, des, src);
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
        Emit(cast, src, src, desArithmType->Width());
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
        Emit(cast, des, src);
        Free(src);
        return des;
    }
}

Register* Generator::GenObject(Object* obj)
{
    // TODO(wgtdkp):
    //if (obj->Linkage() == L_NONE || obj->Storage() & S_STATIC) {
    //    return Memory::New(obj->Type()->Width(), REG(RBP), obj->Offset());
    //} else {
    //    Error("internal error: not implemented yet");
    //}

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

    // Prepare return value
    // TODO(wgtdkp): Tell the Caller where is the return value,
    // or the caller find it by himself?
    auto cls = Classify(funcCall->Type());
    if (cls == ParamClass::MEMORY) {
        // TODO(wgtdkp):
        // Alloc the return value on the stack
        auto addr = AllocArgStack();
        auto reg = AllocArgReg();
        // Address of the return value 
        // should be treated as the first hidden param
        assert(reg->Is(Register::RDI));
        PushReturnAddr(addr, reg);

        desReg = REG(RAX);
    } else if (cls == ParamClass::SSE) {
        desReg = REG(XMM0);
    } else if (cls == ParamClass::INTEGER) {
        desReg = REG(RAX);
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
        auto reg = AllocArgReg();
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
    return nullptr;
}

Register* Generator::GenTempVar(TempVar* tempVar)
{
    // TODO(wgtdkp):
    return nullptr;
}

//statement
void Generator::GenStmt(Stmt* stmt)
{

}

void Generator::GenIfStmt(IfStmt* ifStmt)
{

}

void Generator::GenJumpStmt(JumpStmt* jumpStmt)
{

}

void Generator::GenReturnStmt(ReturnStmt* returnStmt)
{

}

void Generator::GenLabelStmt(LabelStmt* labelStmt)
{
    fprintf(_outFile, ".L%d:\n", labelStmt->Tag());
}

void Generator::GenEmptyStmt(EmptyStmt* emptyStmt)
{

}

void Generator::GenCompoundStmt(CompoundStmt* compoundStmt)
{

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
    EmitMOV(REG(RSP), REG(RBP));


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
            auto reg = AllocArgReg();
            if (reg == nullptr)
                break;
            
            auto width = param->Type()->Width();
            Push(width, param->Type()->Align());
            param->SetOffset(Top());

            auto mem = Memory::New(width, REG(RBP), Top());
            EmitMOV(reg, mem);

            // It may be the return address
            if (reg->Is(Register::RDI))
                retAddrMem = mem;
        }
    }

    // Gen body
    for (auto stmt: funcDef->Body()->Stmts()) {
        stmt->Accept(this);
    }

    // Makeup return register
    Register* retReg = nullptr;
    auto cls = Classify(funcDef->Type()->Derived());
    switch (cls) {
    case ParamClass::MEMORY:
        EmitMOV(retAddrMem, REG(RAX));
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
void Generator::GenTranslationUnit(TranslationUnit* unit)
{
     for (auto node: unit->ExtDecls()) {
         node->Accept(this);
     }
}

void Generator::Gen(void)
{
    GenTranslationUnit(_parser->Unit());
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

void Generator::Emit(const std::string& inst,
        Operand* des, Operand* src, int desWidth)
{
    assert(des->Width() == src->Width());
    if (des != src) {
        Emit("%s\t%s, %s", inst.c_str(), src->Repr().c_str(), des->Repr().c_str());
    } else {
        const char* srcRepr = src->Repr().c_str();
        des->SetWidth(desWidth);
        const char* desRepr = des->Repr().c_str();
        Emit("%s\t%s, %s", inst.c_str(), srcRepr, desRepr);
    }
}


void Generator::EmitLEA(Memory* mem, Register* reg)
{
    Emit("lea\t%s, %s", mem->Repr().c_str(), reg->Repr().c_str());
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
    Emit(".l%d:", label->Tag());
}


Register* Generator::AllocReg(int width, bool flt, Operand* except)
{
    const static std::vector<int> regs {
        Register::RAX, Register::RCX, Register::RDX,
        Register::RSI, Register::RDI, Register::R8,
        Register::R9, Register::R10, Register::R11
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
    if (except->ToRegister())
        exceptReg[0] = except->ToRegister();
    else if (except->ToMemory()) {
        auto mem = except->ToMemory();
        exceptReg[0] = mem->_base;
        exceptReg[1] = mem->_index;
    }

    const std::vector<int>* pregs = flt ? &xregs: &regs; 
    width = flt ? 8: width;
    
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

void Generator::Free(Operand* operand)
{
    if (operand == nullptr)
        return;
    if (operand->ToImmediate())
        return;
    if (operand->ToRegister()) {
        operand->ToRegister()->SetAllocated(false);
    } else {
        auto mem = operand->ToMemory();
        if (mem->_base != REG(RBP) && mem->_base != REG(RIP))
            mem->_base->SetAllocated(false);
        if (mem->_index)
            mem->_base->SetAllocated(false);
    }
}

void Generator::Spill(Register* reg)
{
    assert(reg->Allocated());

    Push(reg->Width(), reg->Width());
    auto mem = Memory::New(reg->Width(), REG(RBP), Top());
    EmitMOV(reg, mem);
    
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
    EmitMOV(mem, reg);

    reg->SetAllocated(true);
}
