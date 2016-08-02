#include "code_gen.h"

#include "ast.h"
#include "type.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <iterator>

Register Register::_regs[N_REG] {
    Register("rax"), Register("rbx"),
    Register("rcx"), Register("rdx"),
    Register("rsi"), Register("rdi"),
    Register("rbp"), Register("rsp"),
    Register("r8"), Register("r9"),
    Register("r10"), Register("r11"),
    Register("r12"), Register("r13"),
    Register("r14"), Register("r15"),
    Register("xmm0"), Register("xmm1"),
    Register("xmm2"), Register("xmm3"),
    Register("xmm4"), Register("xmm5"),
    Register("xmm6"), Register("xmm7")
};

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
void Generator::VisitBinaryOp(BinaryOp* binaryOp)
{

}

void Generator::VisitUnaryOp(UnaryOp* unaryOp)
{

}

void Generator::VisitConditionalOp(ConditionalOp* condOp)
{

}

void Generator::VisitFuncCall(FuncCall* funcCall)
{
    auto args = funcCall->Args();
    //auto params = funcCall->Designator()->Params();
    auto funcType = funcCall->Designator()->Type()->ToFuncType();

    // Prepare arguments
    for (auto arg: *args) {
        //arg->Accept(this);
        //PushFuncArg(arg);
        auto cls = Classify(arg->Type());
        SetupFuncArg(arg, cls);
    }

    if (funcType->Variadic()) {
        // Setup %al to vector register used
    }

    Emit("call %s", funcType->Name());


}

void Generator::SetupFuncArg(Expr* arg, ParamClass cls)
{
    if (cls == ParamClass::INTEGER) {
        auto reg = AllocArgReg();
        if (!reg) {
            // Push the argument on the stack
        } else {
            SetDesReg(reg);
            arg->Accept(this);
        }
    } else if (cls == ParamClass::SSE) {
        auto reg = AllocArgVecReg();
        if (!reg) {
            // Push the argument on the stack
        } else {
            SetDesReg(reg);
            arg->Accept(this);
        }
    } else if (cls == ParamClass::MEMORY) {
        // Push the argument on the stack
        // ERROR(wgtdkp): the argument should be pushed by reversed order
    } else {
        assert(false);
    }
}

void Generator::VisitObject(Object* obj)
{

}

void Generator::VisitConstant(Constant* cons)
{

}

void Generator::VisitTempVar(TempVar* tempVar)
{

}

//statement
void Generator::VisitStmt(Stmt* stmt)
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
    fprintf(_outFile, ".L%d:\n", labelStmt->Tag());
}

void Generator::VisitEmptyStmt(EmptyStmt* emptyStmt)
{

}

void Generator::VisitCompoundStmt(CompoundStmt* compoundStmt)
{

}

//Function Definition
void Generator::VisitFuncDef(FuncDef* funcDef)
{
    Emit("push %rbp");
    Emit("movq %rsp, %rbp");

    Emit("pop %rbp");
    Emit("ret");
}

//Translation Unit
void Generator::VisitTranslationUnit(TranslationUnit* transUnit)
{

}