#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "error.h"
#include "mem_pool.h"
#include "scope.h"
#include "token.h"

#include <cassert>
#include <memory>
#include <stack>

class Preprocessor;

typedef std::pair<Token*, Type*> TokenTypePair;

class Parser
{
    typedef std::vector<Constant*> LiteralList;
    typedef std::vector<Object*> StaticObjectList;
    typedef std::vector<std::pair<Constant*, LabelStmt*>> CaseLabelList;
    typedef std::list<std::pair<Token*, JumpStmt*>> LabelJumpList;
    typedef std::map<std::string, LabelStmt*> LabelMap;
    
public:
    explicit Parser(const TokenSeq& ts) 
        : _unit(TranslationUnit::New()),
          _inEnumeration(false), _ts(ts),
          _externalSymbols(new Scope(nullptr, S_BLOCK)),
          _errTok(nullptr), _curScope(new Scope(nullptr, S_FILE)),
          _curParamScope(nullptr),
          _breakDest(nullptr), _continueDest(nullptr),
          _caseLabels(nullptr), _defaultLabel(nullptr) {}

    ~Parser(void) {}

    Constant* ParseConstant(const Token* tok);
    Constant* ParseLiteral(const Token* tok);

    Expr* ParseGeneric(void);

    void Parse(void);
    void ParseTranslationUnit(void);
    FuncDef* ParseFuncDef(Token* tok, FuncType* funcType);
    /************ Expressions ************/
    
    Expr* ParseExpr(void);

    Expr* ParsePrimaryExpr(void);

    Expr* ParsePostfixExpr(void);
    Expr* ParsePostfixExprTail(Expr* primExpr);
    Expr* ParseSubScripting(Expr* pointer);
    BinaryOp* ParseMemberRef(const Token* tok, int op, Expr* lhs);
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
    StructUnionType* ParseStructUnionDecl(StructUnionType* type);
    Type* ParseEnumerator(ArithmType* type);
    //declarator
    int ParseQual(void);
    Type* ParsePointer(Type* typePointedTo);
    TokenTypePair ParseDeclarator(Type* type);
    Type* ParseArrayFuncDeclarator(Token* ident, Type* base);
    int ParseArrayLength(void);
    bool ParseParamList(std::list<Type*>& paramTypes);
    Type* ParseParamDecl(void);

    //typename
    Type* ParseAbstractDeclarator(Type* type);
    Identifier* ParseDirectDeclarator(Type* type,
            int storageSpec, int funcSpec);

    //initializer
    void ParseInitializer(Declaration* init, Type* type, int offset);
    void ParseArrayInitializer(Declaration* init,
            ArrayType* type, int offset);
            
    void ParseStructInitializer(Declaration* init,
            StructUnionType* type, int offset);

    void ParseLiteralInitializer(Declaration* init,
            ArrayType* type, int offset);

    Declaration* ParseInitDeclarator(Identifier* ident);

    /************* Statements ***************/
    Stmt* ParseStmt(void);
    CompoundStmt* ParseCompoundStmt(FuncType* funcType=nullptr);
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

    Identifier* ProcessDeclarator(Token* tok, Type* type,
            int storageSpec, int funcSpec);

    bool IsTypeName(Token* tok) const{
        if (tok->IsTypeSpecQual())
            return true;

        if (tok->IsIdentifier()) {
            auto ident = _curScope->Find(tok);
            if (ident && ident->ToTypeName())
                return true;
        }
        return false;
    }

    bool IsType(Token* tok) const{
        if (tok->IsDecl())
            return true;

        if (tok->IsIdentifier()) {
            auto ident = _curScope->Find(tok);
            return (ident && ident->ToTypeName());
        }

        return false;
    }

    void EnsureInteger(Expr* expr) {
        if (!expr->Type()->IsInteger()) {
            Error(expr, "expect integer expression");
        }
    }

    void EnterBlock(FuncType* funcType=nullptr);
    
    Scope* ExitBlock(void) {
        auto scope = _curScope;
        _curScope = _curScope->Parent();
        return scope;
    }

    void EnterProto(void) {
        _curScope = new Scope(_curScope, S_PROTO);
        if (_curParamScope == nullptr)
            _curParamScope = _curScope;
    }

    void ExitProto(void) {
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

    TranslationUnit* Unit(void) {
        return _unit;
    }

    //StaticObjectList& StaticObjects(void) {
    //    return _staticObjects;
    //}

private:
    
    // The root of the AST
    TranslationUnit* _unit;
    
    bool _inEnumeration;
    TokenSeq _ts;

    // It is not the real scope,
    // It contains all external symbols(resolved and not resolved)
    Scope* _externalSymbols;
    
    //LiteralList _literals;
    //StaticObjectList _staticObjects;


    Token* _errTok;
    //std::stack<Token*> _buf;

    Scope* _curScope;
    Scope* _curParamScope;
    LabelMap _curLabels;
    LabelJumpList _unresolvedJumps;
    
    LabelStmt* _breakDest;
    LabelStmt* _continueDest;
    CaseLabelList* _caseLabels;
    LabelStmt* _defaultLabel;
};

#endif
