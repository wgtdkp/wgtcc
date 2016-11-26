#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "encoding.h"
#include "error.h"
#include "mem_pool.h"
#include "scope.h"
#include "token.h"

#include <cassert>
#include <memory>
#include <stack>
#include <utility>

class Preprocessor;

using TokenTypePair = std::pair<const Token*, QualType>;
using LiteralList = std::vector<ASTConstant*>;
using StaticObjectList = std::vector<Object*>;
using LabelMap = std::map<std::string, LabelStmt*>;
using SwitchStack = std::stack<SwitchStmt*>;
using GotoStmtList = std::vector<std::pair<const Token*, GotoStmt*>>;


class Parser {
public:
  explicit Parser(const TokenSequence& ts) 
    : unit_(TranslationUnit::New()),
      ts_(ts),
      external_symbols_(Scope::New(nullptr, ScopeType::BLOCK)),
      errTok_(nullptr),
      cur_scope_(Scope::New(nullptr, ScopeType::FILE)),
      cur_func_(nullptr),
      loop_depth_(0) {
        ts_.SetParser(this);
      }

  ~Parser() {}

  ASTConstant* ParseConstant(const Token* tok);
  ASTConstant* ParseFloat(const Token* tok);
  ASTConstant* ParseInteger(const Token* tok);
  ASTConstant* ParseCharacter(const Token* tok);
  Encoding ParseLiteral(std::string& str, const Token* tok);
  ASTConstant* ConcatLiterals(const Token* tok);
  Expr* ParseGeneric();

  void Parse();
  void ParseTranslationUnit();
  FuncDef* ParseFuncDef(Identifier* ident);
  
  
  // Expressions
  Expr*     ParseExpr();
  Expr*     ParsePrimaryExpr();
  QualType  TryCompoundLiteral();
  Object*   ParseCompoundLiteral(QualType type);
  Expr*     ParsePostfixExpr();
  Expr*     ParsePostfixExprTail(Expr* prim_expr);
  Expr*     ParseSubScripting(Expr* pointer);
  BinaryOp* ParseMemberRef(const Token* tok, int op, Expr* lhs);
  UnaryOp*  ParsePostfixIncDec(const Token* tok, Expr* operand);
  FuncCall* ParseFuncCall(Expr* caller);

  Expr*     ParseUnaryExpr();
  ASTConstant* ParseSizeof();
  ASTConstant* ParseAlignof();
  UnaryOp*  ParsePrefixIncDec(const Token* tok);
  UnaryOp*  ParseUnaryOp(const Token* tok, int op);

  QualType ParseTypeName();
  Expr* ParseCastExpr();
  Expr* ParseMultiplicativeExpr();
  Expr* ParseAdditiveExpr();
  Expr* ParseShiftExpr();
  Expr* ParseRelationalExpr();
  Expr* ParseEqualityExpr();
  Expr* ParseBitiwiseAndExpr();
  Expr* ParseBitwiseXorExpr();
  Expr* ParseBitwiseOrExpr();
  Expr* ParseLogicalAndExpr();
  Expr* ParseLogicalOrExpr();
  Expr* ParseConditionalExpr();
  Expr* ParseCommaExpr();
  Expr* ParseAssignExpr();

  // Declarations
  CompoundStmt* ParseDecl();
  void          ParseStaticAssert();
  QualType      ParseDeclSpec(int* storage_spec, int* func_spec, int* align_spec);
  QualType      ParseSpecQual();
  int           ParseAlignas();
  Type*         ParseStructUnionSpec(bool is_struct);
  StructType*   ParseStructUnionDecl(StructType* type);
  void          ParseBitField(StructType* struct_type,
                              const Token* tok, QualType type);
  Type*         ParseEnumSpec();
  Type*         ParseEnumerator(ArithmType* type);
  int           ParseQual();
  QualType      ParsePointer(QualType type_pointed_to);
  TokenTypePair ParseDeclarator(QualType type);
  QualType      ParseArrayFuncDeclarator(const Token* ident, QualType base);
  int           ParseArrayLength();
  bool          ParseParamList(FuncType::ParamList& param_list);
  Object*       ParseParamDecl();

  QualType      ParseAbstractDeclarator(QualType type);
  Identifier*   ParseDirectDeclarator(QualType type, int storage_spec,
                                      int func_spec, int align);
  Identifier*   ProcessDeclarator(const Token* tok, QualType type,
                                  int storage_spec, int func_spec, int align);

  // Initializer
  void          ParseInitializer(Declaration* decl, QualType type, int offset,
                                 bool designated=false, bool force_brace=false,
                                 uint8_t bitfield_begin=0,
                                 uint8_t bitfield_width=0);
  void          ParseArrayInitializer(Declaration* decl, ArrayType* type,
                                      int offset, bool designated);
  void          ParseStructInitializer(Declaration* decl, StructType* type,
                                       int offset, bool designated);
  bool          ParseLiteralInitializer(Declaration* init,
                                        ArrayType* type, int offset);
  Declaration*  ParseInitDeclarator(Identifier* ident);
  Declaration*  ParseInitDeclaratorSub(Object* obj);
  StructType::Iterator ParseStructDesignator(StructType* type,
                                             const std::string& name);

  // Statements
  Stmt*         ParseStmt();
  CompoundStmt* ParseCompoundStmt(FuncType* func_type=nullptr);
  IfStmt*       ParseIfStmt();
  SwitchStmt*   ParseSwitchStmt();
  WhileStmt*    ParseWhileStmt();
  WhileStmt*    ParseDoWhileStmt();
  ForStmt*      ParseForStmt();
  GotoStmt*     ParseGotoStmt();
  ContinueStmt* ParseContinueStmt();
  BreakStmt*    ParseBreakStmt();
  ReturnStmt*   ParseReturnStmt();
  LabelStmt*    ParseLabelStmt(const Token* label);
  CaseStmt*     ParseCaseStmt();
  DefaultStmt*  ParseDefaultStmt();

  
  // GNU extensions
  void TryAttributeSpecList();
  void ParseAttributeSpec();
  void ParseAttribute();

  bool IsTypeName(const Token* tok) const{
    if (tok->IsTypeSpecQual())
      return true;

    if (tok->IsIdentifier()) {
      auto ident = cur_scope_->Find(tok);
      if (ident && ident->ToTypeName())
        return true;
    }
    return false;
  }

  bool IsType(const Token* tok) const{
    if (tok->IsDecl())
      return true;

    if (tok->IsIdentifier()) {
      auto ident = cur_scope_->Find(tok);
      return (ident && ident->ToTypeName());
    }
    return false;
  }

  void EnsureInteger(Expr* expr) {
    if (!expr->type()->IsInteger()) {
      Error(expr, "expect integer expression");
    }
  }

  void EnterBlock() { cur_scope_ = Scope::New(cur_scope_, ScopeType::BLOCK); }
  void ExitBlock() { cur_scope_ = cur_scope_->parent(); }
  void EnterProto() { cur_scope_ = Scope::New(cur_scope_, ScopeType::PROTO); }
  void ExitProto() { cur_scope_ = cur_scope_->parent(); }
  FuncDef* EnterFunc(Identifier* ident);
  void ExitFunc();
  LabelStmt* FindLabel(const std::string& label) {
    auto ret = cur_labels_.find(label);
    if (cur_labels_.end() == ret)
      return nullptr;
    return ret->second;
  }
  void AddLabel(const std::string& label, LabelStmt* label_stmt) {
    assert(FindLabel(label) == nullptr);
    cur_labels_[label] = label_stmt;
  }
  TranslationUnit* Unit() { return unit_; }
  FuncDef* CurFunc() { return cur_func_; }
  const SwitchStmt* CurSwitch() const {
    return const_cast<const SwitchStmt*>(
        const_cast<Parser*>(this)->CurSwitch());
  }
  SwitchStmt* CurSwitch() {
    if (switch_stack_.empty())
      return nullptr;
    return switch_stack_.top();
  }
  void PushSwitch(SwitchStmt* s) { switch_stack_.push(s); }
  void PopSwitch() { switch_stack_.pop(); }
  bool InSwitchOrLoop() const { return CurSwitch() != nullptr || InLoop(); }
  bool InLoop() const { return loop_depth_ > 0; }
  void EnterLoop() {
    EnterBlock();
    ++loop_depth_;
  }
  void ExitLoop() {
    ExitBlock();
    --loop_depth_;
  }
  void AddUnresolvedGoto(const Token* label, GotoStmt* goto_stmt) {
    unresolved_goto_list_.push_back(std::make_pair(label, goto_stmt));
  }

private:
  static bool IsBuiltin(FuncType* type);
  static bool IsBuiltin(const std::string& name);
  static Identifier* GetBuiltin(const Token* tok);
  static void DefineBuiltins();

  static FuncType* va_start_type_;
  static FuncType* va_arg_type_;

  // The root of the AST
  TranslationUnit* unit_;
  TokenSequence ts_;

  // It is not the real scope,
  // It contains all external symbols(resolved and not resolved)
  Scope* external_symbols_;
  
  const Token* errTok_;
  Scope* cur_scope_;
  FuncDef* cur_func_;
  //SwitchStmt* curSwitch_;
  SwitchStack switch_stack_;
  ssize_t loop_depth_;


  LabelMap cur_labels_;
  GotoStmtList unresolved_goto_list_;
};

#endif
