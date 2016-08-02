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
    PC_X87_UP,
    PC_COMPLEX_X87,
    PC_NO_CLASS,
    PC_MEMORY
};

static ParamClass ClassifyParam(Type* paramType)
{
    if (paramType->IsInteger() || paramType->ToPointerType()
            || paramType->ToArrayType()) {
        return PC_INTEGER;
    }
    
    if (paramType->ToArithmType()) {
        auto type = paramType->ToArithmType();
        if (type->Tag() == T_FLOAT || type->Tag() == T_DOUBLE)
            return PC_SSE;
        if (type->Tag() == (T_LONG | T_DOUBLE)) {
            return PC_X87;
        
        // It is complex
        if ((type->Tag() & T_LONG) && (type->Tag() & T_DOUBLE))
            return COMPLEX_X87;
        // TODO(wgtdkp):
    }

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

}

static std::vector<Type*> FieldsIn8Bytes(StructUnionType* type, int idx)
{

}

static ParamClass FieldClass(std::vector<Type*>& types, int begin)
{
    auto leftClass = ClassifyParam(types[begin++]);
    ParamClass rightClass = (begin + 1 == types.size())
            ? ClassifyParam(types[begin]): FieldClass(types, begin);
    
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
