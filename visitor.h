#ifndef _WGTCC_VISITOR_
#define _WGTCC_VISITOR_

#include "ast.h"


class Visitor
{
public:
    virtual ~Visitor(void) {}

    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp) = 0;
    virtual void VisitUnaryOp(UnaryOp* unaryOp) = 0;
    virtual void VisitConditionalOp(ConditionalOp* condOp) = 0;
    virtual void VisitFuncCall(FuncCall* funcCall) = 0;
    virtual void VisitObject(Object* var) = 0;
    virtual void VisitConstant(Constant* cons) = 0;
    virtual void VisitTempVar(TempVar* tempVar) = 0;

    //statement
    virtual void VisitStmt(Stmt* stmt) = 0;
    virtual void VisitIfStmt(IfStmt* ifStmt) = 0;
    virtual void VisitJumpStmt(JumpStmt* jumpStmt) = 0;
    virtual void VisitReturnStmt(ReturnStmt* returnStmt) = 0;
    virtual void VisitLabelStmt(LabelStmt* labelStmt) = 0;
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) = 0;
    virtual void VisitCompoundStmt(CompoundStmt* compoundStmt) = 0;

    //Function Definition
    virtual void VisitFuncDef(FuncDef* funcDef) = 0;

    //Translation Unit
    virtual void VisitTranslationUnit(TranslationUnit* transUnit) = 0;

protected:
    Visitor(void) {}

private:
    Visitor(const Visitor& other);
    const Visitor& operator=(const Visitor& other);
};

#endif
