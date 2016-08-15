#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"

class Parser;

enum class ParamClass
{
    INTEGER,
    SSE,
    SSEUP,
    X87,
    X87_UP,
    COMPLEX_X87,
    NO_CLASS,
    MEMORY
};

class Generator
{
public:
    Generator(Parser* parser, FILE* outFile)
            : _parser(parser), _outFile(outFile) {}

    //Expression
    void GenBinaryOp(BinaryOp* binaryOp);
    void GenUnaryOp(UnaryOp* unaryOp);
    void GenConditionalOp(ConditionalOp* condOp);
    void GenFuncCall(FuncCall* funcCall);
    void GenObject(Object* obj);
    void GenConstant(Constant* cons);
    void GenTempVar(TempVar* tempVar);

    void GenMemberRefOp(BinaryOp* binaryOp);
    void GenSubScriptingOp(BinaryOp* binaryOp);
    void GenAndOp(BinaryOp* binaryOp);
    void GenOrOp(BinaryOp* binaryOp);
    void GenAddOp(BinaryOp* binaryOp);
    void GenSubOp(BinaryOp* binaryOp);
    void GenAssignOp(BinaryOp* binaryOp);
    void GenCastOp(UnaryOp* cast);
    void GenDerefOp(UnaryOp* deref);

    //statement
    void GenIfStmt(IfStmt* ifStmt);
    void GenJumpStmt(JumpStmt* jumpStmt);
    void GenReturnStmt(ReturnStmt* returnStmt);
    void GenLabelStmt(LabelStmt* labelStmt);
    void GenCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    void GenFuncDef(FuncDef* funcDef);

    //Translation Unit
    void GenTranslationUnit(TranslationUnit* unit);
    void Gen(void);

private:
    Parser* _parser;
    FILE* _outFile;
    
    std::vector<int> _offsets {0};

    bool _expectLVal {false};
};

#endif
