#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"
#include "type.h"
#include "visitor.h"

#include <cassert>
#include <cstdio>


class Parser;

enum class ParamClass
{
    INTEGER,
    SSE,
    SSEUP,
    X87,
    X87_UP,
    COMPLEX_X87,
    NO_CLASS,
    MEMORY
};


class Register
{
    friend class Generator;

public:
    enum {
        RAX, RBX, RCX, RDX,
        RSI, RDI, RBP, RSP,
        R8, R9, R10, R11,
        R12, R13, R14, R15,
        XMM0, XMM1, XMM2, XMM3,
        XMM4, XMM5, XMM6, XMM7,
        N_REG
    };

    ~Register(void) {}

    bool Using(void) const {
        return _expr != nullptr;
    }

    const char* Name(void) const {
        return _name;
    }

    void Bind(Expr* expr) {
        _expr = expr;
    }

    void Release(void) {
        _expr = nullptr;
    }
    
    static Register* Get(int tag) {
        return &_regs[tag];
    }

    static Register* Get(const char* name) {
        for (int i = 0; i < N_REG; i++) {
            if (std::string(name) == _regs[i]._name)
                return &_regs[i];
        }
        assert(false);
        return nullptr; // Make Compiler happy
    }

private:
    explicit Register(const char* name)
            : _name(name), _expr(nullptr) {}

    const char* _name;
    Expr* _expr;

    static Register _regs[N_REG];
};


class Generator: public Visitor
{
public:
    Generator(Parser* parser, FILE* outFile)
            : _parser(parser), _outFile(outFile),
              _desReg(Register::Get(Register::RAX)),
              _argRegUsed(0), _argVecRegUsed(0),
              _argStackOffset(-8) {}
    
    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp);
    virtual void VisitUnaryOp(UnaryOp* unaryOp);
    virtual void VisitConditionalOp(ConditionalOp* condOp);
    virtual void VisitFuncCall(FuncCall* funcCall);
    virtual void VisitObject(Object* obj);
    virtual void VisitConstant(Constant* cons);
    virtual void VisitTempVar(TempVar* tempVar);

    //statement
    virtual void VisitStmt(Stmt* stmt);
    virtual void VisitIfStmt(IfStmt* ifStmt);
    virtual void VisitJumpStmt(JumpStmt* jumpStmt);
    virtual void VisitReturnStmt(ReturnStmt* returnStmt);
    virtual void VisitLabelStmt(LabelStmt* labelStmt);
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
    virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    virtual void VisitFuncDef(FuncDef* funcDef);

    //Translation Unit
    virtual void VisitTranslationUnit(TranslationUnit* transUnit);

    void SetupFuncArg(Expr* arg, ParamClass cls);

    void Emit(const char* format, ...);

    Register* AllocArgReg(void) {
        if (_argRegUsed >= N_ARG_REG)
            return nullptr;

        auto ret = _argRegs[_argRegUsed++];
        assert(!ret->Using());
        return ret;
    }

    Register* AllocArgVecReg(void) {
        if (_argVecRegUsed >= N_ARG_VEC_REG)
            return nullptr;
        
        auto ret = _argVecRegs[_argVecRegUsed++];
        assert(!ret->Using());
        return ret;
    }

    int AllocArgStack(void) {
        _argStackOffset -= 8;
        return _argStackOffset;
    }

    void SetDesReg(Register* desReg) {
        assert(!desReg->Using());
        _desReg = desReg;
    }

private:
    Parser* _parser;
    FILE* _outFile;

    // The destination register for current expression
    Register* _desReg;

    // The number of argument passing register used
    int _argRegUsed;

    // The number of argument passing vector register used
    int _argVecRegUsed;

    // The stack pointer after pushed arguments on the stack 
    int _argStackOffset;

    static const int N_ARG_REG = 6;
    static Register* _argRegs[N_ARG_REG];

    static const int N_ARG_VEC_REG = 8;
    static Register* _argVecRegs[N_ARG_VEC_REG];
};

#endif
