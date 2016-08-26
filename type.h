#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include "mem_pool.h"
#include "scope.h"

#include <cassert>

#include <algorithm>
#include <list>


/********* Type System ***********/
class Scope;
class Token;

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
class StructType;
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
        return (_qual == other._qual && _complete == other._complete);
    }

    virtual bool Compatible(const Type& ohter) const = 0;
    virtual ~Type(void) {}

    // For Debugging
    virtual std::string Str(void) const = 0; 

    virtual int Width(void) const = 0;
    
    virtual int Align(void) const = 0;

    static int MakeAlign(int offset, int align) {
        if ((offset % align) == 0)
            return offset;
        if (offset >= 0)
            return offset + align - (offset % align);
        else
            return offset - align - (offset % align);
    } 

    static Type* MayCast(Type* type);

    int Qual(void) const {
        return _qual;
    }
    
    void SetQual(int qual) {
        _qual = qual;
    }
    
    bool Complete(void) const { 
        return _complete;
    }
    
    void SetComplete(bool complete) {
        _complete = complete;
    }

    bool IsConst(void) const {
        return _qual & Q_CONST;
    }
    
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
    
    virtual StructType* ToStructType(void) { return nullptr; }
    
    virtual const StructType* ToStructType(void) const { return nullptr; }

protected:
    Type(MemPool* pool, bool complete)
            : _qual(0), _complete(complete), _pool(pool) {}

    int _qual;
    bool _complete;
    MemPool* _pool;
    
private:
    static MemPoolImp<VoidType>         _voidTypePool;
    static MemPoolImp<ArrayType>        _arrayTypePool;
    static MemPoolImp<FuncType>         _funcTypePool;
    static MemPoolImp<PointerType>      _pointerTypePool;
    static MemPoolImp<StructType>  _structUnionTypePool;
    static MemPoolImp<ArithmType>       _arithmTypePool;
};


class VoidType : public Type
{
    friend class Type;

public:
    static VoidType* New(void);

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

    virtual int Width(void) const {
        return 1;
    }

    virtual int Align(void) const {
        return Width();
    }

    virtual std::string Str(void) const {
        return "void:0";
    }

protected:
    explicit VoidType(MemPool* pool): Type(pool, false) {}
};


class ArithmType : public Type
{
    friend class Type;

public:
    static ArithmType* New(int typeSpec);

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
        // Compatible if both are arithmetic type
        return other.ToArithmType();
    }

    virtual int Width(void) const;

    virtual int Align(void) const {
        return Width();
    }

    virtual std::string Str(void) const;

    bool IsBool(void) const {
        return T_BOOL == _tag;
    }

    bool IsInteger(void) const {
        return !IsFloat() && !IsComplex();
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

protected:
    explicit ArithmType(MemPool* pool, int spec)
        : Type(pool, true), _tag(Spec2Tag(spec)) {}

private:
    static int Spec2Tag(int spec);

    int _tag;
};


class DerivedType : public Type
{
    //friend class Type;
public:
    Type* Derived(void) {
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
    DerivedType(MemPool* pool, Type* derived)
            : Type(pool, true), _derived(derived) {}

    Type* _derived;
};


class PointerType : public DerivedType
{
    friend class Type;

public:
    static PointerType* New(Type* derived);

    ~PointerType(void) {}

    virtual PointerType* ToPointerType(void) {
        return this;
    }

    virtual const PointerType* ToPointerType(void) const {
        return this;
    }

    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;

    virtual int Width(void) const {
        return 8;
    }

    virtual int Align(void) const {
        return Width();
    }

    virtual std::string Str(void) const {
        return _derived->Str() + "*:" + std::to_string(Width());
    }

protected:
    PointerType(MemPool* pool, Type* derived): DerivedType(pool, derived) {}

};


class ArrayType : public DerivedType
{
    friend class Type;

public:
    static ArrayType* New(int len, Type* eleType);
    virtual ~ArrayType(void) {
        delete _derived;
    }

    virtual ArrayType* ToArrayType(void) {
        return this;
    }

    virtual const ArrayType* ToArrayType(void) const {
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
            && Width() == otherArray->Width()
            && _derived->Compatible(*otherArray->_derived));
    }

    virtual int Width(void) const {
        return _derived->Width() * _len;
    }

    virtual int Align(void) const {
        return _derived->Align();
    }

    virtual std::string Str(void) const {
        return _derived->Str() + "[]:" + std::to_string(Width());
    }

    int GetElementOffset(int idx) const {
        return _derived->Width() * idx;
    }

    int Len(void) const {
        return _len;
    }

    bool HasLen(void) const {
        return _len > 0;
    }

    void SetLen(int len) {
        _len = len;
    }

protected:
    ArrayType(MemPool* pool, int len, Type* derived)
            : DerivedType(pool, derived), _len(len) {
        SetComplete(_len >= 0);
        SetQual(Q_CONST);
    }

    int _len;
};


class FuncType : public DerivedType
{
    friend class Type;

public:
    typedef std::vector<Type*> TypeList;

public:
    static FuncType* New(Type* derived, int funcSpec,
            bool hasEllipsis, const FuncType::TypeList& params);

    ~FuncType(void) {}
    
    virtual FuncType* ToFuncType(void) {
        return this;
    }
    
    virtual const FuncType* ToFuncType(void) const {
        return this;
    }
    
    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;

    virtual int Width(void) const {
        return 1;
    }

    virtual int Align(void) const {
        return Width();
    }

    virtual std::string Str(void) const;

    //bool IsInline(void) const { _inlineNoReturn & F_INLINE; }
    //bool IsNoReturn(void) const { return _inlineNoReturn & F_NORETURN; }

    TypeList& ParamTypes(void) {
        return _paramTypes;
    }

    bool Variadic(void) {
        return _variadic;
    }

protected:
    //a function does not has the width property
    FuncType(MemPool* pool, Type* derived, int inlineReturn, bool variadic,
            const TypeList& paramTypes)
        : DerivedType(pool, derived), _inlineNoReturn(inlineReturn),
          _variadic(variadic), _paramTypes(paramTypes) {
        SetComplete(false);
    }

private:
    int _inlineNoReturn;
    bool _variadic;
    TypeList _paramTypes;
};


class StructType : public Type
{
    friend class Type;
    
public:
    /*
    struct StructField {
        StructField(Object* member, bool isAnonyUnionField=false)
                _member(member), _isAnonyUnionField(isAnonyUnionField) {}

        Object* _member;
        bool _isAnonyUnionField;
    }

    class Iterator {
    public:
        Object* operator*(void) {
            return (*_iter);
        }

        Object* operator->(void) {
            return (*iter);
        }

        Iterator& operator++(void) {

        }

        Iterator operator++(void) {

        }

        bool operator==(const Iterator& other) const {

        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        MemberList::iterator _iter;

    };
    */

    typedef std::list<Object*> MemberList;
    typedef std::list<Object*>::iterator Iterator;
    
public:
    static StructType* New(bool isStruct, bool hasTag, Scope* parent);
    
    ~StructType(void) {/*TODO: delete _env ?*/ }
    
    virtual StructType* ToStructType(void) {
        return this;
    }
    
    virtual const StructType* ToStructType(void) const {
        return this;
    }
    
    virtual bool operator==(const Type& other) const;
    
    virtual bool Compatible(const Type& other) const;

    virtual int Width(void) const {
        return _width;
    }

    virtual int Align(void) const {
        return _align;
    }

    virtual std::string Str(void) const;

    // struct/union
    void AddMember(Object* member);

    void AddBitField(Object* member, int offset);

    bool IsStruct(void) const {
        return _isStruct;
    }

    Object* GetMember(const std::string& member);

    Scope* MemberMap(void) {
        return _memberMap;
    }

    MemberList& Members(void) {
        return _members;
    }

    int Offset(void) const {
        return _offset;
    }

    bool HasTag(void) {
        return _hasTag;
    }
    
    void MergeAnony(Object* anony);

protected:
    // default is incomplete
    StructType(MemPool* pool, bool isStruct, bool hasTag, Scope* parent);
    
    StructType(const StructType& other);

private:
    void CalcWidth(void);

    static std::string AnonymousBitField(void) {
        static int id = 0;
        return std::to_string(id++) + "<anonymous>";
    }

    bool _isStruct;
    bool _hasTag;
    Scope* _memberMap;

    MemberList _members;
    int _offset;
    int _width;
    int _align;
};


ArithmType* MaxType(ArithmType* lhsType, ArithmType* rhsType);

#endif
