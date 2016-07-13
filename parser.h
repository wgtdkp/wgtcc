#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "scope.h"
#include "error.h"
#include "lexer.h"
#include "mem_pool.h"

#include <cassert>
#include <memory>
#include <stack>


typedef std::pair<Token*, Type*> TokenTypePair;

class Parser
{
public:
    explicit Parser(Lexer* lexer) 
        : _unit(TranslationUnit::NewTranslationUnit()),
          _inEnumeration(false), _lexer(lexer),
          _externalSymbols(new Scope(nullptr, S_BLOCK)),
          _curScope(new Scope(nullptr, S_FILE)),
          _errTok(nullptr),
          _breakDest(nullptr), _continueDest(nullptr),
          _caseLabels(nullptr), _defaultLabel(nullptr) {}

    ~Parser(void) {}
    
    /*
     * Binary Operator
     */
    BinaryOp* NewBinaryOp(const Token* tok, Expr* lhs, Expr* rhs);
    BinaryOp* NewBinaryOp(const Token* tok, int op, Expr* lhs, Expr* rhs);
    BinaryOp* NewMemberRefOp(const Token* tok,
            Expr* lhs, const std::string& rhsName);
    ConditionalOp* NewConditionalOp(const Token* tok,
            Expr* cond, Expr* exprTrue, Expr* exprFalse);
    
    FuncCall* NewFuncCall(const Token* tok,
            Expr* designator, const std::list<Expr*>& args);
    
    Identifier* NewIdentifier(Type* type, Scope* scope, enum Linkage linkage);
    Object* NewObject(Type* type, Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE, int offset=0);

    Constant* NewConstantInteger(ArithmType* type, long long val);
    Constant* NewConstantFloat(ArithmType* type, double val);
    TempVar* NewTempVar(Type* type);
    UnaryOp* NewUnaryOp(const Token* tok,
        int op, Expr* operand, Type* type=nullptr);
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
    Expr* ParseMemberRef(const Token* tok, Expr* lhs);
    UnaryOp* ParsePostfixIncDec(const Token* tok, Expr* operand);
    FuncCall* ParseFuncCall(Expr* caller);

    Expr* ParseUnaryExpr(void);
    Constant* ParseSizeof(void);
    Constant* ParseAlignof(void);
    UnaryOp* ParsePrefixIncDec(const Token* tok);
    UnaryOp* ParseUnaryOp(const Token* tok, int op);
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
    //Identifier* ParseDeclaratorAndDo(Type* base, int storageSpec, int funcSpec);
    TokenTypePair ParseDeclarator(Type* type);
    Type* ParseArrayFuncDeclarator(Type* base);
    int ParseArrayLength(void);
    bool ParseParamList(std::list<Type*>& params);
    Type* ParseParamDecl(void);

    //typename
    Type* ParseAbstractDeclarator(Type* type);
    Identifier* ParseDirectDeclarator(Type* type,
            int storageSpec, int funcSpec);

    //initializer
    Stmt* ParseInitializer(Object* obj);
    Stmt* ParseArrayInitializer(Object* arr);
    Stmt* ParseStructInitializer(Object* obj);
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

    /*
     * Function Definition
     */
    bool IsFuncDef(void);
    FuncDef* ParseFuncDef(void);

    Identifier* ProcessDeclarator(Token* tok, Type* type,
            int storageSpec, int funcSpec);

    /*
     * Type checking:
     * Do type checking and evaluating the expression type;
     */
    void TypeChecking(Expr* expr, const Token* errTok) {}
    
    // BinaryOp
    void TypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void SubScriptingOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void MemberRefOpTypeChecking(BinaryOp* binaryOp,
            const Token* errTok, const std::string& rhsName);
    void MultiOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void AdditiveOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void ShiftOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void RelationalOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void EqualityOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void BitwiseOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void LogicalOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);
    void AssignOpTypeChecking(BinaryOp* binaryOp, const Token* errTok);

    // UnaryOp
    void TypeChecking(UnaryOp* unaryOp, const Token* errTok);
    void IncDecOpTypeChecking(UnaryOp* unaryOp, const Token* errTok);
    void AddrOpTypeChecking(UnaryOp* unaryOp, const Token* errTok);
    void DerefOpTypeChecking(UnaryOp* unaryOp, const Token* errTok);
    void UnaryArithmOpTypeChecking(UnaryOp* unaryOp, const Token* errTok);
    void CastOpTypeChecking(UnaryOp* unaryOp, const Token* errTok);

    // ConditionalOp
    void TypeChecking(ConditionalOp* condOp, const Token* errTok);
    
    // FuncCall
    void TypeChecking(FuncCall* funcCall, const Token* errTok);

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

    bool IsTypeName(Token* tok) const{
        if (tok->IsTypeSpecQual())
            return true;

        if (tok->IsIdentifier()) {
            auto ident = _curScope->Find(tok->Str());
            if (ident && ident->ToType())
                return true;
        }
        return false;
    }

    bool IsType(Token* tok) const{
        if (tok->IsDecl())
            return true;

        if (tok->IsIdentifier()) {
            auto ident = _curScope->Find(tok->Str());
            return (ident && !ident->ToObject());
        }

        return false;
    }

    void Expect(int expect, int follow1 = ',', int follow2 = ';');

    void Panic(int follow1, int follow2) {
        for (const Token* tok = Next(); !tok->IsEOF(); tok = Next()) {
            if (tok->Tag() == follow1 || tok->Tag() == follow2)
                return PutBack();
        }
    }

    

    
    void EnterBlock(void) {
        _curScope = new Scope(_curScope, S_BLOCK);
    }
    void ExitBlock(void) {
        _curScope = _curScope->Parent();
    }

    void EnterFunc(const char* funcName);

    void ExitFunc(void);

    LabelStmt* FindLabel(const std::string& label) {
        auto ret = _curLabels.find(label);
        if (_curLabels.end() == ret)
            return nullptr;
        return ret->second;
    }

    void AddLabel(const std::string& label, LabelStmt* labelStmt) {
        assert(nullptr == FindLabel(label));
        _curLabels[label] = labelStmt;
    }

    typedef std::vector<std::pair<int, LabelStmt*>> CaseLabelList;
    typedef std::list<std::pair<Token*, JumpStmt*>> LabelJumpList;
    typedef std::map<std::string, LabelStmt*> LabelMap;

private:
    // The root of the AST
    TranslationUnit* _unit;
    
    bool _inEnumeration;
    
    Lexer* _lexer;

    // It is not the real scope,
    // it contains all external symbols(resolved and not resolved)
    Scope* _externalSymbols;
    
    Scope* _curScope;
    LabelMap _curLabels;
    LabelJumpList _unresolvedJumps;
    std::stack<Token*> _buf;

    Token* _errTok;

    LabelStmt* _breakDest;
    LabelStmt* _continueDest;
    CaseLabelList* _caseLabels;
    LabelStmt* _defaultLabel;
    
    // Memory Pools
    MemPoolImp<BinaryOp>        _binaryOpPool;
    MemPoolImp<ConditionalOp>   _conditionalOpPool;
    MemPoolImp<FuncCall>        _funcCallPool;
    MemPoolImp<Object>          _objectPool;
    MemPoolImp<Identifier>      _identifierPool;
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
