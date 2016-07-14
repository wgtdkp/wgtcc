#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include "mem_pool.h"
#include "scope.h"

#include <algorithm>
#include <list>


/********* Type System ***********/
class Scope;

class Type;
class VoidType;
class Identifier;
class Object;
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
    static const int _intWidth = 4;

    bool operator!=(const Type& other) const { return !(*this == other); }

    virtual bool operator==(const Type& other) const {
        return (_width == other._width && _qual == other._qual 
                && _complete == other._complete);
    }

    virtual bool Compatible(const Type& ohter) const = 0;
    virtual ~Type(void) {}

    // For Debugging
    virtual std::string Str(void) const = 0; 

    int Width(void) const {
        return _width;
    }
    
    void SetWidth(int width) {
        _width = width;
    }
    
    int Align(void) const {
        // TOD(wgtdkp):
        //return _align;
        return std::max(_align, _width);
    }

    void SetAlign(int align) { _align = align; }
    
    int Qual(void) const { return _qual; }
    
    void SetQual(int qual) { _qual = qual; }
    
    bool Complete(void) const { 
        return _complete;
    }
    
    void SetComplete(bool complete) { _complete = complete; }

    bool IsConst(void) const { return _qual & Q_CONST; }
    
    bool IsScalar(void) const {
        return (ToArithmType() || ToPointerType());
    }
    
    bool IsFloat(void) const;
    
    bool IsInteger(void) const;
    
    bool IsArithm(void) const {
        return ToArithmType();
    }

    bool IsReal(void) const {
        return IsInteger() || IsFloat();
    }

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
        bool hasEllipsis, const std::list<Type*>& params=std::list<Type*>());
    
    static PointerType* NewPointerType(Type* derived);
    
    static StructUnionType* NewStructUnionType(bool isStruct, bool hasTag, Scope* parent);
    
    //static EnumType* NewEnumType();
    static ArithmType* NewArithmType(int typeSpec);

protected:
    explicit Type(MemPool* pool, int width, bool complete)
        : _pool(pool), _width(width), _complete(complete) {}

    MemPool* _pool;
    int _width;	// the bytes to store object of that type
    int _align;
    int _qual;
    bool _complete;
    
private:
    static MemPoolImp<VoidType>         _voidTypePool;
    static MemPoolImp<ArrayType>        _arrayTypePool;
    static MemPoolImp<FuncType>         _funcTypePool;
    static MemPoolImp<PointerType>      _pointerTypePool;
    static MemPoolImp<StructUnionType>  _structUnionTypePool;
    static MemPoolImp<ArithmType>       _arithmTypePool;
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
        return Type::operator==(other) && other.ToVoidType() != nullptr;
    }

    virtual bool Compatible(const Type& other) const {
        //return *this == other;
        return true;
    }

protected:
    explicit VoidType(MemPool* pool): Type(pool, 0, false) {}
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
        auto arithmType = other.ToArithmType();
        return (Type::operator==(other) 
                && (nullptr != arithmType && _tag == arithmType->_tag));
    }

    virtual bool Compatible(const Type& other) const {
        //TODO: 
        return false;
    }

    bool IsBool(void) const {
        return T_BOOL == _tag;
    }

    bool IsInteger(void) const {
        return (_tag & T_BOOL) || (_tag & T_CHAR)
            || (_tag & T_SHORT) || (_tag & T_INT)
            || (_tag & T_LONG) || (_tag & T_LONG_LONG)
            || (_tag & T_UNSIGNED);
    }

    bool IsFloat(void) const {
        return (_tag & T_FLOAT) || (_tag & T_DOUBLE);
    }

    bool IsComplex(void) const {
        return _tag & T_COMPLEX;
    }

    int Tag(void) const {
        return _tag;
    }

    static int CalcWidth(int tag);
    
    static int Spec2Tag(int spec);

protected:
    explicit ArithmType(MemPool* pool, int tag)
        : Type(pool, CalcWidth(tag), true), _tag(tag) {}

private:
    int _tag;
};


class DerivedType : public Type
{
    //friend class Type;
public:
    Type* Derived(void) {
        return _derived;
    }
    
    const Type* Derived(void) const {
        return _derived;
    }
    
    void SetDerived(Type* derived) {
        _derived = derived;
    }
    
    virtual DerivedType* ToDerivedType(void) {
        return this;
    }
    
    virtual const DerivedType* ToDerivedType(void) const {
        return this;
    }

protected:
    DerivedType(MemPool* pool, Type* derived, int width)
        : Type(pool, width, true), _derived(derived) {}

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
    PointerType(MemPool* pool, Type* derived)
        : DerivedType(pool, derived, _intWidth << 1) {}
};

/*
class StringType: public PointerType
{
    friend class Type;

public:
    ~StringType(void) {}

    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;

protected:
    explicit StringType(void)
        : PointerType(Type::NewArithmType(T_CHAR)) {
    }
};
*/

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
        return Type::operator==(other) && (nullptr != otherArray
            && *_derived == *otherArray->_derived);
    }

    virtual bool Compatible(const Type& other) const {
        auto otherArray = ToArrayType();
        return (nullptr != otherArray
            && _width == otherArray->_width
            && _derived->Compatible(*otherArray->_derived));
    }

    int GetElementOffset(int idx) const {
        return _derived->Width() * idx;
    }

protected:
    ArrayType(MemPool* pool, long long len, Type* derived)
            : PointerType(pool, derived) {
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
    
    virtual FuncType* ToFuncType(void) {
        return this;
    }
    
    virtual const FuncType* ToFuncType(void) const {
        return this;
    }
    
    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;
    //bool IsInline(void) const { _inlineNoReturn & F_INLINE; }
    //bool IsNoReturn(void) const { return _inlineNoReturn & F_NORETURN; }

    std::list<Type*>& Params(void) {
        return _params;
    }

    bool HasEllipsis(void) {
        return _hasEllipsis;
    }

protected:
    //a function does not has the width property
    FuncType(MemPool* pool, Type* derived, int inlineReturn, bool hasEllipsis,
            const std::list<Type*>& params = std::list<Type*>())
        : DerivedType(pool, derived, -1), _inlineNoReturn(inlineReturn),
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
    
    virtual StructUnionType* ToStructUnionType(void) {
        return this;
    }
    
    virtual const StructUnionType* ToStructUnionType(void) const {
        return this;
    }
    
    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;

    // struct/union
    void AddMember(const std::string& name, Object* member);
    
    bool IsStruct(void) const {
        return _isStruct;
    }

    Object* GetMember(const std::string& member);

    Scope* MemberMap(void) {
        return _memberMap;
    }

    bool HasTag(void) {
        return _hasTag;
    }

protected:
    // default is incomplete
    StructUnionType(MemPool* pool, bool isStruct, bool hasTag, Scope* parent);
    
    StructUnionType(const StructUnionType& other);

private:
    void CalcWidth(void);

    bool _isStruct;
    bool _hasTag;
    Scope* _memberMap;
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
EnumType(void) : Type(_intWidth, false) {}
private:

};
*/

ArithmType* MaxType(ArithmType* lhsType, ArithmType* rhsType);

#endif
