#ifndef _WGTCC_VISITOR_H_
#define _WGTCC_VISITOR_H_

#include "ast.h"

class Visitor {
public:
  virtual ~Visitor() {}
  virtual void VisitBinaryOp(BinaryOp* binary) = 0;
  virtual void VisitUnaryOp(UnaryOp* unary) = 0;
  virtual void VisitConditionalOp(ConditionalOp* cond) = 0;
  virtual void VisitFuncCall(FuncCall* funcCall) = 0;
  virtual void VisitEnumerator(Enumerator* enumer) = 0;
  virtual void VisitIdentifier(Identifier* ident) = 0;
  virtual void VisitObject(Object* obj) = 0;
  virtual void VisitASTConstant(ASTConstant* cons) = 0;
  virtual void VisitTempVar(TempVar* tempVar) = 0;

  virtual void VisitDeclaration(Declaration* init) {}

  virtual void VisitIfStmt(IfStmt* ifStmt) {}
  virtual void VisitForStmt(ForStmt* forStmt) {}
  virtual void VisitWhileStmt(WhileStmt* whileStmt) {}
  virtual void VisitSwitchStmt(SwitchStmt* switch_stmt) {}
  virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
  virtual void VisitLabelStmt(LabelStmt* label_stmt) {}
  virtual void VisitBreakStmt(BreakStmt* breakStmt) {}
  virtual void VisitContinueStmt(ContinueStmt* continueStmt) {}
  virtual void VisitCaseStmt(CaseStmt* case_stmt) {}
  virtual void VisitGotoStmt(GotoStmt* goto_stmt) {}
  virtual void VisitDefaultStmt(DefaultStmt* default_stmt) {}
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
  virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
  virtual void VisitFuncDef(FuncDef* func_def) {}
  virtual void VisitTranslationUnit(TranslationUnit* unit) {}
};

#endif
