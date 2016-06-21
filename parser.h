#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "env.h"
#include "error.h"
#include "lexer.h"
#include "mem_pool.h"

#include <cassert>
#include <memory>
#include <stack>


typedef std::pair<std::string, Type*> NameTypePair;

class Parser
{
public:
    explicit Parser(Lexer* lexer) 
        : _unit(TranslationUnit::NewTranslationUnit()),
          _lexer(lexer), _topEnv(new Env(nullptr)),
          _breakDest(nullptr), _continueDest(nullptr),
          _caseLabels(nullptr), _defaultLabel(nullptr) {}

    ~Parser(void) { 
        //delete _lexer; 
    }
    
    /*
     * Binary Operator
     */
    BinaryOp* NewBinaryOp(int op, Expr* lhs, Expr* rhs);
    BinaryOp* NewMemberRefOp(int op, Expr* lhs, const std::string& rhsName);
    ConditionalOp* NewConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse);
    FuncCall* NewFuncCall(Expr* designator, const std::list<Expr*>& args);
    Variable* NewVariable(Type* type, int offset=0);
    Constant* NewConstantInteger(ArithmType* type, long long val);
    Constant* NewConstantFloat(ArithmType* type, double val);
    TempVar* NewTempVar(Type* type);
    UnaryOp* NewUnaryOp(int op, Expr* operand, Type* type=nullptr);

    /*
     * Statement
     */
    EmptyStmt* NewEmptyStmt(void);
    IfStmt* NewIfStmt(Expr* cond, Stmt* then, Stmt* els=nullptr);
    JumpStmt* NewJumpStmt(LabelStmt* label);
    ReturnStmt* NewReturnStmt(Expr* expr);
    LabelStmt* NewLabelStmt(void);
    CompoundStmt* NewCompoundStmt(std::list<Stmt*>& stmts);

    /*
     * Function Definition
     */
    FuncDef* NewFuncDef(FuncType* type, CompoundStmt* stmt);

    void Delete(ASTNode* node);
    

    Constant* ParseConstant(const Token* tok);
    Expr* ParseString(const Token* tok);
    Expr* ParseGeneric(void);

    void ParseTranslationUnit(void);

    /************ Expressions ************/
    
    Expr* ParseExpr(void);

    Expr* ParsePrimaryExpr(void);

    Expr* ParsePostfixExpr(void);
    Expr* ParsePostfixExprTail(Expr* primExpr);
    Expr* ParseSubScripting(Expr* pointer);
    Expr* ParseMemberRef(int tag, Expr* lhs);
    UnaryOp* ParsePostfixIncDec(int tag, Expr* expr);
    FuncCall* ParseFuncCall(Expr* caller);

    Expr* ParseUnaryExpr(void);
    Constant* ParseSizeof(void);
    Constant* ParseAlignof(void);
    UnaryOp* ParsePrefixIncDec(int tag);
    UnaryOp* ParseUnaryOp(int op);
    //UnaryOp* ParseDerefOperand(void);

    Type* ParseTypeName(void);
    Expr* ParseCastExpr(void);
    Expr* ParseMultiplicativeExpr(void);
    Expr* ParseAdditiveExpr(void);
    Expr* ParseShiftExpr(void);
    Expr* ParseRelationalExpr(void);
    Expr* ParseEqualityExpr(void);
    Expr* ParseBitiwiseAndExpr(void);
    Expr* ParseBitwiseXorExpr(void);
    Expr* ParseBitwiseOrExpr(void);
    Expr* ParseLogicalAndExpr(void);
    Expr* ParseLogicalOrExpr(void);
    Expr* ParseConditionalExpr(void);

    Expr* ParseCommaExpr(void);

    Expr* ParseAssignExpr(void);

    /************* Declarations **************/
    CompoundStmt* ParseDecl(void);
    Type* ParseDeclSpec(int* storage, int* func);
    Type* ParseSpecQual(void);
    int ParseAlignas(void);
    Type* ParseStructUnionSpec(bool isStruct);
    Type* ParseEnumSpec(void);
    StructUnionType* ParseStructDecl(StructUnionType* type);
    Type* ParseEnumerator(ArithmType* type);
    //declarator
    int ParseQual(void);
    Type* ParsePointer(Type* typePointedTo);
    Variable* ParseDeclaratorAndDo(Type* base, int storageSpec, int funcSpec);
    NameTypePair ParseDeclarator(Type* type);
    Type* ParseArrayFuncDeclarator(Type* base);
    int ParseArrayLength(void);
    bool ParseParamList(std::list<Type*>& params);
    Type* ParseParamDecl(void);

    //typename
    Type* ParseAbstractDeclarator(Type* type);

    //initializer
    Stmt* ParseInitializer(Variable* var);
    Stmt* ParseArrayInitializer(Variable* arr);
    Stmt* ParseStructInitializer(Variable* var);
    Stmt* ParseInitDeclarator(Type* type, int storageSpec, int funcSpec);

    /************* Statements ***************/
    Stmt* ParseStmt(void);
    CompoundStmt* ParseCompoundStmt(void);
    IfStmt* ParseIfStmt(void);
    CompoundStmt* ParseSwitchStmt(void);
    CompoundStmt* ParseWhileStmt(void);
    CompoundStmt* ParseDoStmt(void);
    CompoundStmt* ParseForStmt(void);
    JumpStmt* ParseGotoStmt(void);
    JumpStmt* ParseContinueStmt(void);
    JumpStmt* ParseBreakStmt(void);
    ReturnStmt* ParseReturnStmt(void);
    CompoundStmt* ParseLabelStmt(const Token* label);
    CompoundStmt* ParseCaseStmt(void);
    CompoundStmt* ParseDefaultStmt(void);

    /*********** Function Definition *************/
    bool IsFuncDef(void);
    FuncDef* ParseFuncDef(void);

private:
    //������ǰtoken���ϲ���������true,��consumeһ��token
    //������tokTag�������򷵻�false�����Ҳ�consume token
    bool Try(int tokTag) {
        auto tok = Next();
        if (tok->Tag() == tokTag)
            return true;
        PutBack();
        return false;
    }

    //���ص�ǰtoken����ǰ��
    Token* Next(void) { return _lexer->Get(); }
    void PutBack(void) { _lexer->Unget(); }

    //���ص�ǰtoken�����ǲ�ǰ��
    Token* Peek(void) { return _lexer->Peek(); }
    const Token* Peek(void) const{ return _lexer->Peek(); }
    bool Test(int tag) const { return Peek()->Tag() == tag; }

    //��¼��ǰtokenλ��
    void Mark(void) { _buf.push(Peek()); }

    //�ص�����һ��Mark�ĵط�
    Token* Release(void) {
        assert(!_buf.empty());
        while (Peek() != _buf.top()) {
            PutBack();
        }
        _buf.pop();
        return Peek();
    }

    bool IsTypeName(const Token* tok) const{
        if (tok->IsTypeSpecQual())
            return true;
        return (tok->IsIdentifier() 
            && nullptr != _topEnv->FindType(tok->Str()));
    }

    bool IsType(const Token* tok) const{
        if (tok->IsDecl())
            return true;
        return (tok->IsIdentifier()
            && nullptr != _topEnv->FindType(tok->Str()));
    }

    void Expect(int expect, int follow1 = ',', int follow2 = ';');

    void Panic(int follow1, int follow2) {
        for (const Token* tok = Next(); !tok->IsEOF(); tok = Next()) {
            if (tok->Tag() == follow1 || tok->Tag() == follow2)
                return PutBack();
        }
    }

    void EnsureModifiableExpr(Expr* expr) const {
        if (!expr->IsLVal()) {
            //TODO: error
            Error(expr, "lvalue expression expected");
        } else if (expr->Ty()->IsConst()) {
            Error(expr, "can't modifiy 'const' qualified expression");
        }
    }

    void EnsureLValExpr(Expr* expr) const {
        if (!expr->IsLVal()) {
            Error(expr, "lvalue expected");
        }
    }

    void EnsureScalarExpr(Expr* expr) const {
        if (!expr->Ty()->IsScalar())
            Error(expr, "scalar type expression expected");
    }

    void EnsureIntegerExpr(Expr* expr) const {
        if (!expr->Ty()->IsInteger())
            Error(expr, "integer expression expected");
    }

    void EnterBlock(void) {
        _topEnv = new Env(_topEnv);
    }
    void ExitBlock(void) {
        _topEnv = _topEnv->Parent();
    }

    void EnterFunc(const char* funcName);

    void ExitFunc(void);

    LabelStmt* FindLabel(const std::string& label) {
        auto ret = _topLabels.find(label);
        if (_topLabels.end() == ret)
            return nullptr;
        return ret->second;
    }

    void AddLabel(const std::string& label, LabelStmt* labelStmt) {
        assert(nullptr == FindLabel(label));
        _topLabels[label] = labelStmt;
    }

    typedef std::vector<std::pair<int, LabelStmt*>> CaseLabelList;
    typedef std::list<std::pair<std::string, JumpStmt*>> LabelJumpList;
    typedef std::map<std::string, LabelStmt*> LabelMap;

private:
    // The root of the AST
    TranslationUnit* _unit;

    Lexer* _lexer;
    Env* _topEnv;
    LabelMap _topLabels;
    LabelJumpList _unresolvedJumps;
    std::stack<Token*> _buf;

    LabelStmt* _breakDest;
    LabelStmt* _continueDest;
    CaseLabelList* _caseLabels;
    LabelStmt* _defaultLabel;
    
    // Memory Pools
    MemPoolImp<BinaryOp>        _binaryOpPool;
    MemPoolImp<ConditionalOp>   _conditionalOpPool;
    MemPoolImp<FuncCall>        _funcCallPool;
    MemPoolImp<Variable>        _variablePool;
    MemPoolImp<Constant>        _constantPool;
    MemPoolImp<TempVar>         _tempVarPool;
    MemPoolImp<UnaryOp>         _unaryOpPool;
    MemPoolImp<EmptyStmt>       _emptyStmtPool;
    MemPoolImp<IfStmt>          _ifStmtPool;
    MemPoolImp<JumpStmt>        _jumpStmtPool;
    MemPoolImp<ReturnStmt>      _returnStmtPool;
    MemPoolImp<LabelStmt>       _labelStmtPool;
    MemPoolImp<CompoundStmt>    _compoundStmtPool;
    MemPoolImp<FuncDef>         _funcDefPool;
};

#endif
