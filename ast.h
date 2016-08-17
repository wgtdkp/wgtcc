#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "error.h"
#include "string_pair.h"
#include "token.h"
#include "type.h"

#include <cassert>
#include <list>
#include <memory>
#include <string>


class Visitor;
template<typename T> class Evaluator;
class AddrEvaluator;
class Generator;

class Scope;
class Parser;
class ASTNode;
class Token;
class TokenSeq;

//Expression
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class TempVar;
class Constant;

//statement
class Stmt;
class IfStmt;
class JumpStmt;
class LabelStmt;
class EmptyStmt;
class CompoundStmt;
class FuncDef;
class TranslationUnit;


/*
 * AST Node
 */

class ASTNode
{
public:
    virtual ~ASTNode(void) {}
    
    virtual void Accept(Visitor* v) = 0;

protected:
    ASTNode(void) {}

    MemPool* _pool {nullptr};
};

typedef ASTNode ExtDecl;


/*********** Statement *************/

class Stmt : public ASTNode
{
public:
    virtual ~Stmt(void) {}

protected:
     Stmt(void) {}
};


class Initialization: public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

    struct Initializer
    {
        int _offset;
        Type* _type;
        Expr* _expr;
    };

    struct StaticInitializer
    {
        int _offset;
        int _width;
        long _val;
        std::string _label;        
    };
    
    typedef std::vector<Initializer> InitList;
    typedef std::vector<StaticInitializer> StaticInitList;

public:
    static Initialization* New(Object* obj);

    virtual ~Initialization(void) {}

    virtual void Accept(Visitor* v);

    InitList& Inits(void) {
        return _inits;
    }

    StaticInitList StaticInits(void) {
        return _staticInits;
    }

    Object* Obj(void) {
        return _obj;
    }

    void AddInit(int offset, Type* type, Expr* _expr);

protected:
    Initialization(Object* obj): _obj(obj) {}

    Object* _obj;
    union {
        InitList _inits;
        StaticInitList _staticInits;
    };
};


class EmptyStmt : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static EmptyStmt* New(void);

    virtual ~EmptyStmt(void) {}
    
    virtual void Accept(Visitor* v);

protected:
    EmptyStmt(void) {}
};


// 构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
public:
    static LabelStmt* New(void);

    ~LabelStmt(void) {}
    
    virtual void Accept(Visitor* v);
    
    int Tag(void) const {
        return Tag();
    }

protected:
    LabelStmt(void): _tag(GenTag()) {}

private:
    static int GenTag(void) {
        static int tag = 0;
        return ++tag;
    }
    
    int _tag; // 使用整型的tag值，而不直接用字符串
};


class IfStmt : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
public:
    static IfStmt* New(Expr* cond, Stmt* then, Stmt* els=nullptr);

    virtual ~IfStmt(void) {}
    
    virtual void Accept(Visitor* v);

protected:
    IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
            : _cond(cond), _then(then), _else(els) {}

private:
    Expr* _cond;
    Stmt* _then;
    Stmt* _else;
};


class JumpStmt : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static JumpStmt* New(LabelStmt* label);

    virtual ~JumpStmt(void) {}
    
    virtual void Accept(Visitor* v);
    
    void SetLabel(LabelStmt* label) {
        _label = label;
    }

protected:
    JumpStmt(LabelStmt* label): _label(label) {}

private:
    LabelStmt* _label;
};


class ReturnStmt: public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static ReturnStmt* New(Expr* expr);

    virtual ~ReturnStmt(void) {}
    
    virtual void Accept(Visitor* v);

protected:
    ReturnStmt(::Expr* expr): _expr(expr) {}

private:
    ::Expr* _expr;
};


typedef std::list<Stmt*> StmtList;

class CompoundStmt : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static CompoundStmt* New(StmtList& stmts, ::Scope* scope=nullptr);

    virtual ~CompoundStmt(void) {}
    
    virtual void Accept(Visitor* v);

    /*
    StmtList& Stmts(void) {
        return _stmts;
    }

    ::Scope* Scope(void) {
        return _scope;
    }
    */

protected:
    CompoundStmt(const StmtList& stmts, ::Scope* scope=nullptr)
            : _stmts(stmts), _scope(scope) {}

private:
    StmtList _stmts;
    ::Scope* _scope;
};


/********** Expr ************/
/*
 * Expr
 *      BinaryOp
 *      UnaryOp
 *      ConditionalOp
 *      FuncCall
 *      Constant
 *      Identifier
 *          Object
 *      TempVar
 */

class Expr : public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    virtual ~Expr(void) {}
    
    ::Type* Type(void) {
        return _type;
    }

    virtual Object* ToObject(void) {
        return nullptr;
    }

    virtual bool IsLVal(void) = 0;

    virtual void TypeChecking(void) = 0;

    std::string Name(void) {
        return _tok->Str();
    }

    const Token* Tok(void) const {
        return _tok;
    }

    void SetTok(const Token* tok) {
        _tok = tok;
    }

protected:
    /*
     * You can construct a expression without specifying a type,
     * then the type should be evaluated in TypeChecking()
     */
    Expr(const Token* tok, ::Type* type): _tok(tok), _type(type) {}

    const Token* _tok;
    ::Type* _type;
};


/***********************************************************
'+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
'=',(复合赋值运算符被拆分为两个运算)
'==', '!=', '<=', '>=',
'&&', '||'
'['(下标运算符), '.'(成员运算符)
','(逗号运算符),
*************************************************************/
class BinaryOp : public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
    friend class Initialization;

public:
    static BinaryOp* New(const Token* tok, Expr* lhs, Expr* rhs);

    static BinaryOp* New(const Token* tok, int op, Expr* lhs, Expr* rhs);

    virtual ~BinaryOp(void) {}
    
    virtual void Accept(Visitor* v);
    
    //like member ref operator is a lvalue
    virtual bool IsLVal(void) {
        switch (_op) {
        case '.':
        case ']': return !Type()->ToArrayType();
        default: return false;
        }
    }

    ArithmType* Promote(void);

    virtual void TypeChecking(void);
    void SubScriptingOpTypeChecking(void);
    void MemberRefOpTypeChecking(void);
    void MultiOpTypeChecking(void);
    void AdditiveOpTypeChecking(void);
    void ShiftOpTypeChecking(void);
    void RelationalOpTypeChecking(void);
    void EqualityOpTypeChecking(void);
    void BitwiseOpTypeChecking(void);
    void LogicalOpTypeChecking(void);
    void AssignOpTypeChecking(void);

protected:
    BinaryOp(int op, Expr* lhs, Expr* rhs)
            : Expr(lhs->Tok(), nullptr), _op(op), _lhs(lhs), _rhs(rhs) {}

    int _op;
    Expr* _lhs;
    Expr* _rhs;
};


/*
 * Unary Operator:
 * '++' (prefix/postfix)
 * '--' (prefix/postfix)
 * '&'  (ADDR)
 * '*'  (DEREF)
 * '+'  (PLUS)
 * '-'  (MINUS)
 * '~'
 * '!'
 * CAST // like (int)3
 */
class UnaryOp : public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static UnaryOp* New(const Token* tok,
            int op, Expr* operand, ::Type* type=nullptr);

    virtual ~UnaryOp(void) {}
    
    virtual void Accept(Visitor* v);

    //TODO: like '*p' is lvalue, but '~i' is not lvalue
    virtual bool IsLVal(void);

    ArithmType* Promote(void);

    void TypeChecking(void);
    void IncDecOpTypeChecking(void);
    void AddrOpTypeChecking(void);
    void DerefOpTypeChecking(void);
    void UnaryArithmOpTypeChecking(void);
    void CastOpTypeChecking(void);

protected:
    UnaryOp(int op, Expr* operand, ::Type* type = nullptr)
        : Expr(operand->Tok(), type), _op(op), _operand(operand) {}

    int _op;
    Expr* _operand;
};


// cond ? true ： false
class ConditionalOp : public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static ConditionalOp* New(const Token* tok,
            Expr* cond, Expr* exprTrue, Expr* exprFalse);
    
    virtual ~ConditionalOp(void) {}
    
    virtual void Accept(Visitor* v);

    virtual bool IsLVal(void) {
        return false;
    }

    ArithmType* Promote(void);
    
    virtual void TypeChecking(void);

protected:
    ConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
            : Expr(cond->Tok(), nullptr), _cond(cond),
              _exprTrue(exprTrue), _exprFalse(exprFalse) {}

private:
    Expr* _cond;
    Expr* _exprTrue;
    Expr* _exprFalse;
};


/************** Function Call ****************/
class FuncCall : public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
        
    typedef std::list<Expr*> ArgList;

public:
    static FuncCall* New(Expr* designator, const std::list<Expr*>& args);

    ~FuncCall(void) {}
    
    virtual void Accept(Visitor* v);

    //a function call is ofcourse not lvalue
    virtual bool IsLVal(void) {
        return false;
    }

    ArgList* Args(void) {
        return &_args;
    }

    Expr* Designator(void) {
        return _designator;
    }

    virtual void TypeChecking(void);

protected:
    FuncCall(Expr* designator, std::list<Expr*> args)
        : Expr(designator->Tok(), nullptr),
          _designator(designator), _args(args) {}

    Expr* _designator;
    ArgList _args;
};


class Constant: public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static Constant* New(const Token* tok, int tag, long val);
    static Constant* New(const Token* tok, int tag, double val);
    static Constant* New(const Token* tok, const std::string* val);

    ~Constant(void) {}
    
    virtual void Accept(Visitor* v);

    virtual bool IsLVal(void) {
        return false;
    }

    virtual void TypeChecking(void) {}

    long IVal(void) const {
        return _ival;
    }

    double FVal(void) const {
        return _fval;
    }

    const std::string* SVal(void) const {
        return _sval;
    }

    std::string Label(void) const {
        return std::string(".LC") + std::to_string(_tag);
    }

protected:
    Constant(const Token* tok, ::Type* type, long val)
            : Expr(tok, type), _ival(val) {}
    Constant(const Token* tok, ::Type* type, double val)
            : Expr(tok, type), _fval(val) {}
    Constant(const Token* tok, ::Type* type, const std::string* val)
            : Expr(tok, type), _sval(val) {}

    union {
        long _ival;
        double _fval;
        struct {
            long _tag;
            const std::string* _sval;
        };
    };
};


//临时变量
class TempVar : public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static TempVar* New(::Type* type);

    virtual ~TempVar(void) {}
    
    virtual void Accept(Visitor* v);
    
    virtual bool IsLVal(void) {
        return true;
    }

    virtual void TypeChecking(void) {}

protected:
    TempVar(::Type* type): Expr(nullptr, type), _tag(GenTag()) {}
    
private:
    static int GenTag(void) {
        static int tag = 0;
        return ++tag;
    }

    int _tag;
};


/*************** Declaration ******************/

class FuncDef : public ExtDecl
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static FuncDef* New(FuncType* type,
            const std::list<Object*>& params, CompoundStmt* stmt);

    virtual ~FuncDef(void) {}
    
    virtual FuncType* Type(void) {
        return _type;
    }

    std::list<Object*>& Params(void) {
        return _params;
    }

    CompoundStmt* Body(void) {
        return _body;
    }
    
    virtual void Accept(Visitor* v);

protected:
    FuncDef(FuncType* type, const std::list<Object*>& params,
            CompoundStmt* stmt): _type(type), _params(params), _body(stmt) {}

private:
    FuncType* _type;
    std::list<Object*> _params;
    CompoundStmt* _body;
};


class TranslationUnit : public ASTNode
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static TranslationUnit* New(void) {
        return new TranslationUnit();
    }

    virtual ~TranslationUnit(void) {}

    virtual void Accept(Visitor* v);
    
    void Add(ExtDecl* extDecl) {
        _extDecls.push_back(extDecl);
    }

    std::list<ExtDecl*>& ExtDecls(void) {
        return _extDecls;
    }

private:
    TranslationUnit(void) {}

    std::list<ExtDecl*> _extDecls;
};


enum Linkage {
    L_NONE,
    L_EXTERNAL,
    L_INTERNAL,
};


class Identifier: public Expr
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static Identifier* New(const Token* tok,
            ::Type* type, Scope* scope, enum Linkage linkage);

    virtual ~Identifier(void) {}

    virtual void Accept(Visitor* v) { assert(false); }

    virtual bool IsLVal(void) {
        return false;
    }

    /*
     * An identifer can be:
     *     object, sturct/union/enum tag, typedef name, function, label.
     */
    virtual ::Type* ToType(void) {
        if (ToObject())
            return nullptr;
        return this->Type();
    }

    ::Scope* Scope(void) {
        return _scope;
    }

    enum Linkage Linkage(void) const {
        return _linkage;
    }

    void SetLinkage(enum Linkage linkage) {
        _linkage = linkage;
    }

    const Token* Tok(void) {
        return _tok;
    }

    virtual std::string Name(void);

    virtual bool operator==(const Identifier& other) const {
        return *_type == *other._type && _scope == other._scope;
    }

    virtual void TypeChecking(void) {}

protected:
    Identifier(const Token* tok, ::Type* type,
            ::Scope* scope, enum Linkage linkage)
            : Expr(tok, type), _scope(scope), _linkage(linkage) {}
    
    // An identifier has property scope
    ::Scope* _scope;
    // An identifier has property linkage
    enum Linkage _linkage;
};


class Enumerator: public Identifier
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static Enumerator* New(const Token* tok, ::Scope* scope, int val);

    virtual ~Enumerator(void) {}

    virtual void Accept(Visitor* v);

    int Val(void) const {
        return _cons->IVal();
    }

protected:
    Enumerator(const Token* tok, ::Scope* scope, int val)
            : Identifier(tok, Type::NewArithmType(T_INT), scope, L_NONE),
              _cons(Constant::New(tok, T_INT, (long)val)) {}

    Constant* _cons;
};


class Object : public Identifier
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    static Object* New(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE);

    ~Object(void) {}

    virtual void Accept(Visitor* v);
    
    virtual Object* ToObject(void) {
        return this;
    }

    virtual bool IsLVal(void) {
        // TODO(wgtdkp): not all object is lval?
        return true;
    }

    bool IsStatic(void) const {
        return (Storage() & S_STATIC) || (Linkage() != L_NONE);
    }

    int Storage(void) const {
        return _storage;
    }

    void SetStorage(int storage) {
        _storage = storage;
    }

    int Offset(void) const {
        return _offset;
    }

    void SetOffset(int offset) {
        _offset = offset;
    }

    Initialization* Init(void) {
        return _init;
    }

    void SetInit(Initialization* init) {
        _init = init;
    }

    bool operator==(const Object& other) const {
        // TODO(wgtdkp): Not implemented
        assert(0);
    }

    bool operator!=(const Object& other) const {
        return !(*this == other);
    }

protected:
    Object(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE)
            : Identifier(tok, type, scope, linkage),
              _storage(storage), _offset(0), _init(nullptr) {}

private:
    int _storage;
    
    // For code gen
    int _offset;

    Initialization* _init;

    //static size_t _labelId {0};
};

#endif
