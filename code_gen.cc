#include "code_gen.h"

#include "ast.h"

#include <cassert>
#include <cstdio>

enum class ParamClass
{
    PC_INTEGER,
    PC_SSE,
    PC_SSEUP,
    PC_X87,
    PC_COMPLEX_X87,
    PC_NO_CLASS,
    PC_MEMORY
};

static ParamClass ClassifyParam(Type* paramType)
{
    if (paramType->IsInteger() || paramType->ToPointerType()
            || paramType->ToArrayType()) {
        return PC_INTEGER;
    } else if (paramType->ToArithmType()) {
        auto type = paramType->ToArithmType();
        if (type->Tag() == T_FLOAT || type->Tag() == T_DOUBLE) {
            return PC_SSE;
        } else if (type->Tag() == (T_LONG | T_DOUBLE)) {
            return PC_X87;
        } else {
            // It is complex
            if ((type->Tag() & T_LONG) && (type->Tag() & T_DOUBLE))     return COMPLEX_X87;
            // TODO(wgtdkp):
        }
    } else {
        auto type = paramType->ToStructUnionType();
        assert(type);

        if (paramType->Width() > 4 * 8)
            return PC_MEMORY;
        
        ParamClass classes[4] = {
            PC_NO_CLASS,
            PC_NO_CLASS,
            PC_NO_CLASS,
            PC_NO_CLASS
        };

        

        
    }
}

void CodeGen::EmitFunc(FuncDef* func)
{
    Emit("push %rbp");
    Emit("movq %rsp, %rbp");



    Emit("pop %rbp");
    Emit("ret");
}


void CodeGen::EmitFuncCall(FuncCall* funcCall)
{

}


void CodeGen::Emit(const char* format, ...)
{
    printf("    ");

    va_list args;
    va_start(args, format);
    vfprintf(_outFile, format, args);
    va_end(args);

    printf("\n");  
}


void CodeGen::EmitLabel(LabelStmt* label)
{
    fprintf(_outFile, ".L%d:\n", label->Tag());
}


void CodeGen::EmitExpr(BinaryOp* binaryOp)
{
    switch (binaryOp->_op) {
    case '[':
        return EmitSubScriptingOp(binaryOp);
    case '*':
        return EmitMulOp(binaryOp);
    case '/':
        return EmitDivOp(binaryOp);
    case '%':
        return EmitModOp(binaryOp);
    case '+':
        return EmitAddOp(binaryOp);
    case '-':
        return EmitSubOp(binaryOp);
    case Token::LEFT_OP:
    case Token::RIGHT_OP:
        return EmitShiftOp(binaryOP);
    case '<':
        return EmitLTOp(binaryOp);
    case '>':
        return EmitGTOp(binaryOp);
    case Token::LE_OP:
        return EmitLEOp(binaryOp);
    case Token::GE_OP:
        return EmitGEOp(binaryOp);
    case Token::EQ_OP:
        return EmitEQOp(binaryOp);
    case Token::NE_OP:
        return EmitNEOp(binaryOp);
    case '&':
        return EmitBitAndOp(binaryOp);
    case '^':
        return EmitBitXorOp(binaryOp);
    case '|':
        return EmitBitOrOp(binaryOp);
    case Token::AND_OP:
        return EmitAndOp(binaryOp);
    case Token::OR_OP:
        return EmitOrOp(binaryOp);
    case '=':
        return EmitAssignOp(binaryOp);
    default:
        assert(0); 
    }
}


void CodeGen::EmitExpr(UnaryOp* unaryOp)
{
    switch (unaryOp->_op) {
    case Token::PREFIX_INC:
        return EmitPreIncOp(unaryOp);
    case Token::PREFIX_DEC:
        return EmitPreDecOp(unaryOp);
    case Token::POSTFIX_INC:
        return EmitPostIncOp(unaryOp);
    case Token::POSTFIX_DEC:
        return EmitPostDecOp(unaryOp);
    case Token::ADDR:
        return EmitAddrOp(unaryOp);
    
    }
}


void CodeGen::EmitExpr(ConditionalOp* condOp)
{

}


void CodeGen::EmitExpr(FuncCall* funcCall)
{

}


void CodeGen::EmitExpr(Constant* cons)
{

}


void CodeGen::EmitExpr(Object* obj)
{

}


void CodeGen::EmitExpr(TempVar* tempVar)
{

}
