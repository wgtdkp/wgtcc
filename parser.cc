#include "parser.h"

#include "cpp.h"
#include "encoding.h"
#include "error.h"
#include "scope.h"
#include "type.h"
#include "evaluator.h"

#include <iostream>
#include <set>
#include <string>

#include <climits>


using namespace std;


FuncDef* Parser::EnterFunc(Identifier* ident) {
  //TODO(wgtdkp): Add __func__ macro

  curParamScope_->SetParent(curScope_);
  curScope_ = curParamScope_;

  curFunc_ = FuncDef::New(ident, LabelStmt::New());
  return curFunc_;
}


void Parser::ExitFunc() {
  // Resolve 那些待定的jump；
  // 如果有jump无法resolve，也就是有未定义的label，报错；
  for (auto iter = unresolvedJumps_.begin();
      iter != unresolvedJumps_.end(); iter++) {
    auto label = iter->first;
    auto labelStmt = FindLabel(label->str_);
    if (labelStmt == nullptr) {
      Error(label, "label '%s' used but not defined",
          label->str_.c_str());
    }
    
    iter->second->SetLabel(labelStmt);
  }
  
  unresolvedJumps_.clear();	//清空未定的 jump 动作
  curLabels_.clear();	//清空 label map

  curScope_ = curScope_->Parent();
  curFunc_ = nullptr;
}


void Parser::EnterBlock(FuncType* funcType)
{
  if (funcType) {
    auto paramScope = curScope_;
    curScope_ = new Scope(curScope_->Parent(), S_BLOCK);
    
    // Merge elements in param scope into current block scope
    auto iter = paramScope->begin();
    for (; iter != paramScope->end(); iter++) {
      curScope_->Insert(iter->second);
    }
  } else {
    curScope_ = new Scope(curScope_, S_BLOCK);
  }
}


void Parser::Parse()
{
  ParseTranslationUnit();
}


void Parser::ParseTranslationUnit()
{
  while (!ts_.Peek()->IsEOF()) {            
    curParamScope_ = nullptr;

    int storageSpec, funcSpec;
    auto type = ParseDeclSpec(&storageSpec, &funcSpec);
    auto tokTypePair = ParseDeclarator(type);
    auto tok = tokTypePair.first;
    type = tokTypePair.second;

    if (tok == nullptr) {
      ts_.Expect(';');
      continue;
    }

    auto ident = ProcessDeclarator(tok, type, storageSpec, funcSpec);
    type = ident->Type();

    if (tok && type->ToFunc() && ts_.Try('{')) { // Function definition
      unit_->Add(ParseFuncDef(ident));
    } else { // Declaration
      auto decl = ParseInitDeclarator(ident);
      if (decl) unit_->Add(decl);

      while (ts_.Try(',')) {
        auto ident = ParseDirectDeclarator(type, storageSpec, funcSpec);
        decl = ParseInitDeclarator(ident);
        if (decl) unit_->Add(decl);
      }
      ts_.Expect(';');
    }
  }
  
  //externalSymbols_->Print();
}


FuncDef* Parser::ParseFuncDef(Identifier* ident)
{
  auto funcDef = EnterFunc(ident);

  if (ident->Type()->Complete()) {
    Error(ident, "redefinition of '%s'", ident->Name().c_str());
  }

  auto funcType = ident->Type()->ToFunc();
  FuncType::TypeList& paramTypes = funcType->ParamTypes();
  if (curScope_->size() != paramTypes.size()) {
    Error(ident, "parameter name omitted");
  }

  for (auto paramType: paramTypes) {
    auto iter = curScope_->begin();
    for (; iter != curScope_->end(); ++iter) {
      if (iter->second->Type() == paramType) {
        funcDef->Params().push_back(iter->second->ToObject());
        break;
      }
    }
  }
  if (funcDef->Params().size() != paramTypes.size()) {
    assert(funcDef->Params().size() == paramTypes.size());
  }

  funcType->SetComplete(true);
  funcDef->SetBody(ParseCompoundStmt(funcType));
  ExitFunc();
  
  return funcDef;
}


Expr* Parser::ParseExpr()
{
  return ParseCommaExpr();
}


Expr* Parser::ParseCommaExpr()
{
  auto lhs = ParseAssignExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(',')) {
    auto rhs = ParseAssignExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  return lhs;
}


Expr* Parser::ParsePrimaryExpr()
{
  if (ts_.Empty()) {
    Error(ts_.Peek(), "premature end of input");
  } else if (ts_.Peek()->IsKeyWord()) {
    Error(ts_.Peek(), "unexpected keyword");
  }

  auto tok = ts_.Next();
  if (tok->tag_ == '(') {
    auto expr = ParseExpr();
    ts_.Expect(')');
    return expr;
  }

  if (tok->IsIdentifier()) {
    auto ident = curScope_->Find(tok);
    /* if (ident == nullptr || ident->ToObject() == nullptr) { */
    if (ident == nullptr) {
      Error(tok, "undefined symbol '%s'", tok->str_.c_str());
    }
    return ident;
  } else if (tok->IsConstant()) {
    return ParseConstant(tok);
  } else if (tok->IsLiteral()) {
    return ConcatLiterals(tok);
  } else if (tok->tag_ == Token::GENERIC) {
    return ParseGeneric();
  }

  Error(tok, "'%s' unexpected", tok->str_.c_str());
  return nullptr; // Make compiler happy
}


static void ConvertLiteral(std::string& val, Encoding enc)
{
  switch (enc) {
  case Encoding::NONE:
  case Encoding::UTF8: break;
  case Encoding::CHAR16: ConvertToUTF16(val); break;
  case Encoding::CHAR32:
  case Encoding::WCHAR: ConvertToUTF32(val); break;
  }
}


Constant* Parser::ConcatLiterals(const Token* tok)
{
  auto val = new std::string;
  auto enc = Scanner(tok).ScanLiteral(*val);
  ConvertLiteral(*val, enc);	
  while (ts_.Test(Token::LITERAL)) {
    auto nextTok = ts_.Next();
    std::string nextVal;
    auto nextEnc = Scanner(nextTok).ScanLiteral(nextVal);
    ConvertLiteral(nextVal, nextEnc);
    if (enc == Encoding::NONE) {
      ConvertLiteral(*val, nextEnc);
      enc = nextEnc;
    }
    if (nextEnc != Encoding::NONE && nextEnc != enc)
      Error(nextTok, "can't concat lietrals with different encodings");
    *val += nextVal;
  }

  int tag;
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

  return Constant::New(tok, tag, val);
}


Encoding Parser::ParseLiteral(std::string& str, const Token* tok)
{
  return Scanner(tok).ScanLiteral(str);
}


// TODO(wgtdkp):
Constant* Parser::ParseConstant(const Token* tok)
{
  assert(tok->IsConstant());

  if (tok->tag_ == Token::I_CONSTANT) {
    return ParseInteger(tok);
  } else if (tok->tag_ == Token::C_CONSTANT) {
    return ParseCharacter(tok);
  } else {
    return ParseFloat(tok);
  }
}


Constant* Parser::ParseFloat(const Token* tok)
{
  const auto& str = tok->str_;
  size_t end = 0;
  double val;
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

  return Constant::New(tok, tag, val);
}


Constant* Parser::ParseCharacter(const Token* tok)
{
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
  return Constant::New(tok, tag, static_cast<long>(val));
}


Constant* Parser::ParseInteger(const Token* tok)
{
  const auto& str = tok->str_;
  size_t end;
  long val;
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
      if (str[end + 1] == 'l' || str[end + 1] =='L')
        ++end;
      if (tag & T_LONG)
        Error(tok, "invalid suffix");
      tag |= T_LONG;
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

  return Constant::New(tok, tag, val);
}


// TODO(wgtdkp):
Expr* Parser::ParseGeneric()
{
  assert(0);
  return nullptr;
}


Expr* Parser::ParsePostfixExpr()
{
  auto tok = ts_.Next();
  if (tok->IsEOF()) {
    //return nullptr;
    Error(tok, "premature end of input");
  }

  if ('(' == tok->tag_ && IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    ts_.Expect(')');
    return ParseCompoundLiteral(type);
  }

  ts_.PutBack();
  auto primExpr = ParsePrimaryExpr();
  
  return ParsePostfixExprTail(primExpr);
}


static std::string AnonymousName(Object* anony) {
  return "anonymous<"
       + std::to_string(reinterpret_cast<unsigned long long>(anony))
       + ">";
}


Object* Parser::ParseCompoundLiteral(Type* type)
{
  auto linkage = curScope_->Type() == S_FILE ? L_INTERNAL: L_NONE;
  auto anony = Object::New(ts_.Peek(), type, 0, linkage);
  anony->SetAnonymous(true);
  curScope_->Insert(AnonymousName(anony), anony);
  ParseInitDeclaratorSub(anony);
  return anony;
}


//return the constructed postfix expression
Expr* Parser::ParsePostfixExprTail(Expr* lhs)
{
  while (true) {
    auto tok = ts_.Next();
    
    switch (tok->tag_) {
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


Expr* Parser::ParseSubScripting(Expr* lhs)
{
  auto rhs = ParseExpr();
  
  auto tok = ts_.Peek();
  ts_.Expect(']');
  //
  auto operand = BinaryOp::New(tok, '+', lhs, rhs);
  return UnaryOp::New(Token::DEREF, operand);
}


BinaryOp* Parser::ParseMemberRef(const Token* tok, int op, Expr* lhs)
{
  auto memberName = ts_.Peek()->str_;
  ts_.Expect(Token::IDENTIFIER);

  auto structUnionType = lhs->Type()->ToStruct();
  if (structUnionType == nullptr) {
    Error(tok, "an struct/union expected");
  }

  auto rhs = structUnionType->GetMember(memberName);
  if (rhs == nullptr) {
    Error(tok, "'%s' is not a member of '%s'",
        memberName.c_str(), "[obj]");
  }

  return  BinaryOp::New(tok, op, lhs, rhs);
}


UnaryOp* Parser::ParsePostfixIncDec(const Token* tok, Expr* operand)
{
  auto op = tok->tag_ == Token::INC ?
      Token::POSTFIX_INC: Token::POSTFIX_DEC;

  return UnaryOp::New(op, operand);
}


FuncCall* Parser::ParseFuncCall(Expr* designator)
{
  FuncCall::ArgList args;
  while (!ts_.Try(')')) {
    args.push_back(Expr::MayCast(ParseAssignExpr()));
    if (!ts_.Test(')'))
      ts_.Expect(',');
  }

  return FuncCall::New(designator, args);
}


Expr* Parser::ParseUnaryExpr()
{
  auto tok = ts_.Next();
  switch (tok->tag_) {
  case Token::ALIGNOF:
    return ParseAlignof();
  case Token::SIZEOF:
    return ParseSizeof();
  case Token::INC:
    return ParsePrefixIncDec(tok);
  case Token::DEC:
    return ParsePrefixIncDec(tok);
  case '&':
    return ParseUnaryOp(tok, Token::ADDR);
  case '*':
    return ParseUnaryOp(tok, Token::DEREF); 
  case '+':
    return ParseUnaryOp(tok, Token::PLUS);
  case '-':
    return ParseUnaryOp(tok, Token::MINUS); 
  case '~':
    return ParseUnaryOp(tok, '~');
  case '!':
    return ParseUnaryOp(tok, '!');
  default:
    ts_.PutBack();
    return ParsePostfixExpr();
  }
}


Constant* Parser::ParseSizeof()
{
  Type* type;
  auto tok = ts_.Next();
  Expr* unaryExpr = nullptr;

  if (tok->tag_ == '(' && IsTypeName(ts_.Peek())) {
    type = ParseTypeName();
    ts_.Expect(')');
  } else {
    ts_.PutBack();
    unaryExpr = ParseUnaryExpr();
    type = unaryExpr->Type();
  }

  if (type->ToFunc() || type->ToVoid()) {
  } else if (!type->Complete()) {
    Error(tok, "sizeof(incomplete type)");
  }

  return Constant::New(tok, T_UNSIGNED | T_LONG, (long)type->Width());
}


Constant* Parser::ParseAlignof()
{
  auto tok = ts_.Expect('(');
  auto type = ParseTypeName();
  ts_.Expect(')');

  return Constant::New(tok, T_UNSIGNED | T_LONG, (long)type->Align());
}


UnaryOp* Parser::ParsePrefixIncDec(const Token* tok)
{
  assert(tok->tag_ == Token::INC || tok->tag_ == Token::DEC);
  
  auto op = tok->tag_ == Token::INC ?
      Token::PREFIX_INC: Token::PREFIX_DEC;
  auto operand = ParseUnaryExpr();
  
  return UnaryOp::New(op, operand);
}


UnaryOp* Parser::ParseUnaryOp(const Token* tok, int op)
{
  auto operand = ParseCastExpr();

  return UnaryOp::New(op, operand);
}


Type* Parser::ParseTypeName()
{
  auto type = ParseSpecQual();
  if (ts_.Test('*') || ts_.Test('(')) //abstract-declarator FIRST set
    return ParseAbstractDeclarator(type);
  
  return type;
}


Expr* Parser::ParseCastExpr()
{
  auto tok = ts_.Next();
  if (tok->tag_ == '(' && IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    ts_.Expect(')');
    if (ts_.Test('{')) {
      return ParseCompoundLiteral(type);
    }
    auto operand = ParseCastExpr();
    return UnaryOp::New(Token::CAST, operand, type);
  }
  
  ts_.PutBack();
  return ParseUnaryExpr();
}


Expr* Parser::ParseMultiplicativeExpr()
{
  auto lhs = ParseCastExpr();
  auto tok = ts_.Next();
  while (tok->tag_ == '*' || tok->tag_ == '/' || tok->tag_ == '%') {
    auto rhs = ParseCastExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseAdditiveExpr()
{
  auto lhs = ParseMultiplicativeExpr();
  auto tok = ts_.Next();
  while (tok->tag_ == '+' || tok->tag_ == '-') {
    auto rhs = ParseMultiplicativeExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseShiftExpr()
{
  auto lhs = ParseAdditiveExpr();
  auto tok = ts_.Next();
  while (tok->tag_ == Token::LEFT || tok->tag_ == Token::RIGHT) {
    auto rhs = ParseAdditiveExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseRelationalExpr()
{
  auto lhs = ParseShiftExpr();
  auto tok = ts_.Next();
  while (tok->tag_ == Token::LE || tok->tag_ == Token::GE 
      || tok->tag_ == '<' || tok->tag_ == '>') {
    auto rhs = ParseShiftExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseEqualityExpr()
{
  auto lhs = ParseRelationalExpr();
  auto tok = ts_.Next();
  while (tok->tag_ == Token::EQ || tok->tag_ == Token::NE) {
    auto rhs = ParseRelationalExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Next();
  }
  
  ts_.PutBack();
  return lhs;
}


Expr* Parser::ParseBitiwiseAndExpr()
{
  auto lhs = ParseEqualityExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('&')) {
    auto rhs = ParseEqualityExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseBitwiseXorExpr()
{
  auto lhs = ParseBitiwiseAndExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('^')) {
    auto rhs = ParseBitiwiseAndExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseBitwiseOrExpr()
{
  auto lhs = ParseBitwiseXorExpr();
  auto tok = ts_.Peek();
  while (ts_.Try('|')) {
    auto rhs = ParseBitwiseXorExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseLogicalAndExpr()
{
  auto lhs = ParseBitwiseOrExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(Token::LOGICAL_AND)) {
    auto rhs = ParseBitwiseOrExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseLogicalOrExpr()
{
  auto lhs = ParseLogicalAndExpr();
  auto tok = ts_.Peek();
  while (ts_.Try(Token::LOGICAL_OR)) {
    auto rhs = ParseLogicalAndExpr();
    lhs = BinaryOp::New(tok, lhs, rhs);

    tok = ts_.Peek();
  }
  
  return lhs;
}


Expr* Parser::ParseConditionalExpr()
{
  auto cond = ParseLogicalOrExpr();
  auto tok = ts_.Peek();
  if (ts_.Try('?')) {
    auto exprTrue = ParseExpr();
    ts_.Expect(':');
    auto exprFalse = ParseConditionalExpr();

    return ConditionalOp::New(tok, cond, exprTrue, exprFalse);
  }
  
  return cond;
}


Expr* Parser::ParseAssignExpr()
{
  // Yes, I know the lhs should be unary expression, 
  // let it handled by type checking
  Expr* lhs = ParseConditionalExpr();
  Expr* rhs;

  auto tok = ts_.Next();
  switch (tok->tag_) {
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


/**************** Declarations ********************/

// Return: list of declarations
CompoundStmt* Parser::ParseDecl()
{
  StmtList stmts;

  if (ts_.Try(Token::STATIC_ASSERT)) {
    //TODO: static_assert();
    assert(false);
  } else {
    int storageSpec, funcSpec;
    auto type = ParseDeclSpec(&storageSpec, &funcSpec);
    
    //init-declarator 的 FIRST 集合：'*', identifier, '('
    if (ts_.Test('*') || ts_.Test(Token::IDENTIFIER) || ts_.Test('(')) {
      do {
        auto ident = ParseDirectDeclarator(type, storageSpec, funcSpec);
        auto init = ParseInitDeclarator(ident);
        if (init) {
          stmts.push_back(init);
        }
      } while (ts_.Try(','));
    }            
    ts_.Expect(';');
  }

  return CompoundStmt::New(stmts);
}


//for state machine
enum {
  //compatibility for these key words
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


static inline void TypeLL(int& typeSpec)
{
  if (typeSpec & T_LONG) {
    typeSpec &= ~T_LONG;
    typeSpec |= T_LLONG;
  } else {
    typeSpec |= T_LONG;
  }
}


Type* Parser::ParseSpecQual()
{
  return ParseDeclSpec(nullptr, nullptr);
}


/*
param: storage: null, only type specifier and qualifier accepted;
*/
Type* Parser::ParseDeclSpec(int* storage, int* func)
{
  Type* type = nullptr;
  int align = -1;
  int storageSpec = 0;
  int funcSpec = 0;
  int qualSpec = 0;
  int typeSpec = 0;
  
  const Token* tok;
  for (; ;) {
    tok = ts_.Next();
    switch (tok->tag_) {
    //function specifier
    case Token::INLINE:
      funcSpec |= F_INLINE;
      break;

    case Token::NORETURN:
      funcSpec |= F_NORETURN;
      break;

    //alignment specifier
    case Token::ALIGNAS:
      align = ParseAlignas();
      break;

    //storage specifier
    //TODO: typedef needs more constraints
    case Token::TYPEDEF:
      if (storageSpec != 0)
        goto error;
      storageSpec |= S_TYPEDEF;
      break;

    case Token::EXTERN:
      if (storageSpec & ~S_THREAD)
        goto error;
      storageSpec |= S_EXTERN;
      break;

    case Token::STATIC:
      if (storageSpec & ~S_THREAD)
        goto error;
      storageSpec |= S_STATIC;
      break;

    case Token::THREAD:
      if (storageSpec & ~COMP_THREAD)
        goto error;
      storageSpec |= S_THREAD;
      break;

    case Token::AUTO:
      if (storageSpec != 0)
        goto error;
      storageSpec |= S_AUTO;
      break;

    case Token::REGISTER:
      if (storageSpec != 0)
        goto error; 
      storageSpec |= S_REGISTER;
      break;
    
    //type qualifier
    case Token::CONST:
      qualSpec |= Q_CONST;
      break;

    case Token::RESTRICT:
      qualSpec |= Q_RESTRICT;
      break;

    case Token::VOLATILE:
      qualSpec |= Q_VOLATILE;
      break;
    /*
    atomic_qual:
      qualSpec |= Q_ATOMIC;
      break;
    */

    //type specifier
    case Token::SIGNED:
      if (typeSpec & ~COMP_SIGNED)
        goto error; 
      typeSpec |= T_SIGNED;
      break;

    case Token::UNSIGNED:
      if (typeSpec & ~COMP_UNSIGNED)
        goto error;
      typeSpec |= T_UNSIGNED;
      break;

    case Token::VOID:
      if (0 != typeSpec)
        goto error;
      typeSpec |= T_VOID;
      break;

    case Token::CHAR:
      if (typeSpec & ~COMP_CHAR)
        goto error;
      typeSpec |= T_CHAR;
      break;

    case Token::SHORT:
      if (typeSpec & ~COMP_SHORT)
        goto error;
      typeSpec |= T_SHORT;
      break;

    case Token::INT:
      if (typeSpec & ~COMP_INT)
        goto error;
      typeSpec |= T_INT;
      break;

    case Token::LONG:
      if (typeSpec & ~COMP_LONG)
        goto error; 
      TypeLL(typeSpec); 
      break;
      
    case Token::FLOAT:
      if (typeSpec & ~T_COMPLEX)
        goto error;
      typeSpec |= T_FLOAT;
      break;

    case Token::DOUBLE:
      if (typeSpec & ~COMP_DOUBLE)
        goto error;
      typeSpec |= T_DOUBLE;
      break;

    case Token::BOOL:
      if (typeSpec != 0)
        goto error;
      typeSpec |= T_BOOL;
      break;

    case Token::COMPLEX:
      if (typeSpec & ~COMP_COMPLEX)
        goto error;
      typeSpec |= T_COMPLEX;
      break;

    case Token::STRUCT: 
    case Token::UNION:
      if (typeSpec)
        goto error; 
      type = ParseStructUnionSpec(Token::STRUCT == tok->tag_); 
      typeSpec |= T_STRUCT_UNION;
      break;

    case Token::ENUM:
      if (typeSpec != 0)
        goto error;
      type = ParseEnumSpec();
      typeSpec |= T_ENUM;
      break;

    case Token::ATOMIC:\
      assert(false);
      /*
      if (ts_.Peek()->tag_ != '(')
        goto atomic_qual;
      if (typeSpec != 0)
        goto error;
      type = ParseAtomicSpec();
      typeSpec |= T_ATOMIC;
      break;
      */
    default:
      if (typeSpec == 0 && IsTypeName(tok)) {
        auto ident = curScope_->Find(tok);
        if (ident) {
          ident = ident->ToTypeName();
          type = ident ? ident->Type(): nullptr;
        }
        typeSpec |= T_TYPEDEF_NAME;
      } else  {
        goto end_of_loop;
      }
    }
  }

end_of_loop:
  ts_.PutBack();
  switch (typeSpec) {
  case 0:
    Error(tok, "expect type specifier");
    break;

  case T_VOID:
    type = VoidType::New();
    break;

  case T_ATOMIC:
  case T_STRUCT_UNION:
  case T_ENUM:
  case T_TYPEDEF_NAME:
    break;

  default:
    type = ArithmType::New(typeSpec);
    break;
  }

  if ((storage == nullptr || func == nullptr)) {
    if (funcSpec && storageSpec && align != -1) {
      Error(tok, "type specifier/qualifier only");
    }
  } else {
    *storage = storageSpec;
    *func = funcSpec;
  }

  type->SetQual(qualSpec);
  return type;

error:
  Error(tok, "type speficier/qualifier/storage error");
  return nullptr;	// Make compiler happy
}


int Parser::ParseAlignas()
{
  int align;
  ts_.Expect('(');
  if (IsTypeName(ts_.Peek())) {
    auto type = ParseTypeName();
    ts_.Expect(')');
    align = type->Align();
  } else {
    auto expr = ParseExpr();
    align = Evaluator<long>().Eval(expr);
    ts_.Expect(')');
    delete expr;
  }

  return align;
}


Type* Parser::ParseEnumSpec()
{
  std::string tagName;
  auto tok = ts_.Peek();
  if (ts_.Try(Token::IDENTIFIER)) {
    tagName = tok->str_;
    if (ts_.Try('{')) {
      //定义enum类型
      auto tagIdent = curScope_->FindTagInCurScope(tok);
      if (tagIdent == nullptr)
        goto enum_decl;

      if (!tagIdent->Type()->Complete()) {
        return ParseEnumerator(tagIdent->Type()->ToArithm());
      } else {
        Error(tok, "redefinition of enumeration tag '%s'",
            tagName.c_str());
      }
    } else {
      //Type* type = curScope_->FindTag(tagName);
      auto tagIdent = curScope_->FindTag(tok);
      if (tagIdent) {
        return tagIdent->Type();
      }
      auto type = ArithmType::New(T_INT);
      type->SetComplete(false);   //尽管我们把 enum 当成 int 看待，但是还是认为他是不完整的
      auto ident = Identifier::New(tok, type, L_NONE);
      curScope_->InsertTag(ident);
      return type;
    }
  }
  
  ts_.Expect('{');

enum_decl:
  auto type = ArithmType::New(T_INT);
  type->SetComplete(false);
  if (tagName.size() != 0) {
    auto ident = Identifier::New(tok, type, L_NONE);
    curScope_->InsertTag(ident);
  }
  
  return ParseEnumerator(type);   //处理反大括号: '}'
}


Type* Parser::ParseEnumerator(ArithmType* type)
{
  assert(type && !type->Complete() && type->IsInteger());
  
  //std::set<int> valSet;
  int val = 0;
  do {
    auto tok = ts_.Expect(Token::IDENTIFIER);
    
    const auto& enumName = tok->str_;
    auto ident = curScope_->FindInCurScope(tok);
    if (ident) {
      Error(tok, "redefinition of enumerator '%s'", enumName.c_str());
    }
    if (ts_.Try('=')) {
      auto expr = ParseAssignExpr();
      val = Evaluator<long>().Eval(expr);

      //if (valSet.find(val) != valSet.end()) {
      //    Error(expr, "conflict enumerator constant '%d'", val);
      //}
    }
    //valSet.insert(val);
    auto enumer = Enumerator::New(tok, val);
    ++val;

    curScope_->Insert(enumer);

    ts_.Try(',');
  } while (!ts_.Try('}'));
  
  type->SetComplete(true);
  
  return type;
}


/*
 * 四种 name space：
 * 1.label, 如 goto end; 它有函数作用域
 * 2.struct/union/enum 的 tag
 * 3.struct/union 的成员
 * 4.其它的普通的变量
 */
Type* Parser::ParseStructUnionSpec(bool isStruct)
{
  std::string tagName;
  auto tok = ts_.Peek();
  if (ts_.Try(Token::IDENTIFIER)) {
    tagName = tok->str_;
    if (ts_.Try('{')) {
      //看见大括号，表明现在将定义该struct/union类型
      auto tagIdent = curScope_->FindTagInCurScope(tok);
      if (tagIdent == nullptr) //我们不用关心上层scope是否定义了此tag，如果定义了，那么就直接覆盖定义
        goto struct_decl; //现在是在当前scope第一次看到name，所以现在是第一次定义，连前向声明都没有；
      
      /*
       * 在当前scope找到了类型，但可能只是声明；注意声明与定义只能出现在同一个scope；
       * 1.如果声明在定义的外层scope,那么即使在内层scope定义了完整的类型，此声明仍然是无效的；
       *   因为如论如何，编译器都不会在内部scope里面去找定义，所以声明的类型仍然是不完整的；
       * 2.如果声明在定义的内层scope,(也就是先定义，再在内部scope声明)，这时，不完整的声明会覆盖掉完整的定义；
       *   因为编译器总是向上查找符号，不管找到的是完整的还是不完整的，都要；
       */
      if (!tagIdent->Type()->Complete()) {
        //找到了此tag的前向声明，并更新其符号表，最后设置为complete type
        return ParseStructUnionDecl(
            tagIdent->Type()->ToStruct());
      } else {
        //在当前作用域找到了完整的定义，并且现在正在定义同名的类型，所以报错；
        Error(tok, "redefinition of struct tag '%s'",
            tagName.c_str());
      }
    } else {
      /*
       * 没有大括号，表明不是定义一个struct/union;那么现在只可能是在：
       * 1.声明；
       * 2.声明的同时，定义指针(指针允许指向不完整类型) (struct Foo* p; 是合法的) 或者其他合法的类型；
       *   如果现在索引符号表，那么：
       *   1.可能找到name的完整定义，也可能只找得到不完整的声明；不管name指示的是不是完整类型，我们都只能选择name指示的类型；
       *   2.如果我们在符号表里面压根找不到name,那么现在是name的第一次声明，创建不完整的类型并插入符号表；
       */
      auto tagIdent = curScope_->FindTag(tok);
      
      //如果tag已经定义或声明，那么直接返回此定义或者声明
      if (tagIdent) {
        return tagIdent->Type();
      }
      
      //如果tag尚没有定义或者声明，那么创建此tag的声明(因为没有见到‘{’，所以不会是定义)
      auto type = StructType::New(isStruct, true, curScope_);
      
      //因为有tag，所以不是匿名的struct/union， 向当前的scope插入此tag
      auto ident = Identifier::New(tok, type, L_NONE);
      curScope_->InsertTag(ident);
      return type;
    }
  }
  //没见到identifier，那就必须有struct/union的定义，这叫做匿名struct/union;
  ts_.Expect('{');

struct_decl:
  //现在，如果是有tag，那它没有前向声明；如果是没有tag，那更加没有前向声明；
  //所以现在是第一次开始定义一个完整的struct/union类型
  auto type = StructType::New(isStruct, tagName.size(), curScope_);
  if (tagName.size() != 0) {
    auto ident = Identifier::New(tok, type, L_NONE);
    curScope_->InsertTag(ident);
  }
  
  return ParseStructUnionDecl(type); //处理反大括号: '}'
}


StructType* Parser::ParseStructUnionDecl(StructType* type)
{
  //既然是定义，那输入肯定是不完整类型，不然就是重定义了
  assert(type && !type->Complete());
  
  auto scopeBackup = curScope_;
  curScope_ = type->MemberMap(); // Internal symbol lookup rely on curScope_

  bool packed = true;
  while (!ts_.Try('}')) {
    if (ts_.Empty()) {
      Error(ts_.Peek(), "premature end of input");
    }
    
    // 解析type specifier/qualifier, 不接受storage等

    auto memberType = ParseSpecQual();
    do {
      auto tokTypePair = ParseDeclarator(memberType);
      auto tok = tokTypePair.first;
      memberType = tokTypePair.second;
      std::string name;

      if (ts_.Try(':')) {
        packed = ParseBitField(type, tok, memberType, packed);
        // TODO(wgtdkp): continue; ?
        goto end_decl;
      }

      if (tok == nullptr) {
        auto suType = memberType->ToStruct();
        if (suType && !suType->HasTag()) {
          // FIXME: setting 'tok' to nullptr is not good
          auto anony = Object::New(ts_.Peek(), suType);
          anony->SetAnonymous(true);
          type->MergeAnony(anony);
          continue;
        } else {
          Error(ts_.Peek(), "declaration does not declare anything");
        }
      }

      name = tok->str_;                
      if (type->GetMember(name)) {
        Error(tok, "duplicate member '%s'", name.c_str());
      }

      if (!memberType->Complete()) {
        Error(tok, "field '%s' has incomplete type", name.c_str());
      }

      if (memberType->ToFunc()) {
        Error(tok, "field '%s' declared as a function", name.c_str());
      }

      type->AddMember(Object::New(tok, memberType));

    end_decl:;

    } while (ts_.Try(','));
    ts_.Expect(';');
  }
  
  //struct/union定义结束，设置其为完整类型
  type->SetComplete(true);
  
  curScope_ = scopeBackup;

  return type;
}


bool Parser::ParseBitField(StructType* structType,
    const Token* tok, Type* type, bool packed)
{
  if (!type->IsInteger()) {
    Error(tok ? tok: ts_.Peek(), "expect integer type for bitfield");
  }

  auto expr = ParseAssignExpr();
  auto width = Evaluator<long>().Eval(expr);
  if (width < 0) {
    Error(expr, "expect non negative value");
  } else if (width == 0 && tok) {
    Error(tok, "no declarator expected for a bitfield with width 0");
  } else if (width == 0) {
    return true;
  } else if (width > type->Width() * 8) {
    Error(expr, "width exceeds its type");
  }

  auto offset = structType->Offset() - type->Width();
  offset = Type::MakeAlign(std::max(offset, 0), type->Align());


  int bitFieldOffset;
  unsigned char begin;

  if (structType->Members().size() == 0) {
    begin = 0;
    bitFieldOffset = 0;
  } else {
    auto last = structType->Members().back();
    auto totalBits = (last->Offset() - offset) * 8;
    if (last->BitFieldWidth()) { // Is not bit field
      totalBits += last->BitFieldEnd();
    } else {
      totalBits += last->Type()->Width() * 8;
    }

    auto bitsOffset = Type::MakeAlign(totalBits, 8);
    if (packed && (width + totalBits <= bitsOffset)) {
      begin = totalBits % 8;
      bitFieldOffset = totalBits / 8;
    } else if (Type::MakeAlign(width, 8) + bitsOffset <= type->Width() * 8) {
      begin = 0;
      bitFieldOffset = bitsOffset / 8;
    } else {
      begin = 0;
      bitFieldOffset = Type::MakeAlign(structType->Offset(), type->Width());
    }
  }

  auto bitField = Object::New(tok, type, 0, L_NONE, begin, width);
  structType->AddBitField(bitField, bitFieldOffset);
  return false;
}


int Parser::ParseQual()
{
  int qualSpec = 0;
  for (; ;) {
    switch (ts_.Next()->tag_) {
    case Token::CONST:
      qualSpec |= Q_CONST;
      break;

    case Token::RESTRICT:
      qualSpec |= Q_RESTRICT;
      break;

    case Token::VOLATILE:
      qualSpec |= Q_VOLATILE;
      break;

    case Token::ATOMIC:
      qualSpec |= Q_ATOMIC;
      break;

    default:
      ts_.PutBack();
      return qualSpec;
    }
  }
}


Type* Parser::ParsePointer(Type* typePointedTo)
{
  Type* retType = typePointedTo;
  while (ts_.Try('*')) {
    retType = PointerType::New(typePointedTo);
    retType->SetQual(ParseQual());
    typePointedTo = retType;
  }

  return retType;
}


static Type* ModifyBase(Type* type, Type* base, Type* newBase)
{
  if (type == base)
    return newBase;
  
  auto ty = type->ToDerived();
  ty->SetDerived(ModifyBase(ty->Derived(), base, newBase));
  
  return ty;
}


/*
 * Return: pair of token(must be identifier) and it's type
 *     if token is nullptr, then we are parsing abstract declarator
 *     else, parsing direct declarator.
 */
TokenTypePair Parser::ParseDeclarator(Type* base)
{
  // May be pointer
  auto pointerType = ParsePointer(base);
  
  if (ts_.Try('(')) {
    //现在的 pointerType 并不是正确的 base type
    auto tokenTypePair = ParseDeclarator(pointerType);
    auto tok = tokenTypePair.first;
    auto type = tokenTypePair.second;

    ts_.Expect(')');

    auto newBase = ParseArrayFuncDeclarator(tok, pointerType);
    
    //修正 base type
    auto retType = ModifyBase(type, pointerType, newBase);
    return TokenTypePair(tokenTypePair.first, retType);
  } else if (ts_.Peek()->IsIdentifier()) {
    auto tok = ts_.Next();
    auto retType = ParseArrayFuncDeclarator(tok, pointerType);
    return TokenTypePair(tok, retType);
  }

  errTok_ = ts_.Peek();

  return TokenTypePair(nullptr, pointerType);
}


Identifier* Parser::ProcessDeclarator(const Token* tok, Type* type,
    int storageSpec, int funcSpec)
{
  assert(tok);
  /*
   * 检查在同一 scope 是否已经定义此变量
   * 如果 storage 是 typedef，那么应该往符号表里面插入 type
   * 定义 void 类型变量是非法的，只能是指向void类型的指针
   * 如果 funcSpec != 0, 那么现在必须是在定义函数，否则出错
   */
  const auto& name = tok->str_;
  Identifier* ident;

  if (storageSpec & S_TYPEDEF) {
    ident = curScope_->FindInCurScope(tok);
    if (ident) { // There is prio declaration in the same scope
      // The same declaration, simply return the prio declaration
      if (type->Compatible(*ident->Type()))
        return ident;
      // TODO(wgtdkp): add previous declaration information
      Error(tok, "conflicting types for '%s'", name.c_str());
    }
    ident = Identifier::New(tok, type, L_NONE);
    curScope_->Insert(ident);
    return ident;
  }

  if (type->ToVoid()) {
    Error(tok, "variable or field '%s' declared void",
        name.c_str());
  }

  if (type->ToFunc() && curScope_->Type() != S_FILE
      && (storageSpec & S_STATIC)) {
    Error(tok, "invalid storage class for function '%s'",
        name.c_str());
  }

  Linkage linkage;
  // Identifiers in function prototype have no linkage
  if (curScope_->Type() == S_PROTO) {
    linkage = L_NONE;
  } else if (curScope_->Type() == S_FILE) {
    linkage = L_EXTERNAL; // Default linkage for file scope identifiers
    if (storageSpec & S_STATIC)
      linkage = L_INTERNAL;
  } else if (!(storageSpec & S_EXTERN)) {
    linkage = L_NONE; // Default linkage for block scope identifiers
    if (type->ToFunc())
      linkage = L_EXTERNAL;
  } else {
    linkage = L_EXTERNAL;
  }

  //curScope_->Print();
  ident = curScope_->FindInCurScope(tok);
  if (ident) { // There is prio declaration in the same scope
    if (!type->Compatible(*ident->Type())) {
      Error(tok, "conflicting types for '%s'", name.c_str());
    }

    // The same scope prio declaration has no linkage,
    // there is a redeclaration error
    if (linkage == L_NONE) {
      Error(tok, "redeclaration of '%s' with no linkage",
          name.c_str());
    } else if (linkage == L_EXTERNAL) {
      if (ident->Linkage() == L_NONE) {
        Error(tok, "conflicting linkage for '%s'", name.c_str());
      }
    } else {
      if (ident->Linkage() != L_INTERNAL) {
        Error(tok, "conflicting linkage for '%s'", name.c_str());
      }
    }
    // The same declaration, simply return the prio declaration
    // TODO(wgtdkp): function type params
    if (!ident->Type()->Complete())
      ident->Type()->SetComplete(type->Complete());
    return ident;
  } else if (linkage == L_EXTERNAL) {
    ident = curScope_->Find(tok);
    if (ident) {
      if (!type->Compatible(*ident->Type())) {
        Error(tok, "conflicting types for '%s'", name.c_str());
      }
      if (ident->Linkage() != L_NONE) {
        linkage = ident->Linkage();
      }
      // Don't return, override it
    } else {
      ident = externalSymbols_->FindInCurScope(tok);
      if (ident) {
        if (!type->Compatible(*ident->Type())) {
          Error(tok, "conflicting types for '%s'", name.c_str());
        }
        // TODO(wgtdkp): ???????
        // Don't return
        // To stop later declaration with the same name in the same scope overriding this declaration
        
        // Useless here, just keep it
        if (!ident->Type()->Complete())
          ident->Type()->SetComplete(type->Complete()); 
        //return ident;
      }
    }
  }

  Identifier* ret;
  // TODO(wgtdkp): Treat function as object ?
  if (type->ToFunc()) {
    ret = Identifier::New(tok, type, linkage);
  } else {
    ret = Object::New(tok, type, storageSpec, linkage);
  }

  curScope_->Insert(ret);
  
  if (linkage == L_EXTERNAL && ident == nullptr) {
    externalSymbols_->Insert(ret);
  }

  return ret;
}


Type* Parser::ParseArrayFuncDeclarator(const Token* ident, Type* base)
{
  if (ts_.Try('[')) {

    if (nullptr != base->ToFunc()) {
      Error(ts_.Peek(), "the element of array can't be a function");
    }

    auto len = ParseArrayLength();
    //if (0 == len) {
    //    Error(ts_.Peek(), "can't declare an array of length 0");
    //}
    ts_.Expect(']');
    base = ParseArrayFuncDeclarator(ident, base);
    if (!base->Complete()) {
      Error(ident, "'%s' has incomplete element type",
          ident->str_.c_str());
    }
    return ArrayType::New(len, base);
  } else if (ts_.Try('(')) {	//function declaration
    if (base->ToFunc()) {
      Error(ts_.Peek(),
          "the return value of function can't be function");
    } else if (nullptr != base->ToArray()) {
      Error(ts_.Peek(),
          "the return value of function can't be array");
    }

    FuncType::TypeList paramTypes;
    EnterProto();
    bool hasEllipsis = ParseParamList(paramTypes);
    ExitProto();
    
    ts_.Expect(')');
    base = ParseArrayFuncDeclarator(ident, base);
    
    return FuncType::New(base, 0, hasEllipsis, paramTypes);
  }

  return base;
}


/*
 * return: -1, 没有指定长度；其它，长度；
 */
int Parser::ParseArrayLength()
{
  auto hasStatic = ts_.Try(Token::STATIC);
  auto qual = ParseQual();
  if (0 != qual)
    hasStatic = ts_.Try(Token::STATIC);
  /*
  if (!hasStatic) {
    if (ts_.Try('*'))
      return ts_.Expect(']'), -1;
    if (ts_.Try(']'))
      return -1;
    else {
      auto expr = ParseAssignExpr();
      auto len = Evaluate(expr);
      ts_.Expect(']');
      return len;
    }
  }*/

  //不支持变长数组
  if (!hasStatic && ts_.Test(']'))
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
 * Return: true, has ellipsis;
 */
bool Parser::ParseParamList(FuncType::TypeList& paramTypes)
{
  if (ts_.Test(')'))
    return false;
  
  auto paramType = ParseParamDecl();
  if (paramType->ToVoid())
    return false;
    
  paramTypes.push_back(paramType);

  while (ts_.Try(',')) {
    if (ts_.Try(Token::ELLIPSIS)) {
      return true;
    }

    auto tok = ts_.Peek();
    paramType = ParseParamDecl();
    if (paramType->ToVoid()) {
      Error(tok, "'void' must be the only parameter");
    }

    paramTypes.push_back(paramType);
  }

  return false;
}


Type* Parser::ParseParamDecl()
{
  int storageSpec, funcSpec;
  auto type = ParseDeclSpec(&storageSpec, &funcSpec);
  
  // No declarator
  if (ts_.Test(',') || ts_.Test(')'))
    return type;
  
  auto tokTypePair = ParseDeclarator(type);
  auto tok = tokTypePair.first;
  type = Type::MayCast(tokTypePair.second);
  if (tok == nullptr) { // Abstract declarator
    return type;
  }

  auto obj = ProcessDeclarator(tok, type, storageSpec, funcSpec);
  assert(obj->ToObject());

  return type;
}


Type* Parser::ParseAbstractDeclarator(Type* type)
{
  auto tokenTypePair = ParseDeclarator(type);
  auto tok = tokenTypePair.first;
  type = tokenTypePair.second;
  if (tok) { // Not a abstract declarator!
    Error(tok, "unexpected identifier '%s'", tok->str_.c_str());
  }
  return type;
  /*
  auto pointerType = ParsePointer(type);
  if (nullptr != pointerType->ToPointer() && !ts_.Try('('))
    return pointerType;
  
  auto ret = ParseAbstractDeclarator(pointerType);
  ts_.Expect(')');
  auto newBase = ParseArrayFuncDeclarator(pointerType);
  
  return ModifyBase(ret, pointerType, newBase);
  */
}


Identifier* Parser::ParseDirectDeclarator(Type* type,
    int storageSpec, int funcSpec)
{
  auto tokenTypePair = ParseDeclarator(type);
  auto tok = tokenTypePair.first;
  type = tokenTypePair.second;
  if (tok == nullptr) {
    Error(errTok_, "expect identifier or '('");
  }

  return ProcessDeclarator(tok, type, storageSpec, funcSpec);
}


Declaration* Parser::ParseInitDeclarator(Identifier* ident)
{
  auto obj = ident->ToObject();
  if (!obj) { // Do not record function Declaration
    return nullptr;
  }

  const auto& name = obj->Name();
  if (ts_.Try('=')) {
    return ParseInitDeclaratorSub(obj);
  }
  
  if (!obj->Type()->Complete()) {
    if (obj->Linkage() == L_NONE) {
      Error(obj, "storage size of '%s' isn’t known", name.c_str());
    }
    return nullptr; // Discards the incomplete object declarations
  }

  if (!obj->Decl()) {
    auto decl = Declaration::New(obj);
    obj->SetDecl(decl);
    return decl;
  }

  return nullptr;
}


Declaration* Parser::ParseInitDeclaratorSub(Object* obj)
{
  const auto& name = obj->Name();
  if ((curScope_->Type() != S_FILE) && obj->Linkage() != L_NONE) {
    Error(obj, "'%s' has both 'extern' and initializer", name.c_str());
  }

  if (!obj->Type()->Complete() && !obj->Type()->ToArray()) {
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
  if (obj->Decl()) {
    ParseInitializer(obj->Decl(), obj->Type(), 0, false);
    return nullptr;
  } else {
    auto decl = Declaration::New(obj);
    ParseInitializer(decl, obj->Type(), 0, false);
    obj->SetDecl(decl);
    return decl;
  }
}


void Parser::ParseInitializer(Declaration* decl, Type* type,
    int offset, bool designated, bool forceBrace)
{
  if (designated && !ts_.Test('.') && !ts_.Test('[')) {
    ts_.Expect('=');
  }

  Expr* expr;
  auto arrType = type->ToArray();
  auto structType = type->ToStruct();
  if (arrType) {
    if (forceBrace && !ts_.Test('{') && !ts_.Test(Token::LITERAL)) {
      ts_.Expect('{');
    } else if (!ParseLiteralInitializer(decl, arrType, offset)) {
      ParseArrayInitializer(decl, arrType, offset, designated);
      arrType->SetComplete(true);
    }
    return;
  } else if (structType) {
    if (forceBrace && !ts_.Test('{')) {
      ts_.Expect('{');
    } else if (!designated && !ts_.Test('{')) {
      auto mark = ts_.Mark();
      expr = ParseAssignExpr();
      if (structType->Compatible(*expr->Type())) {
        decl->AddInit({offset, structType, expr});
        return;
      }
      ts_.ResetTo(mark);
    }
    return ParseStructInitializer(decl, structType, offset, designated);
  }

  // Scalar type
  auto hasBrace = ts_.Try('{');
  expr = ParseAssignExpr();
  if (hasBrace) {
    ts_.Try(',');
    ts_.Expect('}');
  }

  decl->AddInit({offset, type, expr});
}


bool Parser::ParseLiteralInitializer(Declaration* decl,
    ArrayType* type, int offset)
{
  if (!type->Derived()->IsInteger())
    return false;

  auto hasBrace = ts_.Try('{');
  if (!ts_.Test(Token::LITERAL)) {
    if (hasBrace) ts_.PutBack();
    return false;
  }
  auto literal = ConcatLiterals(ts_.Next());
  auto tok = literal->Tok();

  if (hasBrace) {
    ts_.Try(',');
    ts_.Expect('}');
  }

  if (!type->Complete()) {
    type->SetLen(literal->Type()->ToArray()->Len());
    type->SetComplete(true);
  }
  
  //if (decl->Obj()->IsStatic()) {
  //    decl->AddInit({0, type, literal});
  //    return;
  //}

  auto width = std::min(type->Width(), literal->Type()->Width());
  auto str = literal->SVal()->c_str();
  /*
  for (; width > 0; --width) {
    auto p = str;
    auto type = ArithmType::New(T_CHAR);
    auto val = Constant::New(tok, T_CHAR, static_cast<long>(*p));
    decl->AddInit({offset, type, val});
    offset++;
    str++;
  }*/
  for (; width >= 8; width -= 8) {
    auto p = reinterpret_cast<const long*>(str);
    auto type = ArithmType::New(T_LONG);
    auto val = Constant::New(tok, T_LONG, static_cast<long>(*p));
    decl->AddInit({offset, type, val});
    offset += 8;
    str += 8;
  }

  for (; width >= 4; width -= 4) {
    auto p = reinterpret_cast<const int*>(str);
    auto type = ArithmType::New(T_INT);
    auto val = Constant::New(tok, T_INT, static_cast<long>(*p));
    decl->AddInit({offset, type, val});
    offset += 4;
    str += 4;
  }

  for (; width >= 2; width -= 2) {
    auto p = reinterpret_cast<const short*>(str);
    auto type = ArithmType::New(T_SHORT);
    auto val = Constant::New(tok, T_SHORT, static_cast<long>(*p));
    decl->AddInit({offset, type, val});
    offset += 2;
    str += 2;
  }

  for (; width >= 1; width--) {
    auto p = str;
    auto type = ArithmType::New(T_CHAR);
    auto val = Constant::New(tok, T_CHAR, static_cast<long>(*p));
    decl->AddInit({offset, type, val});
    offset++;
    str++;
  }

  return true;
}


void Parser::ParseArrayInitializer(Declaration* decl,
    ArrayType* type, int offset, bool designated)
{
  assert(type);

  if (!type->Complete())
    type->SetLen(0);

  int idx = 0;
  auto width = type->Derived()->Width();
  auto hasBrace = ts_.Try('{');
  while (true) {
    if (ts_.Test('}')) {
      if (hasBrace)
        ts_.Next();
      return;
    }

    if (!designated && !hasBrace && (ts_.Test('.') || ts_.Test('['))) {
      ts_.PutBack(); // Put the read comma(',') back
      return;
    } else if ((designated = ts_.Try('['))) {
      auto expr = ParseAssignExpr();
      EnsureInteger(expr);
      idx = Evaluator<long>().Eval(expr);
      ts_.Expect(']');

      if (idx < 0 || (type->Complete() && idx >= type->Len())) {
        Error(ts_.Peek(), "excess elements in array initializer");
      }
    }

    ParseInitializer(decl, type->Derived(), offset + idx * width, designated);
    ++idx;

    if (type->Complete() && idx >= type->Len()) {
      break;
    } else if (!type->Complete()) {
      type->SetLen(std::max(idx, type->Len()));
    }

    // Needless comma at the end is allowed
    if (!ts_.Try(',')) {
      if (hasBrace)
        ts_.Expect('}');
      return;
    }
  }

  if (hasBrace) {
    ts_.Try(',');
    if (!ts_.Try('}')) {
      Error(ts_.Peek(), "excess elements in array initializer");
    }
  }
}


StructType::Iterator Parser::ParseStructDesignator(StructType* type,
    const std::string& name)
{
  auto iter = type->Members().begin();
  for (; iter != type->Members().end(); ++iter) {
    if ((*iter)->Anonymous()) {
      auto anonyType = (*iter)->Type()->ToStruct();
      assert(anonyType);
      if (anonyType->GetMember(name)) {
        return iter; //ParseStructDesignator(anonyType);
      }
    } else if ((*iter)->Name() == name) {
      break;
    }
  }
  return iter;
}

/*
void Parser::ParseDesignatedInitializer(Declaration* decl,
    Type* type, int offset, bool designated)
{
  if ()
}
*/

void Parser::ParseStructInitializer(Declaration* decl,
    StructType* type, int offset, bool designated)
{
  assert(type);

  auto hasBrace = ts_.Try('{');
  auto member = type->Members().begin();
  while (true) {
    if (ts_.Test('}')) {
      if (hasBrace)
        ts_.Next();
      return;
    }

    if (!designated && !hasBrace && (ts_.Test('.') || ts_.Test('['))) {
      ts_.PutBack(); // Put the read comma(',') back
      return;
    }
    
    if ((designated = ts_.Try('.'))) {
      auto tok = ts_.Expect(Token::IDENTIFIER);
      const auto& name = tok->str_;
      if (!type->GetMember(name)) {
        Error(tok, "member '%s' not found", name.c_str());
      }
      member = ParseStructDesignator(type, name);
    }

    if ((*member)->Anonymous()) {
      ts_.PutBack(); ts_.PutBack();
      ParseInitializer(decl, (*member)->Type(), offset, designated);
    } else {
      ParseInitializer(decl, (*member)->Type(),
          offset + (*member)->Offset(), designated);
    }
    ++member;

    // Union, just init the first member
    if (!type->IsStruct())
      break;

    if (member == type->Members().end())
      break;

    // Needless comma at the end is allowed
    if (!ts_.Try(',')) {
      if (hasBrace)
        ts_.Expect('}');
      return;
    }
  }

  if (hasBrace) {
    ts_.Try(',');
    if (!ts_.Try('}')) {
      Error(ts_.Peek(), "excess members in struct initializer");
    }
  }
}




/*
 * Statements
 */

Stmt* Parser::ParseStmt()
{
  auto tok = ts_.Next();
  if (tok->IsEOF())
    Error(tok, "premature end of input");

  switch (tok->tag_) {
  case ';':
    return EmptyStmt::New();
  case '{':
    return ParseCompoundStmt();
  case Token::IF:
    return ParseIfStmt();
  case Token::SWITCH:
    return ParseSwitchStmt();
  case Token::WHILE:
    return ParseWhileStmt();
  case Token::DO:
    return ParseDoStmt();
  case Token::FOR:
    return ParseForStmt();
  case Token::GOTO:
    return ParseGotoStmt();
  case Token::CONTINUE:
    return ParseContinueStmt();
  case Token::BREAK:
    return ParseBreakStmt();
  case Token::RETURN:
    return ParseReturnStmt();
  case Token::CASE:
    return ParseCaseStmt();
  case Token::DEFAULT:
    return ParseDefaultStmt();
  }

  if (tok->IsIdentifier() && ts_.Try(':'))
    return ParseLabelStmt(tok);
  
  ts_.PutBack();
  auto expr = ParseExpr();
  ts_.Expect(';');

  return expr;
}


CompoundStmt* Parser::ParseCompoundStmt(FuncType* funcType)
{
  if (!funcType)
    EnterBlock();

  std::list<Stmt*> stmts;

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

  auto scope = funcType ? curScope_: ExitBlock();

  return CompoundStmt::New(stmts, scope);
}


IfStmt* Parser::ParseIfStmt()
{
  ts_.Expect('(');
  auto tok = ts_.Peek();
  auto cond = ParseExpr();
  if (!cond->Type()->IsScalar()) {
    Error(tok, "expect scalar");
  }
  ts_.Expect(')');

  auto then = ParseStmt();
  Stmt* els = nullptr;
  if (ts_.Try(Token::ELSE))
    els = ParseStmt();
  
  return IfStmt::New(cond, then, els);
}


/*
 * for 循环结构：
 *      for (declaration; expression1; expression2) statement
 * 展开后的结构：
 *		declaration
 * cond: if (expression1) then empty
 *		else goto end
 *		statement
 * step: expression2
 *		goto cond
 * next:
 */

#define ENTER_LOOP_BODY(breakDest, continueDest)    \
{											        \
  LabelStmt* breakDestBackup = breakDest_;	    \
  LabelStmt* continueDestBackup = continueDest_;  \
  breakDest_ = breakDest;			                \
  continueDest_ = continueDest; 

#define EXIT_LOOP_BODY()		        \
  breakDest_ = breakDestBackup;       \
  continueDest_ = continueDestBackup;	\
}

CompoundStmt* Parser::ParseForStmt()
{
  EnterBlock();
  ts_.Expect('(');
  
  std::list<Stmt*> stmts;

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
  if (nullptr != condExpr) {
    auto gotoEndStmt = JumpStmt::New(endLabel);
    auto ifStmt = IfStmt::New(condExpr, EmptyStmt::New(), gotoEndStmt);
    stmts.push_back(ifStmt);
  }

  //我们需要给break和continue语句提供相应的标号，不然不知往哪里跳
  Stmt* bodyStmt;
  ENTER_LOOP_BODY(endLabel, condLabel);
  bodyStmt = ParseStmt();
  //因为for的嵌套结构，在这里需要回复break和continue的目标标号
  EXIT_LOOP_BODY()
  
  stmts.push_back(bodyStmt);
  stmts.push_back(stepLabel);
  stmts.push_back(stepExpr);
  stmts.push_back(JumpStmt::New(condLabel));
  stmts.push_back(endLabel);

  auto scope = curScope_;
  ExitBlock();
  
  return CompoundStmt::New(stmts, scope);
}


/*
 * while 循环结构：
 * while (expression) statement
 * 展开后的结构：
 * cond: if (expression1) then empty
 *		else goto end
 *		statement
 *		goto cond
 * end:
 */
CompoundStmt* Parser::ParseWhileStmt()
{
  std::list<Stmt*> stmts;
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


/*
 * do-while 循环结构：
 *      do statement while (expression)
 * 展开后的结构：
 * begin: statement
 * cond: if (expression) then goto begin
 *		 else goto end
 * end:
 */
CompoundStmt* Parser::ParseDoStmt()
{
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

  std::list<Stmt*> stmts;
  stmts.push_back(beginLabel);
  stmts.push_back(bodyStmt);
  stmts.push_back(condLabel);
  stmts.push_back(ifStmt);
  stmts.push_back(endLabel);
  
  return CompoundStmt::New(stmts);
}


#define ENTER_SWITCH_BODY(breakDest, caseLabels)    \
{ 												    \
  CaseLabelList* caseLabelsBackup = caseLabels_;  \
  LabelStmt* defaultLabelBackup = defaultLabel_;  \
  LabelStmt* breakDestBackup = breakDest_;        \
  breakDest_ = breakDest;                         \
  caseLabels_ = &caseLabels; 

#define EXIT_SWITCH_BODY()			    \
  caseLabels_ = caseLabelsBackup;     \
  breakDest_ = breakDestBackup;       \
  defaultLabel_ = defaultLabelBackup;	\
}


/*
 * switch
 *  jump stmt (skip case labels)
 *  case labels
 *  jump stmts
 *  default jump stmt
 */
CompoundStmt* Parser::ParseSwitchStmt()
{
  std::list<Stmt*> stmts;
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
      iter != caseLabels.end(); iter++) {
    auto cond = BinaryOp::New(tok, Token::EQ, t, iter->first);
    auto then = JumpStmt::New(iter->second);
    auto ifStmt = IfStmt::New(cond, then, nullptr);
    stmts.push_back(ifStmt);
  }
  
  stmts.push_back(JumpStmt::New(defaultLabel_));
  EXIT_SWITCH_BODY();

  stmts.push_back(endLabel);

  return CompoundStmt::New(stmts);
}


CompoundStmt* Parser::ParseCaseStmt()
{
  auto expr = ParseExpr();
  ts_.Expect(':');
  
  auto val = Evaluator<long>().Eval(expr);
  auto cons = Constant::New(expr->Tok(), T_INT, val);

  auto labelStmt = LabelStmt::New();
  caseLabels_->push_back(std::make_pair(cons, labelStmt));
  
  std::list<Stmt*> stmts;
  stmts.push_back(labelStmt);
  stmts.push_back(ParseStmt());
  
  return CompoundStmt::New(stmts);
}


CompoundStmt* Parser::ParseDefaultStmt()
{
  auto tok = ts_.Peek();
  ts_.Expect(':');
  if (defaultLabel_ != nullptr) { // There is a 'default' stmt
    Error(tok, "multiple default labels in one switch");
  }
  auto labelStmt = LabelStmt::New();
  defaultLabel_ = labelStmt;
  
  std::list<Stmt*> stmts;
  stmts.push_back(labelStmt);
  stmts.push_back(ParseStmt());
  
  return CompoundStmt::New(stmts);
}


JumpStmt* Parser::ParseContinueStmt()
{
  auto tok = ts_.Peek();
  ts_.Expect(';');
  if (continueDest_ == nullptr) {
    Error(tok, "'continue' is allowed only in loop");
  }
  
  return JumpStmt::New(continueDest_);
}


JumpStmt* Parser::ParseBreakStmt()
{
  auto tok = ts_.Peek();
  ts_.Expect(';');
  // ERROR(wgtdkp):
  if (breakDest_ == nullptr) {
    Error(tok, "'break' is allowed only in switch/loop");
  }
  
  return JumpStmt::New(breakDest_);
}


ReturnStmt* Parser::ParseReturnStmt()
{
  Expr* expr;

  if (ts_.Try(';')) {
    expr = nullptr;
  } else {
    expr = ParseExpr();
    ts_.Expect(';');
    
    auto retType = curFunc_->Type()->ToFunc()->Derived();
    expr = Expr::MayCast(expr, retType);
  }

  return ReturnStmt::New(expr);
}


JumpStmt* Parser::ParseGotoStmt()
{
  auto label = ts_.Peek();
  ts_.Expect(Token::IDENTIFIER);
  ts_.Expect(';');

  auto labelStmt = FindLabel(label->str_);
  if (labelStmt) {
    return JumpStmt::New(labelStmt);
  }
  
  auto unresolvedJump = JumpStmt::New(nullptr);;
  unresolvedJumps_.push_back(std::make_pair(label, unresolvedJump));
  
  return unresolvedJump;
}


CompoundStmt* Parser::ParseLabelStmt(const Token* label)
{
  const auto& labelStr = label->str_;
  auto stmt = ParseStmt();
  if (nullptr != FindLabel(labelStr)) {
    Error(label, "redefinition of label '%s'", labelStr.c_str());
  }

  auto labelStmt = LabelStmt::New();
  AddLabel(labelStr, labelStmt);
  std::list<Stmt*> stmts;
  stmts.push_back(labelStmt);
  stmts.push_back(stmt);

  return CompoundStmt::New(stmts);
}
