#include "error.h"
#include "visitor.h"

/*************** Constant Evaluator ******************/
void ConstantEvaluator::VisitBinaryOp(BinaryOp* binaryOp)
{
    //if (!binaryOp->Ty()->IsInteger())
    //    Error(binaryOp, "integer expected");

    ConstantEvaluator lEvaluator;
    ConstantEvaluator rEvaluator;
    //lEvaluator.visit

}

void ConstantEvaluator::VisitUnaryOp(UnaryOp* unaryOp)
{

}

void ConstantEvaluator::VisitConditionalOp(ConditionalOp* condOp)
{

}

void ConstantEvaluator::VisitFuncCall(FuncCall* funcCall)
{

}

void ConstantEvaluator::VisitVariable(Variable* var)
{

}

void ConstantEvaluator::VisitConstant(Constant* cons)
{

}

void ConstantEvaluator::VisitTempVar(TempVar* tempVar)
{

}

//statement
void ConstantEvaluator::VisitStmt(Stmt* stmt)
{

}

void ConstantEvaluator::VisitIfStmt(IfStmt* ifStmt)
{
    
}

void ConstantEvaluator::VisitJumpStmt(JumpStmt* jumpStmt)
{

}

void ConstantEvaluator::VisitReturnStmt(ReturnStmt* returnStmt)
{
    
}

void ConstantEvaluator::VisitLabelStmt(LabelStmt* labelStmt)
{

}

void ConstantEvaluator::VisitEmptyStmt(EmptyStmt* emptyStmt)
{

}

void ConstantEvaluator::VisitCompoundStmt(CompoundStmt* compoundStmt)
{
    
}

//Function Definition
void ConstantEvaluator::VisitFuncDef(FuncDef* funcDef)
{

}

//Translation Unit
void ConstantEvaluator::VisitTranslationUnit(TranslationUnit* transUnit)
{

}
