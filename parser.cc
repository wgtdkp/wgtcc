#include "parser.h"

#include "cpp.h"
#include "encoding.h"
#include "error.h"
#include "evaluator.h"
#include "scope.h"
#include "type.h"

#include <iostream>
#include <set>
#include <string>
#include <climits>

FuncType* Parser::va_start_type_ {nullptr};
FuncType* Parser::va_arg_type_ {nullptr};


FuncDef* Parser::EnterFunc(Identifier* ident) {
  cur_func_ = FuncDef::New(ident);
  return cur_func_;
}


void Parser::ExitFunc() {
  // Resolve 那些待定的jump；
  // 如果有jump无法resolve，也就是有未定义的label，报错；
  for (const auto& kv: unresolved_goto_list_) {
    auto label = kv.first;
    const auto& name = label->str();
    auto label_stmt = FindLabel(name);
    if (label_stmt == nullptr) {
      Error(label, "label '%s' used but not defined", name.c_str());
    }
    kv.second->set_label(label_stmt);
  }

  unresolved_goto_list_.clear();	//清空未定的 jump 动作
  cur_labels_.clear();	//清空 label map
  cur_func_ = nullptr;
}


void Parser::Parse() {
  DefineBuiltins();
  ParseTranslationUnit();
}


void Parser::ParseTranslationUnit() {
  while (!ts_.Peek()->IsEOF()) {            
    if (ts_.Try(Token::STATIC_ASSERT)) {
      ParseStaticAssert();
      continue;
    } else if (ts_.Try(';')) {
      continue;
    }

    int storage_spec, func_spec, align;
    auto type = ParseDeclSpec(&storage_spec, &func_spec, &align);
    auto tok_type_pair = ParseDeclarator(type);
    auto tok = tok_type_pair.first;
    type = tok_type_pair.second;

    if (tok == nullptr) {
      ts_.Expect(';');
      continue;
    }

    auto ident = ProcessDeclarator(tok, type, storage_spec, func_spec, align);
    type = ident->type();

    if (tok && type->ToFunc() && ts_.Try('{')) { // Function definition
      unit_->Add(ParseFuncDef(ident));
    } else { // Declaration
      auto decl = ParseInitDeclarator(ident);
      if (decl) unit_->Add(decl);

      while (ts_.Try(',')) {
        auto ident = ParseDirectDeclarator(type, storage_spec, func_spec, align);
        decl = ParseInitDeclarator(ident);
        if (decl) unit_->Add(decl);
      }
      // GNU extension: function/type/variable attributes
      TryAttributeSpecList();
      ts_.Expect(';');
    }
  }
}


FuncDef* Parser::ParseFuncDef(Identifier* ident) {
  auto func_def = EnterFunc(ident);

  if (func_def->FuncType()->complete()) {
    Error(ident, "redefinition of '%s'", func_def->Name().c_str());
  }

  // TODO(wgtdkp): param checking
  auto func_type = ident->type()->ToFunc();
  func_type->set_complete(true);
  for (auto param: func_type->param_list()) {
    if (param->anonymous())
      Error(param, "param name omitted");
  }
  func_def->set_body(ParseCompoundStmt(func_type));
  ExitFunc();
  
  return func_def;
}


Expr* Parser::ParseExpr() {
  return ParseCommaExpr();
}


Expr* Parser::ParseCommaExpr() {
  auto lhs = ParseAssignExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(',')) {
    auto rhs = ParseAssignExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  return lhs;
}


Expr* Parser::ParsePrimaryExpr() {
  if (ts_.Empty()) {
    Error(ts_.Peek(), "premature end of input");
  }

  auto tok = ts_.Next();
  if (tok->tag() == '(') {
    auto expr = ParseExpr();
    ts_.Expect(')');
    return expr;
  }

  if (tok->IsIdentifier()) {
    auto ident = cur_scope_->Find(tok);
    if (ident) return ident;
    if (IsBuiltin(tok->str())) return GetBuiltin(tok);
    Error(tok, "undefined symbol '%s'", tok->str().c_str());
  } else if (tok->IsConstant()) {
    return ParseConstant(tok);
  } else if (tok->IsLiteral()) {
    return ConcatLiterals(tok);
  } else if (tok->tag() == Token::GENERIC) {
    return ParseGeneric();
  }

  Error(tok, "'%s' unexpected", tok->str().c_str());
  return nullptr; // Make compiler happy
}


static void ConvertLiteral(std::string& val, Encoding enc) {
  switch (enc) {
  case Encoding::NONE:
  case Encoding::UTF8: break;
  case Encoding::CHAR16: ConvertToUTF16(val); break;
  case Encoding::CHAR32:
  case Encoding::WCHAR: ConvertToUTF32(val); break;
  }
}


ASTConstant* Parser::ConcatLiterals(const Token* tok) {
  auto val = new std::string;
  auto enc = Scanner(tok).ScanLiteral(*val);
  ConvertLiteral(*val, enc);	
  while (ts_.Test(Token::LITERAL)) {
    auto next_tok = ts_.Next();
    std::string next_val;
    auto next_enc = Scanner(next_tok).ScanLiteral(next_val);
    ConvertLiteral(next_val, next_enc);
    if (enc == Encoding::NONE) {
      ConvertLiteral(*val, next_enc);
      enc = next_enc;
    }
    if (next_enc != Encoding::NONE && next_enc != enc)
      Error(next_tok, "cannot concat lietrals with different encodings");
    *val += next_val;
  }

  int tag = T_CHAR;
  switch (enc) {
  case Encoding::NONE:
  case Encoding::UTF8:
    tag = T_CHAR; val->append(1, '\0'); break;
  case Encoding::CHAR16:
    tag = T_UNSIGNED | T_SHORT; val->append(2, '\0'); break;
  case Encoding::CHAR32:
  case Encoding::WCHAR:
    tag = T_UNSIGNED | T_INT; val->append(4, '\0'); break;
  }

  return ASTConstant::New(tok, tag, val);
}


Encoding Parser::ParseLiteral(std::string& str, const Token* tok) {
  return Scanner(tok).ScanLiteral(str);
}


ASTConstant* Parser::ParseConstant(const Token* tok) {
  assert(tok->IsConstant());

  if (tok->tag() == Token::I_CONSTANT) {
    return ParseInteger(tok);
  } else if (tok->tag() == Token::C_CONSTANT) {
    return ParseCharacter(tok);
  } else {
    return ParseFloat(tok);
  }
}


ASTConstant* Parser::ParseFloat(const Token* tok) {
  const auto& str = tok->str();
  size_t end = 0;
  double val = 0.0;
  try {
    val = stod(str, &end);
  } catch (const std::out_of_range& oor) {
    Error(tok, "float out of range");
  }

  int tag = T_DOUBLE;
  if (str[end] == 'f' || str[end] == 'F') {
    tag = T_FLOAT;
    ++end;
  } else if (str[end] == 'l' || str[end] == 'L') {
    tag = T_LONG | T_DOUBLE;
    ++end;
  }
  if (str[end] != 0)
    Error(tok, "invalid suffix");

  return ASTConstant::New(tok, tag, val);
}


ASTConstant* Parser::ParseCharacter(const Token* tok) {
  int val;
  auto enc = Scanner(tok).ScanCharacter(val);

  int tag;
  switch (enc) {
  case Encoding::NONE:
    val = (char)val;
    tag = T_INT; break;
  case Encoding::CHAR16:
    val = (char16_t)val; 
    tag = T_UNSIGNED | T_SHORT; break;
  case Encoding::WCHAR:
  case Encoding::CHAR32: tag = T_UNSIGNED | T_INT; break;
  default: assert(false);
  }
  return ASTConstant::New(tok, tag, static_cast<long>(val));
}


ASTConstant* Parser::ParseInteger(const Token* tok) {
  const auto& str = tok->str();
  size_t end = 0;
  long val = 0;
  try {
    val = stoull(str, &end, 0);
  } catch (const std::out_of_range& oor) {
    Error(tok, "integer out of range");
  }

  int tag = 0;
  for (; str[end]; ++end) {
    if (str[end] == 'u' || str[end] == 'U') {
      if (tag & T_UNSIGNED)
        Error(tok, "invalid suffix");
      tag |= T_UNSIGNED;
    } else {
      if ((tag & T_LONG) || (tag & T_LLONG))
        Error(tok, "invalid suffix");
      if (str[end + 1] == 'l' || str[end + 1] =='L') {
        tag |= T_LLONG;
        ++end;
      } else {
        tag |= T_LONG;
      }
    }
  }

  bool decimal = ('1' <= str[0] && str[0] <= '9');
  if (decimal) {
    switch (tag) {
    case 0:
      tag |= !(val & ~(long)INT_MAX) ? T_INT: T_LONG; break;
    case T_UNSIGNED:
      tag |= !(val & ~(long)UINT_MAX) ? T_INT: T_LONG; break;
    case T_LONG: break;
    case T_UNSIGNED | T_LONG: break;
    }
  } else {
    switch (tag) {
    case 0:
      tag |= !(val & ~(long)INT_MAX) ? T_INT
           : !(val & ~(long)UINT_MAX) ? T_UNSIGNED
           : !(val & ~(long)LONG_MAX) ? T_LONG
           : T_UNSIGNED | T_LONG; break;
    case T_UNSIGNED:
      tag |= !(val & ~(long)UINT_MAX) ? T_INT: T_LONG; break;
    case T_LONG:
      tag |= !(val & ~(long)LONG_MAX) ? 0: T_UNSIGNED; break;
    case T_UNSIGNED | T_LONG:
      break;
    }
  }

  return ASTConstant::New(tok, tag, val);
}


Expr* Parser::ParseGeneric() {
  ts_.Expect('(');
  auto control_expr = ParseAssignExpr();
  ts_.Expect(',');
  Expr* selected_expr = nullptr;
  bool is_default = false;
  while (true) {
    if (ts_.Try(Token::DEFAULT)) {
      ts_.Expect(':');
      auto default_expr = ParseAssignExpr();
      if (!selected_expr) {
        selected_expr = default_expr;
        is_default = true;
      }
    } else {
      auto tok = ts_.Peek();
      auto type = ParseTypeName();
      ts_.Expect(':');
      auto expr = ParseAssignExpr();
      if (type->Compatible(*control_expr->type())) {
        if (selected_expr && !is_default) {
          Error(tok, "more than one generic association"
              " are compatible with control expression");
        }
        selected_expr = expr;
        is_default = false;
      }
    }
    if (!ts_.Try(',')) {
      ts_.Expect(')');
      break;
    }
  }

  if (!selected_expr)
    Error(ts_.Peek(), "no compatible generic association");
  return selected_expr;
}


QualType Parser::TryCompoundLiteral() {
  auto mark = ts_.Mark();
  if (ts_.Try('(') && IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    if (ts_.Try(')') && ts_.Test('{'))
      return type;
  }
  ts_.ResetTo(mark);
  return nullptr;
}


Expr* Parser::ParsePostfixExpr() {
  if (ts_.Peek()->IsEOF()) {
    Error(ts_.Peek(), "premature end of input");
  }

  auto type = TryCompoundLiteral();
  if (type) {
    auto anony = ParseCompoundLiteral(type);
    return ParsePostfixExprTail(anony);
  }

  auto prim_expr = ParsePrimaryExpr();
  return ParsePostfixExprTail(prim_expr);
}


Object* Parser::ParseCompoundLiteral(QualType type) {
  auto linkage = cur_scope_->type() == ScopeType::FILE ?
                 Linkage::INTERNAL: Linkage::NONE;
  auto anony = Object::NewAnony(ts_.Peek(), type, 0, linkage);
  auto decl = ParseInitDeclaratorSub(anony);
  
  // Just for generator to find the compound literal
  if (cur_scope_->type() == ScopeType::FILE) {
    unit_->Add(decl);
  } else {
    cur_scope_->Insert(anony->Repr(), anony);
  }
  return anony;
}


//return the constructed postfix expression
Expr* Parser::ParsePostfixExprTail(Expr* lhs) {
  while (true) {
    auto tok = ts_.Next();
    
    switch (tok->tag()) {
    case '[': lhs = ParseSubScripting(lhs); break;
    case '(': lhs = ParseFuncCall(lhs); break;
    case Token::PTR: lhs = UnaryOp::New(Token::DEREF, lhs);
    // Fall through
    case '.': lhs = ParseMemberRef(tok, '.', lhs); break;
    case Token::INC:
    case Token::DEC: lhs = ParsePostfixIncDec(tok, lhs); break;
    default: ts_.PutBack(); return lhs;
    }
  }
}


Expr* Parser::ParseSubScripting(Expr* lhs) {
  auto rhs = ParseExpr();  
  auto tok = ts_.Peek();
  ts_.Expect(']');
  auto operand = BinaryOp::New(tok, '+', lhs, rhs);
  return UnaryOp::New(Token::DEREF, operand);
}


BinaryOp* Parser::ParseMemberRef(const Token* tok, int op, Expr* lhs) {
  auto member_name = ts_.Peek()->str();
  ts_.Expect(Token::IDENTIFIER);

  auto struct_type = lhs->type()->ToStruct();
  if (struct_type == nullptr) {
    Error(tok, "an struct/union expected");
  }

  auto rhs = struct_type->GetMember(member_name);
  if (rhs == nullptr) {
    Error(tok, "'%s' is not a member of '%s'",
        member_name.c_str(), "[obj]");
  }

  return  BinaryOp::New(tok, op, lhs, rhs);
}


UnaryOp* Parser::ParsePostfixIncDec(const Token* tok, Expr* operand) {
  auto op = tok->tag() == Token::INC ?
            Token::POSTFIX_INC: Token::POSTFIX_DEC;
  return UnaryOp::New(op, operand);
}


FuncCall* Parser::ParseFuncCall(Expr* designator) {
  ArgList args;
  while (!ts_.Try(')')) {
    args.push_back(Expr::MayCast(ParseAssignExpr()));
    if (!ts_.Test(')'))
      ts_.Expect(',');
  }

  return FuncCall::New(designator, args);
}


Expr* Parser::ParseUnaryExpr() {
  auto tok = ts_.Next();
  switch (tok->tag()) {
  case Token::ALIGNOF: return ParseAlignof();
  case Token::SIZEOF: return ParseSizeof();
  case Token::INC: return ParsePrefixIncDec(tok);
  case Token::DEC: return ParsePrefixIncDec(tok);
  case '&': return ParseUnaryOp(tok, Token::ADDR);
  case '*': return ParseUnaryOp(tok, Token::DEREF); 
  case '+': return ParseUnaryOp(tok, Token::PLUS);
  case '-': return ParseUnaryOp(tok, Token::MINUS); 
  case '~': return ParseUnaryOp(tok, '~');
  case '!': return ParseUnaryOp(tok, '!');
  default:
    ts_.PutBack();
    return ParsePostfixExpr();
  }
}


ASTConstant* Parser::ParseSizeof() {
  QualType type {nullptr};  
  auto tok = ts_.Next();
  if (tok->tag() == '(' && IsTypeName(ts_.Peek())) {
    type = ParseTypeName();
    ts_.Expect(')');
  } else {
    ts_.PutBack();
    auto expr = ParseUnaryExpr();
    type = expr->type();
  }

  if (type->ToFunc() || type->ToVoid()) {
  } else if (!type->complete()) {
    Error(tok, "sizeof(incomplete type)");
  }
  long val = type->width();
  return ASTConstant::New(tok, T_UNSIGNED | T_LONG, val);
}


ASTConstant* Parser::ParseAlignof() {
  ts_.Expect('(');
  auto tok = ts_.Peek();
  auto type = ParseTypeName();
  ts_.Expect(')');

  long val = type->align();
  return ASTConstant::New(tok, T_UNSIGNED| T_LONG, val);
}


UnaryOp* Parser::ParsePrefixIncDec(const Token* tok) {
  assert(tok->tag() == Token::INC || tok->tag() == Token::DEC);
  
  auto op = tok->tag() == Token::INC ?
            Token::PREFIX_INC: Token::PREFIX_DEC;
  auto operand = ParseUnaryExpr();
  return UnaryOp::New(op, operand);
}


UnaryOp* Parser::ParseUnaryOp(const Token* tok, int op) {
  auto operand = ParseCastExpr();
  return UnaryOp::New(op, operand);
}


QualType Parser::ParseTypeName() {
  auto type = ParseSpecQual();
  if (ts_.Test('*') || ts_.Test('(') || ts_.Test('[')) //abstract-declarator FIRST set
    return ParseAbstractDeclarator(type);
  return type;
}


Expr* Parser::ParseCastExpr() {
  auto tok = ts_.Next();
  if (tok->tag() == '(' && IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    ts_.Expect(')');
    if (ts_.Test('{')) {
      auto anony = ParseCompoundLiteral(type);
      return ParsePostfixExprTail(anony);
    }
    auto operand = ParseCastExpr();
    return UnaryOp::New(Token::CAST, operand, type);
  }
  
  ts_.PutBack();
  return ParseUnaryExpr();
}


Expr* Parser::ParseMultiplicativeExpr() {
  auto lhs = ParseCastExpr();
  auto tok = ts_.Next();
  while (tok->tag() == '*' || tok->tag() == '/' || tok->tag() == '%') {
    auto rhs = ParseCastExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseAdditiveExpr() {
  auto lhs = ParseMultiplicativeExpr();
  auto tok = ts_.Next();
  while (tok->tag() == '+' || tok->tag() == '-') {
    auto rhs = ParseMultiplicativeExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseShiftExpr() {
  auto lhs = ParseAdditiveExpr();
  auto tok = ts_.Next();
  while (tok->tag() == Token::LEFT || tok->tag() == Token::RIGHT) {
    auto rhs = ParseAdditiveExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseRelationalExpr() {
  auto lhs = ParseShiftExpr();
  auto tok = ts_.Next();
  while (tok->tag() == Token::LE || tok->tag() == Token::GE 
      || tok->tag() == '<' || tok->tag() == '>') {
    auto rhs = ParseShiftExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseEqualityExpr() {
  auto lhs = ParseRelationalExpr();
  auto tok = ts_.Next();
  while (tok->tag() == Token::EQ || tok->tag() == Token::NE) {
    auto rhs = ParseRelationalExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseBitiwiseAndExpr() {
  auto lhs = ParseEqualityExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('&')) {
    auto rhs = ParseEqualityExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseBitwiseXorExpr() {
  auto lhs = ParseBitiwiseAndExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('^')) {
    auto rhs = ParseBitiwiseAndExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseBitwiseOrExpr() {
  auto lhs = ParseBitwiseXorExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('|')) {
    auto rhs = ParseBitwiseXorExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseLogicalAndExpr() {
  auto lhs = ParseBitwiseOrExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(Token::LOGICAL_AND)) {
    auto rhs = ParseBitwiseOrExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseLogicalOrExpr() {
  auto lhs = ParseLogicalAndExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(Token::LOGICAL_OR)) {
    auto rhs = ParseLogicalAndExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseConditionalExpr() {
  auto cond = ParseLogicalOrExpr();
  auto tok = ts_.Peek();
  if (ts_.Try('?')) {
    // Non-standard GNU extension
    // a ?: b equals a ? a: c
    auto expr_true = ts_.Test(':') ? cond: ParseExpr();
    ts_.Expect(':');
    auto expr_false = ParseConditionalExpr();

    return ConditionalOp::New(tok, cond, expr_true, expr_false);
  }
  
  return cond;
}


Expr* Parser::ParseAssignExpr() {
  // Yes, I know the lhs should be unary expression, 
  // let it handled by type checking
  Expr* lhs = ParseConditionalExpr();
  Expr* rhs;

  auto tok = ts_.Next();
  switch (tok->tag()) {
  case Token::MUL_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '*', lhs, rhs);
    break;

  case Token::DIV_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '/', lhs, rhs);
    break;

  case Token::MOD_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '%', lhs, rhs);
    break;

  case Token::ADD_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '+', lhs, rhs);
    break;

  case Token::SUB_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '-', lhs, rhs);
    break;

  case Token::LEFT_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, Token::LEFT, lhs, rhs);
    break;

  case Token::RIGHT_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, Token::RIGHT, lhs, rhs);
    break;

  case Token::AND_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '&', lhs, rhs);
    break;

  case Token::XOR_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '^', lhs, rhs);
    break;

  case Token::OR_ASSIGN:
    rhs = ParseAssignExpr();
    rhs = BinaryOp::New(tok, '|', lhs, rhs);
    break;

  case '=':
    rhs = ParseAssignExpr();
    break;

  default:
    ts_.PutBack();
    return lhs; // Could be constant
  }

  return BinaryOp::New(tok, '=', lhs, rhs);
}


void Parser::ParseStaticAssert() {
  ts_.Expect('(');
  auto cond_expr = ParseAssignExpr();
  ts_.Expect(',');
  auto msg = ConcatLiterals(ts_.Expect(Token::LITERAL));
  ts_.Expect(')');
  ts_.Expect(';');
  if (!Evaluator<long>().Eval(cond_expr)) {
    Error(ts_.Peek(), "static assertion failed: %s\n",
          msg->sval()->c_str());
  }
}


// Return: list of declarations
CompoundStmt* Parser::ParseDecl() {
  StmtList stmts;
  if (ts_.Try(Token::STATIC_ASSERT)) {
    ParseStaticAssert();
  } else {
    int storage_spec, func_spec, align;
    auto type = ParseDeclSpec(&storage_spec, &func_spec, &align);
    if (!ts_.Test(';')) {
      do {
        auto ident = ParseDirectDeclarator(type, storage_spec, func_spec, align);
        auto init = ParseInitDeclarator(ident);
        if (init) stmts.push_back(init);
      } while (ts_.Try(','));
    }
    ts_.Expect(';');
  }

  return CompoundStmt::New(stmts);
}


// For state machine
enum {
  // Compatibility for these key words
  COMP_SIGNED = T_SHORT | T_INT | T_LONG | T_LLONG,
  COMP_UNSIGNED = T_SHORT | T_INT | T_LONG | T_LLONG,
  COMP_CHAR = T_SIGNED | T_UNSIGNED,
  COMP_SHORT = T_SIGNED | T_UNSIGNED | T_INT,
  COMP_INT = T_SIGNED | T_UNSIGNED | T_LONG | T_SHORT | T_LLONG,
  COMP_LONG = T_SIGNED | T_UNSIGNED | T_LONG | T_INT,
  COMP_DOUBLE = T_LONG | T_COMPLEX,
  COMP_COMPLEX = T_FLOAT | T_DOUBLE | T_LONG,

  COMP_THREAD = S_EXTERN | S_STATIC,
};


static inline void TypeLL(int& type_spec) {
  if (type_spec & T_LONG) {
    type_spec &= ~T_LONG;
    type_spec |= T_LLONG;
  } else {
    type_spec |= T_LONG;
  }
}


QualType Parser::ParseSpecQual() {
  return ParseDeclSpec(nullptr, nullptr, nullptr);
}


static void EnsureAndSetStorageSpec(const Token* tok, int* storage, int spec) {
  if (!storage)
    Error(tok, "unexpected storage specifier");
  if (*storage != 0)
    Error(tok, "duplicated storage specifier");
  *storage |= spec;
}


/*
 * param: storage: null, only type specifier and qualifier accepted;
 */
QualType Parser::ParseDeclSpec(int* storage_spec, int* func_spec, int* align_spec) {
#define ERR_FUNC_SPEC ("unexpected function specifier")
#define ERR_STOR_SPEC ("unexpected storage specifier")
#define ERR_DECL_SPEC ("two or more data types in declaration specifiers")

  QualType type {nullptr};
  int qual_spec = 0;
  int type_spec = 0;    

  if (storage_spec) *storage_spec = 0;
  if (func_spec) *func_spec = 0;
  if (align_spec) *align_spec = 0;

  const Token* tok;
  for (; ;) {
    tok = ts_.Next();
    switch (tok->tag()) {
    //function specifier
    case Token::INLINE:
      if (!func_spec)
        Error(tok, ERR_FUNC_SPEC);
      *func_spec |= F_INLINE;
      break;

    case Token::NORETURN:
      if (!func_spec)
        Error(tok, ERR_FUNC_SPEC);
      *func_spec |= F_NORETURN;
      break;

    //alignment specifier
    case Token::ALIGNAS: {
      if (!align_spec)
        Error(tok, "unexpected alignment specifier");
      auto align = ParseAlignas();
      if (align)
        *align_spec = align;
      break;
    }
    //storage specifier
    //TODO: typedef needs more constraints
    case Token::TYPEDEF:
      EnsureAndSetStorageSpec(tok, storage_spec, S_TYPEDEF);
      break;

    case Token::EXTERN:
      EnsureAndSetStorageSpec(tok, storage_spec, S_EXTERN);
      break;

    case Token::STATIC:
      if (!storage_spec)
        Error(tok, ERR_FUNC_SPEC);
      if (*storage_spec & ~S_THREAD)
        Error(tok, "duplicated storage specifier");
      *storage_spec |= S_STATIC;
      break;

    case Token::THREAD:
      if (!storage_spec)
        Error(tok, ERR_FUNC_SPEC);
      if (*storage_spec & ~COMP_THREAD)
        Error(tok, "duplicated storage specifier");
      *storage_spec |= S_THREAD;
      break;

    case Token::AUTO:
      EnsureAndSetStorageSpec(tok, storage_spec, S_AUTO);
      break;

    case Token::REGISTER:
      EnsureAndSetStorageSpec(tok, storage_spec, S_REGISTER);
      break;
    
    //type qualifier
    case Token::CONST:    qual_spec |= Qualifier::CONST;    break;
    case Token::RESTRICT: qual_spec |= Qualifier::RESTRICT; break;
    case Token::VOLATILE: qual_spec |= Qualifier::VOLATILE; break;

    //type specifier
    case Token::SIGNED:
      if (type_spec & ~COMP_SIGNED)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_SIGNED;
      break;

    case Token::UNSIGNED:
      if (type_spec & ~COMP_UNSIGNED)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_UNSIGNED;
      break;

    case Token::VOID:
      if (type_spec & ~0)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_VOID;
      break;

    case Token::CHAR:
      if (type_spec & ~COMP_CHAR)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_CHAR;
      break;

    case Token::SHORT:
      if (type_spec & ~COMP_SHORT)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_SHORT;
      break;

    case Token::INT:
      if (type_spec & ~COMP_INT)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_INT;
      break;

    case Token::LONG:
      if (type_spec & ~COMP_LONG)
        Error(tok, ERR_DECL_SPEC);
      TypeLL(type_spec); 
      break;
      
    case Token::FLOAT:
      if (type_spec & ~T_COMPLEX)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_FLOAT;
      break;

    case Token::DOUBLE:
      if (type_spec & ~COMP_DOUBLE)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_DOUBLE;
      break;

    case Token::BOOL:
      if (type_spec != 0)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_BOOL;
      break;

    case Token::COMPLEX:
      if (type_spec & ~COMP_COMPLEX)
        Error(tok, ERR_DECL_SPEC);
      type_spec |= T_COMPLEX;
      break;

    case Token::STRUCT: 
    case Token::UNION:
      if (type_spec & ~0)
        Error(tok, ERR_DECL_SPEC);
      type = ParseStructUnionSpec(Token::STRUCT == tok->tag()); 
      type_spec |= T_STRUCT_UNION;
      break;

    case Token::ENUM:
      if (type_spec != 0)
        Error(tok, ERR_DECL_SPEC);
      type = ParseEnumSpec();
      type_spec |= T_ENUM;
      break;

    case Token::ATOMIC:
      Error(tok, "atomic not supported");
      break;

    default:
      if (type_spec == 0 && IsTypeName(tok)) {
        auto ident = cur_scope_->Find(tok);
        type = ident->type();
        // We may change the length of a array type by initializer,
        // thus, make a copy of this type.
        auto arr_type = type->ToArray();
        if (arr_type && !type->complete())
          type = ArrayType::New(arr_type->len(), arr_type->derived());
        type_spec |= T_TYPEDEF_NAME;
      } else  {
        goto end_of_loop;
      }
    }
  }

end_of_loop:
  ts_.PutBack();
  switch (type_spec) {
  case 0:
    Error(tok, "expect type specifier");
    break;

  case T_VOID:
    type = VoidType::New();
    break;

  case T_STRUCT_UNION:
  case T_ENUM:
  case T_TYPEDEF_NAME:
    break;

  default:
    type = ArithmType::New(type_spec);
    break;
  }
  // GNU extension: type attributes
  //if (storage_spec && (*storage_spec & S_TYPEDEF))
  //  TryAttributeSpecList();

  return QualType(type.ptr(), qual_spec | type.Qual());

#undef ERR_FUNC_SPEC
#undef ERR_STOR_SPEC
#undef ERR_DECL_SPEC
}


int Parser::ParseAlignas() {
  int align;
  ts_.Expect('(');
  auto tok = ts_.Peek();
  if (IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    ts_.Expect(')');
    align = type->align();
  } else {
    auto expr = ParseExpr();
    align = Evaluator<long>().Eval(expr);
    ts_.Expect(')');
  }
  if (align < 0 || ((align - 1) & align))
    Error(tok, "requested alignment is not a positive power of 2");
  return align;
}


Type* Parser::ParseEnumSpec() {
  // GNU extension: type attributes
  TryAttributeSpecList();

  std::string tag_name;
  auto tok = ts_.Peek();
  if (ts_.Try(Token::IDENTIFIER)) {
    tag_name = tok->str();
    if (ts_.Try('{')) {
      //定义enum类型
      auto tag_ident = cur_scope_->FindTagInCurScope(tok);
      if (!tag_ident) {
        auto type = ArithmType::New(T_INT);
        auto ident = Identifier::New(tok, type, Linkage::NONE);
        cur_scope_->InsertTag(ident);
        return ParseEnumerator(type);   //处理反大括号: '}'
      }

      if (!tag_ident->type()->IsInteger()) // struct/union tag
        Error(tok, "redefinition of enumeration tag '%s'", tag_name.c_str());
      return ParseEnumerator(tag_ident->type()->ToArithm());
    } else {
      auto tag_ident = cur_scope_->FindTag(tok);
      if (tag_ident) {
        return tag_ident->type();
      }
      auto type = ArithmType::New(T_INT);
      auto ident = Identifier::New(tok, type, Linkage::NONE);
      cur_scope_->InsertTag(ident);
      return type;
    }
  }
  
  ts_.Expect('{');
  auto type = ArithmType::New(T_INT);
  return ParseEnumerator(type);   //处理反大括号: '}'
}


Type* Parser::ParseEnumerator(ArithmType* type) {
  assert(type && type->IsInteger());
  int val = 0;
  do {
    auto tok = ts_.Expect(Token::IDENTIFIER);
    // GNU extension: enumerator attributes
    TryAttributeSpecList();

    const auto& enum_name = tok->str();
    auto ident = cur_scope_->FindInCurScope(tok);
    if (ident) {
      Error(tok, "redefinition of enumerator '%s'", enum_name.c_str());
    }
    if (ts_.Try('=')) {
      auto expr = ParseAssignExpr();
      val = Evaluator<long>().Eval(expr);
    }
    auto enumer = Enumerator::New(tok, val);
    ++val;
    cur_scope_->Insert(enumer);
    ts_.Try(',');
  } while (!ts_.Try('}'));
  
  type->set_complete(true);
  return type;
}


/*
 * 四种 name space：
 * 1.label, 如 goto end; 它有函数作用域
 * 2.struct/union/enum 的 tag
 * 3.struct/union 的成员
 * 4.其它的普通的变量
 */
Type* Parser::ParseStructUnionSpec(bool is_struct) {
  // GNU extension: type attributes
  TryAttributeSpecList();

  std::string tag_name;
  auto tok = ts_.Peek();
  if (ts_.Try(Token::IDENTIFIER)) {
    tag_name = tok->str();
    if (ts_.Try('{')) {
      //看见大括号，表明现在将定义该struct/union类型
      //我们不用关心上层scope是否定义了此tag，如果定义了，那么就直接覆盖定义      
      auto tag_ident = cur_scope_->FindTagInCurScope(tok);
      if (!tag_ident) {
        //现在是在当前scope第一次看到name，所以现在是第一次定义，连前向声明都没有；
        auto type = StructType::New(is_struct, tag_name.size(), cur_scope_);
        auto ident = Identifier::New(tok, type, Linkage::NONE);
        cur_scope_->InsertTag(ident); 
        return ParseStructUnionDecl(type); //处理反大括号: '}'
      }
      
      
      // 在当前scope找到了类型，但可能只是声明；注意声明与定义只能出现在同一个scope；
      // 1.如果声明在定义的外层scope,那么即使在内层scope定义了完整的类型，此声明仍然是无效的；
      //   因为如论如何，编译器都不会在内部scope里面去找定义，所以声明的类型仍然是不完整的；
      // 2.如果声明在定义的内层scope,(也就是先定义，再在内部scope声明)，这时，不完整的声明会覆盖掉完整的定义；
      //   因为编译器总是向上查找符号，不管找到的是完整的还是不完整的，都要；
      if (!tag_ident->type()->complete()) {
        //找到了此tag的前向声明，并更新其符号表，最后设置为complete type
        return ParseStructUnionDecl(tag_ident->type()->ToStruct());
      } else {
        //在当前作用域找到了完整的定义，并且现在正在定义同名的类型，所以报错；
        Error(tok, "redefinition of struct tag '%s'", tag_name.c_str());
      }
    } else {
      // 没有大括号，表明不是定义一个struct/union;那么现在只可能是在：
      // 1.声明；
      // 2.声明的同时，定义指针(指针允许指向不完整类型) (struct Foo* p; 是合法的) 或者其他合法的类型；
      //   如果现在索引符号表，那么：
      //   1.可能找到name的完整定义，也可能只找得到不完整的声明；不管name指示的是不是完整类型，我们都只能选择name指示的类型；
      //   2.如果我们在符号表里面压根找不到name,那么现在是name的第一次声明，创建不完整的类型并插入符号表；
      auto tag_ident = cur_scope_->FindTag(tok);
      
      //如果tag已经定义或声明，那么直接返回此定义或者声明
      if (tag_ident) {
        return tag_ident->type();
      }
      //如果tag尚没有定义或者声明，那么创建此tag的声明(因为没有见到‘{’，所以不会是定义)
      auto type = StructType::New(is_struct, true, cur_scope_);
      
      //因为有tag，所以不是匿名的struct/union， 向当前的scope插入此tag
      auto ident = Identifier::New(tok, type, Linkage::NONE);
      cur_scope_->InsertTag(ident);
      return type;
    }
  }
  //没见到identifier，那就必须有struct/union的定义，这叫做匿名struct/union;
  ts_.Expect('{');

  //现在，如果是有tag，那它没有前向声明；如果是没有tag，那更加没有前向声明；
  //所以现在是第一次开始定义一个完整的struct/union类型
  auto type = StructType::New(is_struct, tag_name.size(), cur_scope_);  
  return ParseStructUnionDecl(type); //处理反大括号: '}'
}


StructType* Parser::ParseStructUnionDecl(StructType* type) {
#define ADD_MEMBER() {                          \
  auto member = Object::New(tok, member_type);  \
  if (align > 0)                                \
    member->set_align(align);                   \
  type->AddMember(member);                      \
}

  //既然是定义，那输入肯定是不完整类型，不然就是重定义了
  assert(type && !type->complete());

  auto scope_backup = cur_scope_;
  cur_scope_ = type->member_map(); // Internal symbol lookup rely on cur_scope_
  while (!ts_.Try('}')) {
    if (ts_.Empty()) {
      Error(ts_.Peek(), "premature end of input");
    }
    
    if(ts_.Try(Token::STATIC_ASSERT)) {
      ParseStaticAssert();
      continue;
    }

    // 解析type specifier/qualifier, 不接受storage等
    int align;
    auto base_type = ParseDeclSpec(nullptr, nullptr, &align);
    do {
      auto tok_type_pair = ParseDeclarator(base_type);
      auto tok = tok_type_pair.first;
      auto member_type = tok_type_pair.second;
      
      if (ts_.Try(':')) {
        ParseBitField(type, tok, member_type);
        continue;
      }

      if (tok == nullptr) {
        auto su_type = member_type->ToStruct();
        if (su_type && !su_type->HasTag()) {
          auto anony = Object::NewAnony(ts_.Peek(), su_type);
          type->MergeAnony(anony);
          continue;
        } else {
          Error(ts_.Peek(), "declaration does not declare anything");
        }
      }

      const auto& name = tok->str();                
      if (type->GetMember(name)) {
        Error(tok, "duplicate member '%s'", name.c_str());
      } else if (!member_type->complete()) {
        // C11 6.7.2.1 [3]:
        if (type->IsStruct() &&
            // Struct has more than one named member
            type->member_map()->size() > 0 &&
            member_type->ToArray()) {
          ts_.Expect(';'); ts_.Expect('}');
          ADD_MEMBER();
          goto finalize;
        } else {
          Error(tok, "field '%s' has incomplete type", name.c_str());
        }
      } else if (member_type->ToFunc()) {
        Error(tok, "field '%s' declared as a function", name.c_str());
      }

      ADD_MEMBER();
    } while (ts_.Try(','));
    ts_.Expect(';');
  }
finalize:
  // GNU extension: type attributes
  TryAttributeSpecList();

  //struct/union定义结束，设置其为完整类型
  type->Finalize();
  type->set_complete(true);
  // TODO(wgtdkp): we need to export tags defined inside struct
  const auto& tags = cur_scope_->AllTagsInCurScope();
  for (auto tag: tags) {
    if (scope_backup->FindTag(tag->tok()))
      Error(tag, "redefinition of tag '%s'\n", tag->Name().c_str());
    scope_backup->InsertTag(tag);
  }
  cur_scope_ = scope_backup;
  
  return type;
}


void Parser::ParseBitField(StructType* struct_type,
                           const Token* tok,
                           QualType type) {
  if (!type->IsInteger()) {
    Error(tok ? tok: ts_.Peek(), "expect integer type for bitfield");
  }

  auto expr = ParseAssignExpr();
  auto width = Evaluator<long>().Eval(expr);
  if (width < 0) {
    Error(expr, "expect non negative value");
  } else if (width == 0 && tok) {
    Error(tok, "no declarator expected for a bitfield with width 0");
  } else if (width > type->width() * 8) {
    Error(expr, "width exceeds its type");
  }

  auto offset = struct_type->offset() - type->width();
  // C11 6.7.5 [2]: alignment attribute shall not be specified in declaration of a bit field
  // so here is ok to use type->align() 
  offset = Type::MakeAlign(std::max(offset, 0), type->align());

  int bitfield_offset;
  uint8_t begin;

  if (!struct_type->IsStruct()) {
    begin = 0;
    bitfield_offset = 0;
  } else if (struct_type->member_list().size() == 0) {
    begin = 0;
    bitfield_offset = 0;
  } else {
    auto last = struct_type->member_list().back();
    auto total_bits = last->offset() * 8;
    if (last->bitfield_width()) {
      total_bits += last->bitfield_end();
    } else { // Is not bit field
      total_bits += last->type()->width() * 8;
    }

    if (width == 0)
      width = type->width() * 8 - total_bits; // So posterior bitfield would be packed
    if (width == 0) // A bitfield with zero width is never added to member list 
      return;       // Because we use bitfield width to tell if a member is bitfield or not.
    if (width + total_bits <= type->width() * 8) {
      begin = total_bits % 8;
      bitfield_offset = total_bits / 8;
    } else {
      begin = 0;
      bitfield_offset = Type::MakeAlign(struct_type->offset(), type->width());
    }
  }

  Object* bitfield;
  if (tok) {
    bitfield = Object::New(tok, type, 0, Linkage::NONE, begin, width);
  } else {
    bitfield = Object::NewAnony(ts_.Peek(), type, 0,
                                Linkage::NONE, begin, width);
  }
  struct_type->AddBitField(bitfield, bitfield_offset);
}


int Parser::ParseQual() {
  int qual_spec = 0;
  for (; ;) {
    auto tok = ts_.Next();
    switch (tok->tag()) {
    case Token::CONST:    qual_spec |= Qualifier::CONST;    break;
    case Token::RESTRICT: qual_spec |= Qualifier::RESTRICT; break;
    case Token::VOLATILE: qual_spec |= Qualifier::VOLATILE; break;
    case Token::ATOMIC:   Error(tok, "do not support 'atomic'"); break;
    default: ts_.PutBack(); return qual_spec;
    }
  }
}


QualType Parser::ParsePointer(QualType type_pointed_to) {
  while (ts_.Try('*')) {
    auto t = PointerType::New(type_pointed_to);
    type_pointed_to = QualType(t, ParseQual());
  }
  return type_pointed_to;
}


static QualType ModifyBase(QualType type, QualType base, QualType new_base) {
  if (type == base)
    return new_base;
  
  auto ty = type->ToDerived();
  ty->set_derived(ModifyBase(ty->derived(), base, new_base));
  
  return ty;
}


/*
 * Return: pair of token(must be identifier) and it's type
 *     if token is nullptr, then we are parsing abstract declarator
 *     else, parsing direct declarator.
 */
TokenTypePair Parser::ParseDeclarator(QualType base) {
  // May be pointer
  auto pointer_type = ParsePointer(base);
  
  if (ts_.Try('(')) {
    //现在的 pointer_type 并不是正确的 base type
    auto tok_type_pair = ParseDeclarator(pointer_type);
    auto tok = tok_type_pair.first;
    auto type = tok_type_pair.second;

    ts_.Expect(')');

    auto new_base = ParseArrayFuncDeclarator(tok, pointer_type);
    
    //修正 base type
    auto ret_type = ModifyBase(type, pointer_type, new_base);
    return TokenTypePair(tok_type_pair.first, ret_type);
  } else if (ts_.Peek()->IsIdentifier()) {
    auto tok = ts_.Next();
    // GNU extension: variable attributes
    TryAttributeSpecList();
    auto ret_type = ParseArrayFuncDeclarator(tok, pointer_type);
    return TokenTypePair(tok, ret_type);
  } else {
    errTok_ = ts_.Peek();
    auto ret_type = ParseArrayFuncDeclarator(nullptr, pointer_type);
    return TokenTypePair(nullptr, ret_type);
  }
}


Identifier* Parser::ProcessDeclarator(const Token* tok,
                                      QualType type,
                                      int storage_spec,
                                      int func_spec,
                                      int align) {
  assert(tok);
  
  // 检查在同一 scope 是否已经定义此变量
  // 如果 storage 是 typedef，那么应该往符号表里面插入 type
  // 定义 void 类型变量是非法的，只能是指向void类型的指针
  // 如果 func_spec != 0, 那么现在必须是在定义函数，否则出错
  const auto& name = tok->str();
  Identifier* ident;

  if (storage_spec & S_TYPEDEF) {
    // C11 6.7.5 [2]: alignment specifier
    if (align > 0)
      Error(tok, "alignment specified for typedef");

    ident = cur_scope_->FindInCurScope(tok);
    if (ident) { // There is prio declaration in the same scope
      // The same declaration, simply return the prio declaration
      if (!type->Compatible(*ident->type()))
        Error(tok, "conflicting types for '%s'", name.c_str());

      // TODO(wgtdkp): add previous declaration information
      return ident;        
    }
    ident = Identifier::New(tok, type, Linkage::NONE);
    cur_scope_->Insert(ident);
    return ident;
  }

  if (type->ToVoid()) {
    Error(tok, "variable or field '%s' declared void",
        name.c_str());
  }

  if (type->ToFunc() && cur_scope_->type() != ScopeType::FILE
      && (storage_spec & S_STATIC)) {
    Error(tok, "invalid storage class for function '%s'", name.c_str());
  }

  Linkage linkage;
  // Identifiers in function prototype have no linkage
  if (cur_scope_->type() == ScopeType::PROTO) {
    linkage = Linkage::NONE;
  } else if (cur_scope_->type() == ScopeType::FILE) {
    linkage = Linkage::EXTERNAL; // Default linkage for file scope identifiers
    if (storage_spec & S_STATIC)
      linkage = Linkage::INTERNAL;
  } else if (!(storage_spec & S_EXTERN)) {
    linkage = Linkage::NONE; // Default linkage for block scope identifiers
    if (type->ToFunc())
      linkage = Linkage::EXTERNAL;
  } else {
    linkage = Linkage::EXTERNAL;
  }

  //cur_scope_->Print();
  ident = cur_scope_->FindInCurScope(tok);
  if (ident) { // There is prio declaration in the same scope
    if (!type->Compatible(*ident->type())) {
      Error(tok, "conflicting types for '%s'", name.c_str());
    }

    // The same scope prio declaration has no linkage,
    // there is a redeclaration error
    if (linkage == Linkage::NONE) {
      Error(tok, "redeclaration of '%s' with no linkage", name.c_str());
    } else if (linkage == Linkage::EXTERNAL) {
      if (ident->linkage() == Linkage::NONE) {
        Error(tok, "conflicting linkage for '%s'", name.c_str());
      }
    } else {
      if (ident->linkage() != Linkage::INTERNAL) {
        Error(tok, "conflicting linkage for '%s'", name.c_str());
      }
    }
    // The same declaration, simply return the prio declaration
    if (!ident->type()->complete())
      ident->type()->set_complete(type->complete());
    // Prio declaration of a function may omit the param name
    if (type->ToFunc())
      ident->type()->ToFunc()->set_param_list(type->ToFunc()->param_list());
    else if (ident->ToObject() && !(storage_spec & S_EXTERN))
      ident->ToObject()->set_storage(ident->ToObject()->storage() & ~S_EXTERN);
    return ident;
  } else if (linkage == Linkage::EXTERNAL) {
    ident = cur_scope_->Find(tok);
    if (ident) {
      if (!type->Compatible(*ident->type())) {
        Error(tok, "conflicting types for '%s'", name.c_str());
      }
      if (ident->linkage() != Linkage::NONE) {
        linkage = ident->linkage();
      }
      // Don't return, override it
    } else {
      ident = external_symbols_->FindInCurScope(tok);
      if (ident) {
        if (!type->Compatible(*ident->type())) {
          Error(tok, "conflicting types for '%s'", name.c_str());
        }
        // TODO(wgtdkp): ???????
        // Don't return
        // To stop later declaration with the same name in the same scope overriding this declaration
        
        // Useless here, just keep it
        if (!ident->type()->complete())
          ident->type()->set_complete(type->complete()); 
        //return ident;
      }
    }
  }

  Identifier* ret;
  // TODO(wgtdkp): Treat function as object ?
  if (type->ToFunc()) {
    // C11 6.7.5 [2]: alignment specifier
    if (align > 0)
      Error(tok, "alignment specified for function");
    ret = Identifier::New(tok, type, linkage);
  } else {
    auto obj = Object::New(tok, type, storage_spec, linkage);
    if (align > 0)
      obj->set_align(align);
    ret = obj;
  }
  cur_scope_->Insert(ret);
  if (linkage == Linkage::EXTERNAL && ident == nullptr) {
      external_symbols_->Insert(ret);
  }

  return ret;
}


QualType Parser::ParseArrayFuncDeclarator(const Token* ident, QualType base) {
  if (ts_.Try('[')) {

    if (base->ToFunc()) {
      Error(ts_.Peek(), "the element of array cannot be a function");
    }

    auto len = ParseArrayLength();
    ts_.Expect(']');
    base = ParseArrayFuncDeclarator(ident, base);
    if (!base->complete()) {
      // FIXME(wgtdkp): ident could be nullptr
      Error(ident, "'%s' has incomplete element type",
          ident->str().c_str());
    }
    return ArrayType::New(len, base);
  } else if (ts_.Try('(')) {	//function declaration
    if (base->ToFunc()) {
      Error(ts_.Peek(),
          "the return value of function cannot be function");
    } else if (nullptr != base->ToArray()) {
      Error(ts_.Peek(),
          "the return value of function cannot be array");
    }

    FuncType::ParamList param_list;
    EnterProto();
    auto variadic = ParseParamList(param_list);
    ExitProto();
    
    ts_.Expect(')');
    base = ParseArrayFuncDeclarator(ident, base);
    
    return FuncType::New(base, 0, variadic, param_list);
  }

  return base;
}


/*
 * return: -1, length not specified
 */
int Parser::ParseArrayLength() {
  auto has_static = ts_.Try(Token::STATIC);
  auto qual = ParseQual();
  if (0 != qual)
    has_static = ts_.Try(Token::STATIC);

  //不支持变长数组
  if (!has_static && ts_.Test(']'))
    return -1;
  
  auto expr = ParseAssignExpr();
  EnsureInteger(expr);
  auto ret = Evaluator<long>().Eval(expr);
  if (ret < 0) {
    Error(expr, "size of array is negative");
  }
  return ret;
}


/*
 * Return: true, variadic;
 */
bool Parser::ParseParamList(FuncType::ParamList& param_list) {
  if (ts_.Test(')'))
    return false;
  auto param = ParseParamDecl();
  if (param->type()->ToVoid())
    return false;
  param_list.push_back(param);

  while (ts_.Try(',')) {
    if (ts_.Try(Token::ELLIPSIS))
      return true;
    param = ParseParamDecl();
    if (param->type()->ToVoid())
      Error(param, "'void' must be the only parameter");
    param_list.push_back(param);
  }
  return false;
}


Object* Parser::ParseParamDecl() {
  int storage_spec, func_spec;
  // C11 6.7.5 [2]: alignment specifier cannot be specified in param_list
  auto type = ParseDeclSpec(&storage_spec, &func_spec, nullptr);
  auto tok_type_pair = ParseDeclarator(type);
  auto tok = tok_type_pair.first;
  type = Type::MayCast(tok_type_pair.second);
  if (!tok) { // Abstract declarator
    return Object::NewAnony(ts_.Peek(), type, 0, Linkage::NONE);
  }

  // align set to non positive, stands for not specified
  auto ident = ProcessDeclarator(tok, type, storage_spec, func_spec, -1);
  if (!ident->ToObject())
    Error(ident, "expect object in param list");

  return ident->ToObject();
}


QualType Parser::ParseAbstractDeclarator(QualType type) {
  auto tok_type_pair = ParseDeclarator(type);
  auto tok = tok_type_pair.first;
  type = tok_type_pair.second;
  if (tok) { // Not a abstract declarator!
    Error(tok, "unexpected identifier '%s'", tok->str().c_str());
  }
  return type;
}


Identifier* Parser::ParseDirectDeclarator(QualType type,
                                          int storage_spec,
                                          int func_spec,
                                          int align) {
  auto tok_type_pair = ParseDeclarator(type);
  auto tok = tok_type_pair.first;
  type = tok_type_pair.second;
  if (tok == nullptr) {
    Error(errTok_, "expect identifier or '('");
  }

  return ProcessDeclarator(tok, type, storage_spec, func_spec, align);
}


Declaration* Parser::ParseInitDeclarator(Identifier* ident) {
  auto obj = ident->ToObject();
  if (!obj) { // Do not record function Declaration
    return nullptr;
  }

  const auto& name = obj->Name();
  if (ts_.Try('=')) {
    return ParseInitDeclaratorSub(obj);
  }
  
  if (!obj->type()->complete()) {
    if (obj->linkage() == Linkage::NONE) {
      Error(obj, "storage size of '%s' isn’t known", name.c_str());
    }
    return nullptr; // Discards the incomplete object declarations
  }

  if (!obj->decl()) {
    auto decl = Declaration::New(obj);
    obj->set_decl(decl);
    return decl;
  }

  return nullptr;
}


Declaration* Parser::ParseInitDeclaratorSub(Object* obj) {
  const auto& name = obj->Name();
  if ((cur_scope_->type() != ScopeType::FILE) &&
      (obj->linkage() != Linkage::NONE)) {
    Error(obj, "'%s' has both 'extern' and initializer", name.c_str());
  }

  if (!obj->type()->complete() && !obj->type()->ToArray()) {
    Error(obj, "variable '%s' has initializer but incomplete type",
        name.c_str());
  }

  if (obj->HasInit()) {
    Error(obj, "redefinition of variable '%s'", name.c_str());
  }

  // There could be more than one declaration for 
  // an object in the same scope.
  // But it must has external or internal linkage.
  // So, for external/internal objects,
  // the initialization will always go to
  // the first declaration. As the initialization 
  // is evaluated at compile time, 
  // the order doesn't matter.
  // For objects with no linkage, there is
  // always only one declaration.
  // Once again, we need not to worry about 
  // the order of the initialization.
  if (obj->decl()) {
    ParseInitializer(obj->decl(), obj->type(), 0, false, true);
    return nullptr;
  } else {
    auto decl = Declaration::New(obj);
    ParseInitializer(decl, obj->type(), 0, false, true);
    obj->set_decl(decl);
    return decl;
  }
}


void Parser::ParseInitializer(Declaration* decl,
                              QualType type,
                              int offset,
                              bool designated,
                              bool force_brace,
                              uint8_t bitfield_begin,
                              uint8_t bitfield_width) {
  if (designated && !ts_.Test('.') && !ts_.Test('[')) {
    ts_.Expect('=');
  }

  Expr* expr;
  auto arr_type = type->ToArray();
  auto struct_type = type->ToStruct();
  // A compound literal in initializer is reduced to a initializer directly
  // It means that the compound literal will never be created
  auto literal_type = TryCompoundLiteral();
  if (literal_type && !literal_type->Compatible(*type))
      Error("incompatible type of initializer");
  if (arr_type) {
    if (force_brace && !ts_.Test('{') && !ts_.Test(Token::LITERAL)) {
      ts_.Expect('{');
    } else if (!ParseLiteralInitializer(decl, arr_type, offset)) {
      ParseArrayInitializer(decl, arr_type, offset, designated);
      arr_type->set_complete(true);
    }
    return;
  } else if (struct_type) {
    if (!designated && !ts_.Test('{')) {
      auto mark = ts_.Mark();
      expr = ParseAssignExpr();
      if (struct_type->Compatible(*expr->type())) {
        decl->AddInit({struct_type, offset, expr});
        return;
      }
      ts_.ResetTo(mark);
      if (force_brace)
        ts_.Expect('{');
    }
    return ParseStructInitializer(decl, struct_type, offset, designated);
  }

  // Scalar type
  auto has_brace = ts_.Try('{');
  expr = ParseAssignExpr();
  if (has_brace) {
    ts_.Try(',');
    ts_.Expect('}');
  }
  decl->AddInit({type.ptr(), offset, expr, bitfield_begin, bitfield_width});
}


bool Parser::ParseLiteralInitializer(Declaration* decl,
                                     ArrayType* type,
                                     int offset) {
  if (!type->derived()->IsInteger())
    return false;

  auto has_brace = ts_.Try('{');
  if (!ts_.Test(Token::LITERAL)) {
    if (has_brace) ts_.PutBack();
    return false;
  }
  auto literal = ConcatLiterals(ts_.Next());
  auto tok = literal->tok();

  if (has_brace) {
    ts_.Try(',');
    ts_.Expect('}');
  }

  if (!type->complete()) {
    type->set_len(literal->type()->ToArray()->len());
    type->set_complete(true);
  }

  auto width = std::min(type->width(), literal->type()->width());
  auto str = literal->sval()->c_str();

  for (; width >= 8; width -= 8) {
    auto p = reinterpret_cast<const long*>(str);
    auto type = ArithmType::New(T_LONG);
    auto val = ASTConstant::New(tok, T_LONG, static_cast<long>(*p));
    decl->AddInit({type, offset, val});
    offset += 8;
    str += 8;
  }

  for (; width >= 4; width -= 4) {
    auto p = reinterpret_cast<const int*>(str);
    auto type = ArithmType::New(T_INT);
    auto val = ASTConstant::New(tok, T_INT, static_cast<long>(*p));
    decl->AddInit({type, offset, val});
    offset += 4;
    str += 4;
  }

  for (; width >= 2; width -= 2) {
    auto p = reinterpret_cast<const short*>(str);
    auto type = ArithmType::New(T_SHORT);
    auto val = ASTConstant::New(tok, T_SHORT, static_cast<long>(*p));
    decl->AddInit({type, offset, val});
    offset += 2;
    str += 2;
  }

  for (; width >= 1; width--) {
    auto p = str;
    auto type = ArithmType::New(T_CHAR);
    auto val = ASTConstant::New(tok, T_CHAR, static_cast<long>(*p));
    decl->AddInit({type, offset, val});
    offset++;
    str++;
  }

  return true;
}


void Parser::ParseArrayInitializer(Declaration* decl,
                                   ArrayType* type,
                                   int offset,
                                   bool designated) {
  assert(type);

  if (!type->complete())
    type->set_len(0);

  int idx = 0;
  auto width = type->derived()->width();
  auto has_brace = ts_.Try('{');
  while (true) {
    if (ts_.Test('}')) {
      if (has_brace)
        ts_.Next();
      return;
    }

    if (!designated && !has_brace && (ts_.Test('.') || ts_.Test('['))) {
      ts_.PutBack(); // Put the read comma(',') back
      return;
    } else if ((designated = ts_.Try('['))) {
      auto expr = ParseAssignExpr();
      EnsureInteger(expr);
      idx = Evaluator<long>().Eval(expr);
      ts_.Expect(']');

      if (idx < 0 || (type->complete() && idx >= type->len())) {
        Error(ts_.Peek(), "excess elements in array initializer");
      }
    }

    ParseInitializer(decl, type->derived(), offset + idx * width, designated);
    designated = false;
    ++idx;

    if (type->complete() && idx >= type->len()) {
      break;
    } else if (!type->complete()) {
      type->set_len(std::max(idx, type->len()));
    }

    // Needless comma at the end is legal
    if (!ts_.Try(',')) {
      if (has_brace)
        ts_.Expect('}');
      return;
    }
  }

  if (has_brace) {
    ts_.Try(',');
    if (!ts_.Try('}')) {
      Error(ts_.Peek(), "excess elements in array initializer");
    }
  }
}


StructType::Iterator Parser::ParseStructDesignator(StructType* type,
                                                   const std::string& name) {
  auto iter = type->member_list().begin();
  for (; iter != type->member_list().end(); ++iter) {
    if ((*iter)->anonymous()) {
      auto anony_type = (*iter)->type()->ToStruct();
      assert(anony_type);
      if (anony_type->GetMember(name)) {
        return iter; //ParseStructDesignator(anony_type);
      }
    } else if ((*iter)->Name() == name) {
      return iter;
    }
  }
  assert(false);
  return iter;
}


void Parser::ParseStructInitializer(Declaration* decl,
                                    StructType* type,
                                    int offset,
                                    bool designated) {
  assert(type);

  auto has_brace = ts_.Try('{');
  auto member = type->member_list().begin();
  while (true) {
    if (ts_.Test('}')) {
      if (has_brace)
        ts_.Next();
      return;
    }

    if (!designated && !has_brace && (ts_.Test('.') || ts_.Test('['))) {
      ts_.PutBack(); // Put the read comma(',') back
      return;
    }
    
    if ((designated = ts_.Try('.'))) {
      auto tok = ts_.Expect(Token::IDENTIFIER);
      const auto& name = tok->str();
      if (!type->GetMember(name)) {
        Error(tok, "member '%s' not found", name.c_str());
      }
      member = ParseStructDesignator(type, name);
    }
    if (member == type->member_list().end())
      break;

    if ((*member)->anonymous()) {
      if (designated) { // Put back '.' and member name. 
        ts_.PutBack();
        ts_.PutBack();
      }
      // Because offsets of member of anonymous struct/union are based
      // directly on external struct/union
      ParseInitializer(decl, (*member)->type(), offset, designated, false,
          (*member)->bitfield_begin(), (*member)->bitfield_width());
    } else {
      ParseInitializer(decl, (*member)->type(),
          offset + (*member)->offset(), designated, false,
          (*member)->bitfield_begin(), (*member)->bitfield_width());
    }
    designated = false;
    ++member;

    // Union, just init the first member
    if (!type->IsStruct())
      break;

    if (!has_brace && member == type->member_list().end())
      break;

    // Needless comma at the end is allowed
    if (!ts_.Try(',')) {
      if (has_brace)
        ts_.Expect('}');
      return;
    }
  }

  if (has_brace) {
    ts_.Try(',');
    if (!ts_.Try('}')) {
      Error(ts_.Peek(), "excess members in struct initializer");
    }
  }
}


/*
 * Statements
 */

Stmt* Parser::ParseStmt() {
  auto tok = ts_.Next();
  if (tok->IsEOF())
    Error(tok, "premature end of input");

  switch (tok->tag()) {
  // GNU extension: statement attributes
  case Token::ATTRIBUTE: TryAttributeSpecList();
  case ';':             return EmptyStmt::New();
  case '{':             return ParseCompoundStmt();
  case Token::IF:       return ParseIfStmt();
  case Token::SWITCH:   return ParseSwitchStmt();
  case Token::WHILE:    return ParseWhileStmt();
  case Token::DO:       return ParseDoWhileStmt();
  case Token::FOR:      return ParseForStmt();
  case Token::GOTO:     return ParseGotoStmt();
  case Token::CONTINUE: return ParseContinueStmt();
  case Token::BREAK:    return ParseBreakStmt();
  case Token::RETURN:   return ParseReturnStmt();
  case Token::CASE:     return ParseCaseStmt();
  case Token::DEFAULT:  return ParseDefaultStmt();
  }

  if (tok->IsIdentifier() && ts_.Try(':')) {
    // GNU extension: label attributes
    TryAttributeSpecList();
    return ParseLabelStmt(tok);
  }
  
  ts_.PutBack();
  auto expr = ParseExpr();
  ts_.Expect(';');

  return expr;
}


CompoundStmt* Parser::ParseCompoundStmt(FuncType* func_type) {  
  EnterBlock();

  if (func_type) {
    // Merge elements in param scope into current block scope
    for (auto param: func_type->param_list())
      cur_scope_->Insert(param);
  }

  StmtList stmts;
  while (!ts_.Try('}')) {
    if (ts_.Peek()->IsEOF()) {
      Error(ts_.Peek(), "premature end of input");
    }

    if (IsType(ts_.Peek())) {
      stmts.push_back(ParseDecl());
    } else {
      stmts.push_back(ParseStmt());
    }
  }

  auto scope = cur_scope_;
  ExitBlock();
  return CompoundStmt::New(stmts, scope);
}


IfStmt* Parser::ParseIfStmt() {
  ts_.Expect('(');
  auto tok = ts_.Peek();
  auto cond = ParseExpr();
  if (!cond->type()->IsScalar()) {
    Error(tok, "expect scalar");
  }
  ts_.Expect(')');

  auto then = ParseStmt();
  Stmt* els = nullptr;
  if (ts_.Try(Token::ELSE))
    els = ParseStmt();
  
  return IfStmt::New(cond, then, els);
}


ForStmt* Parser::ParseForStmt() {
  EnterLoop();
 
  CompoundStmt* decl = nullptr;
  Expr*         init = nullptr;
  Expr*         cond = nullptr;
  Expr*         step = nullptr;
  Stmt*         body = nullptr;

  ts_.Expect('(');
  if (IsType(ts_.Peek())) {
    decl = ParseDecl();
  } else if (!ts_.Try(';')) {
    init = ParseExpr();
    ts_.Expect(';');
  }

  if (!ts_.Try(';')) {
    cond = ParseExpr();
    ts_.Expect(';');
  }

  if (!ts_.Try(')')) {
    step = ParseExpr();
    ts_.Expect(')');
  }

  body = ParseStmt();

  ExitLoop();
  return ForStmt::New(decl, init, cond, step, body);
}



WhileStmt* Parser::ParseWhileStmt() {
  EnterLoop();
  
  Expr* cond;
  Stmt* body;

  ts_.Expect('(');
  auto tok = ts_.Peek();
  cond = ParseExpr();
  ts_.Expect(')');

  if (!cond->type()->IsScalar()) {
    Error(tok, "scalar expression expected");
  }

  body = ParseStmt();
  ExitLoop();
  return WhileStmt::New(cond, body, false);
}


WhileStmt* Parser::ParseDoWhileStmt() {
  EnterLoop();

  Expr* cond;
  Stmt* body;

  body = ParseStmt();
  ts_.Expect(Token::WHILE);
  ts_.Expect('(');
  cond = ParseExpr();
  ts_.Expect(')');
  ts_.Expect(';');

  ExitLoop();
  return WhileStmt::New(cond, body, true);
}


/*
 * switch:
 *   SWITCH '(' expression ')' statement
 */
SwitchStmt* Parser::ParseSwitchStmt() {
  Expr* select;
  Stmt* body;
  
  ts_.Expect('(');
  auto tok = ts_.Peek();
  select = ParseExpr();
  ts_.Expect(')');

  if (!select->type()->IsInteger()) {
    Error(tok, "switch quantity not an integer");
  }

  auto s = SwitchStmt::New(select, nullptr);
  PushSwitch(s);
  body = ParseStmt();
  PopSwitch();
  s->set_body(body);
  return s;
}


CaseStmt* Parser::ParseCaseStmt() {
  auto tok = ts_.Peek();

  // case ranges: Non-standard GNU extension
  long begin, end;
  begin = end = Evaluator<long>().Eval(ParseAssignExpr());
  if (ts_.Try(Token::ELLIPSIS))
    end = Evaluator<long>().Eval(ParseAssignExpr());

  ts_.Expect(':');

  auto switch_stmt = CurSwitch();
  if (switch_stmt == nullptr) {
    Error(tok, "case is allowed only in switch");
  }
  if (begin < INT_MIN || end > INT_MAX) {
    Error(tok, "case range overflow");
  }
  auto c = CaseStmt::New(begin, end, ParseStmt());
  if (switch_stmt->IsCaseOverlapped(c)) {
    Error(tok, "case range overlapped");
  }

  switch_stmt->AddCase(c);
  return c;
}


DefaultStmt* Parser::ParseDefaultStmt() {
  auto tok = ts_.Peek();
  ts_.Expect(':');
  
  auto switch_stmt = CurSwitch();
  if (switch_stmt == nullptr) {
    Error(tok, "default allowed only in switch");
  }
  if (switch_stmt->deft() != nullptr) {
    Error(tok, "multiple default labels in one switch");
  }

  auto default_stmt = DefaultStmt::New(ParseStmt());
  switch_stmt->set_deft(default_stmt);
  return default_stmt;
}


ContinueStmt* Parser::ParseContinueStmt() {
  auto tok = ts_.Peek();
  ts_.Expect(';');
  if (!InLoop()) {
    Error(tok, "'continue' is allowed only in loop");
  }
  return ContinueStmt::New();
}


BreakStmt* Parser::ParseBreakStmt() {
  auto tok = ts_.Peek();
  ts_.Expect(';');
  if (!InSwitchOrLoop()) {
    Error(tok, "'break' is allowed only in switch/loop");
  }
  return BreakStmt::New();
}


ReturnStmt* Parser::ParseReturnStmt() {
  Expr* expr = nullptr;
  if (!ts_.Try(';')) {
    expr = ParseExpr();
    ts_.Expect(';');
    auto ret_type = cur_func_->FuncType()->derived();
    expr = Expr::MayCast(expr, ret_type);
  }

  return ReturnStmt::New(expr);
}


GotoStmt* Parser::ParseGotoStmt() {
  auto label = ts_.Peek();
  ts_.Expect(Token::IDENTIFIER);
  ts_.Expect(';');

  auto label_stmt = FindLabel(label->str());
  if (label_stmt) {
    return GotoStmt::New(label_stmt);
  }

  auto unresolved_goto = GotoStmt::New(nullptr);
  AddUnresolvedGoto(label, unresolved_goto);
  return unresolved_goto;
}


LabelStmt* Parser::ParseLabelStmt(const Token* label) {
  const auto& label_str = label->str();
  if (FindLabel(label_str) != nullptr) {
    Error(label, "redefinition of label '%s'", label_str.c_str());
  }

  auto label_stmt = LabelStmt::New(ParseStmt());
  AddLabel(label_str, label_stmt);
  return label_stmt;
}


bool Parser::IsBuiltin(const std::string& name) {
  return name == "__builtin_va_arg" ||
         name == "__builtin_va_start";
}


bool Parser::IsBuiltin(FuncType* type) {
  assert(va_start_type_ && va_arg_type_);
  return type == va_start_type_ || type == va_arg_type_;
}


// Builtin functions will be inlined
void Parser::DefineBuiltins() {
  // FIXME: potential bug: using same object for param_list!!!
  auto void_ptr = PointerType::New(VoidType::New());
  auto param = Object::New(nullptr, void_ptr);
  FuncType::ParamList pl;
  pl.push_back(param);
  pl.push_back(param);
  va_start_type_ = FuncType::New(VoidType::New(), F_INLINE, false, pl);
  va_arg_type_ = FuncType::New(void_ptr, F_INLINE, false, pl);
}


Identifier* Parser::GetBuiltin(const Token* tok) {
  assert(va_start_type_ && va_arg_type_);
  static Identifier* vastart = nullptr;
  static Identifier* vaarg = nullptr;
  const auto& name = tok->str();
  if (name == "__builtin_va_start") {
    if (!vastart)
      vastart = Identifier::New(tok, va_start_type_, Linkage::EXTERNAL);
    return vastart;
  } else if (name == "__builtin_va_arg") {
    if (!vaarg)
      vaarg = Identifier::New(tok, va_arg_type_, Linkage::EXTERNAL);
    return vaarg;
  }
  assert(false);
  return nullptr;
}


/*
 * GNU extensions
 */

// Attribute
void Parser::TryAttributeSpecList() {
  while (ts_.Try(Token::ATTRIBUTE))
    ParseAttributeSpec();
}


void Parser::ParseAttributeSpec() {
  ts_.Expect('(');
  ts_.Expect('(');

  while (!ts_.Try(')')) {
    ParseAttribute();
    if (!ts_.Try(',')) {
      ts_.Expect(')');
      break;
    }
  }
  ts_.Expect(')');
}


void Parser::ParseAttribute() {
  if (!ts_.Test(Token::IDENTIFIER))
    return;
  if (ts_.Try('(')) {
    if (ts_.Try(')'))
      return;
    
    ts_.Expect(Token::IDENTIFIER);
    if (ts_.Test(',')) {
      while (ts_.Try(',')) {}
    }
    ts_.Try(')');
  }
}
