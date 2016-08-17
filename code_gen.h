#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"
#include "visitor.h"


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

class Generator: public Visitor
{
public:
    Generator(Parser* parser, FILE* outFile)
            : _parser(parser), _outFile(outFile) {}

    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp);
    virtual void VisitUnaryOp(UnaryOp* unaryOp);
    virtual void VisitConditionalOp(ConditionalOp* condOp);
    virtual void VisitFuncCall(FuncCall* funcCall);
    virtual void VisitObject(Object* obj);
    virtual void VisitEnumerator(Enumerator* enumer);
    virtual void VisitIdentifier(Identifier* ident);
    virtual void VisitConstant(Constant* cons);
    virtual void VisitTempVar(TempVar* tempVar);

    void GenMemberRefOp(BinaryOp* binaryOp);
    void GenSubScriptingOp(BinaryOp* binaryOp);
    void GenAndOp(BinaryOp* binaryOp);
    void GenOrOp(BinaryOp* binaryOp);
    void GenAddOp(BinaryOp* binaryOp);
    void GenSubOp(BinaryOp* binaryOp);
    void GenAssignOp(BinaryOp* assign);
    void GenCastOp(UnaryOp* cast);
    void GenDerefOp(UnaryOp* deref);
    void GenPointerArithm(BinaryOp* binary);
    void GenDivOp(bool flt, bool sign, int width, int op);
    void GenCompOp(bool flt, int width, const char* set);

    //statement
    virtual void VisitInitialization(Initialization* init);
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
    virtual void VisitIfStmt(IfStmt* ifStmt);
    virtual void VisitJumpStmt(JumpStmt* jumpStmt);
    virtual void VisitReturnStmt(ReturnStmt* returnStmt);
    virtual void VisitLabelStmt(LabelStmt* labelStmt);
    virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

    virtual void VisitFuncDef(FuncDef* funcDef);
    virtual void VisitTranslationUnit(TranslationUnit* unit);

    void Gen(void);

    void Emit(const char* format, ...);
    void EmitLabel(const std::string& label);

    void Push(const char* reg);
    void Pop(const char* reg);

    void Spill(bool flt) {
        Push(flt ? "xmm0": "rax");
    }

    void Restore(bool flt);

private:
    Parser* _parser;
    FILE* _outFile;
    
    std::vector<int> _offsets {0};

    bool _expectLVal {false};
};

#endif
