#include "code_gen.h"

#include "ast.h"
#include "type.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <iterator>


/*
 * Register
 */

Register Register::_regs[N_REG];

Register* Generator::_argRegs[N_ARG_REG] = {
    Register::Get(Register::RDI),
    Register::Get(Register::RSI),
    Register::Get(Register::RDX),
    Register::Get(Register::RCX),
    Register::Get(Register::R8),
    Register::Get(Register::R9)
};

Register* Generator::_argVecRegs[N_ARG_VEC_REG] = {
    Register::Get(Register::XMM0),
    Register::Get(Register::XMM1),
    Register::Get(Register::XMM2),
    Register::Get(Register::XMM3),
    Register::Get(Register::XMM4),
    Register::Get(Register::XMM5),
    Register::Get(Register::XMM6),
    Register::Get(Register::XMM7)
};

std::string Register::Repr(void) const
{
    assert(0);
    return "";
}


/*
 * Immediate
 */

std::string Immediate::Repr(void) const
{
    assert(0);
    return "";
}


/*
 * Memory
 */

std::string Memory::Repr(void) const
{
    assert(0);
    return "";
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


void Generator::Emit(const char* format, ...)
{
    printf("    ");

    va_list args;
    va_start(args, format);
    vfprintf(_outFile, format, args);
    va_end(args);

    printf("\n");  
}


//Expression
Operand* Generator::GenBinaryOp(BinaryOp* binaryOp)
{
    // TODO(wgtdkp):
    return nullptr;
}

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

        desReg = Register::Get(Register::RAX);
    } else if (cls == ParamClass::SSE) {
        desReg = Register::Get(Register::XMM0);
    } else if (cls == ParamClass::INTEGER) {
        desReg = Register::Get(Register::RAX);
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

//Function Definition
void Generator::GenFuncDef(FuncDef* funcDef)
{
    Emit("push %%rbp");
    Emit("mov %%rsp, %%rbp");

    // Copy Params
    // Init the offset of objects on stack


    Emit("pop %%rbp");
    Emit("ret");
}

//Translation Unit
void Generator::GenTranslationUnit(TranslationUnit* transUnit)
{

}
