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

class Identifier;
class Object;
class Enumerator;

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


struct Initializer
{
    int _offset;
    Type* _type;
    Expr* _expr;
};

class Declaration: public Stmt
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
    
    typedef std::vector<Initializer> InitList;

public:
    static Declaration* New(Object* obj);

    virtual ~Declaration(void) {}

    virtual void Accept(Visitor* v);

    InitList& Inits(void) {
        return _inits;
    }

    //StaticInitList StaticInits(void) {
    //    return _staticInits;
    //}

    Object* Obj(void) {
        return _obj;
    }

    void AddInit(int offset, Type* type, Expr* _expr);

protected:
    Declaration(Object* obj): _obj(obj) {}

    Object* _obj;
    //union {
        InitList _inits;
    //    StaticInitList _staticInits;
    //};
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
    
    std::string Label(void) const {
        return ".L" + std::to_string(_tag);
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

    StmtList& Stmts(void) {
        return _stmts;
    }

    ::Scope* Scope(void) {
        return _scope;
    }

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
    friend class LValGenerator;

public:
    virtual ~Expr(void) {}
    
    ::Type* Type(void) {
        return _type;
    }

    virtual bool IsLVal(void) = 0;

    virtual void TypeChecking(void) = 0;

    const Token* Tok(void) const {
        return _tok;
    }

    void SetTok(const Token* tok) {
        _tok = tok;
    }

    static Expr* MayCast(Expr* expr);
    static Expr* MayCast(Expr* expr, ::Type* desType);

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
    friend class LValGenerator;
    friend class Declaration;

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
    BinaryOp(const Token* tok, int op, Expr* lhs, Expr* rhs)
            : Expr(tok, nullptr), _op(op),
              _lhs(MayCast(lhs)), _rhs(MayCast(rhs)) {}

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
    friend class LValGenerator;

public:
    static UnaryOp* New(int op, Expr* operand, ::Type* type=nullptr);

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
        : Expr(operand->Tok(), type), _op(op) {
            _operand = operand;
            if (_op != Token::CAST && _op != Token::ADDR) {
                _operand = MayCast(operand);
            }
        }

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
            : Expr(cond->Tok(), nullptr), _cond(MayCast(cond)),
              _exprTrue(MayCast(exprTrue)), _exprFalse(MayCast(exprFalse)) {}

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

public:        
    typedef std::vector<Expr*> ArgList;

public:
    static FuncCall* New(Expr* designator, const ArgList& args);

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

    std::string Name(void) const {
        return _tok->Str();
    }

    ::FuncType* FuncType(void) {
        return _designator->Type()->ToFuncType();
    }

    virtual void TypeChecking(void);

protected:
    FuncCall(Expr* designator, const ArgList& args)
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
    static Constant* New(const Token* tok, int tag, const std::string* val);

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
        return std::string(".LC") + std::to_string(_id);
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
            long _id;
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
    friend class LValGenerator;

public:
    static Identifier* New(const Token* tok,
            ::Type* type, Scope* scope, Linkage linkage);

    virtual ~Identifier(void) {}

    virtual void Accept(Visitor* v);

    virtual bool IsLVal(void) {
        return false;
    }

    virtual Object* ToObject(void) {
        return nullptr;
    }

    virtual Enumerator* ToEnumerator(void) {
        return nullptr;
    }

    /*
     * An identifer can be:
     *     object, sturct/union/enum tag, typedef name, function, label.
     */
     Identifier* ToTypeName(void) {
        // A typename has no linkage
        // And a function has external or internal linkage
        if (ToObject() || ToEnumerator() || _linkage != L_NONE)
            return nullptr;
        return this;
    }


    std::string Name(void) const {
        return _tok->Str();
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

    virtual bool operator==(const Identifier& other) const {
        return Name() == other.Name()
            && *_type == *other._type
            && _scope == other._scope;
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

    virtual Enumerator* ToEnumerator(void) {
        return this;
    }

    int Val(void) const {
        return _cons->IVal();
    }

protected:
    Enumerator(const Token* tok, ::Scope* scope, int val)
            : Identifier(tok, ArithmType::New(T_INT), scope, L_NONE),
              _cons(Constant::New(tok, T_INT, (long)val)) {}

    Constant* _cons;
};


class Object : public Identifier
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;
    friend class LValGenerator;

public:
    static Object* New(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE,
            unsigned char bitFieldBegin=0, unsigned char bitFieldWidth=0);

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

    Declaration* Decl(void) {
        return _decl;
    }

    void SetDecl(Declaration* decl) {
        _decl = decl;
    }

    unsigned char BitFieldBegin(void) const {
        return _bitFieldBegin;
    }

    unsigned char BitFieldEnd(void) const {
        return _bitFieldBegin + _bitFieldWidth;
    }

    unsigned char BitFieldWidth(void) const {
        return _bitFieldWidth;
    }

    static unsigned long BitFieldMask(Object* bitField) {
        return BitFieldMask(bitField->_bitFieldBegin, bitField->_bitFieldWidth);
    }

    static unsigned long BitFieldMask(unsigned char begin, unsigned char width) {
        auto end = begin + width;
        return ((0xFFFFFFFFFFFFFFFFUL << (64 - end)) >> (64 - width)) << begin;
    }


    bool HasInit(void) const {
        return _decl && _decl->Inits().size();
    }

    bool operator==(const Object& other) const {
        // TODO(wgtdkp): Not implemented
        assert(false);
    }

    bool operator!=(const Object& other) const {
        return !(*this == other);
    }

protected:
    Object(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE,
            unsigned char bitFieldBegin=0, unsigned char bitFieldWidth=0)
            : Identifier(tok, type, scope, linkage),
              _storage(storage), _offset(0), _decl(nullptr),
              _bitFieldBegin(bitFieldBegin), _bitFieldWidth(bitFieldWidth) {}

private:
    int _storage;
    
    // For code gen
    int _offset;

    Declaration* _decl;

    unsigned char _bitFieldBegin;
    // 0 means it's not a bitfield
    unsigned char _bitFieldWidth;

    //static size_t _labelId {0};
};



/*************** Declaration ******************/

class FuncDef : public ExtDecl
{
    template<typename T> friend class Evaluator;
    friend class AddrEvaluator;
    friend class Generator;

public:
    typedef std::vector<Object*> ParamList;
    
public:
    static FuncDef* New(Identifier* ident, LabelStmt* retLabel);

    virtual ~FuncDef(void) {}
    
    virtual FuncType* Type(void) {
        return _ident->Type()->ToFuncType();
    }

    ParamList& Params(void) {
        return _params;
    }

    CompoundStmt* Body(void) {
        return _body;
    }

    void SetBody(CompoundStmt* body) {
        _body = body;
    }

    std::string Name(void) const {
        return _ident->Name();
    }

    enum Linkage Linkage(void) {
        return _ident->Linkage();
    }
    
    virtual void Accept(Visitor* v);

protected:
    FuncDef(Identifier* ident, LabelStmt* retLabel)
            : _ident(ident), _retLabel(retLabel) {}

private:
    Identifier* _ident;
    LabelStmt* _retLabel;
    ParamList _params;
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


#endif
