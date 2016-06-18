#ifndef _WGTCC_VISITOR_
#define _WGTCC_VISITOR_

#include "ast.h"


class BinaryOp;
class UnaryOp;
class condOp;
class FuncCall;
class Variable;
class Constant;
class TempVar;

class Stmt;
class IfStmt;
class JumpStmt;
class LabelStmt;
class EmptyStmt;

class TranslationUnit;

class Visitor
{
public:
    virtual ~Visitor(void) {}

    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp) = 0;
    virtual void VisitUnaryOp(UnaryOp* unaryOp) = 0;
    virtual void VisitConditionalOp(ConditionalOp* condOp) = 0;
    virtual void VisitFuncCall(FuncCall* funcCall) = 0;
    virtual void VisitVariable(Variable* var) = 0;
    virtual void VisitConstant(Constant* cons) = 0;
    virtual void VisitTempVar(TempVar* tempVar) = 0;

    //statement
    virtual void VisitStmt(Stmt* stmt) = 0;
    virtual void VisitIfStmt(IfStmt* ifStmt) = 0;
    virtual void VisitJumpStmt(JumpStmt* jumpStmt) = 0;
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

class ConstantEvaluator : Visitor
{
public:
    ConstantEvaluator(void) : _val(0) {}
    virtual ~ConstantEvaluator(void) {}
    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp);
    virtual void VisitUnaryOp(UnaryOp* unaryOp);
    virtual void VisitConditionalOp(ConditionalOp* condOp);
    virtual void VisitFuncCall(FuncCall* funcCall);
    virtual void VisitVariable(Variable* var);
    virtual void VisitConstant(Constant* cons);
    virtual void VisitTempVar(TempVar* tempVar);

    //statement
    virtual void VisitStmt(Stmt* stmt);
    virtual void VisitIfStmt(IfStmt* ifStmt);
    virtual void VisitJumpStmt(JumpStmt* jumpStmt);
    virtual void VisitLabelStmt(LabelStmt* labelStmt);
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
    virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    virtual void VisitFuncDef(FuncDef* funcDef);

    //Translation Unit
    virtual void VisitTranslationUnit(TranslationUnit* transUnit);

    int64_t Val(void) const { return _val; }
    //static ConstantEvaluator* NewConstantEvaluator(void) {
    //	return new ConstantEvaluator();
    //}

private:
    int64_t _val;
};

#endif
