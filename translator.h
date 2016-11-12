#ifndef _WGTCC_TRANSLATOR_H_
#define _WGTCC_TRANSLATOR_H_

#include "ast.h"
#include "tac.h"
#include "visitor.h"

#include <list>
#include <vector>

using TACList = std::vector<tac::TAC*>;

class TACTranslator: public Visitor {
public:

  virtual ~TACTranslator() {}
  tac::Operand* Visit(ASTNode* node) {
    node->Accept(this);
    return operand_;
  }

  //Expression
  virtual void VisitBinaryOp(BinaryOp* binary);
  virtual void VisitUnaryOp(UnaryOp* unary);
  virtual void VisitConditionalOp(ConditionalOp* cond);
  virtual void VisitFuncCall(FuncCall* funcCall);
  virtual void VisitObject(Object* obj);
  virtual void VisitEnumerator(Enumerator* enumer);
  virtual void VisitIdentifier(Identifier* ident);
  virtual void VisitConstant(Constant* cons);
  virtual void VisitTempVar(TempVar* tempVar);
  
  //statement
  virtual void VisitDeclaration(Declaration* init);
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
  virtual void VisitIfStmt(IfStmt* ifStmt);
  virtual void VisitJumpStmt(JumpStmt* jumpStmt);
  virtual void VisitReturnStmt(ReturnStmt* returnStmt);
  virtual void VisitLabelStmt(LabelStmt* labelStmt);
  virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

  virtual void VisitFuncDef(FuncDef* funcDef);
  virtual void VisitTranslationUnit(TranslationUnit* unit);

  static void Gen(tac::TAC* tac) { tacList_.push_back(tac); }

protected:
  // Binary
  void GenCommaOp(BinaryOp* comma);
  void GenMemberRefOp(BinaryOp* binaryOp);
  void GenAndOp(BinaryOp* andOp);
  void GenOrOp(BinaryOp* orOp);
  void GenAddOp(BinaryOp* add);
  void GenSubOp(BinaryOp* sub);
  void GenAssignOp(BinaryOp* assign);
  void GenCastOp(UnaryOp* cast);
  void GenDerefOp(UnaryOp* deref);
  void GenMinusOp(UnaryOp* minus);
  void GenPointerArithm(BinaryOp* binary);
  void GenDivOp(bool flt, bool sign, int width, int op);
  void GenMulOp(int width, bool flt, bool sign);
  void GenCompOp(int width, bool flt, const char* set);
  void GenCompZero(Type* type);

  // Unary
  void GenIncDec(Expr* operand, bool postfix, const std::string& inst);

private:
  tac::Operand* operand_;
  static TACList tacList_;
};


class LValTranslator: public Translator {
public:
  LValTranslator() {}
  virtual void VisitBinaryOp(BinaryOp* binaryOp);
  virtual void VisitUnaryOp(UnaryOp* unaryOp);
  virtual void VisitObject(Object* obj);
  virtual void VisitIdentifier(Identifier* ident);
};

#endif
