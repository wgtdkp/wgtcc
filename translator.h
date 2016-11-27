#ifndef _WGTCC_TRANSLATOR_H_
#define _WGTCC_TRANSLATOR_H_

#include "ast.h"
#include "tac.h"
#include "visitor.h"

#include <list>
#include <vector>

using TACList = std::vector<TAC*>;

class Translator: public Visitor {
public:
  Translator(): operand_(nullptr) {}
  virtual ~Translator() {}
  Operand* Visit(ASTNode* node) {
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
  virtual void VisitASTConstant(ASTConstant* cons);
  virtual void VisitTempVar(TempVar* tempVar);
  
  //statement
  virtual void VisitDeclaration(Declaration* init);
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
  virtual void VisitIfStmt(IfStmt* ifStmt);
  virtual void VisitJumpStmt(GotoStmt* goto_stmt);
  virtual void VisitReturnStmt(ReturnStmt* returnStmt);
  virtual void VisitLabelStmt(LabelStmt* label_stmt);
  virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

  virtual void VisitFuncDef(FuncDef* func_def);
  virtual void VisitTranslationUnit(TranslationUnit* unit);

  static void Gen(TAC* tac) { tac_list_.push_back(tac); }

protected:
  // Binary
  void TranslateMemberRef(BinaryOp* member_ref);
  void TranslateAssign(BinaryOp* assign);

  void GenCommaOp(BinaryOp* comma);
  void GenMemberRefOp(BinaryOp* binaryOp);
  void GenAndOp(BinaryOp* and_op);
  void GenOrOp(BinaryOp* or_op);
  void GenAddOp(BinaryOp* add);
  void GenSubOp(BinaryOp* sub);

  void GenCastOp(UnaryOp* cast);
  void GenDerefOp(UnaryOp* deref);
  void GenMinusOp(UnaryOp* minus);
  void GenPointerArithm(BinaryOp* binary);
  void GenDivOp(bool flt, bool sign, int width, int op);
  void GenMulOp(int width, bool flt, bool sign);
  void GenCompOp(int width, bool flt, const char* set);
  void GenCompZero(Type* type);

  // Unary
  void TranslateDeref(UnaryOp* deref);
  void GenIncDec(Expr* operand, bool postfix, const std::string& inst);

private:
  Operand* operand_;
  static TACList tac_list_;
};


/*
 * LValue expression:
 *  a[b]  (*(a + b))
 *  a.b   (a->b)
 *  *a
 *  a
 */
class LValTranslator: public Translator {
public:
  LValTranslator() {}
  virtual ~LValTranslator() {}
  LValue Visit(ASTNode* node) {
    node->Accept(this);
    return lvalue_;
  }
  virtual void VisitBinaryOp(BinaryOp* binaryOp);
  virtual void VisitUnaryOp(UnaryOp* unaryOp);
  virtual void VisitObject(Object* obj);
  virtual void VisitIdentifier(Identifier* ident);

private:
  LValue lvalue_;
};

#endif
