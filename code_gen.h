#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"
#include "visitor.h"


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


struct ROData
{
    ROData(long ival, int align): _ival(ival), _align(align) {
        _label = ".LC" + std::to_string(GenTag());
    }

    explicit ROData(const std::string& sval): _sval(sval), _align(1) {
        _label = ".LC" + std::to_string(GenTag());
    }


    ~ROData(void) {}

    std::string _sval;
    long _ival;

    int _align;
    std::string _label;

private:
    static long GenTag(void) {
        static long tag = 0;
        return tag++;
    }
};


typedef std::vector<ROData> RODataList;



class Generator: public Visitor
{
public:
    Generator(void) {}

    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp);
    virtual void VisitUnaryOp(UnaryOp* unaryOp);
    virtual void VisitConditionalOp(ConditionalOp* condOp);
    virtual void VisitFuncCall(FuncCall* funcCall);
    virtual void VisitObject(Object* obj);
    virtual void VisitEnumerator(Enumerator* enumer);
    virtual void VisitIdentifier(Identifier* ident);
    virtual void VisitConstant(Constant* cons);
    virtual void VisitTempVar(TempVar* tempVar);

    // Binary
    void GenMemberRefOp(BinaryOp* binaryOp);
    //void GenSubScriptingOp(BinaryOp* binaryOp);
    void GenAndOp(BinaryOp* binaryOp);
    void GenOrOp(BinaryOp* binaryOp);
    void GenAddOp(BinaryOp* binaryOp);
    void GenSubOp(BinaryOp* binaryOp);
    void GenAssignOp(BinaryOp* assign);
    void GenCastOp(UnaryOp* cast);
    void GenDerefOp(UnaryOp* deref);
    void GenPointerArithm(BinaryOp* binary);
    void GenDivOp(bool flt, bool sign, int width, int op);
    void GenCompOp(bool flt, int width, const char* set);

    // Unary
    void GenPrefixIncDec(UnaryOp* unary, const std::string& inst);
    void GenPostfixIncDec(UnaryOp* unary, const std::string& inst);



    //statement
    virtual void VisitInitialization(Initialization* init);
    virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
    virtual void VisitIfStmt(IfStmt* ifStmt);
    virtual void VisitJumpStmt(JumpStmt* jumpStmt);
    virtual void VisitReturnStmt(ReturnStmt* returnStmt);
    virtual void VisitLabelStmt(LabelStmt* labelStmt);
    virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

    virtual void VisitFuncDef(FuncDef* funcDef);
    virtual void VisitTranslationUnit(TranslationUnit* unit);

    void Gen(void);

    void GenExpr(Expr* expr) {
        expr->Accept(this);
    }

    void Emit(const char* format, ...);
    void EmitLabel(const std::string& label);
    std::string EmitLoad(const std::string& addr, Type* type);
    void EmitStore(const std::string& addr, Type* type);

    void Push(const char* reg);
    void Pop(const char* reg);

    void Spill(bool flt) {
        Push(flt ? "xmm0": "rax");
    }

    void Restore(bool flt);

    std::string Save(const std::string& src) {
        assert(src == "rax" || src == "xmm0");
        if (src == "rax") {
            Emit("movq #rax, #rcx");
            return "rcx";
        } else {
            Emit("movsd #xmm0, xmm1");
            return "xmm1";
        }
    }

    void Exchange(const std::string& lhs, const std::string& rhs);

    static void SetInOut(Parser* parser, FILE* outFile) {
        _parser = parser;
        _outFile = outFile;
    }

protected:
    static Parser* _parser;
    static FILE* _outFile;

    static std::string _cons;
    static RODataList _rodatas;
};


struct ObjectAddr
{
    std::string Repr(void);
    
    std::string _label;
    std::string _base;
    int _offset;
};


class LValGenerator: public Generator
{
public:
    LValGenerator(void) {}
    
    //Expression
    virtual void VisitBinaryOp(BinaryOp* binaryOp);
    virtual void VisitUnaryOp(UnaryOp* unaryOp);
    virtual void VisitObject(Object* obj);
    virtual void VisitIdentifier(Identifier* ident);

    virtual void VisitConditionalOp(ConditionalOp* condOp) {
        assert(false);
    }
    
    virtual void VisitFuncCall(FuncCall* funcCall) {
        assert(false);
    }

    virtual void VisitEnumerator(Enumerator* enumer) {
        assert(false);
    }

    virtual void VisitConstant(Constant* cons) {
        assert(false);
    }

    virtual void VisitTempVar(TempVar* tempVar) {
        assert(false);
    }

    ObjectAddr GenExpr(Expr* expr) {
        expr->Accept(this);
        return _addr;
    }
private:
    ObjectAddr _addr;
};

#endif
