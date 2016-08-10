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
    New(XMM4), New(XMM5), New(XMM6), New(XMM7)
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
    {"xmm7", "xmm7", "xmm7", "xmm7"}
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

    auto lhs = binaryOp->_lhs->Accept(this);
    if (lhs->ToImmediate()) {
        auto imm = lhs->ToImmediate();
        auto cond = imm->Cons()->IVal();
        if (cond == 0)
            return immFalse;
    } else {
        EmitCMP(immFalse, lhs);
        EmitJE(labelFalse = LabelStmt::New());
    }

    auto ret = AllocReg(binaryOp);

    auto labelTrue = LabelStmt::New();

    auto rhs = binaryOp->_rhs->Accept(this);
    if (rhs->ToImmediate()) {
        auto imm = rhs->ToImmediate();
        auto cond = imm->Cons()->IVal();
        if (cond == 0) {
            // Do nothing
        } else {
            EmitMOV(immTrue, ret);
            EmitJMP(labelTrue);
        }
    } else {
        EmitCMP(immFalse, rhs);
        if (labelFalse == nullptr)
            labelFalse = LabelStmt::New();
        EmitJE(labelFalse);

        EmitMOV(immTrue, ret);
        EmitJMP(labelTrue);
    }

    EmitLabel(labelFalse);
    EmitMOV(immFalse, ret);
    EmitLabel(labelTrue);

    return ret;
}


Operand* Generator::GenOrOp(BinaryOp* binaryOp)
{
    LabelStmt* labelTrue = nullptr;
    auto lhs = binaryOp->_lhs->Accept(this);
    if (lhs->ToImmediate()) {
        auto imm = lhs->ToImmediate();
        auto cond = imm->Cons()->IVal();
        if (cond)
            return immTrue;
    } else {
        EmitCMP(immFalse, lhs);
        EmitJNE(labelTrue = LabelStmt::New());
    }

    auto ret = AllocReg(binaryOp);

    auto labelFalse = LabelStmt::New();

    auto rhs = binaryOp->_rhs->Accept(this);
    if (rhs->ToImmediate()) {
        auto imm = rhs->ToImmediate();
        auto cond = imm->Cons()->IVal();
        if (cond) {
            // Do nothing
        } else {
            EmitMOV(immFalse, ret);
            EmitJMP(labelFalse);
        }
    } else {
        EmitCMP(immFalse, rhs);
        if (labelTrue == nullptr)
            labelTrue = LabelStmt::New();
        EmitJNE(labelTrue);

        EmitMOV(immFalse, ret);
        EmitJMP(labelFalse);
    }

    EmitLabel(labelTrue);
    EmitMOV(immTrue, ret);
    EmitLabel(labelFalse);

    return ret;
}


Operand* Generator::GenSubScriptingOp(BinaryOp* binaryOp)
{
    auto lhs = binaryOp->_lhs->Accept(this);
    auto rhs = binaryOp->_rhs->Accept(this);
    auto scale = binaryOp->_rhs->Type()->Width();

    int disp;
    Register* index;

    if (rhs->ToMemory()) {
        index = AllocReg(binaryOp->_rhs);
        EmitMOV(rhs, index);
    } else if (rhs->ToRegister()){
        index = rhs->ToRegister();
    } else {
        disp = rhs->ToImmediate()->Cons()->IVal() * scale;
    }

    if (lhs->ToRegister()) {
        // The register has the address of lhs
        if (rhs->ToImmediate()) {
            return Memory::New(scale, lhs->ToRegister(), disp);
        }
        return Memory::New(scale, lhs->ToRegister(), 0, index, scale);
    }

    if (rhs->ToImmediate()) {
        lhs->ToMemory()->_disp += disp;
        return lhs;
    }

    // lhs is memory
    auto base = AllocReg(binaryOp->_lhs, true);
    EmitLEA(lhs->ToMemory(), base);
    return Memory::New(scale, base, 0, index, scale);
}

// Generate no assembly code
Operand* Generator::GenMemberRefOp(BinaryOp* binaryOp)
{
    auto lhs = binaryOp->_lhs->Accept(this);
    auto rhs = binaryOp->_rhs->Accept(this)->ToMemory();
    auto width = binaryOp->_rhs->Type()->Width();
    assert(rhs);

    if (lhs->ToRegister()) {
        // Expression: int c = a->b;
        // Translation:
        //     leaq a(%rbp), %rax
        //     movl b(%rax), %eax
        //     movl %eax, c(%rbp)
        return Memory::New(width, lhs->ToRegister(), rhs->_disp);
    } else if (lhs->ToMemory()) {
        // Expression: int c = a.b;
        // Translation:
        //     movl b+a(%rbp), %eax
        //     movl %eax, c(%rbp)
        auto operand = lhs->ToMemory();
        operand->_disp += rhs->_disp;
        return operand;
    }

    assert(false);
    return nullptr; // Make compiler happy
}

/*
Operand* Generator::GenAddOp(Expr* lhsExpr, Expr* rhsExpr)
{
    // Evaluate from left to right

    if (!lhsOperand->ToRegister() && !rhsOperand->ToRegister()) {
        auto reg = AllocReg();
        EmitMOV(lhsOperand, reg);
        lhsOperand = reg;
    } else if (rhsOperand->ToRegister() && !lhsOperand->ToRegister()) {
        // Instruction 'add' obeys commutative law
        std::swap(lhsOperand, rhsOperand);
    }

    Emit("add", rhsOperand, lhsOperand);

    return lhsOperand; // Must be a register
}
*/

Operand* Generator::GenUnaryOp(UnaryOp* unaryOp)
{
    // TODO(wgtdkp):
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

Memory* Generator::GenObject(Object* obj)
{
    // TODO(wgtdkp):
    return nullptr;
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
    Emit("%s:\n", funcDef->Type()->Name().c_str());

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

        heap.push(iter->second->ToObject());
    next:;
    }

    int offset = _stackOffset;
    assert(offset == 0);

    // Alloc memory for objects
    while (!heap.empty()) {
        auto obj = heap.top();
        heap.pop();

        offset -= obj->Type()->Width();
        offset = Type::MakeAlign(offset, obj->Type()->Align());
        obj->SetOffset(offset);
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
            offset -= width;
            offset = Type::MakeAlign(offset, param->Type()->Align());
            param->SetOffset(offset);

            auto mem = Memory::New(width, REG(RBP), offset);
            EmitMOV(reg, mem);

            // It may be the return address
            if (reg->Is(Register::RDI))
                retAddrMem = mem;
        }
    }

    _stackOffset = offset;

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
    va_list args;
    va_start(args, format);
    vfprintf(_outFile, format, args);
    va_end(args);
}

void Generator::Emit(const char* inst, Operand* lhs, Operand* rhs)
{
    assert(lhs->Width() == rhs->Width());

    Emit(inst);

    // TODO(wgtdkp):
    switch (rhs->Width()) {
    case 1: Emit("b"); break;
    case 2: Emit("w"); break;
    case 4: Emit("l"); break;
    case 8: Emit("q"); break;
    default: assert(false);
    }
}

void Generator::EmitMOV(Operand* src, Operand* des)
{
    Emit("\tmov\t%s, %s\n", src->Repr().c_str(), des->Repr().c_str());
}

void Generator::EmitLEA(Memory* mem, Register* reg)
{
    Emit("\tlea\t%s, %s\n", mem->Repr().c_str(), reg->Repr().c_str());
}

void Generator::EmitJE(LabelStmt* label)
{
    Emit("\tje\t.l%d\n", label->Tag());
}

void Generator::EmitJNE(LabelStmt* label)
{
    Emit("\tjne\t.l%d\n", label->Tag());
}

void Generator::EmitJMP(LabelStmt* label)
{
    Emit("\tjmp\t.l%d\n", label->Tag());
}

void Generator::EmitCMP(Immediate* lhs, Operand* rhs)
{
    Emit("\tcmp\t%s, %s\n", lhs->Repr().c_str(), rhs->Repr().c_str());
}

void Generator::EmitPUSH(Operand* operand)
{
    Emit("\tpush\t%s\n", operand->Repr().c_str());
}

void Generator::EmitPOP(Operand* operand)
{
    Emit("\tpop\t%s\n", operand->Repr().c_str());
}

void Generator::EmitLabel(LabelStmt* label)
{
    Emit(".l%d:\n", label->Tag());
}

Register* Generator::AllocReg(Expr* expr, bool addr)
{
    const static int callerSaved[] = {
        Register::RAX, Register::RCX, Register::RDX,
        Register::RSI, Register::RDI, Register::R8,
        Register::R9, Register::R10, Register::R11
    };

    // TODO(wgtdkp):
    /*
    const static int calleeSaved[] = {
        Register::RBX, Register::R12, Register::R13,
        Register::R14, Register::R15, Register::RBP,
        Register::RSP
    };
    */

    // Get free caller saved general purpose register
    for (auto tag: callerSaved) {
        auto reg = Register::Get(tag);
        if (reg->Using())
            continue;
        reg->Bind(expr, addr ? 8: expr->Type()->Width());
        return reg;
    }

    // TODO(wgtdkp):
    // Saved register to stack, and free it
    Error("internal error: no enough register");
    return nullptr; // Make compiler happy
}

void Generator::FreeReg(Register* reg)
{
    if (!reg->Using()) {
        Error("internal error: free register than is free");
    }
    reg->Unbind();
}
