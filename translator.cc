#include "translator.h"

#include <queue>
#include <set>

TACJumpMap      Translator::jump_map_;
TACFunctionList Translator::tac_func_list_;

class Comp {
public:
  bool operator()(Object* lhs, Object* rhs) {
    return lhs->align() < rhs->align();
  }
};


void TACFunction::AllocAutoObjects() {
  auto offset = offset_;
  auto scope = func_def_->body()->scope();
  auto param_list = func_def_->FuncType()->param_list();

  auto paramSet = std::set<Object*>(param_list.begin(), param_list.end());
  std::priority_queue<Object*, std::vector<Object*>, Comp> heap;

  // Sort the objects by its alignment
  // Wgtcc alloc high memory for objects with bigger aligment
  for (auto iter = scope->begin(); iter != scope->end(); ++iter) {
    auto obj = iter->second->ToObject();
    if (!obj || obj->IsStatic())
      continue;
    // Skip parameters
    if (paramSet.find(obj) == paramSet.end())
      heap.push(obj);
  }

  while (!heap.empty()) {
    auto obj = heap.top();
    heap.pop();

    offset -= obj->type()->width();
    auto align = obj->align();
    if (obj->type()->ToArray()) {
      // The alignment of an array is at least the aligment of a pointer
      // (as it is always cast to a pointer)
      align = std::min(align, 8);
    }
    offset = Type::MakeAlign(offset, align);
    obj->set_offset(offset);
  }

  offset_ = offset;
}


void Translator::VisitBinaryOp(BinaryOp* binary) {
  auto op = binary->op();

  if (op == '=') {
    return TranslateAssign(binary);
  } else if (op == Token::LOGICAL_AND) {
    return GenAndOp(binary);
  } else if (op == Token::LOGICAL_OR) {
    return GenOrOp(binary);
  } else if (op == '.') {
    return TranslateMemberRef(binary);
  } else if (op == ',') {
    //return GenCommaOp(binary);
  } else if (binary->lhs()->type()->ToPointer() &&
             (op == '+' || op == '-')) {
    //return GenPointerArithm(binary);
  }

  //auto lhs = Translator().Visit(binary->lhs());
  //auto rhs = Translator().Visit(binary->rhs());
  //Operator tacop;
  switch (op) {

  }
}


void Translator::TranslateMemberRef(BinaryOp* member_ref) {
  assert(member_ref->op() == '.');
  auto lvalue = LValTranslator().Visit(member_ref);
  // TODO(wgtdkp): assign to a temporary
  operand_ = Variable::New(lvalue, member_ref->type());
}


void Translator::TranslateAssign(BinaryOp* assign) {
  auto lvalue = LValTranslator().Visit(assign->lhs());
  auto lhs = Variable::New(lvalue, assign->type());
  switch (assign->op()) {
  case '=': {
    auto rhs = Visit(assign->rhs());
    Gen(TAC::NewAssign(lhs, rhs));
  } break;
  case Token::ADD_ASSIGN:;
  case Token::SUB_ASSIGN:;
  }

  operand_ = lhs;
}


// TODO(wgtdkp): elimit construction of label_end
void Translator::GenAndOp(BinaryOp* and_op) {
  auto t = Temporary::New(and_op->type());
  auto lhs = Translator().Visit(and_op->lhs());
  
  auto label_end = TAC::NewLabel();
  auto assign_zero = TAC::NewAssign(t, Constant::Zero());
  auto assign_one = TAC::NewAssign(t, Constant::One());
  Gen(TAC::NewIfFalse(lhs, assign_zero));

  auto rhs = Translator().Visit(and_op->rhs());
  Gen(TAC::NewIfFalse(rhs, assign_zero));
  Gen(assign_one);
  Gen(TAC::NewJump(label_end));
  Gen(assign_zero);
  Gen(label_end);
}


void Translator::GenOrOp(BinaryOp* or_op) {
  auto t = Temporary::New(or_op->type());
  auto lhs = Translator().Visit(or_op->lhs());
  
  auto label_end = TAC::NewLabel();
  auto assign_zero = TAC::NewAssign(t, Constant::Zero());
  auto assign_one = TAC::NewAssign(t, Constant::One());
  Gen(TAC::NewIfFalse(lhs, assign_one));

  auto rhs = Translator().Visit(or_op->rhs());
  Gen(TAC::NewIfFalse(rhs, assign_one));
  Gen(assign_zero);
  Gen(TAC::NewJump(label_end));
  Gen(assign_one);
  Gen(label_end);
}


void Translator::VisitUnaryOp(UnaryOp* unary) {
  switch (unary->op()) {
  case Token::PREFIX_INC:;
  case Token::PREFIX_DEC:;
  case Token::POSTFIX_INC:;
  case Token::POSTFIX_DEC:;
  case Token::ADDR:; // &a
  case Token::DEREF:; // *p
  case Token::PLUS:;
  case Token::MINUS:;
  case '~':;
  case '!':;
  case Token::CAST:;
  default: assert(false);
  }
}


void Translator::TranslateDeref(UnaryOp* deref) {
  auto base = Translator().Visit(deref->operand());
  auto type = deref->type();
  operand_ = Variable::NewDerefed(type, base);
}


void Translator::VisitConditionalOp(ConditionalOp* cond) {

}


void Translator::VisitFuncCall(FuncCall* funcCall) {

}


void Translator::VisitObject(Object* obj) {
  if (!obj->IsStatic() && obj->anonymous()) {
    assert(obj->decl());
    Visit(obj->decl());
    obj->set_decl(nullptr);
  }

  operand_ = Variable::New(obj);
}


void Translator::VisitEnumerator(Enumerator* enumer) {

}


void Translator::VisitIdentifier(Identifier* ident) {

}


void Translator::VisitASTConstant(ASTConstant* cons) {

}


void Translator::VisitTempVar(TempVar* tempVar) {

}


void Translator::VisitDeclaration(Declaration* init) {

}


void Translator::VisitEmptyStmt(EmptyStmt* emptyStmt) {

}


void Translator::VisitIfStmt(IfStmt* ifStmt) {

}


void Translator::VisitGotoStmt(GotoStmt* goto_stmt) {
  // the destination is not resolved
  auto jump = TAC::NewJump(nullptr);
  Gen(jump);

  jump_map_[goto_stmt->label()].second.push_back(jump);
}


void Translator::VisitReturnStmt(ReturnStmt* return_stmt) {
  auto ret = Visit(return_stmt->expr());
  Gen(TAC::NewReturn(ret));
}


void Translator::VisitLabelStmt(LabelStmt* label_stmt) {
  auto label = TAC::NewLabel();
  jump_map_[label_stmt].first = label;
  
  Gen(label);
  Visit(label_stmt->sub_stmt());
}


void Translator::VisitCompoundStmt(CompoundStmt* compound_stmt) {
  if (compound_stmt->scope()) {
    cur_tac_func()->AllocAutoObjects();
  }
  for (auto stmt: compound_stmt->stmt_list()) {
    Visit(stmt);
  }
}


// Prologue and epilogue are job of code generator
void Translator::VisitFuncDef(FuncDef* func_def) {
  tac_func_list_.emplace_back(func_def);
  jump_map_.clear();

  cur_tac_func()->AllocAutoObjects();

  for (auto stmt: func_def->body()->stmt_list()) {
    Visit(stmt);
  }

  // Resolve jumps
  for (auto& kv: jump_map_) {
    auto label = kv.second.first;
    for (auto jump: kv.second.second)
      jump->set_jump_des(label);
  }
}


void Translator::VisitTranslationUnit(TranslationUnit* unit) {
  for (auto ext_decl: unit->ext_decl_list()) {
    Visit(ext_decl);
  }
}

/*
 * LValue expression translation
 *  a.b   *p    a
 */

// LValue expression: a.b
void LValTranslator::VisitBinaryOp(BinaryOp* binary) {
  assert(binary->op() == '.');
  
  lvalue_ = LValTranslator().Visit(binary->lhs());
  const auto& name = binary->rhs()->tok()->str();
  auto struct_type = binary->lhs()->type()->ToStruct();
  auto member = struct_type->GetMember(name);

  lvalue_.offset += member->offset();
}


// LValue expression: *a
void LValTranslator::VisitUnaryOp(UnaryOp* unary) {
  assert(unary->op() == Token::DEREF);
  // Could be variable or temporary
  lvalue_.base = Translator().Visit(unary->operand());
}


void LValTranslator::VisitObject(Object* obj) {
  if (!obj->IsStatic() && obj->anonymous()) {
    assert(obj->decl());
    Translator().Visit(obj->decl());
    obj->set_decl(nullptr);
  }
  lvalue_ = LValue(*obj);
}


// A function
void LValTranslator::VisitIdentifier(Identifier* ident) {
  assert(!ident->ToTypeName());
  // TODO(wgtdkp): make function an object
  lvalue_.name = ident->Name();
}


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

  Expr* cond_expr = nullptr;
  if (!ts_.Try(';')) {
    cond_expr = ParseExpr();
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
  if (cond_expr) {
    auto gotoEndStmt = GotoStmt::New(endLabel);
    auto ifStmt = IfStmt::New(cond_expr, EmptyStmt::New(), gotoEndStmt);
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
  stmts.push_back(GotoStmt::New(condLabel));
  stmts.push_back(endLabel);

  auto scope = cur_scope_;
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
  auto cond_expr = ParseExpr();
  ts_.Expect(')');

  if (!cond_expr->type()->IsScalar()) {
    Error(tok, "scalar expression expected");
  }

  auto condLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  auto gotoEndStmt = GotoStmt::New(endLabel);
  auto ifStmt = IfStmt::New(cond_expr, EmptyStmt::New(), gotoEndStmt);
  stmts.push_back(condLabel);
  stmts.push_back(ifStmt);
  
  Stmt* bodyStmt;
  ENTER_LOOP_BODY(endLabel, condLabel)
  bodyStmt = ParseStmt();
  EXIT_LOOP_BODY()
  
  stmts.push_back(bodyStmt);
  stmts.push_back(GotoStmt::New(condLabel));
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
  auto cond_expr = ParseExpr();
  ts_.Expect(')');
  ts_.Expect(';');

  auto gotoBeginStmt = GotoStmt::New(beginLabel);
  auto gotoEndStmt = GotoStmt::New(endLabel);
  auto ifStmt = IfStmt::New(cond_expr, gotoBeginStmt, gotoEndStmt);

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

  if (!expr->type()->IsInteger()) {
    Error(tok, "switch quantity not an integer");
  }

  auto testLabel = LabelStmt::New();
  auto endLabel = LabelStmt::New();
  auto t = TempVar::New(expr->type());
  auto assign = BinaryOp::New(tok, '=', t, expr);
  stmts.push_back(assign);
  stmts.push_back(GotoStmt::New(testLabel));

  CaseLabelList caseLabels;
  ENTER_SWITCH_BODY(endLabel, caseLabels);

  auto bodyStmt = ParseStmt(); // Fill caseLabels and defaultLabel
  stmts.push_back(bodyStmt);
  stmts.push_back(GotoStmt::New(endLabel));
  stmts.push_back(testLabel);

  for (auto iter = caseLabels.begin();
       iter != caseLabels.end(); ++iter) {
    auto cond = BinaryOp::New(tok, Token::EQ, t, iter->first);
    auto then = GotoStmt::New(iter->second);
    auto ifStmt = IfStmt::New(cond, then, nullptr);
    stmts.push_back(ifStmt);
  }
  if (defaultLabel_)
    stmts.push_back(GotoStmt::New(defaultLabel_));
  EXIT_SWITCH_BODY();

  stmts.push_back(endLabel);

  return CompoundStmt::New(stmts);
}
*/


