#include "translator.h"

TACList TACTranslator::tacList_;


void TACTranslator::VisitBinaryOp(BinaryOp* binary) {
  auto op = binary->op_;

  if (op == '=') {
    return GenAssignOp(binary);
  } else if (op == Token::LOGICAL_AND) {
    return GenAndOp(binary);
  } else if (op == Token::LOGICAL_OR) {
    return GenOrOp(binary);
  } else if (op == '.') {
    return GenMemberRefOp(binary);
  } else if (op == ',') {
    return GenCommaOp(binary);
  } else if (binary->lhs_->Type()->ToPointer() &&
             (op == '+' || op == '-')) {
    return GenPointerArithm(binary);
  }

  auto lhs = TACTranslator().Visit(binary->lhs_);
  auto rhs = TACTranslator().Visit(binary->rhs_);
  tac::Operator tacop;
  switch (op) {

  }
}


void TACTranslator::GenAssignOp(BinaryOp* assign) {
  auto lhs = LValGenerator().Visit(assign->lhs_);
  auto rhs = TACTranslator().Visit(assign->rhs_);
  operand_ = tac::TAC::New(tac::Operator::ASSIGN, lhs, rhs);
  Gen(operand_);
}

// TODO(wgtdkp): elimit construction of labelEnd
void TACTranslator::GenAndOp(BinaryOp* andOp) {
  auto t = tac::Temporary::New(andOp->Type());
  auto lhs = TACTranslator().Visit(andOp->lhs_);
  
  auto labelEnd = tac::TAC::NewLabel();
  auto assignZero = tac::TAC::New(t, tac::Constant::Zero());
  auto assignOne = tac::TAC::New(t, tac::Constant::One());
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignZero, lhs));

  auto rhs = TACTranslator().Visit(andOp->rhs_);
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignZero, rhs));
  Gen(assignOne);
  Gen(tac::TAC::New(tac::Operator::JUMP, LabelEnd));
  Gen(assignZero);
  Gen(labelEnd);
}


void TACTranslator::GenOrOp(BinaryOp* orOp) {
  auto t = tac::Temporary::New(orOp->Type());
  auto lhs = TACTranslator().Visit(orOp->lhs_);
  
  auto labelEnd = tac::TAC::NewLabel();
  auto assignZero = tac::TAC::New(t, tac::Constant::Zero());
  auto assignOne = tac::TAC::New(t, tac::Constant::One());
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignOne, lhs));

  auto rhs = TACTranslator().Visit(orOp->rhs_);
  Gen(tac::TAC::New(tac::Operator::IF_FALSE, assignOne, rhs));
  Gen(assignZero);
  Gen(tac::TAC::New(tac::Operator::JUMP, labelEnd));
  Gen(assignOne);
  Gen(labelEnd);
}


void LValTranslator::VisitBinaryOp(BinaryOp* binaryOp) {
  assert(binary->op_ == '.');
  
}

void VisitUnaryOp(UnaryOp* unaryOp);
void VisitObject(Object* obj);
void VisitIdentifier(Identifier* ident);


/*
 * for 循环结构：
 *      for (declaration; expression1; expression2) statement
 * 展开后的结构：
 *		declaration
 * cond: ifFalse (expression1) then goto next
 *		   statement
 * step: expression2
 *		   goto cond
 * next:
 */

/*
CompoundStmt* Parser::ParseForStmt() {
  EnterBlock();
  ts_.Expect('(');
  
  StmtList stmts;

  if (IsType(ts_.Peek())) {
    stmts.push_back(ParseDecl());
  } else if (!ts_.Try(';')) {
    stmts.push_back(ParseExpr());
    ts_.Expect(';');
  }

  Expr* condExpr = nullptr;
  if (!ts_.Try(';')) {
    condExpr = ParseExpr();
    ts_.Expect(';');
  }

  Expr* stepExpr = nullptr;
  if (!ts_.Try(')')) {
    stepExpr = ParseExpr();
    ts_.Expect(')');
  }

  auto condLabel = LabelStmt::New();
  auto stepLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  stmts.push_back(condLabel);
  if (condExpr) {
    auto gotoEndStmt = JumpStmt::New(endLabel);
    auto ifStmt = IfStmt::New(condExpr, EmptyStmt::New(), gotoEndStmt);
    stmts.push_back(ifStmt);
  }

  //我们需要给break和continue语句提供相应的标号，不然不知往哪里跳
  Stmt* bodyStmt;
  ENTER_LOOP_BODY(endLabel, stepLabel);
  bodyStmt = ParseStmt();
  //因为for的嵌套结构，在这里需要回复break和continue的目标标号
  EXIT_LOOP_BODY()
  
  stmts.push_back(bodyStmt);
  stmts.push_back(stepLabel);
  if (stepExpr)
    stmts.push_back(stepExpr);
  else
    stmts.push_back(EmptyStmt::New());
  stmts.push_back(JumpStmt::New(condLabel));
  stmts.push_back(endLabel);

  auto scope = curScope_;
  ExitBlock();
  
  return CompoundStmt::New(stmts, scope);
}
*/


/*
 * while 循环结构：
 * while (expression) statement
 * 展开后的结构：
 * cond: if (expression) then empty
 *		else goto end
 *		statement
 *		goto cond
 * end:
 */

/*
CompoundStmt* Parser::ParseWhileStmt() {
  StmtList stmts;
  ts_.Expect('(');
  auto tok = ts_.Peek();
  auto condExpr = ParseExpr();
  ts_.Expect(')');

  if (!condExpr->Type()->IsScalar()) {
    Error(tok, "scalar expression expected");
  }

  auto condLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  auto gotoEndStmt = JumpStmt::New(endLabel);
  auto ifStmt = IfStmt::New(condExpr, EmptyStmt::New(), gotoEndStmt);
  stmts.push_back(condLabel);
  stmts.push_back(ifStmt);
  
  Stmt* bodyStmt;
  ENTER_LOOP_BODY(endLabel, condLabel)
  bodyStmt = ParseStmt();
  EXIT_LOOP_BODY()
  
  stmts.push_back(bodyStmt);
  stmts.push_back(JumpStmt::New(condLabel));
  stmts.push_back(endLabel);
  
  return CompoundStmt::New(stmts);
}
*/


/*
 * do-while 循环结构：
 *      do statement while (expression)
 * 展开后的结构：
 * begin: statement
 * cond: if (expression) then goto begin
 *		 else goto end
 * end:
 */

/*
CompoundStmt* Parser::ParseDoWhileStmt() {
  auto beginLabel = LabelStmt::New();
  auto condLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  
  Stmt* bodyStmt;
  ENTER_LOOP_BODY(endLabel, beginLabel)
  bodyStmt = ParseStmt();
  EXIT_LOOP_BODY()

  ts_.Expect(Token::WHILE);
  ts_.Expect('(');
  auto condExpr = ParseExpr();
  ts_.Expect(')');
  ts_.Expect(';');

  auto gotoBeginStmt = JumpStmt::New(beginLabel);
  auto gotoEndStmt = JumpStmt::New(endLabel);
  auto ifStmt = IfStmt::New(condExpr, gotoBeginStmt, gotoEndStmt);

  StmtList stmts;
  stmts.push_back(beginLabel);
  stmts.push_back(bodyStmt);
  stmts.push_back(condLabel);
  stmts.push_back(ifStmt);
  stmts.push_back(endLabel);
  
  return CompoundStmt::New(stmts);
}
*/


/*
 * switch
 *  jump stmt (skip case labels)
 *  case labels
 *  jump stmts
 *  default jump stmt
 */

/*
CompoundStmt* Parser::ParseSwitchStmt() {
  StmtList stmts;
  ts_.Expect('(');
  auto tok = ts_.Peek();
  auto expr = ParseExpr();
  ts_.Expect(')');

  if (!expr->Type()->IsInteger()) {
    Error(tok, "switch quantity not an integer");
  }

  auto testLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  auto t = TempVar::New(expr->Type());
  auto assign = BinaryOp::New(tok, '=', t, expr);
  stmts.push_back(assign);
  stmts.push_back(JumpStmt::New(testLabel));

  CaseLabelList caseLabels;
  ENTER_SWITCH_BODY(endLabel, caseLabels);

  auto bodyStmt = ParseStmt(); // Fill caseLabels and defaultLabel
  stmts.push_back(bodyStmt);
  stmts.push_back(JumpStmt::New(endLabel));
  stmts.push_back(testLabel);

  for (auto iter = caseLabels.begin();
       iter != caseLabels.end(); ++iter) {
    auto cond = BinaryOp::New(tok, Token::EQ, t, iter->first);
    auto then = JumpStmt::New(iter->second);
    auto ifStmt = IfStmt::New(cond, then, nullptr);
    stmts.push_back(ifStmt);
  }
  if (defaultLabel_)
    stmts.push_back(JumpStmt::New(defaultLabel_));
  EXIT_SWITCH_BODY();

  stmts.push_back(endLabel);

  return CompoundStmt::New(stmts);
}
*/


