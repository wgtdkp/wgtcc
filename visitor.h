#ifndef _WGTCC_VISITOR_H_
#define _WGTCC_VISITOR_H_


class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class Identifier;
class Object;
class Enumerator;
class Constant;
class TempVar;

class Declaration;
class IfStmt;
class JumpStmt;
class ReturnStmt;
class LabelStmt;
class EmptyStmt;
class CompoundStmt;
class FuncDef;
class TranslationUnit;


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
  virtual void VisitConstant(Constant* cons) = 0;
  virtual void VisitTempVar(TempVar* tempVar) = 0;

  virtual void VisitDeclaration(Declaration* init) {}

  virtual void VisitIfStmt(IfStmt* ifStmt) {}
  virtual void VisitForStmt(ForStmt* forStmt) {}
  virtual void VisitWhileStmt(WhileStmt* whileStmt) {}
  virtual void VisitSwitchStmt(SwitchStmt* switchStmt) {}
  virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
  virtual void VisitLabelStmt(LabelStmt* labelStmt) {}
  virtual void VisitBreakStmt(BreakStmt* breakStmt) {}
  virtual void VisitContinueStmt(ContinueStmt* continueStmt) {}
  virtual void VisitCaseStmt(CaseStmt* caseStmt) {}
  virtual void VisitGotoStmt(GotoStmt* gotoStmt) {}
  virtual void VisitDefaultStmt(DefaultStmt* defaultStmt) {}
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
  virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
  virtual void VisitFuncDef(FuncDef* funcDef) {}
  virtual void VisitTranslationUnit(TranslationUnit* unit) {}
};

#endif
