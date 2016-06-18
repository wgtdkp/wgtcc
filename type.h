#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include <list>


/********* Type System ***********/
class Env;
class Type;
class VoidType;
class Variable;
class Constant;
class ArithmType;
class DerivedType;
class ArrayType;
class FuncType;
class PointerType;
class StructUnionType;
class EnumType;

enum {
    /*****storage-class-specifiers*****/
    S_TYPEDEF = 0x01,
    S_EXTERN = 0x02,
    S_STATIC = 0x04,
    S_THREAD = 0x08,
    S_AUTO = 0x10,
    S_REGISTER = 0x20,

    /*****type-specifier*****/
    T_SIGNED = 0x40,
    T_UNSIGNED = 0x80,
    T_CHAR = 0x100,
    T_SHORT = 0x200,
    T_INT = 0x400,
    T_LONG = 0x800,
    T_VOID = 0x1000,
    T_FLOAT = 0x2000,
    T_DOUBLE = 0x4000,
    T_BOOL = 0x8000,
    T_COMPLEX = 0x10000,
    T_ATOMIC = 0x20000,
    T_STRUCT_UNION = 0x40000,
    T_ENUM = 0x80000,
    T_TYPEDEF_NAME = 0x100000,

    /*****type-qualifier*****/
    Q_CONST = 0x200000,
    Q_RESTRICT = 0x400000,
    Q_VOLATILE = 0x800000,
    Q_ATOMIC = 0x1000000,

    T_LONG_LONG = 0x2000000,

    /*****function specifier*****/
    F_INLINE = 0x4000000,
    F_NORETURN = 0x8000000,
};


class Type
{
public:
    static const int _machineWord = 4;

    bool operator!=(const Type& other) const { return !(*this == other); }

    virtual bool operator==(const Type& other) const = 0;
    virtual bool Compatible(const Type& ohter) const = 0;
    virtual ~Type(void) {}

    int Width(void) const { return _width; }
    void SetWidth(int width) { _width = width; }
    int Align(void) const { return _align; }
    void SetAlign(int align) { _align = align; }
    int Qual(void) const { return _qual; }
    void SetQual(int qual) { _qual = qual; }
    bool IsComplete(void) const { return _complete; }
    void SetComplete(bool complete) { _complete = complete; }

    bool IsConst(void) const { return _qual & Q_CONST; }
    bool IsScalar(void) const {
        return (nullptr != ToArithmType()
            || nullptr != ToPointerType());
    }
    bool IsFloat(void) const;
    bool IsInteger(void) const;
    bool IsArithm(void) const { return (nullptr != ToArithmType()); }

    virtual VoidType* ToVoidType(void) { return nullptr; }
    virtual const VoidType* ToVoidType(void) const { return nullptr; }
    virtual ArithmType* ToArithmType(void) { return nullptr; }
    virtual const ArithmType* ToArithmType(void) const { return nullptr; }
    virtual ArrayType* ToArrayType(void) { return nullptr; }
    virtual const ArrayType* ToArrayType(void) const { return nullptr; }
    virtual FuncType* ToFuncType(void) { return nullptr; }
    virtual const FuncType* ToFuncType(void) const { return nullptr; }
    virtual PointerType* ToPointerType(void) { return nullptr; }
    virtual const PointerType* ToPointerType(void) const { return nullptr; }
    virtual DerivedType* ToDerivedType(void) { return nullptr; }
    virtual const DerivedType* ToDerivedType(void) const { return nullptr; }
    virtual StructUnionType* ToStructUnionType(void) { return nullptr; }
    virtual const StructUnionType* ToStructUnionType(void) const { return nullptr; }


    virtual EnumType* ToEnumType(void) { return nullptr; }

    virtual const EnumType* ToEnumType(void) const { return nullptr; }

    //static IntType* NewIntType();
    static VoidType* NewVoidType(void);
    static ArrayType* NewArrayType(long long len, Type* eleType);
    static FuncType* NewFuncType(Type* derived, int funcSpec, \
        bool hasEllipsis, const std::list<Type*>& params = std::list<Type*>());
    static PointerType* NewPointerType(Type* derived);
    static StructUnionType* NewStructUnionType(bool isStruct);
    //static EnumType* NewEnumType();
    static ArithmType* NewArithmType(int tag);

protected:
    explicit Type(int width, bool complete)
        : _width(width), _complete(complete) {}

    int _width;	// the bytes to store object of that type
    int _align;
    int _qual;
    bool _complete;
};


class VoidType : public Type
{
    friend class Type;

public:
    virtual ~VoidType(void) {}

    virtual VoidType* ToVoidType(void) {
        return this;
    }

    virtual const VoidType* ToVoidType(void) const {
        return this;
    }

    virtual bool operator==(const Type& other) const {
        return other.ToVoidType() != nullptr;
    }

    virtual bool Compatible(const Type& other) const {
        return *this == other;
    }

protected:
    VoidType(void) : Type(0, true) {}
};

class ArithmType : public Type
{
    friend class Type;

public:
    virtual ~ArithmType(void) {}

    virtual ArithmType* ToArithmType(void) {
        return this;
    }

    virtual const ArithmType* ToArithmType(void) const {
        return this;
    }

    virtual bool operator==(const Type& other) const {
        auto ArithmType = other.ToArithmType();
        return (nullptr != ArithmType && _tag == ArithmType->_tag);
    }

    virtual bool Compatible(const Type& other) const {
        //TODO: 
        return false;
    }

    bool IsBool(void) const {
        return T_BOOL == _tag;
    }

    bool IsInteger(void) const {
        return (_tag & T_BOOL) || \
            (_tag & T_SHORT) || \
            (_tag & T_INT) || \
            (_tag & T_LONG) || \
            (_tag & T_LONG_LONG) || \
            (_tag & T_UNSIGNED);
    }

    bool IsFloat(void) const {
        return (_tag & T_FLOAT) || (_tag & T_DOUBLE);
    }

    bool IsComplex(void) const {
        return _tag & T_COMPLEX;
    }

protected:
    explicit ArithmType(int tag)
        : Type(CalcWidth(tag), true), _tag(tag) {}

private:
    int _tag;
    static int CalcWidth(int tag);
};


class DerivedType : public Type
{
    //friend class Type;
public:
    Type* Derived(void) { return _derived; }
    const Type* Derived(void) const { return _derived; }
    void SetDerived(Type* derived) { _derived = derived; }
    virtual DerivedType* ToDerivedType(void) { return this; }
    virtual const DerivedType* ToDerivedType(void) const { return this; }

protected:
    DerivedType(Type* derived, int width)
        : Type(width, true), _derived(derived) {}

    Type* _derived;
};


class PointerType : public DerivedType
{
    friend class Type;

public:
    ~PointerType(void) {}

    virtual PointerType* ToPointerType(void) {
        return this;
    }

    virtual const PointerType* ToPointerType(void) const {
        return this;
    }

    virtual bool operator==(const Type& other) const;
    virtual bool Compatible(const Type& other) const;

protected:
    PointerType(Type* derived)
        : DerivedType(derived, _machineWord) {}
};


class ArrayType : public PointerType
{
    friend class Type;

public:
    virtual ~ArrayType(void) {
        delete _derived;
    }

    virtual ArrayType* ToArrayType(void) {
        return this;
    }

    virtual const ArrayType* ToArrayType(void) const {
        return this;
    }

    virtual PointerType* ToPointerType(void) {
        return this;
    }

    virtual const PointerType* ToPointerType(void) const {
        return this;
    }

    virtual bool operator==(const Type& other) const {
        auto otherArray = ToArrayType();
        return (nullptr != otherArray
            && _width == otherArray->_width
            && *_derived == *otherArray->_derived);
    }

    virtual bool Compactible(const Type& other) const {
        auto otherArray = ToArrayType();
        return (nullptr != otherArray
            && _width == otherArray->_width
            && _derived->Compatible(*otherArray->_derived));
    }

protected:
    ArrayType(long long len, Type* derived)
        : PointerType(derived) {
        SetComplete(len > 0);	//����len < 0,��ô�����Ͳ�����
        SetWidth(len > 0 ? len * derived->Width() : 0);
        SetQual(Q_CONST);

    }
};


class FuncType : public DerivedType
{
    friend class Type;

public:
    ~FuncType(void) {}
    virtual FuncType* ToFuncType(void) { return this; }
    virtual const FuncType* ToFuncType(void) const { return this; }
    virtual bool operator==(const Type& other) const;
    virtual bool Compatible(const Type& other) const;
    //bool IsInline(void) const { _inlineNoReturn & F_INLINE; }
    //bool IsNoReturn(void) const { return _inlineNoReturn & F_NORETURN; }

protected:
    //a function does not has the width property
    FuncType(Type* derived, int inlineReturn, bool hasEllipsis,
            const std::list<Type*>& params = std::list<Type*>())
        : DerivedType(_derived, -1), _inlineNoReturn(inlineReturn),
          _hasEllipsis(hasEllipsis), _params(params) {
    }

private:
    int _inlineNoReturn;
    bool _hasEllipsis;
    std::list<Type*> _params;
};


class StructUnionType : public Type
{
    friend class Type;

public:
    ~StructUnionType(void) {/*TODO: delete _env ?*/ }
    virtual StructUnionType* ToStructUnionType(void) { return this; }
    virtual const StructUnionType* ToStructUnionType(void) const { return this; }
    virtual bool operator==(const Type& other) const;
    virtual bool Compatible(const Type& other) const;
    Variable* Find(const char* name);
    const Variable* Find(const char* name) const;

    // struct/union
    void AddMember(const char* name, Type* type);
    bool IsStruct(void) const { return _isStruct; }
    
protected:
    // default is incomplete
    explicit StructUnionType(bool isStruct);
    StructUnionType(const StructUnionType& other);

private:
    static int CalcWidth(const Env* env);

    bool _isStruct;
    Env* _mapMember;
};

/*
class EnumType : public Type
{
friend class Type;
public:
virtual ~EnumType(void) {}
virtual EnumType* ToEnumType(void) { return this; }
virtual const EnumType* ToEnumType(void) const { return this; }
virtual bool operator==(const Type& other) const;
virtual bool Compatible(const Type& other) const;
protected:
EnumType(void) : Type(_machineWord, false) {}
private:

};
*/

#endif
