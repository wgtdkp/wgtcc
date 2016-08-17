#ifndef _WGTCC_EVALUATOR_H_
#define _WGTCC_EVALUATOR_H_

#include "ast.h"
#include "error.h"
#include "visitor.h"


class Expr;

template<typename T>
class Evaluator: public Visitor
{
public:
    Evaluator(void) {}

    virtual ~Evaluator(void) {}

    virtual void VisitBinaryOp(BinaryOp* binary);
    virtual void VisitUnaryOp(UnaryOp* unary);
    virtual void VisitConditionalOp(ConditionalOp* cond);
    
    virtual void VisitFuncCall(FuncCall* funcCall) {
        Error(funcCall, "expect constant expression");
    }

    virtual void VisitEnumerator(Enumerator* enumer) {
        _val = static_cast<T>(enumer->Val());
    }

    virtual void VisitIdentifier(Identifier* ident) {
        Error(ident, "expect constant expression");
    }

    virtual void VisitObject(Object* obj) {
        Error(obj, "expect constant expression");
    }

    virtual void VisitConstant(Constant* cons) {
        if (cons->Type()->IsFloat()) {
            _val = static_cast<T>(cons->FVal());
        } else if (cons->Type()->IsInteger()) {
            _val = static_cast<T>(cons->IVal());
        } else {
            assert(false);
        }
    }

    virtual void VisitTempVar(TempVar* tempVar) {
        assert(false);
    }

    // We may should assert here
    virtual void VisitInitialization(Initialization* init) {}
    virtual void VisitIfStmt(IfStmt* ifStmt) {}
    virtual void VisitJumpStmt(JumpStmt* jumpStmt) {}
    virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
    virtual void VisitLabelStmt(LabelStmt* labelStmt) {}
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
    virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
    virtual void VisitFuncDef(FuncDef* funcDef) {}
    virtual void VisitTranslationUnit(TranslationUnit* unit) {}

    T Eval(Expr* expr) {
        expr->Accept(this);
        return _val;
    }

private:
    T _val;
};


class AddrEvaluator: public Visitor
{
    struct AddrConstant {
        std::string _label;
        int _offset;
    };
public:
    AddrEvaluator(void) {}
    
    virtual ~AddrEvaluator(void) {}

    virtual void VisitBinaryOp(BinaryOp* binary);
    virtual void VisitUnaryOp(UnaryOp* unary);
    virtual void VisitConditionalOp(ConditionalOp* cond);
    
    virtual void VisitFuncCall(FuncCall* funcCall) {
        Error(funcCall, "expect constant expression");
    }

    virtual void VisitEnumerator(Enumerator* enumer) {
        _addr._offset = enumer->Val();
    }

    virtual void VisitIdentifier(Identifier* ident) {
        _addr._label = ident->Name();
        _addr._offset = 0;
    }

    virtual void VisitObject(Object* obj) {
        if (!(obj->Storage() & S_STATIC) || obj->Linkage() == L_NONE) {
            Error(obj, "expect static object");
        }
        _addr._label = obj->Name();
        _addr._offset = 0;
    }

    virtual void VisitConstant(Constant* cons) {
        if (cons->Type()->IsInteger()) {
            _addr._offset = cons->IVal();
        } else if (cons->Type()->ToPointerType()) {
            _addr._label = cons->Label();
            _addr._offset = 0;
        } else {
            assert(false);
        }
    }

    virtual void VisitTempVar(TempVar* tempVar) {
        assert(false);
    }

    // We may should assert here
    virtual void VisitInitialization(Initialization* init) {}
    virtual void VisitIfStmt(IfStmt* ifStmt) {}
    virtual void VisitJumpStmt(JumpStmt* jumpStmt) {}
    virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
    virtual void VisitLabelStmt(LabelStmt* labelStmt) {}
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
    virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
    virtual void VisitFuncDef(FuncDef* funcDef) {}
    virtual void VisitTranslationUnit(TranslationUnit* unit) {}

    AddrConstant Eval(Expr* expr) {
        expr->Accept(this);
        return _addr;
    }

private:
    AddrConstant _addr;
};

#endif
